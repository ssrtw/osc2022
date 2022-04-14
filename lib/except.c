#include "except.h"

#include "event.h"
#include "timer.h"
#include "uart.h"

void enable_el1_interrupt() { asm volatile("msr DAIFClr, 0xf"); }
void disable_el1_interrupt() { asm volatile("msr DAIFSet, 0xf"); }

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
void sync_el0_handler(size_t x0) {
    size_t spsr_el1, elr_el1, esr_el1;
    asm volatile(
        "mrs %0, spsr_el1\n\t"
        "mrs %1, elr_el1\n\t"
        "mrs %2, esr_el1\n\t"
        : "=r"(spsr_el1), "=r"(elr_el1), "=r"(esr_el1)::"memory");
    uart_printf("from el0 -> el1, spsr_el1: %x, elr_el1: %x, esr_el1: %x\n", spsr_el1, elr_el1, esr_el1);
}

void irq_handler(void) {
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
    run_irq_event();
}