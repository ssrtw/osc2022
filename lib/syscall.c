#include "syscall.h"

#include "cpio.h"
#include "malloc.h"
#include "mbox.h"
#include "sched.h"
#include "signal.h"
#include "stddef.h"
#include "string.h"
#include "thread.h"
#include "uart.h"

typedef void (*syscall_func)();

void sys_getpid(trapframe_t *tf);
void sys_uart_read(trapframe_t *tf, char buf[], size_t size);
void sys_uart_write(trapframe_t *tf, const char buf[], size_t size);
void sys_exec(trapframe_t *tf, const char *name, char *const argv[]);
void sys_fork(trapframe_t *tf);
void sys_exit(trapframe_t *tf);
void sys_mbox_call(trapframe_t *tf, byte ch, uint32_t *mb);
void sys_signal(trapframe_t *tf, uint32_t signum, sig_handler_t handler);
void sys_kill(trapframe_t *tf, int pid, uint32_t signum);
void sys_mmap(trapframe_t *tf, void *addr, size_t len, int prot, int flags, int fd, int file_offset);
void sys_sigret(trapframe_t *tf);

syscall_func syscall_table[] = {
    (syscall_func)sys_getpid,      // 0
    (syscall_func)sys_uart_read,   // 1
    (syscall_func)sys_uart_write,  // 2
    (syscall_func)sys_exec,        // 3
    (syscall_func)sys_fork,        // 4
    (syscall_func)sys_exit,        // 5
    (syscall_func)sys_mbox_call,   // 6
    (syscall_func)sys_kill_pid,    // 7
    (syscall_func)sys_signal,      // 8
    (syscall_func)sys_kill,        // 9
    (syscall_func)sys_mmap,        // 10
    (syscall_func)sys_sigret,      // 11
};

void syscall_handler(trapframe_t *tf) {
    enable_el1_interrupt();
    int syscall_num = tf->x[8];
    if (syscall_num > ARRAY_SIZE(syscall_table)) return;
    (syscall_table[syscall_num])(tf, tf->x[0], tf->x[1], tf->x[2], tf->x[3], tf->x[4], tf->x[5]);
}

void sys_getpid(trapframe_t *tf) {
    tf->x[0] = curr_thread->pid;
}

void sys_uart_read(trapframe_t *tf, char buf[], size_t size) {
    uart_getn(buf, size);
    tf->x[0] = size;
}

void sys_uart_write(trapframe_t *tf, const char buf[], size_t size) {
    uart_putn(buf, size);
    tf->x[0] = size;
}

void sys_exec(trapframe_t *tf, const char *name, char *const argv[]) {
    // search cpio get file, and exec_thread
    cpio_file_info *cfi = cpio_traverse(name, NULL);

    if (cfi != NULL) {
        // 蓋掉原本的資料，所以有malloc的全都先砍一砍
        // 先砍vma_list
        list_head_t *pos = curr_thread->vma_list.next, *nxt_pos;
        while (pos != &curr_thread->vma_list) {
            // 如果是該thread自己malloc的就砍
            if (((vm_area_struct_t *)pos)->allocated) {
                // uart_printf("phys: %X", ((vm_area_struct_t *)pos)->phys_addr);
                kfree((void *)PHYS_TO_VIRT(((vm_area_struct_t *)pos)->phys_addr));
            }
            nxt_pos = pos->next;
            kfree(pos);
            pos = nxt_pos;
        }
        INIT_LIST_HEAD(&curr_thread->vma_list);
        // 重設檔案段
        curr_thread->data_size = cfi->size;
        // TODO: 用參照次數看要不要先kfree這段data。目前直接蓋掉原本的空間
        curr_thread->data = kmalloc(cfi->size);
        memcpy(curr_thread->data, (void *)cfi->start, cfi->size);
        // TODO: 用參照看要不要free先。目前直接蓋成新的一塊stack
        curr_thread->ustack_ptr = kmalloc(USTACK_SIZE);

        asm("dsb ish\n\t");  // ensure write has completed
        // 把pgd清空
        free_pages(curr_thread->cxt.ttbr0_el1, 0);
        memset(PHYS_TO_VIRT(curr_thread->cxt.ttbr0_el1), 0, 0x1000);
        asm("tlbi vmalle1is\n\t"  // invalidate all TLB entries
            "dsb ish\n\t"         // ensure completion of TLB invalidatation
            "isb\n\t");           // clear pipeline

        // text
        add_vma(curr_thread, 0x0, cfi->size, (size_t)VIRT_TO_PHYS(curr_thread->data), PROT_R | PROT_W | PROT_X, PG_NEED_ALLOC);
        // stack
        add_vma(curr_thread, 0xffffffffb000, USTACK_SIZE, (size_t)VIRT_TO_PHYS(curr_thread->ustack_ptr), PROT_R | PROT_W | PROT_X, PG_NEED_ALLOC);
        // device
        add_vma(curr_thread, 0x3C000000L, 0x3000000L, 0x3C000000L, PROT_R | PROT_W, PG_DONT_ALLOC);
        // for signal wrapper
        add_vma(curr_thread, USER_SIG_WRAPPER_VIRT_ADDR, 0x2000, (size_t)VIRT_TO_PHYS(sig_handler_wrapper), PROT_R | PROT_X, PG_DONT_ALLOC);

        // reset signal
        curr_thread->sigcheck = 0;
        for (int i = 0; i < SIG_MAX; i++) {
            curr_thread->sig_handler[i] = default_sig_handler;
            curr_thread->sigcount[i] = 0;
        }
        tf->elr_el1 = 0;
        tf->sp_el0 = 0xfffffffff000;
        tf->x[0] = 0;
        kfree(cfi);
    }
}

void sys_fork(trapframe_t *tf) {
    lock();
    int parent_pid = curr_thread->pid;
    thread_t *new_thread = thread_create(curr_thread->data, curr_thread->data_size);

    // copy_signal
    for (int i = 0; i < SIG_MAX; i++) {
        new_thread->sig_handler[i] = curr_thread->sig_handler[i];
    }

    vm_area_struct_t *pos;
    list_for_each_entry(pos, &curr_thread->vma_list, lh) {
        // // 如果是自己malloc的就要複製一份出來
        // if (pos->allocated == PG_NEED_ALLOC) {
        // 如果是device或signal那塊，就不用malloc新的空間出來後面直接新增就好
        if (pos->virt_addr == USER_SIG_WRAPPER_VIRT_ADDR || pos->virt_addr == 0x3C000000L) continue;
        char *new_alloc = kmalloc(pos->area_size);
        add_vma(new_thread, pos->virt_addr, pos->area_size, (uint64_t)VIRT_TO_PHYS(new_alloc), pos->protected, PG_NEED_ALLOC);
        memcpy(new_alloc, (void *)PHYS_TO_VIRT(pos->phys_addr), pos->area_size);
        // }
    }

    // device
    add_vma(new_thread, 0x3C000000L, 0x3000000L, 0x3C000000L, PROT_R | PROT_W, PG_DONT_ALLOC);
    // for signal wrapper
    add_vma(new_thread, USER_SIG_WRAPPER_VIRT_ADDR, 0x2000, (size_t)VIRT_TO_PHYS(sig_handler_wrapper), PROT_R | PROT_X, PG_DONT_ALLOC);

    memcpy((char *)new_thread->kstack_ptr, (char *)curr_thread->kstack_ptr, KSTACK_SIZE);
    store_cxt(get_current());
    // child way
    if (parent_pid != curr_thread->pid) {
        goto sys_fork_child;
    }

    void *temp_ttbr0_el1 = new_thread->cxt.ttbr0_el1;
    // 這邊複製cxt內容
    new_thread->cxt = curr_thread->cxt;
    new_thread->cxt.ttbr0_el1 = VIRT_TO_PHYS(temp_ttbr0_el1);
    // 必須讓fork出來的，他們的sp跟fp在當前parent的狀態
    new_thread->cxt.fp += new_thread->kstack_ptr - curr_thread->kstack_ptr;
    new_thread->cxt.sp += new_thread->kstack_ptr - curr_thread->kstack_ptr;

    unlock();

    tf->x[0] = new_thread->pid;
    return;

sys_fork_child:
    tf->x[0] = 0;
    return;
}

void sys_exit(trapframe_t *tf) {
    thread_exit();
}

void sys_mbox_call(trapframe_t *tf, byte ch, uint32_t *mb) {
    // mbox just one entry
    lock();
    uint32_t mbox_size = mb[0];
    memcpy((char *)mbox, mb, mbox_size);
    mbox_call(ch);
    memcpy(mb, (char *)mbox, mbox_size);
    unlock();
}

void sys_kill_pid(trapframe_t *tf, int pid) {
    lock();
    if (pid >= PID_MAX || pid < 0 || threads[pid].state == THREAD_UNUSED) {
        unlock();
        return;
    }
    if (pid == 0) {
        uart_printf("You can't kill idle task!\n");
        unlock();
        return;
    }
    threads[pid].state = THREAD_ZOMBIE;
    unlock();
    schedule();
}

void sys_signal(trapframe_t *tf, uint32_t signum, sig_handler_t handler) {
    if (signum >= SIG_MAX) return;
    uart_printf("handler addr: %X\n", handler);
    curr_thread->sig_handler[signum] = handler;
}

void sys_kill(trapframe_t *tf, int pid, uint32_t signum) {
    if (pid >= PID_MAX || threads[pid].state == THREAD_UNUSED) {
        return;
    }
    lock();
    threads[pid].sigcount[signum]++;
    unlock();
}

void sys_sigret(trapframe_t *tf) {
    load_cxt(&curr_thread->sig_save_cxt);
}

// only need to implement the anonymous page
void sys_mmap(trapframe_t *tf, void *addr, size_t len, int prot, int flags, int fd, int file_offset) {
    uart_printf("** Do mmap, addr=%X, len=%X\n", addr, len);
    // 地址超過就回到0再找
    if (len + (uint64_t)addr >= 0xfffffffff000L) addr = 0L;

    // 把長度對齊4K
    len = len % 0x1000 ? len + (0x1000 - len % 0x1000) : len;
    addr = (uint64_t)addr % 0x1000 ? addr + (0x1000 - (uint64_t)addr % 0x1000) : addr;

    // 找出有沒有重疊的段
    vm_area_struct_t *pos;
    list_for_each_entry(pos, &curr_thread->vma_list, lh) {
        if (!(pos->virt_addr >= (uint64_t)(addr + len) || pos->virt_addr + pos->area_size <= (uint64_t)addr)) {
            break;
        }
    }

    // 如果空間撞到了，kernel這邊要幫忙搬到別的地方去
    if (pos != (vm_area_struct_t *)(&curr_thread->vma_list)) {
        // 從剛剛找到重疊那個vma的後面開始繼續找可用空間
        sys_mmap(tf, (void *)(pos->virt_addr + pos->area_size), len, prot, flags, fd, file_offset);
        return;
    }
    // 地址沒撞到，直接分配就好
    add_vma(curr_thread, (uint64_t)addr, len, VIRT_TO_PHYS((uint64_t)kmalloc(len)), prot, 1);
    // 找到後，最後回傳ㄉx0
    tf->x[0] = (uint64_t)addr;
    uart_printf("** Get mmap addr = %X\n", tf->x[0]);
}