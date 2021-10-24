/* Linker script for STM32F051x8, 64K flash, 8K RAM. */

/* Define memory regions. */
MEMORY
{
    rom (rx) : ORIGIN = 0x08000000, LENGTH =  64K
    ram (rwx) : ORIGIN = 0x20000000, LENGTH = 8K
}

PROVIDE(_BLOCKSIZE = 1024);

/* Include the common ld script. */
INCLUDE stm32f01234.ld

