#include "compiler.h"
#ifndef _UART_H
#define _UART_H

void uart_init();
uint uart_read();
char uart_getc();
void uart_send(uint data);
void uart_puts(char* data);
void uart_puti(int in);
void uart_printf(char* format, ...);

#endif