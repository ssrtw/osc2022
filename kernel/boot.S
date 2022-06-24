#include "mmu.h"
.section ".text.boot"

.global _start

_start:
    // save dtb addr to callee save reg
    mov     x19, x0
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1,#3
    // cpu id>0, standby
    // cpu id==0, init
    cbnz    x1, _standby
    
    //  set exception level
    bl _from_el2_to_el1

    bl kernel_mmu_init

    ldr x4, =TCR_CONFIG_DEFAULT
    msr tcr_el1, x4

    ldr x4, =((MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) | (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)))
    msr mair_el1, x4

    ldr x4, =BOOT_PGD_ADDR // PGD's page frame at 0x1000
    msr ttbr0_el1, x4 // load PGD to the bottom translation-based register.
    msr ttbr1_el1, x4 // also load PGD to the upper translation based register.

    mrs x4, sctlr_el1
    orr x4, x4, 1
    msr sctlr_el1, x4

    // indirect branch to the upper virtual address
    ldr x2, =set_exception_vector_table
    br x2

    // set el1 vector table
set_exception_vector_table:
    adr x1, exception_vector_table
    msr vbar_el1, x1

    // move sp before MMIO address
    // at 0xffff00003c000000
    mov     x3, xzr
    movk    x3, 0x3c00, lsl 16
    movk    x3, 0xffff, lsl 48
    mov     sp, x3

_bss_init:
    // set relocate bss area to zero
    ldr     x2, =__bss_start
    ldr     x4, =__bss_size
    cbz     x4, _bl_main
    add     x4, x4, #1
_bss_set_zero:
    str     xzr, [x2], #8 // *x2=0
    sub     x4, x4, #1
    cbnz    x4, _bss_set_zero

_bl_main:
    // restore dtb addr to x0
    mov x0, x19
    // goto main function
    bl      main     // link because if occur exception, make cpu wfe(die).
_standby:
    wfe
    b   _standby

_from_el2_to_el1:
    mov x1, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x1
    mov x1, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x1
    msr elr_el2, lr
    eret // return to EL1