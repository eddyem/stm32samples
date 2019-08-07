Chronometer for downhill competitions
=====================================

## Pinout

- PA11/12 - USB
- PA9(Tx),PA10 (debug mode) - USART1 - debug console
- PA2(Tx), PA3 - USART2 - GPS
- PB10(Tx), PB11 - USART3 - LIDAR - TRIG3

- PA1  - PPS signal from GPS (EXTI)

- PA4  - TRIG2 - 12V trigger (EXTI)
- PA13 - TRIG0 - button0 (EXTI)
- PA14 - TRIG1 - button1/laser/etc (EXTI)
- PA15 - USB pullup

- PB8, PB9 - onboard LEDs (0/1)

- PC13 - buzzer

### Not implemented yet:

- PA5,6,7 (SCK, MISO, MOSI) - SPI
- PB0 - TRIG4 - ADC channel 8
- PB6/7 (SCL, SDA) - I2C
