Multiport board
====================================

Seven isolated interfaces:

- 1 CAN
- 3 RS-485
- 2 or 1 RS-232
- 1 SSI or 1 RS-422 (in this case only one RS-232)

Inner USB interfaces (IFx):

1.  RS-485 (1)
2.  RS-485 (2)
3.  RS-485 (3)
4.  RS-232 (1)
5.  RS-232 (2) or RS-422 (by jumpers)
6.  CAN
7.  SSI (over SPI, by jumpers) or configuration interface (if "Config mode" jumper opened)

# Pinout

### Sorted by pin number

| Pin #   | Pin name    | function    | settings      | comment             |
|---------|-------------|-------------|---------------|---------------------|
|   1     | (VBAT)      |             |               |                     |
|   2     | PC13        | NC          |               |                     |
|   3     | PC14        | NC          |               |                     |
|   4     | PC15        | NC          |               |                     |
|   5     | (OSC IN)    |             |               |                     |
|   6     | (OSC OUT)   |             |               |                     |
|   7     | (NRST)      |             |               |                     |
|   8     | PC0         | NC          |               |                     |
|   9     | PC1         | NC          |               |                     |
|  10     | PC2         | NC          |               |                     |
|  11     | PC3         | NC          |               |                     |
|  12     | (VREF-)     |             |               |                     |
|  13     | (VREF+)     |             |               |                     |
|  14     | PA0         | NC          |               |                     |
|  15     | PA1         | USART2 DE   | AF7 or PP     | RS-485 (3) DE       |
|  16     | PA2         | USART2 TX   | AF7           | RS-485 (3) Tx       |
|  17     | PA3         | USART2 RX   | AF7           | RS-485 (3) Rx       |
|  18     | PF4         | NC          |               |                     |
|  19     | (VDD)       |             |               |                     |
|  20     | PA4         | NC          |               |                     |
|  21     | PA5         | SPI1 SCK    | AF5           | SSI CLK             |
|  22     | PA6         | SPI1 MISO   | AF5           | SSI DAT             |
|  23     | PA7         | NC          |               |                     |
|  24     | PC4         | USART1 TX   | AF7           | RS-485 (2) Tx       |
|  25     | PC5         | USART1 RX   | AF7           | RS-485 (2) Rx       |
|  26     | PB0         | (USART1 DE) | PP OUT        | RS-485 (2) DE       |
|  27     | PB1         | NC          |               |                     |
|  28     | PB2         | NC          |               |                     |
|  29     | PB10        | USART3 TX   | AF7           | RS-485 (1) Tx       |
|  30     | PB11        | USART3 RX   | AF7           | RS-485 (1) Rx       |
|  31     | (VSS)       |             |               |                     |
|  32     | (VDD)       |             |               |                     |
|  33     | PB12        | NC          |               |                     |
|  34     | PB13        | NC          |               |                     |
|  35     | PB14        | USART3 DE   | AF7 or PP     | RS-485 (1) DE       |
|  36     | PB15        | NC          |               |                     |
|  37     | PC6         | NC          |               |                     |
|  38     | PC7         | NC          |               |                     |
|  39     | PC8         | NC          |               |                     |
|  40     | PC9         | NC          |               |                     |
|  41     | PA8         | NC          |               |                     |
|  42     | PA9         | (CONF EN)   | PU IN         | Config jumper       |
|  43     | PA10        | (USB PU)    | PP OUT        | USB pullup          |
|  44     | PA11        | USB DM      | AF14          |                     |
|  45     | PA12        | USB DP      | AF14          |                     |
|  46     | PA13        | SWDIO       | AF0           |                     |
|  47     | (VSS)       |             |               |                     |
|  48     | (VDD)       |             |               |                     |
|  49     | PA14        | SWCLK       | AF0           |                     |
|  50     | PA15        | NC          |               |                     |
|  51     | PC10        | UART4 TX    | AF5           | RS-232 (1) Tx       |
|  52     | PC11        | UART4 RX    | AF5           | RS-232 (1) Rx       |
|  53     | PC12        | UART5 TX    | AF5           | RS-232(2) / 485 Tx  |
|  54     | PD2         | UART5 RX    | AF5           | RS-232(2) / 485 Rx  |
|  55     | PB3         | NC          |               |                     |
|  56     | PB4         | NC          |               |                     |
|  57     | PB5         | NC          |               |                     |
|  58     | PB6         | NC          |               |                     |
|  59     | PB7         | NC          |               |                     |
|  60     | (BOOT0)     |             |               |                     |
|  61     | PB8         | CAN RX      | AF9           |                     |
|  62     | PB9         | CAN TX      | AF9           |                     |
|  63     | (VSS)       |             |               |                     |
|  64     | (VDD)       |             |               |                     |

### Some comments.

Interfaces:

- IF1: RS-485 over USART3.
- IF2: RS-485 over USART1.
- IF3: RS-485 over USART2.
- IF4: RS-232 over UART4.
- IF5: RS-232 or RS-422 (by jumpers "IF5" and "SSI") over UART5.
- IF6: CAN bus.
- IF7: SSI (inaccessible when RS-422 selected) or interface for device configuration (if jumper "Config mode" is opened when device started").


DMA1 channels:

- Ch2: USART3_Tx
- Ch3: USART3_Rx
- Ch4: USART1_Tx
- Ch5: USART1_Rx
- Ch6: USART2_Rx
- Ch7: USART2_Tx

DMA2 channels:

- Ch3: UART4_Rx
- Ch5: UART4_Tx

UART5 have no DMA channels, so used in interrupts.

You can try to use hardware DE management on two of RS-485, but I decide that as I can't use hardware DE for all three, it would be
simpler to use software DE for all.

### Sorted by ports 

| Pin #   | Pin name    | function    | settings      | comment             |
|---------|-------------|-------------|---------------|---------------------|
|  14     | PA0         | NC          |               |                     |
|  15     | PA1         | USART2 DE   | AF7 or PP     | RS-485 (3) DE       |
|  16     | PA2         | USART2 TX   | AF7           | RS-485 (3) Tx       |
|  17     | PA3         | USART2 RX   | AF7           | RS-485 (3) Rx       |
|  20     | PA4         | NC          |               |                     |
|  21     | PA5         | SPI1 SCK    | AF5           | SSI CLK             |
|  22     | PA6         | SPI1 MISO   | AF5           | SSI DAT             |
|  23     | PA7         | NC          |               |                     |
|  41     | PA8         | NC          |               |                     |
|  42     | PA9         | (CONF EN)   | PU IN         | Config jumper       |
|  43     | PA10        | (USB PU)    | PP OUT        | USB pullup          |
|  44     | PA11        | USB DM      | AF14          |                     |
|  45     | PA12        | USB DP      | AF14          |                     |
|  46     | PA13        | SWDIO       | AF0           |                     |
|  49     | PA14        | SWCLK       | AF0           |                     |
|  50     | PA15        | NC          |               |                     |
|  26     | PB0         | (USART1 DE) | PP OUT        | RS-485 (2) DE       |
|  27     | PB1         | NC          |               |                     |
|  28     | PB2         | NC          |               |                     |
|  55     | PB3         | NC          |               |                     |
|  56     | PB4         | NC          |               |                     |
|  57     | PB5         | NC          |               |                     |
|  58     | PB6         | NC          |               |                     |
|  59     | PB7         | NC          |               |                     |
|  61     | PB8         | CAN RX      | AF9           |                     |
|  62     | PB9         | CAN TX      | AF9           |                     |
|  29     | PB10        | USART3 TX   | AF7           | RS-485 (1) Tx       |
|  30     | PB11        | USART3 RX   | AF7           | RS-485 (1) Rx       |
|  33     | PB12        | NC          |               |                     |
|  34     | PB13        | NC          |               |                     |
|  35     | PB14        | USART3 DE   | AF7 or PP     | RS-485 (1) DE       |
|  36     | PB15        | NC          |               |                     |
|   8     | PC0         | NC          |               |                     |
|   9     | PC1         | NC          |               |                     |
|  10     | PC2         | NC          |               |                     |
|  11     | PC3         | NC          |               |                     |
|  24     | PC4         | USART1 TX   | AF7           | RS-485 (2) Tx       |
|  25     | PC5         | USART1 RX   | AF7           | RS-485 (2) Rx       |
|  37     | PC6         | NC          |               |                     |
|  38     | PC7         | NC          |               |                     |
|  39     | PC8         | NC          |               |                     |
|  40     | PC9         | NC          |               |                     |
|  51     | PC10        | UART4 TX    | AF5           | RS-232 (1) Tx       |
|  52     | PC11        | UART4 RX    | AF5           | RS-232 (1) Rx       |
|  53     | PC12        | UART5 TX    | AF5           | RS-232(2) / 485 Tx  |
|   2     | PC13        | NC          |               |                     |
|   3     | PC14        | NC          |               |                     |
|   4     | PC15        | NC          |               |                     |
|  54     | PD2         | UART5 RX    | AF5           | RS-232(2) / 485 Rx  |
|  18     | PF4         | NC          |               |                     |

