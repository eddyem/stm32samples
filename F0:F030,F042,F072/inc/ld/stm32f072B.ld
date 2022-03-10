/* Linker script for STM32F072xB, 128K flash, 16K RAM. */

/* Define memory regions. */
MEMORY
{
    rom   (rx) : ORIGIN = 0x08000000, LENGTH = 128K
    ram  (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}

_BLOCKSIZE = 2048;

/* Include the common ld script. */
INCLUDE stm32f01234.ld

