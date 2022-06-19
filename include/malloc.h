#ifndef _MALLOC_H
#define _MALLOC_H

#include "list.h"
#include "mmu.h"
#include "stddef.h"

#define MAXORDER     6
#define MAXBINORDER  4                  // bin val, base byte size: 32(0x20)
#define FRAME_START  PHYS_TO_VIRT(0x0)  // 0x10000000
#define FRAMES_COUNT 0x3c000            // (0x0000_0000 ~ 0x3c00_0000)/0x1000

typedef struct frame {
    list_head_t list_head;
    int val;  // val == order
    int used;
    int bin_val;
    uint32_t idx;
} frame_t;

frame_t* get_buddy(frame_t* frame);
frame_t* split_frame(frame_t* frame);
int combine_page(frame_t* frame);

void init_allocator();
void* kmalloc(uint32_t size);
void kfree(void* ptr);
void* simple_malloc(size_t size);
void allocate_test();

#endif