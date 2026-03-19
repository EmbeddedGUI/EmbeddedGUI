/*
 * Linker script for QEMU ARM emulation (mps2-an385 / microbit)
 *
 * Memory map for MPS2-AN385 (Cortex-M3):
 *   ZBT SSRAM1 (code):  0x00000000, 4MB
 *   ZBT SSRAM2/3 (data): 0x20000000, 4MB
 *
 * For microbit (Cortex-M0):
 *   FLASH: 0x00000000, 256KB
 *   RAM:   0x20000000, 16KB
 *
 * We use generous sizes for QEMU (performance testing needs large resources).
 * For microbit (M0), QEMU also provides enough virtual memory.
 */

ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);

_Min_Heap_Size = 0x40000;  /* 256KB heap (needed for image transform external resource loading) */
_Min_Stack_Size = 0x2000; /* 8KB stack */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 4M
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 4M
}

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    __code_start = .;
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;
    __code_end = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    __rodata_start = .;
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
    __rodata_end = .;
  } >FLASH

  .ARM.extab (READONLY) :
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >FLASH

  .ARM (READONLY) :
  {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >FLASH

  .preinit_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .init_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .fini_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    __data_start = .;
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)

    . = ALIGN(4);
    _edata = .;
    __data_end = .;
  } >RAM AT> FLASH

  . = ALIGN(4);
  .bss :
  {
    __bss_start = .;
    _sbss = .;
    __bss_start__ = _sbss;
    __bss_pfb_start = .;
    *(.bss.pfb_area*)
    __bss_pfb_end = .;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
    __bss_end = .;
  } >RAM

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }

  __code_size = __code_end - __code_start;
  __rodata_size = __rodata_end - __rodata_start;
  __data_size = __data_end - __data_start;
  __bss_size = __bss_end - __bss_start;
  __bss_pfb_size = __bss_pfb_end - __bss_pfb_start;
}
