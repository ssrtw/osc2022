#include "cpio.h"
#include "dtb.h"
#include "event.h"
#include "except.h"
#include "malloc.h"
#include "mmu.h"
#include "sched.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

void main(void* __dtb) {
    dtb_addr = PHYS_TO_VIRT(__dtb);
    uart_init();
    // init cpio addr
    fdt_traverse(fdt_callback_initramfs);
    init_allocator();
    enable_el1_interrupt();
    enable_uart_interrupt();
    init_threads();
    init_timer_list();
    init_irq_event();
    timer_enable();
    cpio_traverse("vm.img", cpio_exec);
    // shell();
}