Servo motors SG-90 management
=============================

## GPIO

- PA0 - (ADC_IN0) - Servo1 control,
- PA1 - (ADC_IN1) - Servo2 control,
- PA2 - (ADC_IN2) - Servo3 control,
- PA3 - (ADC_IN3) - external analogue signal,
- PA4 - (PullUp in) - ext. input 0,
- PA5 - (PullUp in) - ext. input 1,
- PA6 - (TIM3_CH1) - Servo1,
- PA7 - (TIM3_CH2) - Servo2,
- PA9 - (USART_Tx) - TX,
- PA10 - (USART_Rx) - RX,
- PA13 - (PullUp in) - Jumper 0,
- PA14 - (PullUp in) - Jumper 1,
- PB1 - (TIM3_CH4) - Servo3,
- PF0 - (OpenDrain) - Buzzer,
- PF1 - (OpenDrain) - External LED (or weak laser module).

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

