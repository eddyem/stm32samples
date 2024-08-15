MEMORY
{
	rom (rx) : ORIGIN = 0x08000000, LENGTH = 384K
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
	ccmram (rwx) : ORIGIN = 0x10000000, LENGTH = 16K
}

PROVIDE(_BLOCKSIZE = 2048);

/* Include the common ld script. */
INCLUDE stm32f3.ld
