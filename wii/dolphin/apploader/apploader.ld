/* Flat image at the Wii apploader load address; everything in one progbits section. */
ENTRY(apploader_entry)
SECTIONS
{
  . = 0x81200000;
  .text : {
    *(.text.entry)
    *(.text .text.*)
    *(.rodata .rodata.*)
    *(.data .data.* .sdata .sdata.*)
    *(.bss .bss.* .sbss .sbss.* COMMON)
    . = ALIGN(32);
  }
  /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) *(.gnu.attributes) }
}
