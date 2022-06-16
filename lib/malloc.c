#include "malloc.h"
// #define KERNEL_TRACE

#include "cpio.h"
#include "dtb.h"
#include "except.h"
#include "uart.h"
#include "util.h"

extern byte __kernel_start;                   // get linker verible
extern byte __kernel_end;                     // get linker verible
extern byte __heap_start;                     // get linker verible
static void* heap_top = &__heap_start;        // set to heap start address
static void* kernel_start = &__kernel_start;  // set to heap start address
static void* kernel_end = &__kernel_end;      // set to heap start address
static frame_t* frames;
static list_head_t free_list[MAXORDER + 1];
static list_head_t bin_list[MAXBINORDER + 1];

void* alloc_page(uint32_t size);
void free_page(void* ptr);
void from_page_get_bins(uint32_t bin_val);
void* alloc_bin(uint32_t size);
void free_bin(void* ptr);
void reserve_page(size_t start, size_t end);

void* simple_malloc(size_t size) {
    void* curr = heap_top;
    heap_top = (void*)align_up((uint64_t)heap_top + size, 0x10);
    return curr;
}

void init_allocator() {
    frames = simple_malloc(FRAMES_COUNT * sizeof(frame_t));
    uart_printf("frames addr:%x\n", frames);
    // init freelist
    for (int i = 0; i <= MAXORDER; i++) {
        INIT_LIST_HEAD(&free_list[i]);
    }
    // init binlist
    for (int i = 0; i <= MAXBINORDER; i++) {
        INIT_LIST_HEAD(&bin_list[i]);
    }
    // set maxframe init
    for (int i = 0; i < FRAMES_COUNT; i++) {
        INIT_LIST_HEAD(&frames[i].list_head);
        frames[i].idx = i;
        frames[i].bin_val = -1;

        // set orders
        // append max frame to freelist
        if (i % (1 << MAXORDER) == 0) {
            frames[i].val = 6;
            frames[i].used = 0;
            list_add(&frames[i].list_head, &free_list[MAXORDER]);
        }
    }
#ifdef KERNEL_TRACE
    uart_printf("init_allocator. page count: %x, frame start: %x\n", FRAMES_COUNT, frames);
#endif
    // spin-table
    reserve_page(0x0, 0x1000);
    // kernel image size
    reserve_page((size_t)kernel_start, (size_t)kernel_end);
    // initramfs
    reserve_page(cpio_ramfs, cpio_ramfs_end);
    // dtb reserve(spin table?)
    dtb_reserve_mem(reserve_page);
    // simple_malloc(all malloc frames)
    reserve_page(&__heap_start, heap_top);
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

void* alloc_page(uint32_t size) {
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
        return NULL;
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
    frame_t* target_frame = (frame_t*)free_list[canuse_val].next;  // get can use frame
    list_del_entry(&target_frame->list_head);
    for (int j = canuse_val; j > val; j--) {
#ifdef KERNEL_TRACE
        uart_printf("frame val:%d\n", j);
#endif
        target_frame = split_frame(target_frame);
    }
    target_frame->used = 1;
#ifdef KERNEL_TRACE
    uart_printf("canuse val:%d\n", canuse_val);
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

void free_page(void* ptr) {
    frame_t* target_frame = &frames[((size_t)ptr - FRAME_START) >> 12];
#ifdef KERNEL_TRACE
    uart_printf("freepage: frame idx=%d, val=%d\n", target_frame->idx, target_frame->val);
#endif

    target_frame->used = 0;

    while (combine_page(target_frame) == 0)
        ;
    list_add(&target_frame->list_head, &free_list[target_frame->val]);
}

void from_page_get_bins(uint32_t bin_val) {
    void* page = alloc_page(0x1000);

    int binsize = (0x20 << bin_val);
    // 設置這個frame他當bin的大小
    frames[((size_t)page - FRAME_START) >> 12].bin_val = bin_val;
    for (int i = 0; i < 0x1000; i += binsize) {
        list_head_t* bin = (list_head_t*)((byte*)page + i);
        list_add(bin, &bin_list[bin_val]);
    }
}

void* alloc_bin(uint32_t size) {
    int bin_val = 0;
    for (int i = 0; i <= MAXBINORDER; i++) {
        if (size <= (0x20 << i)) {
            bin_val = i;
            break;
        }
    }

    if (list_empty(&bin_list[bin_val])) {
#ifdef KERNEL_TRACE
        uart_puts("alloc page to store bins\n");
#endif
        from_page_get_bins(bin_val);
    }
    list_head_t* bin = bin_list[bin_val].next;
#ifdef KERNEL_TRACE
    uart_printf("before alloc bin:%x, %x, %x\n", bin, bin->next, bin->prev);
#endif
    list_del_entry(bin);
#ifdef KERNEL_TRACE
    uart_printf("after alloc bin:%x, %x, %x\n", bin_list[bin_val].next, bin_list[bin_val].next->next, bin_list[bin_val].next->prev);
#endif
    return (void*)bin;
}

void free_bin(void* ptr) {
    list_head_t* bin = (list_head_t*)ptr;
    frame_t* frame = &frames[((size_t)ptr - FRAME_START) >> 12];
    list_add(bin, &bin_list[frame->bin_val]);
#ifdef KERNEL_TRACE
    uart_printf("free bin addr=%x, val=%d\n", ptr, frame->bin_val);
#endif
}

void* kmalloc(uint32_t size) {
    void* ptr;
    // new a bin size
    if (size <= (0x20 << MAXBINORDER)) {
#ifdef KERNEL_TRACE
        uart_puts("alloc bin\n");
#endif
        ptr = alloc_bin(size);
        return ptr;
    }
#ifdef KERNEL_TRACE
    uart_puts("alloc page\n");
#endif
    // malloc from page
    ptr = alloc_page(size);
    return ptr;
}

void kfree(void* ptr) {
    frame_t* curr_frame = &frames[((size_t)ptr - FRAME_START) >> 12];
    if ((size_t)ptr % 0x1000 != 0 && curr_frame->bin_val != -1) {
#ifdef KERNEL_TRACE
        uart_puts("free bin\n");
#endif
        free_bin(ptr);
    } else {
#ifdef KERNEL_TRACE
        uart_puts("free page\n");
#endif
        free_page(ptr);
    }
}

void reserve_page(size_t start, size_t end) {
    lock();
    // ------------------
    // |   |s|      |e| |
    // |s|            |e|
    // ------------------
    start &= ~0xfff;              // 4k align
    end = align_up(end, 0x1000);  // 4k align
    // check all list, if reserve page in list, delete them.
    for (int val = MAXORDER; val >= 0; val--) {
        list_head_t* curr;
        list_for_each(curr, &free_list[val]) {
            size_t page_start = (((frame_t*)curr)->idx << 12) + FRAME_START;
            size_t nxt_page_start = page_start + (0x1000 << val);
            if (start <= page_start && end >= nxt_page_start) {
                ((frame_t*)curr)->used = 1;
                list_del_entry(curr);
#ifdef KERNEL_TRACE
                uart_printf("reserve addr: %x\n", page_start);
#endif
            } else if (start >= nxt_page_start || end <= page_start) {
                continue;
            } else {
#ifdef KERNEL_TRACE
                uart_printf("reserve range: %x~%x, order=%d\n", page_start, nxt_page_start, ((frame_t*)curr)->val);
#endif
                // next foreach use
                list_del_entry(curr);
                list_head_t* prev = curr->prev;
                // split page, next forloop process two page
                curr = split_frame((frame_t*)curr);
                list_add(&((frame_t*)curr)->list_head, &free_list[val - 1]);
                curr = prev;
            }
        }
    }
    unlock();
}

void allocate_test() {
    uart_printf("%x", 0x4200 & (~0xfff));  // 4k align
    byte* p1 = kmalloc(0x1000);            // 1=0001, order=0
    byte* p5 = kmalloc(0x1001);            // 1=0001, but > 4k, order=1
    byte* p2 = kmalloc(0x4000);            // 4=0100, order=2
    byte* p3 = kmalloc(0x8000);            // 8=1000, order=3
    byte* p4 = kmalloc(0x3000);            // 3=0011, order=2
    kfree(p1);
    kfree(p5);
    kfree(p2);
    kfree(p3);
    kfree(p4);
    byte* bin1 = kmalloc(32);
    bin1[0] = 'a';
    bin1[1] = '\n';
    bin1[2] = '\0';
    byte* bin2 = kmalloc(30);
    uart_printf("%x, %x\n", bin1, bin2);
    uart_printf("%s, %s\n", bin1, bin2);
    byte* bin3 = kmalloc(64);
    kfree(bin2);
    bin2 = kmalloc(69);
    byte* bin4 = kmalloc(256);
    kfree(bin4);
    bin4 = kmalloc(222);
    byte* bin5 = kmalloc(763);  // >=512, alloc a page
    byte* bin6 = kmalloc(500);
    kfree(bin1);
    kfree(bin2);
    kfree(bin3);
    kfree(bin4);
    kfree(bin5);
    kfree(bin6);
}