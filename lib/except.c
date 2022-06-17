#include "except.h"

#include "event.h"
#include "sched.h"
#include "signal.h"
#include "syscall.h"
#include "timer.h"
#include "uart.h"
#include "util.h"

void enable_el1_interrupt() { asm volatile("msr DAIFClr, 0xf"); }
void disable_el1_interrupt() { asm volatile("msr DAIFSet, 0xf"); }

static size_t lock_counter = 0;

void lock() {
    lock_counter += 1;
    disable_el1_interrupt();
}

void unlock() {
    lock_counter -= 1;
    if (lock_counter == 0) {
        enable_el1_interrupt();
    }
}

void inv_handler(size_t x0) {
    size_t elr_el1;
    asm volatile(
        "mrs %0, ELR_EL1\n\t"
        : "=r"(elr_el1)
        :
        : "memory");
    uart_printf("invalid user call address: %x\n", elr_el1);
    uart_printf("invalid exception no.: %x\n", x0);
    while (1)
        ;
}

// user proc system call
void sync_el0_handler(trapframe_t* tf) {
    syscall_handler(tf);
}

void irq_handler(trapframe_t* tf) {
    // https://blog.csdn.net/Roland_Sun/article/details/105547271
    size_t cntp_ctl_el0;
    asm volatile("mrs %0, cntp_ctl_el0"
                 : "=r"(cntp_ctl_el0));
    // if is timer interrupt, do timer handler, bit 3(istatus)
    if (cntp_ctl_el0 & 0b100 && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_PNSIRQ) {
        timer_disable();  // mask
        add_irq_event(timer_handler, 0);
        run_irq_event();
        timer_enable();
        // do schedule after add timer task
        // if has more than one task, do schedule
        if (rq->next->next != rq) {
            schedule();
        }
    }
    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) {
        // IIR[2:1]==01, Transmit holding register empty
        if (*AUX_MU_IIR & 0b010) {
            disable_uart_w_interrupt();  // mask
            add_irq_event(uart_interrupt_w_handler, 1);
            run_irq_event();
        }
        // IIR[2:1]==10, Receiver holds valid byte
        if (*AUX_MU_IIR & 0b100) {
            disable_uart_r_interrupt();  // mask
            // 如果收到資料，放到buff上
            add_irq_event(uart_interrupt_r_handler, 1);
            run_irq_event();
        }
    }
    // if user, https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/SPSR-EL1--Saved-Program-Status-Register--EL1-
    if ((tf->spsr_el1 & 0b1100) == 0) {
        check_sig(tf);
    }
}