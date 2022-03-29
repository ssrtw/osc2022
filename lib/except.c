#include "except.h"

#include "uart.h"

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