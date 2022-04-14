#ifndef _MALLOC_H
#define _MALLOC_H

#include "list.h"
#include "stddef.h"

#define MAXORDER      6
#define FRAME_START   0x10000000
#define FRAMES_COUNT  0x10000  // (0x2000_0000 ~ 0x1000_0000)/0x1000

typedef struct frame {
    list_head_t list_head;
    int val;  // val == order
    int used;
    uint32_t idx;
} frame_t;

frame_t* get_buddy(frame_t* frame);
frame_t* split_frame(frame_t* frame);
int combine_page(frame_t* frame);

void init_allocator();
void* malloc_size(size_t size);
void freepage(void* ptr);
void allocate_test();

#endif