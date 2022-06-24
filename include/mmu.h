#ifndef _MMU_H
#define _MMU_H

#define ENTRY_ADDR_MASK 0xfffffffff000L

#define PHYS_TO_VIRT(x) (x + 0xffff000000000000)
#define VIRT_TO_PHYS(x) (x - 0xffff000000000000)

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB          ((0b00 << 14) | (0b10 << 30))
#define TCR_CONFIG_DEFAULT      (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE      0b00000000
#define MAIR_NORMAL_NOCACHE     0b01000100
#define MAIR_IDX_DEVICE_nGnRnE  0
#define MAIR_IDX_NORMAL_NOCACHE 1

#define BOOT_PGD_ADDR 0x1000
#define BOOT_PUD_ADDR 0x2000
#define BOOT_PMD_ADDR 0x3000

#define DATA_ABORT_LOWER 0b100100
#define INS_ABORT_LOWER  0b100000

#define BOOT_PGD ((pd_t *)BOOT_PGD_ADDR)
#define BOOT_PUD ((pd_t *)BOOT_PUD_ADDR)
#define BOOT_PMD ((pd_t *)BOOT_PMD_ADDR)

#define PD_TABLE     0b11L
#define PD_BLOCK     0b01L
#define PD_UNX       (1L << 54)
#define PD_KNX       (1L << 53)
#define PD_ACCESS    (1L << 10)
#define PD_UK_ACCESS (1L << 6)
#define PD_RDONLY    (1L << 7)

// 怎ㄇ沒有跟linux一樣= =
// #define PROT_R        0x4
// #define PROT_W        0x2
// #define PROT_X        0x1

#define PROT_X        0x4
#define PROT_W        0x2
#define PROT_R        0x1

#define PG_NEED_ALLOC 0x1
#define PG_DONT_ALLOC 0x0

#define TF_LEVEL0 0b000100
#define TF_LEVEL1 0b000101
#define TF_LEVEL2 0b000110
#define TF_LEVEL3 0b000111

#ifndef __ASSEMBLER__

#include "list.h"
#include "thread.h"

typedef struct vm_area_struct {
    list_head_t lh;
    uint64_t virt_addr;
    uint64_t phys_addr;
    uint64_t area_size;
    uint64_t protected;
    uint32_t allocated;
    // TODO: 多做檢查參照次數
    // uint32_t ref_cnt;
} vm_area_struct_t;

typedef struct {
    // TODO: 換成uint64_t?
    uint32_t iss : 25,  // Instruction specific syndrome
        il : 1,         // Instruction length bit
        ec : 6;         // Exception class
} esr_el1_t;

typedef uint64_t pd_t;

void add_vma(thread_t *thread, uint64_t virt_addr, uint64_t size, uint64_t phys_addr, uint64_t protected, uint64_t allocated);
void handle_abort(esr_el1_t *esr_el1);
void mapping_set_prot(pd_t *pgd_p, size_t virt_addr, size_t phys_addr, size_t protected);
void mapping(pd_t *pgd, size_t virt_addr, size_t phys_addr, size_t protected);
void map_pages(size_t *virt_pgd_p, size_t va, size_t size, size_t pa, size_t flag);
void free_pages(size_t *page_table, int level);

#endif

#endif