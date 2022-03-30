#include "uart.h"

#include "compiler.h"
#include "except.h"
#include "stdarg.h"

#define INT_STR_LEN      10
#define UINT_HEX_LEN     8
#define UART_BUFFER_SIZE 0x100

// 四個cursor，用於ring的判定
uint32_t sended_cursor = 0, send_insert_cursor = 0, unread_cursor = 0, read_push_cursor = 0;
// 兩個ring
byte send_buffer[UART_BUFFER_SIZE] = {0};
byte read_buffer[UART_BUFFER_SIZE] = {0};

void uart_init() {
    register uint32_t r;
    // enable mini UART
    *AUX_ENABLE |= 1;
    // start setting, disable TX/RX
    *AUX_MU_IER = 0;
    *AUX_MU_LCR = 3;
    *AUX_MU_MCR = 0;
    *AUX_MU_CNTL = 0;
    *AUX_MU_BAUD = 270;
    *AUX_MU_IIR = 6;

    // need set GPIO 14-15 as ALT Func 5
    r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15));
    r |= (2 << 12) | (2 << 15);
    *GPFSEL1 = r;  // set alt5
    *GPPUD = 0;
    r = 150;
    WAITING(r--);
    *GPPUDCLK0 = (1 << 14) | (1 << 15);  // GPPUDCLK0 is 0..31 clock
    r = 150;
    WAITING(r--);
    *GPPUDCLK0 = 0;  // NO EFFECT

    // setup finish, enable TX/RX
    *AUX_MU_CNTL = 3;

    // WAITING(!(*AUX_MU_LSR & 0x01));  // clean rx noise
}

uint32_t uart_read() {
    WAITING(!(*AUX_MU_LSR & 0x01));
    return *AUX_MU_IO;
}

char uart_getc() {
    char c;
    // Data ready
    WAITING(!(*AUX_MU_LSR & 0x01));
    c = (char)(*AUX_MU_IO);
    return unlikely(c == '\r') ? '\n' : c;
}

void uart_send(uint32_t data) {
    // Transmitter idle
    WAITING(!(*AUX_MU_LSR & 0x20));
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

void uart_puti(int in) {
    char s[11] = {0};
    int i = 0;
    for (i = 0; i < 11; i++) {
        s[i] = in % 10;
        in /= 10;
        if (in == 0) break;
    }
    for (; i >= 0; i--) {
        uart_send(s[i] + '0');
    }
}

void uart_putx(uint32_t in) {
    char s[UINT_HEX_LEN] = {0};
    uchar i = UINT_HEX_LEN - 1;
    while (in) {
        s[i--] = in % 16;
        in /= 16;
    }
    i = 0;
    uart_puts("0x");
    while (i < UINT_HEX_LEN) {
        if (s[i] >= 10) {
            uart_send(0x37 + s[i]);
        } else {
            uart_send(0x30 + s[i]);
        }
        ++i;
    }
}

// https://stackoverflow.com/a/54352534/13177700
void uart_printf(char* format, ...) {
    va_list l;
    va_start(l, format);
    char *ptr, *s;
    int i;
    for (ptr = format; *ptr != '\0'; ptr++) {
        if (*ptr == '%') {
            ++ptr;
            switch (*ptr) {
                case 'd':
                    i = va_arg(l, int);
                    uart_puti(i);
                    break;
                case 'x':
                    i = va_arg(l, int);
                    uart_putx((uint32_t)i);
                    break;
                case 's':
                    s = va_arg(l, char*);
                    uart_puts(s);
                    break;
                case '%':
                    uart_send('%');
            }
        } else {
            if (*ptr == '\n') uart_send('\r');
            uart_send(*ptr);
        }
    }
    va_end(l);
}

void uart_async_puts(char* str) {
    while (*str) {
        if (unlikely(*str == '\n')) {
            uart_async_send('\r');
        }
        uart_async_send(*str++);
    }
}

void uart_async_send(uint32_t data) {
    // 把data送到buffer array
    while ((send_insert_cursor + 1) % UART_BUFFER_SIZE == sended_cursor) {
        // 滿了應該要先繼續送資料
        enable_uart_w_interrupt();
    }
    // critical section start
    disable_el1_interrupt();
    send_buffer[send_insert_cursor++] = data;
    if (send_insert_cursor >= UART_BUFFER_SIZE)
        send_insert_cursor = 0;  // ring
    enable_el1_interrupt();
    // critical section end

    // 可以開始送資料
    enable_uart_w_interrupt();
}

void uart_interrupt_w_handler() {
    if (send_insert_cursor == sended_cursor) {
        disable_uart_w_interrupt();
        return;
    }
    *AUX_MU_IO=send_buffer[sended_cursor++];
    if (sended_cursor >= UART_BUFFER_SIZE) {
        sended_cursor = 0;
    }
    enable_uart_w_interrupt();
}

byte uart_async_getc() {
    char c = uart_async_read();
    return unlikely(c == '\r') ? '\n' : c;
}

byte uart_async_read() {
    // 開始讀
    enable_uart_r_interrupt();
    while (read_push_cursor == unread_cursor)
        enable_uart_r_interrupt();
    // critical section start
    disable_el1_interrupt();
    byte data = read_buffer[unread_cursor++];
    if (unread_cursor == UART_BUFFER_SIZE) {
        unread_cursor = 0;
    }
    enable_el1_interrupt();
    // critical section end
    return data;
}

void uart_interrupt_r_handler() {
    // buffer滿了，先不要讀
    if ((read_push_cursor + 1) % UART_BUFFER_SIZE == unread_cursor) {
        disable_uart_r_interrupt();
        return;
    }
    read_buffer[read_push_cursor++] = (byte)*AUX_MU_IO;
    if (read_push_cursor >= UART_BUFFER_SIZE) {
        read_push_cursor = 0;
    }
    enable_uart_r_interrupt();
}

void enable_uart_interrupt() {
    *IRQs1 |= 1 << 29;
}

void disable_uart_interrupt() {
    disable_uart_r_interrupt();
    disable_uart_w_interrupt();
}

void enable_uart_r_interrupt() {
    *AUX_MU_IER |= 1;  // read interrupt
}

void enable_uart_w_interrupt() {
    *AUX_MU_IER |= 2;  // write interrupt
}

void disable_uart_r_interrupt() {
    *AUX_MU_IER &= ~(1);
}

void disable_uart_w_interrupt() {
    *AUX_MU_IER &= ~(2);
}

int uart_r_interrupt_is_enable() {
    return *AUX_MU_IER & 1;
}

int uart_w_interrupt_is_enable() {
    return *AUX_MU_IER & 2;
}