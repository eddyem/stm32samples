4-digit "time" LED indicator based on TM1637
=============================

FOR STM32F051!!!
WARNING! Only writing data!!! Can't read keys state.

## GPIO
- I2C: PB6 (SCL) & PB7 (SDA)
- USART1: PA9/PA10

## UART 
115200N1, not more than 100ms between data bytes in command.
To turn ON human terminal (without timeout) send "####".

## Protocol
All commands are in brackets: `[ command line ]`.

'[' clears earlier input; '\n', '\r', ' ', '\t' are ignored.
All messages are asynchronous!

## Commands
0..9 - send data
A - display 'ABCD'
G - get keyboard status (don't work)
Hhex - display 'hex' as hex number
Nnum - display 'num' as decimal number
