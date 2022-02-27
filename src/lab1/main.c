#include "compiler.h"
#include "shell.h"
#include "uart.h"

void main(void) {
    uart_init();
    shell();
}