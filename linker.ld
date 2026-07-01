ENTRY(_start)

SECTIONS
{
    . = 0x1000;
    .text : { *(.text) }
    .data : { *(.data) }
    .rodata : { *(.rodata) }
    . = 0x20000;
    .bss (NOLOAD) : { 
        _bss_start = .;
        *(.bss)
        *(COMMON)
        _bss_end = .;
        }
}
