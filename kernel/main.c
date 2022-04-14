#include "compiler.h"
#include "dtb.h"
#include "event.h"
#include "except.h"
#include "malloc.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

void main(void* __dtb) {
    dtb_addr = __dtb;
    uart_init();
    init_allocator();
    enable_el1_interrupt();
    enable_uart_interrupt();
    init_task_list();
    init_irq_event();
    timer_enable();
    // init cpio addr
    fdt_traverse(fdt_callback_initramfs);
    shell();
}