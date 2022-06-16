#ifndef _UART_H
#define _UART_H

#include "gpio.h"
#include "stddef.h"

#define INT_REG_BASE            MMIO_BASE + 0x0000b000                             // 2835 page.112
#define IRQ_PENDING_1           ((volatile uint32_t*)(INT_REG_BASE + 0x00000204))  // 2835 page.112
#define IRQs1                   ((volatile uint32_t*)(INT_REG_BASE + 0x00000210))  // 2835 page.112
#define IRQ_PENDING_1_AUX_INT   (1 << 29)                                          // 2835 page.113
#define CORE0_INTERRUPT_SOURCE  ((volatile uint32_t*)(0x40000060))                 // 2836 page. 16
#define INTERRUPT_SOURCE_GPU    (1 << 8)                                           // 2836 page. 16
#define INTERRUPT_SOURCE_PNSIRQ (1 << 1)                                           // 2836 page. 16

// BCM2837 peripheral: https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf#page=8
// AUX UART
#define AUX_ENABLE     ((volatile uint32_t*)(MMIO_BASE + 0x00215004))  // Auxiliary enables
#define AUX_MU_IO      ((volatile uint32_t*)(MMIO_BASE + 0x00215040))  // Mini Uart I/O Data
#define AUX_MU_IER     ((volatile uint32_t*)(MMIO_BASE + 0x00215044))  // Mini Uart Interrupt Enable
#define AUX_MU_IIR     ((volatile uint32_t*)(MMIO_BASE + 0x00215048))  // Mini Uart Interrupt Identify
#define AUX_MU_LCR     ((volatile uint32_t*)(MMIO_BASE + 0x0021504C))  // Mini Uart Line Control
#define AUX_MU_MCR     ((volatile uint32_t*)(MMIO_BASE + 0x00215050))  // Mini Uart Modem Control
#define AUX_MU_LSR     ((volatile uint32_t*)(MMIO_BASE + 0x00215054))  // Mini Uart Line Status
#define AUX_MU_MSR     ((volatile uint32_t*)(MMIO_BASE + 0x00215058))  // Mini Uart Modem Status
#define AUX_MU_SCRATCH ((volatile uint32_t*)(MMIO_BASE + 0x0021505C))  // Mini Uart Scratch
#define AUX_MU_CNTL    ((volatile uint32_t*)(MMIO_BASE + 0x00215060))  // Mini Uart Extra Control
#define AUX_MU_STAT    ((volatile uint32_t*)(MMIO_BASE + 0x00215064))  // Mini Uart Extra Status
#define AUX_MU_BAUD    ((volatile uint32_t*)(MMIO_BASE + 0x00215068))  // Mini Uart Baudrate

void uart_init();
uint32_t uart_read();
char uart_getc();
void uart_getn(char* buf, size_t n);
void uart_send(uint32_t data);
void uart_putn(const char * buf, size_t n);
void uart_puts(const char * data);
void uart_puti(int in);
void uart_putx(uint32_t in);
void uart_printf(char* format, ...);
void uart_async_puts(char* str);
void uart_async_send(uint32_t data);
void uart_interrupt_w_handler();
byte uart_async_getc();
byte uart_async_read();
void uart_interrupt_r_handler();
void enable_uart_interrupt();
void disable_uart_interrupt();
void enable_uart_r_interrupt();
void enable_uart_w_interrupt();
void disable_uart_r_interrupt();
void disable_uart_w_interrupt();
int uart_r_interrupt_is_enable();
int uart_w_interrupt_is_enable();

#endif