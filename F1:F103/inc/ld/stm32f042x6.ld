/* Linker script for STM32F042x6, 32K flash, 6K RAM. */

/* Define memory regions. */
MEMORY
{
    rom   (rx) : ORIGIN = 0x08000000, LENGTH = 32K
    ram  (rwx) : ORIGIN = 0x20000000, LENGTH = 6K
}

PROVIDE(_BLOCKSIZE = 1024);

/* Include the common ld script. */
INCLUDE stm32f01234.ld

