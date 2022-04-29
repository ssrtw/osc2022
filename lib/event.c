#include "event.h"

#include "except.h"
#include "malloc.h"
#include "uart.h"

struct list_head *irq_event_head;
size_t curr_priority = 0x10000;  // priority

void init_irq_event() {
    irq_event_head = simple_malloc(sizeof(irq_event_t));
    INIT_LIST_HEAD(irq_event_head);
}

void add_irq_event(void *func, size_t priority) {
    irq_event_t *new_event = (irq_event_t *)simple_malloc(sizeof(irq_event_t));
    new_event->func = func;
    new_event->priority = priority;
    INIT_LIST_HEAD(&new_event->list_head);
    struct list_head *curr;
    // critical section start
    lock();
    list_for_each(curr, irq_event_head) {
        if (((irq_event_t *)curr)->priority > priority) {  // 小priority=>高優先
            list_add(&new_event->list_head, curr->prev);
            break;
        }
    }
    if (list_is_head(curr, irq_event_head)) {
        list_add_tail(&new_event->list_head, irq_event_head);
    }
    unlock();
    // critical section end
}

void run_irq_event() {
    while (!list_empty(irq_event_head)) {
        // critical section start
        lock();
        irq_event_t *nxt_event = (irq_event_t *)(irq_event_head->next);
        if (curr_priority <= nxt_event->priority) {
            unlock();
            break;
        }
        list_del_entry(irq_event_head->next);
        // kfree(irq_event_head);
        // handle nest proirity
        int prev_priority = curr_priority;
        curr_priority = nxt_event->priority;
        unlock();
        // critical section end
        nxt_event->func();
        lock();
        curr_priority = prev_priority;
        unlock();
    }
}