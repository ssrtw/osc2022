#ifndef _EXCEPT_H
#define _EXCEPT_H

#include "stddef.h"

void inv_handler(size_t x0);
void irq_el1h_handler(size_t x0);
void sync_el0_handler(size_t x0);
void irq_el0_handler(size_t x0);

#endif