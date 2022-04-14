#ifndef _EVENT_H
#define _EVENT_H

#include "list.h"
#include "stddef.h"

typedef struct irq_event {
    struct list_head list_head;
    void (*func)(void);
    size_t priority;
} irq_event_t;

void init_irq_event();
void add_irq_event(void *func, size_t priority);
void run_irq_event();

#endif