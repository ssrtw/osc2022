#include "mmu.h"

#include "util.h"

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB          ((0b00 << 14) | (0b10 << 30))
#define TCR_CONFIG_DEFAULT      (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE      0b00000000
#define MAIR_NORMAL_NOCACHE     0b01000100
#define MAIR_IDX_DEVICE_nGnRnE  0
#define MAIR_IDX_NORMAL_NOCACHE 1

#define PD_TABLE  0b11L
#define PD_BLOCK  0b01L
#define PD_ACCESS (1L << 10)
// block entry
#define PD_BE PD_BLOCK | PD_ACCESS

typedef uint64_t pd_t;

#define BOOT_PGD ((pd_t *)0x1000)
#define BOOT_PUD ((pd_t *)0x2000)
#define BOOT_PMD ((pd_t *)0x3000)

void kernel_mmu_init() {
    write_sysreg(tcr_el1, TCR_CONFIG_DEFAULT);
    write_sysreg(mair_el1,
                 (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) |
                     (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)));
    BOOT_PGD[0] = (uint64_t)BOOT_PMD | PD_TABLE;
    // 1st 1GB mapped by the 1st entry of PUD
    BOOT_PUD[0] = (uint64_t)BOOT_PMD | PD_TABLE;
    // 2nd 1GB mapped by the 2nd entry of PUD
    // 0x40000000 ~ 0x80000000: Device
    BOOT_PUD[1] = (uint64_t)BOOT_PMD | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BE;

    // 0x00000000 ~ 0x3f000000: Normal
    for (int i = 0; i < 504; i++) {
        BOOT_PMD[i] = (i * (1 << 21)) | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BE;
    }
    // 0x3f000000 ~ 0x40000000: Device
    for (int i = 504; i < 512; i++) {
        BOOT_PMD[i] = (i * (1 << 21)) | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BE;
    }
    write_sysreg(TTBR0_EL1, BOOT_PGD);
    write_sysreg(TTBR1_EL1, BOOT_PGD);

    // Enable MMU
    uint64_t sctlr_el1;
    sctlr_el1 = read_sysreg(sctlr_el1);
    write_sysreg(sctlr_el1, sctlr_el1 | 1);
}