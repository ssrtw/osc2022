SECTIONS{
    . = 0x80000;
    __btlder_start = .;
    .text : {
        KEEP(*(.text.boot))
        *(.text)
    }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
    __btlder_end = .;

    . = ALIGN(16);
    __bss_start = .;
    .bss : {
        *(.bss.*)
    }
    __bss_end = .;
}
__bss_size = (__bss_end - __bss_start)>>3;
__btlder_size = (__btlder_end - __btlder_start)>>3;