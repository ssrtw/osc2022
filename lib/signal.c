#include "signal.h"

#include "except.h"
#include "malloc.h"
#include "sched.h"
#include "syscall.h"
#include "uart.h"

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

void run_sig(trapframe_t *tf, uint32_t signum) {
    curr_thread->curr_sig_handler = curr_thread->sig_handler[signum];

    // run default handler in kernel
    if (curr_thread->curr_sig_handler == default_sig_handler) {
        default_sig_handler();
        return;
    }

    asm volatile(
        "msr elr_el1, %0\n\t"
        "msr sp_el0, %1\n\t"
        "msr spsr_el1, %2\n\t"
        "mov x0, %3\n\t"
        "eret\n\t" ::
            // elr的地址是map到wrapper的地方，再加上wrapper真正address小於4K的值，才會是正確地址
        "r"(USER_SIG_WRAPPER_VIRT_ADDR + ((size_t)sig_handler_wrapper % 0x1000)),
        "r"(tf->sp_el0),
        "r"(tf->spsr_el1), "r"(curr_thread->curr_sig_handler));
}

void sig_handler_wrapper() {
    // system call sigreturn
    asm volatile(
        "blr x0\n\t"
        "ldr x8, =" XSTR(SYSNUM_sigret)
            "\n\t"
            "svc 0\n\t");
    // 編譯器壞壞
    // asm volatile(
    //     "blr x0\n\t"
    //     "mov x8, %0\n\t"
    //     "svc 0\n\t" ::"r"(SYSNUM_sigret):"w0");
}

void default_sig_handler() {
    sys_kill_pid(0, curr_thread->pid);
}