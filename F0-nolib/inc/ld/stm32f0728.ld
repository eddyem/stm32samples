/* Linker script for STM32F072x8, 64K flash, 16K RAM. */

/* Define memory regions. */
MEMORY
{
    rom   (rx) : ORIGIN = 0x08000000, LENGTH = 64K
    ram  (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
}

/* Include the common ld script. */
INCLUDE stm32f0.ld

