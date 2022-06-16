#include "user.h"
#include "syscall.h"
#include "uart.h"

void exec_test() {
    uart_printf("\nExec Test, pid\n");
    asm volatile("svc 0\n\t");
}