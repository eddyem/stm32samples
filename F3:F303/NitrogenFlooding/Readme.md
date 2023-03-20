Automated liquid nitrogen flooding machine
====================================

# Pinout

(all GPIO outs are push-pull if not mentioned another)

### Sorted by pin number

|**Pin #**|**Pin name **| **function**| **settings**|**comment **      |
|---------|-------------|-------------|-------------|------------------|
|   1     | PE2         | PWM0        | AF2         | TIM3 channels    |
|   2     | PE3         | PWM1        | AF2         |   as PWM output  |
|   3     | PE4         | PWM2        | AF2         |                  |
|   4     | PE5         | PWM3        | AF2         |                  |
|   5     | PE6         | -           | -           |                  |
|   6     | (vbat)      |             |             |                  |
|   7     | PC13        | -           | -           |                  |
|   8     | PC14        | -           | -           |                  |
|   9     | PC15        | -           | -           |                  |
|  10     | PF9         | -           | -           |                  |
|  11     | PF10        | VadcON      | slow out    | turn ON ADC power|
|  12     | PF0         | OSC IN      |             |                  |
|  13     | PF1         | OSC OUT     |             |                  |
|  14     | (nrst)      |             |             |                  |
|  15     | PC0         | ADC5        | ADC1 in 6   | Thermometers     |
|  16     | PC1         | ADC6        | ADC1 in 7   |  (level-meter)   |
|  17     | PC2         | ADC7        | ADC1 in 8   |  ADC channels    |
|  18     | PC3         | ADC8        | ADC1 in 9   |                  |
|  19     | PF2         | ADC9        | ADC1 in 10  |                  |
|  20     | (VSSA)      |             |             |                  |
|  21     | (VREF+)     |             |             |                  |
|  22     | (VDDA)      |             |             |                  |
|  23     | PA0         | ADC0        | ADC1 in1    |                  |
|  24     | PA1         | ADC1        | ADC1 in2    |                  |
|  25     | PA2         | ADC2        | ADC1 in3    |                  |
|  26     | PA3         | ADC3        | ADC1 in4    |                  |
|  27     | PF4         | ADC4        | ADC1 in5    |                  |
|  28     | (VDD)       |             |             |                  |
|  29     | PA4         | ADC ext.    | ADC2 in1    | ext. ADC channel |
|  30     | PA5         | -           | -           |                  |
|  31     | PA6         | -           | -           |                  |
|  32     | PA7         | -           | -           |                  |
|  33     | PC4         | -           | -           | External USART   |
|  34     | PC5         | -           | -           |                  |
|  35     | PB0         | Buzzer      | slow out    | Signal buzzer    |
|  36     | PB1         | -           | -           |                  |
|  37     | PB2         | -           | -           |                  |
|  38     | PE7         | -           | -           |                  |
|  39     | PE8         | LED0        | slow out    | External LEDs    |
|  40     | PE9         | LED1        | slow out    |                  |
|  41     | PE10        | LED2        | slow out    |                  |
|  42     | PE11        | LED3        | slow out    |                  |
|  43     | PE12        | -           | -           |                  |
|  44     | PE13        | -           | -           |                  |
|  45     | PE14        | -           | -           |                  |
|  46     | PE15        | -           | -           |                  |
|  47     | PB10        | SCRN DCRS   | slow out    | Screen data/cmd  |
|  48     | PB11        | SCRN RST    | slow out    | Screen reset     |
|  49     | (VSS)       |             |             |                  |
|  50     | (VDD)       |             |             |                  |
|  51     | PB12        | SCRN NSS    | slow out    | Screen activate  |
|  52     | PB13        | SCRN SCK    | AF5         | SPI for screen   |
|  53     | PB14        | SCRN MISO   | AF5         |                  |
|  54     | PB15        | SCRN MOSI   | AF5         |                  |
|  55     | PD8         | -           | -           |                  |
|  56     | PD9         | BTN0        | slow in PU  | User buttons     |
|  57     | PD10        | BTN1        | slow in PU  |                  |
|  58     | PD11        | BTN2        | slow in PU  |                  |
|  59     | PD12        | BTN3        | slow in PU  |                  |
|  60     | PD13        | BTN4        | slow in PU  |                  |
|  61     | PD14        | BTN5        | slow in PU  |                  |
|  62     | PD15        | BTN6        | slow in PU  |                  |
|  63     | PC6         | -           | -           |                  |
|  64     | PC7         | -           | -           |                  |
|  65     | PC8         | -           | -           |                  |
|  66     | PC9         | USB PU      | slow out    | USB DP pullup    |
|  67     | PA8         | -           | -           |                  |
|  68     | PA9         | SCL         | AF4 (I2C2)  | external ADC     |
|  69     | PA10        | SDA         | AF4         |                  |
|  70     | PA11        | USB DM      | AF14        | USB              |
|  71     | PA12        | USB DP      | AF14        |                  |
|  72     | PA13        | SWDIO       | dflt        | dbg              |
|  73     | PF6         | -           | -           |                  |
|  74     | (VSS)       |             |             |                  |
|  75     | (VDD)       |             |             |                  |
|  76     | PA14        | SWCLK       | dflt        | dbg              |
|  77     | PA15        | -           | -           |                  |
|  78     | PC10        | Tx          | AF7 (USART3)| Ext. UART        |
|  79     | PC11        | Rx          | AF7         |  (alt.: AF5 - U4)|
|  80     | PC12        | -           | -           |                  |
|  81     | PD0         | CAN Rx      | AF7 (CAN)   | CAN bus          |
|  82     | PD1         | CAN Tx      | AF7         |                  |
|  83     | PD2         | -           | -           |                  |
|  84     | PD3         | -           | -           |                  |
|  85     | PD4         | DE          | slow out    | 0-rx, 1-tx (485) |
|  86     | PD5         | U2Tx        | AF7         | RS-485           |
|  87     | PD6         | U2Rx        | AF7         |                  |
|  88     | PD7         | -           | -           |                  |
|  89     | PB3         | -           | -           |                  |
|  90     | PB4         | -           | -           |                  |
|  91     | PB5         | -           | -           |                  |
|  92     | PB6         | I2C1 SCL    | AF4 (I2C1)  | I2C for BME280   |
|  93     | PB7         | I2C1 SDA    | AF4         |                  |
|  94     | (BOOT0)     |             |             | boot mode        |
|  95     | PB8         | -           | -           |                  |
|  96     | PB9         | -           | -           |                  |
|  97     | PE0         | -           | -           |                  |
|  98     | PE1         | -           | -           |                  |
|  99     | (VSS)       |             |             |                  |
| 100     | (VDD)       |             |             |                  |


### Sorted by port
// sort -Vk4 Readme.md 

|**Pin #**|**Pin name **| **function**| **settings**|**comment **      |
|  23     | PA0         | ADC0        | ADC1 in1    |                  |
|  24     | PA1         | ADC1        | ADC1 in2    |                  |
|  25     | PA2         | ADC2        | ADC1 in3    |                  |
|  26     | PA3         | ADC3        | ADC1 in4    |                  |
|  29     | PA4         | ADC ext.    | ADC2 in1    | ext. ADC channel |
|  30     | PA5         | -           | -           |                  |
|  31     | PA6         | -           | -           |                  |
|  32     | PA7         | -           | -           |                  |
|  67     | PA8         | -           | -           |                  |
|  68     | PA9         | SCL         | AF4 (I2C2)  | external ADC     |
|  69     | PA10        | SDA         | AF4         |                  |
|  70     | PA11        | USB DM      | AF14        | USB              |
|  71     | PA12        | USB DP      | AF14        |                  |
|  72     | PA13        | SWDIO       | AF0         | dbg              |
|  76     | PA14        | SWCLK       | AF0         | dbg              |
|  77     | PA15        | -           | -           |                  |
|  35     | PB0         | Buzzer      | slow out    | Signal buzzer    |
|  36     | PB1         | -           | -           |                  |
|  37     | PB2         | -           | -           |                  |
|  89     | PB3         | -           | -           |                  |
|  90     | PB4         | -           | -           |                  |
|  91     | PB5         | -           | -           |                  |
|  92     | PB6         | I2C1 SCL    | AF4 (I2C1)  | I2C for BME280   |
|  93     | PB7         | I2C1 SDA    | AF4         |                  |
|  95     | PB8         | -           | -           |                  |
|  96     | PB9         | -           | -           |                  |
|  47     | PB10        | SCRN DCRS   | slow out    | Screen data/cmd  |
|  48     | PB11        | SCRN RST    | slow out    | Screen reset     |
|  51     | PB12        | SCRN NSS    | slow out    | Screen activate  |
|  52     | PB13        | SCRN SCK    | AF5         | SPI for screen   |
|  53     | PB14        | SCRN MISO   | AF5         |                  |
|  54     | PB15        | SCRN MOSI   | AF5         |                  |
|  15     | PC0         | ADC5        | ADC1 in 6   | Thermometers     |
|  16     | PC1         | ADC6        | ADC1 in 7   |  (level-meter)   |
|  17     | PC2         | ADC7        | ADC1 in 8   |  ADC channels    |
|  18     | PC3         | ADC8        | ADC1 in 9   |                  |
|  33     | PC4         | -           | -           | External USART   |
|  34     | PC5         | -           | -           |                  |
|  63     | PC6         | -           | -           |                  |
|  64     | PC7         | -           | -           |                  |
|  65     | PC8         | -           | -           |                  |
|  66     | PC9         | USB PU      | slow out    | USB DP pullup    |
|  78     | PC10        | Tx          | AF7 (USART3)| Ext. UART        |
|  79     | PC11        | Rx          | AF7         |  (alt.: AF5 - U4)|
|  80     | PC12        | -           | -           |                  |
|   7     | PC13        | -           | -           |                  |
|   8     | PC14        | -           | -           |                  |
|   9     | PC15        | -           | -           |                  |
|  81     | PD0         | CAN Rx      | AF7 (CAN)   | CAN bus          |
|  82     | PD1         | CAN Tx      | AF7         |                  |
|  83     | PD2         | -           | -           |                  |
|  84     | PD3         | -           | -           |                  |
|  85     | PD4         | DE          | slow out    | 0-rx, 1-tx (485) |
|  86     | PD5         | U2Tx        | AF7         | RS-485           |
|  87     | PD6         | U2Rx        | AF7         |                  |
|  88     | PD7         | -           | -           |                  |
|  55     | PD8         | -           | -           |                  |
|  56     | PD9         | BTN0        | slow in PU  | User buttons     |
|  57     | PD10        | BTN1        | slow in PU  |                  |
|  58     | PD11        | BTN2        | slow in PU  |                  |
|  59     | PD12        | BTN3        | slow in PU  |                  |
|  60     | PD13        | BTN4        | slow in PU  |                  |
|  61     | PD14        | BTN5        | slow in PU  |                  |
|  62     | PD15        | BTN6        | slow in PU  |                  |
|  97     | PE0         | -           | -           |                  |
|  98     | PE1         | -           | -           |                  |
|   1     | PE2         | PWM0        | AF2         | TIM3 channels    |
|   2     | PE3         | PWM1        | AF2         |   as PWM output  |
|   3     | PE4         | PWM2        | AF2         |                  |
|   4     | PE5         | PWM3        | AF2         |                  |
|   5     | PE6         | -           | -           |                  |
|  38     | PE7         | -           | -           |                  |
|  39     | PE8         | LED0        | slow out    | External LEDs    |
|  40     | PE9         | LED1        | slow out    |                  |
|  41     | PE10        | LED2        | slow out    |                  |
|  42     | PE11        | LED3        | slow out    |                  |
|  43     | PE12        | -           | -           |                  |
|  44     | PE13        | -           | -           |                  |
|  45     | PE14        | -           | -           |                  |
|  46     | PE15        | -           | -           |                  |
|  12     | PF0         | OSC IN      |             |                  |
|  13     | PF1         | OSC OUT     |             |                  |
|  19     | PF2         | ADC9        | ADC1 in 10  |                  |
|  73     | PF6         | -           | -           |                  |
|  10     | PF9         | -           | -           |                  |
|  11     | PF10        | VadcON      | slow out    | turn ON ADC power|
|---------|-------------|-------------|-------------|------------------|
