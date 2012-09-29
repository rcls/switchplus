SECTIONS
{
  .text 0x10000000 : {
     *(.start)
     *(.text)
     *(.rodata*)
     . = NEXT(4);
  }
}
