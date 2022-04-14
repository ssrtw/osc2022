#include "malloc.h"
#define KERNEL_TRACE

#include "except.h"
#include "uart.h"
#include "util.h"

extern byte __heap_start;               // get linker verible
static void* heap_top = &__heap_start;  // set to heap start address
static frame_t* frames;
static list_head_t free_list[MAXORDER + 1];

void* malloc_size(size_t size) {
    void* curr = heap_top;
    heap_top = (void*)align_up((uint64_t)heap_top + size, 0x10);
    return curr;
}

void init_allocator() {
    frames = malloc_size(FRAMES_COUNT * sizeof(frame_t));
    uart_printf("frames addr:%x\n", frames);
    // init freelist
    for (int i = 0; i <= MAXORDER; i++) {
        INIT_LIST_HEAD(&free_list[i]);
    }
    // set maxframe init
    for (int i = 0; i < FRAMES_COUNT; i++) {
        INIT_LIST_HEAD(&frames[i].list_head);
        frames[i].idx = i;

        // set orders
        // append max frame to freelist
        if (i % (1 << MAXORDER)) {
            frames[i].val = 6;
            frames[i].used = 0;
            list_add(&frames[i].list_head, &free_list[MAXORDER]);
        }
    }
#ifdef KERNEL_TRACE
    uart_printf("init_allocator. page count: %x, frame start: %x\n", FRAMES_COUNT, frames);
#endif
}

frame_t* get_buddy(frame_t* frame) {
    return &frames[frame->idx ^ (1 << frame->val)];
}

frame_t* split_frame(frame_t* frame) {
    frame->val -= 1;
    frame_t* aibou = get_buddy(frame);
    aibou->used = 0;
    aibou->val = frame->val;
    list_add(&aibou->list_head, &free_list[aibou->val]);
    return frame;
}

void* allocpage(uint32_t size) {
    int val = 0;

    // 如果先減1要再想想，好像不太行
    // uint32_t i = size >> 12;
    // i -= 1;
    // while (i != 0) {
    //     i >>= 1;
    //     ++val;
    // }

    for (int i = 0; i <= MAXORDER; i++) {
        if (size <= (0x1000 << i)) {
            val = i;
            break;
        }
    }
    if (val > MAXORDER) {
        uart_puts("Size is too large, Stanley is 500kg!\n");
    }
    int canuse_val;
    for (canuse_val = val; canuse_val <= MAXORDER; canuse_val++) {
        if (!list_empty(&free_list[canuse_val])) {
            break;
        }
    }
    if (canuse_val > MAXORDER) {
        uart_puts("Current free memory not exist!\n");
    }
    uart_printf("canuse val:%d\n", canuse_val);
    frame_t* target_frame = (frame_t*)free_list[canuse_val].next;  // get can use frame
    uart_printf("free_list addr:%x\n", free_list[canuse_val].next);
    list_del_entry(&target_frame->list_head);
    for (int j = canuse_val; j > val; j--) {
        uart_printf("frame val:%d\n", j);
        target_frame = split_frame(target_frame);
    }
    target_frame->used = 1;
#ifdef KERNEL_TRACE
    uart_printf("alloc size: %x, alloc addr: %x, page val: %d\n", size, (void*)(FRAME_START + (size_t)(0x1000 * (target_frame->idx))), target_frame->val);
#endif
    return (void*)(FRAME_START + (size_t)(0x1000 * (target_frame->idx)));
}

int combine_page(frame_t* frame) {
    frame_t* buddy = get_buddy(frame);

    if (frame->val == MAXORDER)
        return -1;
    if (buddy->val != frame->val)
        return -1;
    if (buddy->used == 1)
        return -1;
    list_del_entry((struct list_head*)buddy);  // 活的buddy從free刪了，留給當前的用
    frame->val += 1;
#ifdef KERNEL_TRACE
    uart_printf("combine page: idx(%d, %d), val=%d\n", frame->idx, buddy->idx, frame->val);
#endif
    return 0;
}

void freepage(void* ptr) {
    frame_t* target_frame = &frames[((size_t)ptr - FRAME_START) >> 12];
#ifdef KERNEL_TRACE
    uart_printf("freepage: frame idx=%d, val=%d\n", target_frame->idx, target_frame->val);
#endif

    target_frame->used = 0;

    while (combine_page(target_frame) == 0)
        ;
    list_add(&target_frame->list_head, &free_list[target_frame->val]);
}

void allocate_test() {
    byte* p1 = allocpage(0x1000);  // 1=0001, order=0
    byte* p5 = allocpage(0x1001);  // 1=0001, but > 4k, order=1
    byte* p2 = allocpage(0x4000);  // 4=0100, order=2
    byte* p3 = allocpage(0x8000);  // 8=1000, order=3
    byte* p4 = allocpage(0x3000);  // 3=0011, order=2
    freepage(p1);
    freepage(p5);
    freepage(p2);
    freepage(p3);
    freepage(p4);
}