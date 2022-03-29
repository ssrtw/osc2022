#include "exec.h"

#include "malloc.h"
#include "uart.h"

#define USER_STACK_SIZE 0x8000

void __do_exec(void *text_addr, void *stack_addr);

void cpio_exec(void *filename, void *user_task_addr){
    void *stack_top = malloc_size(USER_STACK_SIZE);
    __do_exec(user_task_addr, stack_top);
}

void __do_exec(void *text_addr, void *stack_addr) {
    asm volatile(
        // x0 is uesr code address
        "msr elr_el1, %0\n\t"
        "mov x1, 0x3c0\n\t"
        "msr spsr_el1, x1\n\t"
        "msr sp_el0, %1\n\t"  // set user stack addr
        "eret\n\t" ::"r"(text_addr),
        "r"(stack_addr));
}
