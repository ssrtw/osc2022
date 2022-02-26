#ifndef _UART_H
#define _UART_H

void uart_init();
unsigned int uart_read();
char uart_getc();
void uart_send(unsigned int data);
void uart_puts(char* data);

#endif