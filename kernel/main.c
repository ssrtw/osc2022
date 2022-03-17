#include "compiler.h"
#include "dtb.h"
#include "shell.h"
#include "uart.h"

void main(void* __dtb) {
    dtb_addr = __dtb;
    uart_init();
    // init cpio addr
    fdt_traverse(fdt_callback_initramfs);
    shell();
}