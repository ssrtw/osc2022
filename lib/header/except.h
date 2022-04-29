#ifndef _EXCEPT_H
#define _EXCEPT_H

#include "gpio.h"
#include "stddef.h"

void lock();
void unlock();
void init_irq_event();
void enable_el1_interrupt();
void disable_el1_interrupt();
void inv_handler(size_t x0);
void irq_el1h_handler(size_t x0);
void sync_el0_handler(size_t x0);
void irq_el0_handler(size_t x0);
void irq_handler(void);

#endif