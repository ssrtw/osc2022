SECTIONS{
    . = 0xffff000000000000;
    . += 0x80000;
    __kernel_start = .;
    .text : {
        KEEP(*(.text.boot))
        *(.text)
    }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
    . = ALIGN(16);
    __bss_start = .;
    .bss : {
        *(.bss.*)
    }
    __bss_end = .;
    . = ALIGN(4096); /* 4K align */
    __heap_start = .;
    __kernel_end = .;
}
__bss_size = (__bss_end - __bss_start)>>3;