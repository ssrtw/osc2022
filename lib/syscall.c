#include "syscall.h"

#include "cpio.h"
#include "malloc.h"
#include "mbox.h"
#include "sched.h"
#include "signal.h"
#include "stddef.h"
#include "string.h"
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
    (syscall_func)sys_sigret,      // 10
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
        memcpy(curr_thread->data, cfi->start, cfi->size);
        // reset signal
        curr_thread->sigcheck = 0;
        for (int i = 0; i < SIG_MAX; i++) {
            curr_thread->sig_handler[i] = default_sig_handler;
            curr_thread->sigcount[i] = 0;
        }
        tf->elr_el1 = (uint64_t)curr_thread->data;
        tf->sp_el0 = (unsigned long)curr_thread->ustack_ptr + USTACK_SIZE;
        tf->x[0] = 0;
        kfree(cfi);
    }
}

void sys_fork(trapframe_t *tf) {
    lock();
    int parent_pid = curr_thread->pid;
    thread_t *parent_thread = curr_thread;
    thread_t *new_thread = thread_create(curr_thread->data);

    // copy_signal
    for (int i = 0; i < SIG_MAX; i++) {
        new_thread->sig_handler[i] = curr_thread->sig_handler[i];
    }

    memcpy((char *)new_thread->ustack_ptr, (char *)curr_thread->ustack_ptr, USTACK_SIZE);
    memcpy((char *)new_thread->kstack_ptr, (char *)curr_thread->kstack_ptr, KSTACK_SIZE);
    store_cxt(get_current());
    // child way
    if (parent_pid != curr_thread->pid) {
        goto sys_fork_child;
    }

    new_thread->cxt = curr_thread->cxt;
    new_thread->cxt.fp += new_thread->kstack_ptr - curr_thread->kstack_ptr;
    new_thread->cxt.sp += new_thread->kstack_ptr - curr_thread->kstack_ptr;

    unlock();

    tf->x[0] = new_thread->pid;
    return;

sys_fork_child:
    tf = (trapframe_t *)((byte *)tf + (uint64_t)new_thread->kstack_ptr - (uint64_t)parent_thread->kstack_ptr);
    tf->sp_el0 += new_thread->ustack_ptr - parent_thread->ustack_ptr;
    tf->x[0] = 0;
    return;
}

void sys_exit(trapframe_t *tf) {
    thread_exit();
}

void sys_mbox_call(trapframe_t *tf, byte ch, uint32_t *mb) {
    // mbox just one entry
    lock();
    mbox_call(ch, mb);
    unlock();
}

void sys_kill_pid(trapframe_t *tf, int pid) {
    lock();
    if (pid >= PID_MAX || pid < 0 || threads[pid].state == THREAD_UNUSED) {
        unlock();
        return;
    }
    threads[pid].state = THREAD_ZOMBIE;
    unlock();
    schedule();
}

void sys_signal(trapframe_t *tf, uint32_t signum, sig_handler_t handler) {
    if (signum >= SIG_MAX) return;
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
    unsigned long sig_ustack = tf->sp_el0 % USTACK_SIZE == 0 ? tf->sp_el0 - USTACK_SIZE : tf->sp_el0 & (~(USTACK_SIZE - 1));
    kfree((char *)sig_ustack);
    load_cxt(&curr_thread->sig_save_cxt);
}