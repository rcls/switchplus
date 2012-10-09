SECTIONS
{
  .text 0x10000000 : {
     *(.start)
     *(.text)
     *(.rodata*)
     *(.data*)
     . = NEXT(4);
  }
  .bss : {
     bss_start = .;
     *(.bss*);
     bss_end = .;
  }
}
