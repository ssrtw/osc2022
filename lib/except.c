#include "except.h"

#include "event.h"
#include "mmu.h"
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

void inv_handler(uint64_t type, uint64_t esr, uint64_t elr, uint64_t spsr, uint64_t far) {
    // print out interruption type
    switch (type % 4) {
        case 0:
            uart_printf("Synchronous %d: \n", type);
            break;
        case 1:
            uart_printf("IRQ %d: \n", type);
            break;
        case 2:
            uart_printf("FIQ %d: \n", type);
            break;
        case 3:
            uart_printf("SError %d: \n", type);
            break;
    }
    uart_puts(": ");
    // decode exception type (some, not all. See ARM DDI0487B_b chapter D10.2.28)
    switch (esr >> 26) {
        case 0b000000:
            uart_puts("Unknown");
            break;
        case 0b000001:
            uart_puts("Trapped WFI/WFE");
            break;
        case 0b001110:
            uart_puts("Illegal execution");
            break;
        case 0b010101:
            uart_puts("System call");
            break;
        case 0b100000:
            uart_puts("Instruction abort, lower EL");
            break;
        case 0b100001:
            uart_puts("Instruction abort, same EL");
            break;
        case 0b100010:
            uart_puts("Instruction alignment fault");
            break;
        case 0b100100:
            uart_puts("Data abort, lower EL");
            break;
        case 0b100101:
            uart_puts("Data abort, same EL");
            break;
        case 0b100110:
            uart_puts("Stack alignment fault");
            break;
        case 0b101100:
            uart_puts("Floating point");
            break;
        default:
            uart_puts("Unknown");
            break;
    }
    // decode data abort cause
    if (esr >> 26 == 0b100100 || esr >> 26 == 0b100101) {
        uart_puts(", ");
        switch ((esr >> 2) & 0x3) {
            case 0:
                uart_puts("Address size fault");
                break;
            case 1:
                uart_puts("Translation fault");
                break;
            case 2:
                uart_puts("Access flag fault");
                break;
            case 3:
                uart_puts("Permission fault");
                break;
        }
        switch (esr & 0x3) {
            case 0:
                uart_puts(" at level 0");
                break;
            case 1:
                uart_puts(" at level 1");
                break;
            case 2:
                uart_puts(" at level 2");
                break;
            case 3:
                uart_puts(" at level 3");
                break;
        }
    }
    // dump registers
    uart_puts(":\n  ESR_EL1 ");
    uart_putX(esr);
    uart_puts(" ELR_EL1 ");
    uart_putX(elr);
    uart_puts("\n SPSR_EL1 ");
    uart_putX(spsr);
    uart_puts(" FAR_EL1 ");
    uart_putX(far);
    uart_puts("\n");
    // no return from exception for now
    while (1)
        ;
}

// user proc system call
void sync_el0_handler(trapframe_t* tf, uint64_t esr) {
    esr_el1_t* esr_strcut = (esr_el1_t*)&esr;
    if (esr_strcut->ec == DATA_ABORT_LOWER || esr_strcut->ec == INS_ABORT_LOWER) {
        handle_abort(esr_strcut);
        return;
    }
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