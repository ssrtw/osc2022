#ifndef _COMPILER_H
#define _COMPILER_H

#define WAITING(condition)   \
    do {                     \
        asm volatile("nop"); \
    } while (condition)

#define likely(x)   __builtin_expect(!!(x), 1)  // x很可能為真
#define unlikely(x) __builtin_expect(!!(x), 0)  // x很可能為假

#define __WRITE_ONCE(x, val)                 \
    do {                                     \
        *(volatile typeof(x) *)&(x) = (val); \
    } while (0)

#define WRITE_ONCE(x, val)                 \
    do {                                   \
        __WRITE_ONCE(x, val);              \
    } while (0)

#endif