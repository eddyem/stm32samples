Samples for STM32F303 
=================================

Most of these samples works with [my non-solder devboard](https://github.com/eddyem/stm32samples/tree/master/F0_F1_F3-LQFP48_testboard)

- **blink** - simple LEDs blink on PB0 and PB1
- **usart1** - read data from USART1 and send it back (blocking sending)
- **usarts** - work with all three USARTs with Tx DMA, send to USART1 data read from other, you can send "2\n" or "3\n" to USART1 for tests of USART2/USART3
- **floatPrintf** - work with floats, convert floating point number into a string
- **usart1fullDMA** - USART1 Rx and Tx DMA access (Rx terminated by character match of '\n' or buffer overflow).
- **PL2303** - emulates PL2303
