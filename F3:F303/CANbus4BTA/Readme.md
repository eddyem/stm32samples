BTA CANbus node controller
====================================

# Pinout

PP - push-pull, OD - open drain, I - floating input, A - analog input, AFn - alternate function number n.
1 - active high, 0 - active low.
### Sorted by pin number

|**Pin #**|**Pin name** | **function**| **settings**| **comment**      |
|---------|-------------|-------------|-------------|------------------|
|   1     | VBAT        |  3v3        |             |                  |
|   2     | PC13        | buzzer      | PP1         |                  |
|   3     | PC14        | relay       | PP1         |                  |
|   4     | PC15        | usb pullup  | PP1         |                  |
|   5     | PF0         |  OSCIN      |             |                  |
|   6     | PF1         |  OSCOUT     |             |                  |
|   7     | NRST        |  reset      |             |                  |
|   8     | VSSA        |  gnd        |             |                  |
|   9     | VDDA        |  3v3        |             |                  |
|  10     | PA0         | AIN0        | A           |                  |
|  11     | PA1         | AIN1        | A           |                  |
|  12     | PA2         | ANI2        | A           |                  |
|  13     | PA3         | AIN3        | A           |                  |
|  14     | PA4         | DIN         | I           | mul0/1 input     |
|  15     | PA5         | DEN0        | PP0         | enable mul0      |
|  16     | PA6         | DEN1        | PP0         | enable mul1      |
|  17     | PA7         | PWM         | AF1         | TIM17CH1 PWM     |
|  18     | PB0         | ADDR0       | PP          | MUL address      |
|  19     | PB1         | ADDR1       | PP          |     selection    |
|  20     | PB2         | ADDR2       | PP          |                  |
|  21     | PB10        | PB10        | gpio        | reserved gpio    |
|  22     | PB11        | PB11        | gpio        | reserved gpio    |
|  23     | VSS         |  gnd        |             |                  |
|  24     | VDD         |  3v3        |             |                  |
|  25     | PB12        | SPI2 NSS    | AF5         | isolated         |
|  26     | PB13        | SPI2 SCK    | AF5         |   external       |
|  27     | PB14        | SPI2 MISO   | AF5         |   SPI            |
|  28     | PB15        | SPI2 MOSI   | AF5         |                  |
|  29     | PA8         | PA8         | gpio        | reserved gpio    |
|  30     | PA9         | USART1 TX   | AF7         | RS-422 interface |
|  31     | PA10        | USART1 RX   | AF7         |     (opt)        |
|  32     | PA11        | USB DM      | AF14        |                  |
|  33     | PA12        | USB DP      | AF14        |                  |
|  34     | PA13        |  SWDIO      | AF0         |                  |
|  35     | VSS         |  gnd        |             |                  |
|  36     | VDD         |  3v3        |             |                  |
|  37     | PA14        |  SWCLK      | AF0         |                  |
|  38     | PA15        | PA15        | gpio        | reserved gpio    |
|  39     | PB3         | SPI1 SCK    | AF5         | SSI interface    |
|  40     | PB4         | SPI1 MISO   | AF5         |    (opt)         |
|  41     | PB5         |     X       |             |                  |
|  42     | PB6         | I2C1 SCL    | AF4         | non-isolated     |
|  43     | PB7         | I2C1 SDA    | AF4         |     ext. I2C     |
|  44     | BOOT0       |  boot       |             |                  |
|  45     | PB8         | CAN RX      | AF9         |                  |
|  46     | PB9         | CAN TX      | AF9         |                  |
|  47     | VSS         |  gnd        |             |                  |
|  48     | VDD         |  3v3        |             |                  |
|---------|-------------|-------------|-------------|------------------|


### Sorted by port
// sort -Vk4 Readme.md 

|**Pin #**|**Pin name** | **function**| **settings**| **comment**      |
|---------|-------------|-------------|-------------|------------------|
|  44     | BOOT0       |  boot       |             |                  |
|   7     | NRST        |  reset      |             |                  |
|  10     | PA0         | AIN0        | A           |                  |
|  11     | PA1         | AIN1        | A           |                  |
|  12     | PA2         | ANI2        | A           |                  |
|  13     | PA3         | AIN3        | A           |                  |
|  14     | PA4         | DIN         | I           | mul0/1 input     |
|  15     | PA5         | DEN0        | PP0         | enable mul0      |
|  16     | PA6         | DEN1        | PP0         | enable mul1      |
|  17     | PA7         | PWM         | AF1         | TIM17CH1 PWM     |
|  29     | PA8         | PA8         | gpio        | reserved gpio    |
|  30     | PA9         | USART1 TX   | AF7         | RS-422 interface |
|  31     | PA10        | USART1 RX   | AF7         |     (opt)        |
|  32     | PA11        | USB DM      | AF14        |                  |
|  33     | PA12        | USB DP      | AF14        |                  |
|  34     | PA13        |  SWDIO      | AF0         |                  |
|  37     | PA14        |  SWCLK      | AF0         |                  |
|  38     | PA15        | PA15        | gpio        | reserved gpio    |
|  18     | PB0         | ADDR0       | PP          | MUL address      |
|  19     | PB1         | ADDR1       | PP          |     selection    |
|  20     | PB2         | ADDR2       | PP          |                  |
|  39     | PB3         | SPI1 SCK    | AF5         | SSI interface    |
|  40     | PB4         | SPI1 MISO   | AF5         |    (opt)         |
|  41     | PB5         |     X       |             |                  |
|  42     | PB6         | I2C1 SCL    | AF4         | non-isolated     |
|  43     | PB7         | I2C1 SDA    | AF4         |     ext. I2C     |
|  45     | PB8         | CAN RX      | AF9         |                  |
|  46     | PB9         | CAN TX      | AF9         |                  |
|  21     | PB10        | PB10        | gpio        | reserved gpio    |
|  22     | PB11        | PB11        | gpio        | reserved gpio    |
|  25     | PB12        | SPI2 NSS    | AF5         | isolated         |
|  26     | PB13        | SPI2 SCK    | AF5         |   external       |
|  27     | PB14        | SPI2 MISO   | AF5         |   SPI            |
|  28     | PB15        | SPI2 MOSI   | AF5         |                  |
|   2     | PC13        | buzzer      | PP1         |                  |
|   3     | PC14        | relay       | PP1         |                  |
|   4     | PC15        | usb pullup  | PP1         |                  |
|   5     | PF0         |  OSCIN      |             |                  |
|   6     | PF1         |  OSCOUT     |             |                  |
|   1     | VBAT        |  3v3        |             |                  |
|   9     | VDDA        |  3v3        |             |                  |
|  24     | VDD         |  3v3        |             |                  |
|  36     | VDD         |  3v3        |             |                  |
|  48     | VDD         |  3v3        |             |                  |
|   8     | VSSA        |  gnd        |             |                  |
|  23     | VSS         |  gnd        |             |                  |
|  35     | VSS         |  gnd        |             |                  |
|  47     | VSS         |  gnd        |             |                  |
|---------|-------------|-------------|-------------|------------------|

## DMA usage
### DMA1

Channel1 - ADC1.

### DMA2

