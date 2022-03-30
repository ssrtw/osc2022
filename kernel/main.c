#include "compiler.h"
#include "dtb.h"
#include "except.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

void main(void* __dtb) {
    dtb_addr = __dtb;
    uart_init();
    enable_el1_interrupt();
    enable_uart_interrupt();
    // init cpio addr
    fdt_traverse(fdt_callback_initramfs);
    shell();
}