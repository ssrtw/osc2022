#ifndef _UTIL_H
#define _UTIL_H

#include "stddef.h"

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

// https://www.microchip.com/forums/m1095553.aspx
#define STR(x)  #x
#define XSTR(s) STR(s)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Reference from https://elixir.bootlin.com/linux/latest/source/tools/lib/perf/mmap.c#L299
#define read_sysreg(r) ({                       \
    uint64_t __val;                               \
    asm volatile("mrs %0, " #r : "=r" (__val)); \
    __val;                                      \
})

// Reference from https://elixir.bootlin.com/linux/latest/source/arch/arm64/include/asm/sysreg.h#L1281
#define write_sysreg(r, v) do {    \
    uint64_t __val = (uint64_t)(v);          \
    asm volatile("msr " #r ", %x0" \
             : : "rZ" (__val));    \
} while (0)

uint64_t align_up(uint64_t n, uint64_t align);

#endif