#include "malloc.h"

#include "util.h"

extern byte __heap_start;               // get linker verible
static void* heap_top = &__heap_start;  // set to heap start address

void* malloc_size(size_t size) {
    void* curr = heap_top;
    curr += size;
    heap_top = (void*)align_up((uint64_t)curr, 0x10);
    return curr;
}