#ifndef _TIMER_H
#define _TIMER_H

#include "list.h"
#include "stddef.h"
// unsecure warudo no taimu address
#define CORE0_TIMER_IRQ_CTRL 0x40000040

typedef void (*timer_callback)(char *args);

typedef struct timer_task {
    struct list_head list_head;
    timer_callback func;
    size_t expire;
    char *args;
} timer_task_t;

void timer_enable();
void timer_disable();
void set_timer_interrupt_by_secs(size_t secs);
void set_timer_interrupt_by_tick(size_t tick);
void two_sec_callback(char *args);
void timer_alert_callback(char *args);
void timer_handler();
void init_task_list();
size_t get_expire_tick_from_secs(size_t seconds);
void add_timer_task(timer_callback func, size_t seconds, char *args);

#endif