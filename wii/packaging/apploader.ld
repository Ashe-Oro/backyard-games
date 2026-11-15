ENTRY(entry)
SECTIONS {
    . = 0x81200000;
    .text : { KEEP(*(.text.entry)) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    /* Include zero-initialized state in the binary, no runtime CRT required. */
    .state : { *(.state) }
    /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) }
}
