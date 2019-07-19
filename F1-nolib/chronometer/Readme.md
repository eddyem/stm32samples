Chronometer for downhill competitions
=====================================

## Pinout

- PA9(Tx),PA10 (debug mode) - USART1 - debug console
- PA2(Tx), PA3 - USART2 - GPS
- PB10(Tx), PB11 - USART3 - LIDAR

- PA1  - PPS signal from GPS (EXTI)

- PB8, PB9 - onboard LEDs

- PA4  - TRIG2 - 12V trigger (EXTI)  -- not implemented yet
- PA13 - TRIG0 - button0 (EXTI)
- PA14 - TRIG1 - button1/laser/etc (EXTI)
- PA15 - USB pullup

- PB0 - ADC channel 8
- PB1,2 - free for other functions
