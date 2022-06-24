#include "sched.h"

#include "except.h"
#include "malloc.h"
#include "string.h"
#include "timer.h"
#include "uart.h"

void init_threads() {
    lock();
    nxt_pid = 0;
    rq = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(rq);
    for (int i = 0; i <= PID_MAX; i++) {
        threads[i].pid = i;
        threads[i].state = THREAD_UNUSED;
    }
    curr_thread = thread_create(idle, 0x4000);
    asm volatile("msr tpidr_el1, %0" ::"r"(&curr_thread->cxt));
    unlock();
}

void idle() {
    for (;;) {
        kill_zombies();
        schedule();
    }
}

void schedule() {
    lock();
    do {
        curr_thread = list_next_entry(curr_thread, lh);
    } while (list_is_head(&curr_thread->lh, rq) || curr_thread->state == THREAD_ZOMBIE);
    unlock();
    switch_to(get_current(), &curr_thread->cxt);
}

void kill_zombies() {
    lock();
    thread_t *curr;
    list_for_each_entry(curr, rq, lh) {
        if (curr->state == THREAD_ZOMBIE) {
            list_del_entry(&curr->lh);
            // 先清空kernel stack
            kfree(curr->kstack_ptr);
            // 去清空所有有malloc的page
            free_pages(curr->cxt.ttbr0_el1, 0);
            //
            list_head_t *pos = ((thread_t *)curr)->vma_list.next;
            while (pos != &((thread_t *)curr)->vma_list) {
                if (((vm_area_struct_t *)pos)->allocated)
                    kfree((void *)PHYS_TO_VIRT(((vm_area_struct_t *)pos)->phys_addr));

                list_head_t *nxt_pos = pos->next;
                kfree(pos);
                pos = nxt_pos;
            }
            curr->state = THREAD_UNUSED;
        }
    }
    unlock();
}

void exec_thread(byte *data, uint32_t filesize) {
    thread_t *t = thread_create(data, filesize);

    // text
    add_vma(t, 0x0, filesize, (size_t)VIRT_TO_PHYS(t->data), PROT_R | PROT_W | PROT_X, PG_NEED_ALLOC);
    // stack
    add_vma(t, 0xffffffffb000, USTACK_SIZE, (size_t)VIRT_TO_PHYS(t->ustack_ptr), PROT_R | PROT_W | PROT_X, PG_NEED_ALLOC);
    // device
    add_vma(t, 0x3C000000L, 0x3000000L, 0x3C000000L, PROT_R | PROT_W, PG_DONT_ALLOC);
    // for signal wrapper
    add_vma(t, USER_SIG_WRAPPER_VIRT_ADDR, 0x2000, (size_t)VIRT_TO_PHYS(sig_handler_wrapper), PROT_R | PROT_X, PG_DONT_ALLOC);

    // 填入text段
    memcpy(t->data, (char *)data, filesize);
    t->cxt.ttbr0_el1 = VIRT_TO_PHYS(t->cxt.ttbr0_el1);
    t->cxt.sp = 0xfffffffff000;
    t->cxt.fp = 0xfffffffff000;
    t->cxt.lr = 0L;
    curr_thread = t;
    timer_enable();
    add_timer_task(TIMER_BY_SESC, 1, schedule_timer_task, "");
    asm volatile(
        "msr tpidr_el1, %0\n\t"  // thread info addr
        "msr elr_el1, %1\n\t"    // ereturn addr, back to el0 start address
        "msr spsr_el1, xzr\n\t"  // enable interrupt in EL0. You can do it by setting spsr_el1 to 0 before returning to EL0.
        "msr sp_el0, %2\n\t"     // el0的stack(user stack)
        "mov sp, %3\n\t"         // el1的stack(kernel stack)
        "mov fp, sp\n\t"         // 難道是這行= =?

        "dsb ish\n\t"            // ensure write has completed
        "msr ttbr0_el1, %4\n\t"  // set user pgd addr
        "tlbi vmalle1is\n\t"     // invalidate all TLB entries
        "dsb ish\n\t"            // ensure completion of TLB invalidatation
        "isb\n\t"                // clear pipeline

        "eret\n\t" ::"r"(&t->cxt),
        "r"(t->cxt.lr), "r"(t->cxt.sp), "r"(t->kstack_ptr + KSTACK_SIZE), "r"(t->cxt.ttbr0_el1));
}

thread_t *thread_create(void *start, uint32_t filesize) {
    lock();
    thread_t *use_thread;
    // 先檢查是否已經拿完一輪了，不然直接從下個pid找起
    if (unlikely(nxt_pid > PID_MAX)) {
        int i;
        for (i = 0; i <= PID_MAX; i++) {
            if (threads[i].state == THREAD_UNUSED) {
                use_thread = &threads[i];
                break;
            }
        }
        if (i > PID_MAX) {
            uart_puts("error, haven't any can use PID\n");
            return NULL;
        }
    } else {
        use_thread = &threads[nxt_pid++];
    }
    uart_printf("new pid:%d\n", nxt_pid - 1);
    use_thread->state = THREAD_RUNNING;
    // TODO: 真的不董為啥這邊要放data
    use_thread->data = kmalloc(filesize);
    use_thread->data_size = filesize;
    use_thread->kstack_ptr = kmalloc(KSTACK_SIZE);
    use_thread->ustack_ptr = kmalloc(USTACK_SIZE);
    use_thread->cxt.ttbr0_el1 = kmalloc(0x1000);
    use_thread->cxt.lr = (uint64_t)start;
    // TODO: 要用kernel stack是因為層級問題?
    use_thread->cxt.sp = (uint64_t)use_thread->kstack_ptr + KSTACK_SIZE;
    use_thread->cxt.fp = use_thread->cxt.sp;
    memset(use_thread->cxt.ttbr0_el1, 0, 0x1000);
    // TODO: 之後再來補vma_list，先全部map看看?
    /* virtual memory */
    INIT_LIST_HEAD(&use_thread->vma_list);
    /* sig */
    use_thread->sigcheck = 0;
    for (int i = 0; i < SIG_MAX; i++) {
        use_thread->sig_handler[i] = default_sig_handler;
        use_thread->sigcount[i] = 0;
    }
    list_add(&use_thread->lh, rq);
    unlock();
    return use_thread;
}

void thread_exit() {
    lock();
    curr_thread->state = THREAD_ZOMBIE;
    unlock();
    schedule();
}

void schedule_timer_task(char *args) {
    unsigned long long cntfrq_el0;
    asm volatile("mrs %0, cntfrq_el0\n\t"
                 : "=r"(cntfrq_el0));  // tick frequency
    // add a new schedule task
    // Set the expired time as core timer frequency shift right 5 bits.
    add_timer_task(TIMER_BY_TICK, cntfrq_el0 >> 5, schedule_timer_task, "");
}