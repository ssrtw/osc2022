#ifndef _LIST_H
#define _LIST_H

#include "compiler.h"

typedef struct list_head {
    struct list_head *next, *prev;
} list_head_t;

#define LIST_HEAD_INIT(name) \
    { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list;
    list->prev = list;
}

#define offsetof(type, member) ((size_t) & ((type *)0)->member)

#define container_of(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	((type *)(__mptr - offsetof(type, member))); })

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

static inline int list_empty(const struct list_head *head) {
    return head->next == head;
}

static inline int list_is_head(const struct list_head *list, const struct list_head *head) {
    return list == head;
}

static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    WRITE_ONCE(prev->next, new);
}

static inline void list_add(struct list_head *new, struct list_head *head) {
    __list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head) {
    __list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next) {
    next->prev = prev;
    prev->next = next;
}

static inline void list_del_entry(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
}

#define list_for_each(pos, head) \
    for (pos = (head)->next; !list_is_head(pos, (head)); pos = pos->next)

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_next_entry(pos, member) \
    list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_for_each_entry(pos, head, member)               \
    for (pos = list_first_entry(head, typeof(*pos), member); \
         &pos->member != (head);                             \
         pos = list_next_entry(pos, member))

#endif