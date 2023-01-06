/* Define memory regions. */
MEMORY
{
    rom   (rx) : ORIGIN = 0x08000000, LENGTH = 128K
    ram  (rwx) : ORIGIN = 0x20000000, LENGTH = 36K
}

PROVIDE(_BLOCKSIZE = 2048);

/* Include the common ld script. */
INCLUDE stm32.ld

