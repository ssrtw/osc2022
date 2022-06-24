#ifndef THREAD_H
#define THREAD_H

#include "signal.h"
#include "stddef.h"

typedef struct thread_context {
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t fp;
    uint64_t lr;
    uint64_t sp;
    void *ttbr0_el1;
} thread_context_t;

typedef struct thread {
    list_head_t lh;
    thread_context_t cxt;  // aka thread_info
    uint32_t state;
    uint32_t pid;
    void *data;
    size_t data_size;
    void *ustack_ptr;
    void *kstack_ptr;
    list_head_t vma_list;
    sig_handler_t sig_handler[SIG_MAX];
    int sigcount[SIG_MAX];
    int sigcheck;
    thread_context_t sig_save_cxt;
    sig_handler_t curr_sig_handler;
} thread_t;

#endif