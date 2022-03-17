#include "c8763.h"
#include "uart.h"

// arg is dtb address(reg x0)
void main(void* arg) {
    void* __dtb = arg;
    uart_init();
    c8763_reader(__dtb);
}