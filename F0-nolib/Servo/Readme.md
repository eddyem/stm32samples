Servo motors SG-90 management
=============================

## GPIO
- PA0..PA3 - 4 ADC inputs
- PA6 - (TIM3_CH1) - Servo1,
- PA7 - (TIM3_CH2) - Servo2,
- PA9 - (USART_Tx) - TX,
- PA10 - (USART_Rx) - RX,
- PB1 - (TIM3_CH4) - Servo3,
- PF1 - (OpenDrain) - external LED or laser (0 - active)

## UART 
115200N1, not more than 100ms between data bytes in command.
To turn ON human terminal (without timeout) send "####".

## Protocol
All commands are in brackets: `[ command line ]`.

'[' clears earlier input; '\n', '\r', ' ', '\t' are ignored.
All messages are asynchronous!

## Commands
* **d** - debugging commands:
    * **A** - get raw ADC values,
    * **w** - watchdog test;
* **R** - reset;
* **t** - get MCU temperature;
* **V** - get VDD value.

## Messages



## Servos
The board controls up to three servos like SG-90.
Three timer's outputs used for this purpose. Timer frequency 50Hz, pulse width from 500 to 2400us.

