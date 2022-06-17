#ifndef _EXCEPT_H
#define _EXCEPT_H

#include "gpio.h"
#include "stddef.h"

typedef struct trapframe {
    uint64_t x[31];
    uint64_t spsr_el1;
    uint64_t elr_el1;
    uint64_t sp_el0;
} trapframe_t;

void lock();
void unlock();
void init_irq_event();
void enable_el1_interrupt();
void disable_el1_interrupt();
void inv_handler(size_t x0);
void irq_el1h_handler(size_t x0);
void sync_el0_handler(trapframe_t* tp);
void irq_el0_handler(size_t x0);
void irq_handler(trapframe_t* tf);

#endif