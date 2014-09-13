MEMORY {
  sram (WX) : ORIGIN = 0x10000000, LENGTH = 32K
  /* sram2 is actually 40K but leave room for the stack.  */
  sram2 (W) : ORIGIN = 0x10080000, LENGTH = 16K
  flashA (RX) : ORIGIN = 0x1a000000, LENGTH = 512K
  flashB (RX) : ORIGIN = 0x1b000000, LENGTH = 512K
  ahb0 (W) : ORIGIN = 0x20000000, LENGTH = 32K
  ahb1 (W) : ORIGIN = 0x20008000, LENGTH = 16K
  ahb2 (W) : ORIGIN = 0x2000c000, LENGTH = 16K
  dram (W) : ORIGIN = 0x60000000, LENGTH = 32M
}

SECTIONS
{
  .text : {
     KEEP(*(.start))
     *(.text*)
     *(.rodata*)
     . = NEXT(4);
  } > sram
  /* The linker puts initialised data into sram and we copy to sram2.  */
  .data : {
     rw_data_start = .;
     *(.data*)
     rw_data_end = .;
  } > sram2 AT>sram
  .bss (NOLOAD) : {
     bss_start = .;
     *(SORT_BY_ALIGNMENT(.bss*))
     bss_end = .;
  } > sram

  rw_data_load = LOADADDR(.data);

  .ahb0 (NOLOAD) : {
     *(SORT_BY_ALIGNMENT(ahb0*))
  } > ahb0
  .ahb1 (NOLOAD) : {
     *(SORT_BY_ALIGNMENT(ahb1*))
  } > ahb1
  .ahb2 (NOLOAD) : {
     *(SORT_BY_ALIGNMENT(ahb2*))
  } > ahb2
  .dram (NOLOAD) : {
     *(SORT_BY_ALIGNMENT(dram*))
  } > dram
}
