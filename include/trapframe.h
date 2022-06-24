#ifndef TRAPFRAME_H
#define TRAPFRAME_H

#include "stddef.h"

typedef struct trapframe {
    uint64_t x[31];
    uint64_t spsr_el1;
    uint64_t elr_el1;
    uint64_t sp_el0;
} trapframe_t;

#endif