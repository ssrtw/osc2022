#include "mmu.h"

#include "malloc.h"
#include "sched.h"
#include "string.h"
#include "uart.h"
#include "util.h"
// block entry
#define PD_BE PD_BLOCK | PD_ACCESS

typedef uint64_t pd_t;

void seg_fault();

void kernel_mmu_init(int x0) {
    BOOT_PGD[0] = (uint64_t)BOOT_PUD | PD_TABLE;
    // 1st 1GB mapped by the 1st entry of PUD
    BOOT_PUD[0] = (uint64_t)BOOT_PMD | PD_TABLE;
    // 2nd 1GB mapped by the 2nd entry of PUD
    // 0x40000000 ~ 0x80000000: Device
    BOOT_PUD[1] = 0x40000000 | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BE;

    // 0x00000000 ~ 0x3f000000: Normal
    for (int i = 0; i < 504; i++) {
        BOOT_PMD[i] = (i * (1 << 21)) | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BE;
    }
    // 0x3f000000 ~ 0x40000000: Device
    for (int i = 504; i < 512; i++) {
        BOOT_PMD[i] = (i * (1 << 21)) | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BE;
    }
}

void add_vma(thread_t *thread, uint64_t virt_addr, uint64_t size, uint64_t phys_addr, uint64_t protected, uint64_t allocated) {
    size = size % 0x1000 ? size + (0x1000 - size % 0x1000) : size;
    vm_area_struct_t *new_vma = kmalloc(sizeof(vm_area_struct_t));
    new_vma->protected = protected;
    new_vma->area_size = size;
    new_vma->virt_addr = virt_addr;
    new_vma->phys_addr = phys_addr;
    new_vma->allocated = allocated;
    list_add_tail(&new_vma->lh, &thread->vma_list);
}

void free_pages(size_t *page_table, int level) {
    size_t *table = (size_t *)PHYS_TO_VIRT((char *)page_table);
    for (int i = 0; i < 512; i++) {
        // 檢查是不是有效page
        if ((table[i] & 0b1L) == 0) {
            size_t *next_table = (size_t *)(table[i] & ENTRY_ADDR_MASK);
            if (table[i] & PD_TABLE) {
                if (level != 2) free_pages(next_table, level + 1);
                table[i] = 0L;
                kfree(PHYS_TO_VIRT((char *)next_table));
            }
        }
    }
}

void handle_abort(esr_el1_t *esr_el1) {
    uint64_t far_el1;
    __asm__ __volatile__("mrs %0, FAR_EL1\n\t"
                         : "=r"(far_el1));
    vm_area_struct_t *pos;
    list_for_each_entry(pos, &curr_thread->vma_list, lh) {
        if (pos->virt_addr <= far_el1 && far_el1 <= pos->virt_addr + pos->area_size)
            break;
    }
    // 找不存在於vma list的地址，送seg fault
    if ((uint64_t)pos == (uint64_t)&curr_thread->vma_list) {
        uart_printf("seg fault at:%X\n", far_el1);
        seg_fault();
    }

    // 只處理 Translation fault
    byte ifsc = (esr_el1->iss & 0x3f);
    if (ifsc == TF_LEVEL0 || ifsc == TF_LEVEL1 || ifsc == TF_LEVEL2 || ifsc == TF_LEVEL3) {
        uart_printf("[Translation fault]: %X\r\n", far_el1);
        size_t addr = (far_el1 - pos->virt_addr);
        addr = (addr % 0x1000) == 0 ? addr : addr - (addr % 0x1000);
        mapping_set_prot(PHYS_TO_VIRT(curr_thread->cxt.ttbr0_el1), pos->virt_addr + addr, pos->phys_addr + addr, pos->protected);
    } else {
        // 其他的error就先不管ㄌ，都送seg fault，像是Access或Permission的fault
        seg_fault();
    }
}

void mapping_set_prot(pd_t *pgd_p, size_t virt_addr, size_t phys_addr, size_t protected) {
    size_t flag = 0;
    // 不能執行，non-executable page
    if (!(protected & PROT_X)) flag |= PD_UNX;
    // 不能寫入，RDONLY
    if (!(protected & PROT_W)) flag |= PD_RDONLY;
    // 不能讀取，串`only kernel access`
    if (protected & PROT_R) flag |= PD_UK_ACCESS;
    mapping(pgd_p, virt_addr, phys_addr, flag);
}

// direct map page
void mapping(pd_t *pgd, size_t virt_addr, size_t phys_addr, size_t protected) {
    size_t *table = pgd;
    for (int level = 0; level < 4; level++) {
        uint32_t idx = (virt_addr >> (39 - level * 9)) & 0x1ff;
        if (level == 3) {
            table[idx] = phys_addr;
            table[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_KNX | protected;
            return;
        }

        // 檢查是不是有效page
        if ((table[idx] & 0b1L) == 0) {
            size_t *newtable = kmalloc(0x1000);
            memset(newtable, 0, 0x1000);
            table[idx] = VIRT_TO_PHYS((size_t)newtable);
            table[idx] |= PD_ACCESS | PD_TABLE | (MAIR_IDX_NORMAL_NOCACHE << 2);
        }

        table = (size_t *)PHYS_TO_VIRT((size_t)(table[idx] & ENTRY_ADDR_MASK));
    }
}

void map_pages(size_t *virt_pgd_p, size_t va, size_t size, size_t pa, size_t flag) {
    pa = pa - (pa % 0x1000);  // align
    for (size_t s = 0; s < size; s += 0x1000) {
        mapping(virt_pgd_p, va + s, pa + s, flag);
    }
}

void seg_fault() {
    uart_printf("[Segmentation fault]: Kill Process\n");
    thread_exit();
}