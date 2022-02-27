#include "uart.h"

#include "compiler.h"
#include "gpio.h"

void uart_init() {
    // enable mini UART
    *AUX_ENABLE |= 1;
    // start setting, disable TX/RX
    *AUX_MU_IER = 0;
    *AUX_MU_LCR = 3;
    *AUX_MU_MCR = 0;
    *AUX_MU_CNTL = 0;
    *AUX_MU_BAUD = 270;
    *AUX_MU_IIR = 6;
    // setup finish, enable TX/RX
    *AUX_MU_CNTL = 3;
}

uint uart_read() {
    while (!(*AUX_MU_LSR & 0x01)) {
        asm volatile("nop");
    }
    return *AUX_MU_IO;
}

char uart_getc() {
    char c;
    // Data ready
    while (!(*AUX_MU_LSR & 0x01)) {
        asm volatile("nop");
    }
    c = (char)(*AUX_MU_IO);
    return unlikely(c == '\r') ? '\n' : c;
}

void uart_send(uint data) {
    // Transmitter idle
    while (!(*AUX_MU_LSR & 0x20)) {
        asm volatile("nop");
    }
    *AUX_MU_IO = data;
}

void uart_puts(char* data) {
    while (*data) {
        if (unlikely(*data == '\n')) {
            uart_send('\r');
        }
        uart_send(*data++);
    }
}
