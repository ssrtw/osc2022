#ifndef _EXCEPT_H
#define _EXCEPT_H

#include "gpio.h"
#include "stddef.h"
#include "trapframe.h"

void lock();
void unlock();
void init_irq_event();
void enable_el1_interrupt();
void disable_el1_interrupt();
void inv_handler(uint64_t type, uint64_t esr, uint64_t elr, uint64_t spsr, uint64_t far);
void irq_el1h_handler(size_t x0);
void sync_el0_handler(trapframe_t* tf, uint64_t esr);
void irq_el0_handler(size_t x0);
void irq_handler(trapframe_t* tf);

#endif