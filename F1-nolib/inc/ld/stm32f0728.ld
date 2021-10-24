/* Linker script for STM32F072x8, 64K flash, 16K RAM. */

/* Define memory regions. */
MEMORY
{
    rom   (rx) : ORIGIN = 0x08000000, LENGTH = 64K
    ram  (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}

PROVIDE(_BLOCKSIZE = 1024);

/* Include the common ld script. */
INCLUDE stm32f01234.ld

