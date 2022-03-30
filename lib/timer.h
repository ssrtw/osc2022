#ifndef _TIMER_H
#define _TIMER_H

#include "stddef.h"
// unsecure warudo no taimu address
#define CORE0_TIMER_IRQ_CTRL 0x40000040

void timer_enable();
void timer_disable();
void set_timer_interrupt_by_secs(size_t secs);
void set_timer_interrupt_by_tick(size_t tick);
void timer_handler();

#endif