.section ".text.boot"

.global _start

_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1,#3
    // cpu id>0, standby
    // cpu id==0, init
    cbnz    x1, _standby

    //  set exception level
    bl _from_el2_to_el1

    // stack pointer before MMIO address
    ldr     x1, =_start  // move sp before MMIO address
    mov     sp, x1  // move sp before MMIO address

    // set el1 vector table
set_exception_vector_table:
    adr x1, exception_vector_table
    msr vbar_el1, x1

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