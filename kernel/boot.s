.section ".text.boot"

.global _start

_start:
// read cpu id, stop slave cores
    mrs     x1,mpidr_el1
    and     x1,x1,#3
    cbz     x1,2f
// cpu id>0, stop
1:
    wfe
    b   1b
// cpu id==0, init
2:
    // stack pointer before text section(stack grows to lower addr.)
    ldr     x1, =_start
    mov     sp, x1

    // clear a block save bss section variable
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  
    cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b
4:
    // goto main function
    bl      main
    // maybe error, stop the core
    b       1b
