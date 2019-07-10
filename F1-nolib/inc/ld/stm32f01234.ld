/*
********************************************************************************
*                                                                              *
* Copyright (c) 2017 Andrea Loi                                                *
*                                                                              *
* Permission is hereby granted, free of charge, to any person obtaining a      *
* copy of this software and associated documentation files (the "Software"),   *
* to deal in the Software without restriction, including without limitation    *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,     *
* and/or sell copies of the Software, and to permit persons to whom the        *
* Software is furnished to do so, subject to the following conditions:         *
*                                                                              *
* The above copyright notice and this permission notice shall be included      *
* in all copies or substantial portions of the Software.                       *
*                                                                              *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL      *
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER   *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER          *
* DEALINGS IN THE SOFTWARE.                                                    *
*                                                                              *
********************************************************************************
*/



/******************************************************************************/
/*          DON'T EDIT THIS FILE UNLESS YOU KNOW WHAT YOU'RE DOING!           */
/******************************************************************************/

/* _isrvectors_tend =    0x00000150; - different for different series */

ENTRY(reset_handler)

SECTIONS {
  .vector_table 0x08000000 :
  {
    _sisrvectors = .;
    KEEP(*(.vector_table))
    /* ASSERT(. == _isrvectors_tend, "The vector table needs to be 84 elements long!"); */
    _eisrvectors = .;
  } >rom

  .text :
  {
    . = ALIGN(4);
    _stext = .;
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;
  } >rom

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } >rom
  
  .ARM : {
    *(.ARM.exidx*)
  } >rom
  
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >ram AT >rom

  .myvars :
  {
    . = ALIGN(1024);
    KEEP(*(.myvars))
  } > rom

  _ldata = LOADADDR(.data);

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } >ram
}

PROVIDE(_stack = ORIGIN(ram) + LENGTH(ram));
