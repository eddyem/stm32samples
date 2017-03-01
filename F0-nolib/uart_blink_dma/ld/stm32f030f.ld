/* Linker script for STM32F030f4, 16K flash, 4K RAM. */

/* Define memory regions. */
MEMORY
{
    rom   (rx) : ORIGIN = 0x08000000, LENGTH = 16K
    ram  (rwx) : ORIGIN = 0x20000000, LENGTH = 4K
}

/* Include the common ld script. */
INCLUDE stm32f0.ld

