SECTIONS
{
  .text 0x10000000 : {
     *(.start)
     *(.text)
     *(.rodata*)
     *(.data*)
     *(.bss*)
     . = NEXT(4);
  }
}
