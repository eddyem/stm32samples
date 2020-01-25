Chronometer for downhill competitions
=====================================

The binary have two models: DEBUG (make debug) and RELEASE (make release or just make).
DEBUGGing model use USART1 as debugging console, showing many messages.

When typing commands you can fix them using backspace key. ESC-sequences don't work.

## Pinout

### Interfaces

- PA11/12 -- USB
- PA9(Tx), PA10 (Rx) -- USART1 (debug console / Bluetooth / GPS proxy)
- PA2(Tx), PA3 -- USART2 (GPS)
- PB10(Tx), PB11 -- USART3 (LIDAR / console)

- PA13/14 - SWDIO

### Other

- PA1  -- PPS signal from GPS (EXTI)
- PA8 -- Bluetooth "State" pin (not implemented yet)
- PA15 -- USB pullup

- PB0/1 -- TRIG0/1
- PB3 -- TRIG2
- PB8, PB9 -- onboard LEDs (PB8 - LED1, PB9 - LED0)

- PC13 -- buzzer

### LED screen:

- PA5,6,7 (SCK, MISO, MOSI) -- SPI for LED screen: PA5/7 - SCK/MOSI, PA6 - SCLK/nOE (connected together)
- PB6/7 -- A/B for LED screen

## LEDS

- LED0 -- shining when there's no PPS signal, fades for 0.25s on PPS
- LED1 -- don't shines if no GPS found, shines when time not valid, blinks when time valid


