// save general registers to stack
.global save_all
.macro save_all
    sub sp, sp, 0x120
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]
    //using for nested interrupt
    mrs x0,  spsr_el1
    str x0,  [sp, 0xf8]
    mrs x0,  elr_el1
    str x0,  [sp, 0x100]
    mrs x0,  sp_el0
    str x0,  [sp, 0x108]
    // restore x0
    ldp x0, x1, [sp ,16 * 0]
.endm

// load general registers from stack
.global load_all
.macro load_all
    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    //using for nested interrupt
    ldr x0, [sp, 0xf8]
    msr spsr_el1,x0
    ldr x0,  [sp, 0x100]
    msr elr_el1, x0
    ldr x0,  [sp, 0x108]
    msr sp_el0, x0
    // restore x0
    ldp x0, x1, [sp ,16 * 0]
    add sp, sp, 0x120
.endm

// https://kaiiiz.github.io/notes/nctu/osdi/lab3/exception-vector-table/
.macro ventry label
    .align 7 // entry size is 0x80, .align will pad 0
    b \label
    mrs x1, esr_el1     // check fault information
    mrs x2, elr_el1     // check return address
    mrs x3, spsr_el1    // check stack address
    mrs x4, far_el1     // check fault address
.endm

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
    // Exception from the curr EL while using SP_EL0
    ventry inv_sync_el1t            // Synchronous EL1t
    ventry inv_irq_el1t             // IRQ EL1t
    ventry inv_fiq_el1t             // FIQ EL1t
    ventry inv_error_el1t           // Error EL1t

    // Exception from the curr EL while using SP_ELX
    ventry inv_sync_el1h            // Synchronous EL1h
    ventry irq_el1h                 // IRQ EL1h
    ventry inv_fiq_el1h             // FIQ EL1h
    ventry inv_error_el1h           // Error EL1h

    // Exception from a lower EL and at least one EL is AArch64
    ventry sync_el0                 // Synchronous 64-bit EL0
    ventry irq_el0                  // IRQ 64-bit EL0
    ventry inv_fiq_el0_64           // FIQ 64-bit EL0
    ventry inv_error_el0_64         // Error 64-bit EL0

    // Exception from a lower EL and at least one EL is AArch32
    ventry inv_sync_el0_32          // Synchronous 32-bit EL0
    ventry inv_irq_el0_32           // IRQ 32-bit EL0
    ventry inv_fiq_el0_32           // FIQ 32-bit EL0
    ventry inv_error_el0_32         // Error 32-bit EL0

.macro inv_entry call_no
    save_all
    mov x0, \call_no
    bl inv_handler
    load_all
    eret
.endm

inv_sync_el1t:
    inv_entry        0
inv_irq_el1t:
    inv_entry        1
inv_fiq_el1t:
    inv_entry        2
inv_error_el1t:
    inv_entry        3

inv_sync_el1h:
    // kernel error
    inv_entry        4
irq_el1h:
    save_all
    mov x0, sp  // trap_frame
    bl irq_handler      // irq_handler(void)
    load_all
    eret
inv_fiq_el1h:
    inv_entry        6
inv_error_el1h:
    inv_entry        7

sync_el0:
    save_all
    mov x0, sp  // trap_frame
    mrs x1, esr_el1 // check fault information
    bl sync_el0_handler // the syscall handler, sync_el0_handler(size_t x0)
    load_all
    eret
irq_el0:
    save_all
    mov x0, sp  // trap_frame
    bl irq_handler      // irq_handler(void)
    load_all
    eret
inv_fiq_el0_64:
    inv_entry       10
inv_error_el0_64:
    inv_entry       11

inv_sync_el0_32:
    inv_entry       12
inv_irq_el0_32:
    inv_entry       13
inv_fiq_el0_32:
    inv_entry       14
inv_error_el0_32:
    inv_entry       15
