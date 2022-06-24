#include "mmu.h"

#include "util.h"
// block entry
#define PD_BE PD_BLOCK | PD_ACCESS

typedef uint64_t pd_t;

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