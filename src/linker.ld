ENTRY(_start)

SECTIONS
{
    . = 1M; /* Load address */
    _kernel_start = .;

    .text ALIGN(4096) :
    {
        *(.multiboot)
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4096) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data ALIGN(4096) :
    {
        *(.data)
        *(.data.*)
    }

    .bss ALIGN(4096) :
    {
        *(COMMON)
        *(.bss)
    }

    _kernel_end = .;
}
