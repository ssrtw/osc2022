.section ".text.boot"

.global _start

__relocated_addr = 0x60000; // move bootloader to 0x60000

_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1,#3
    // cpu id>0, standby
    // cpu id==0, init
    cbnz    x1, _standby

    // stack pointer before MMIO address
    mov     sp, #0x60000  // move sp before MMIO address

    // relocate_btlder
    ldr     x2, =__btlder_start
    ldr     x3, =__relocated_addr
    ldr     x4, =__btlder_size
    // calc relocate main address
    ldr     x6, =main
    sub     x5, x2, x3  // x5=relocate distance
    sub     x6, x6, x5  // x6=relocate main addr
    add     x4, x4, #1
    cbz     x4, _bss_init
_relocate_loop:
    ldr     x1, [x2], #8 // x1=*x2
    str     x1, [x3], #8 // *x3=x1
    sub     x4, x4, #1
    cbnz    x4, _relocate_loop

_bss_init:
    // set relocate bss area to zero
    ldr     x2, =__bss_start
    ldr     x4, =__bss_size
    sub     x2, x2, x5  // x2=relocate bss addr
    cbz     x4, _bl_main
_bss_set_zero:
    str     xzr, [x2], #8 // *x2=0
    sub     x4, x4, #1
    cbnz    x4, _bss_set_zero

_bl_main:
    // goto main function
    blr     x6     // link because if occur exception, make cpu wfe(die).
_standby:
    wfe
    b   _standby