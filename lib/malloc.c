#include "malloc.h"

extern byte __heap_start;               // get linker verible
static void* heap_top = &__heap_start;  // set to heap start address

void* malloc_size(size_t size) {
    void* curr = heap_top;
    heap_top += size;
    return curr;
}