#include "c8763.h"
#include "uart.h"

void main(void) {
    uart_init();
    c8763_reader();
}