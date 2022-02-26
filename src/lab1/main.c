#include "compiler.h"
#include "uart.h"

void main(void) {
    uart_init();
    // echo hello world
    uart_puts("Hello World!\n");
    // echo back
    while (1) {
        uart_send(uart_getc());
    }
}