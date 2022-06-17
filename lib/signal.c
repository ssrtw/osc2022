#include "signal.h"

#include "except.h"
#include "malloc.h"
#include "sched.h"
#include "syscall.h"
#include "uart.h"

void sig_handler_wrapper();

void check_sig(trapframe_t *tf) {
    lock();
    if (curr_thread->sigcheck) {
        unlock();
        return;
    }
    // prevent nested running signal handler
    curr_thread->sigcheck = 1;
    unlock();
    for (int i = 0; i < SIG_MAX; i++) {
        store_cxt(&curr_thread->sig_save_cxt);
        if (curr_thread->sigcount[i] > 0) {
            lock();
            curr_thread->sigcount[i]--;
            unlock();
            run_sig(tf, i);
        }
    }
    lock();
    curr_thread->sigcheck = 0;
    unlock();
}

void run_sig(trapframe_t *tpf, uint32_t signum) {
    curr_thread->curr_sig_handler = curr_thread->sig_handler[signum];

    // run default handler in kernel
    if (curr_thread->curr_sig_handler == default_sig_handler) {
        default_sig_handler();
        return;
    }

    char *tmp_sig_ustack = kmalloc(USTACK_SIZE);

    asm("msr elr_el1, %0\n\t"
        "msr sp_el0, %1\n\t"
        "msr spsr_el1, %2\n\t"
        "eret\n\t" ::"r"(sig_handler_wrapper),
        "r"(tmp_sig_ustack + USTACK_SIZE),
        "r"(tpf->spsr_el1));
}

void sig_handler_wrapper() {
    (curr_thread->curr_sig_handler)();
    // system call sigreturn
    asm("mov x8,%0\n\t"
        "svc 0\n\t" ::"r"(SYSNUM_sigret));
}

void default_sig_handler() {
    sys_kill_pid(0, curr_thread->pid);
}