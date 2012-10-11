SECTIONS
{
  .text 0x1a000000 : {
     *(.start)
     *(.text)
     *(.rodata*)
     . = NEXT(4);
  }

  /DISCARD/ : {
     *(.data*)
  }

  .bss 0x10000000 : {
     bss_start = .;
     *(.bss*)
     bss_end = .;
  }

}
