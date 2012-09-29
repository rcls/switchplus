SECTIONS
{
  .text 0x0 : {
     *(.start)
     *(.text)
     *(.rodata*)
     . = NEXT(4);
  }
}
