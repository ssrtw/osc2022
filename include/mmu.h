#ifndef _MMU_H
#define _MMU_H

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

#ifndef __ASSEMBLER__

#endif

#endif