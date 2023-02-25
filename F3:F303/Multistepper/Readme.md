Management of 8 independend steppers
====================================

Eighth stepper could be changed to 8 dependent multiplexed. Based on STM32F303VDT6.

# Pinout

(all GPIO outs are push-pull if not mentioned another)

### Sorted by pin number

|**Pin #**|**Pin name **| **function**| **settings**  |**comment **         |
|---------|-------------|-------------|---------------|---------------------|
|   1     | PE2         | DIAG        | in            | `diag` output of TMC|
|   2     | PE3         | MUL0        | slow out      | multiplexer address |
|   3     | PE4         | MUL1        | slow out      |  for DIAG input     |
|   4     | PE5         | MUL2        | slow out      |                     |
|   5     | PE6         | MUL EN      | slow out      | enable mul.         |
|   6     | (vbat)      | -           | -             |                     |
|   7     | PC13        | M0 L1       | slow in PU/out| enable motor        |
|   8     | PC14        | M0 L0       | slow in PU    | direction of motor  |
|   9     | PC15        | M0 DIR      | slow out      | limit-switch 0      |
|  10     | PF9         | M0 STEP     | AF            | clock of motor      |
|  11     | PF10        | M0 EN       | slow out      | l-s 0 or CS of SPI  |
|  12     | (OSC IN)    |             |               |                     |
|  13     | (OSC OUT)   |             |               |                     |
|  14     | (nrst)      |             |               | reset               |
|  15     | PC0         | MOTMUL0     | slow out      | external motors     |
|  16     | PC1         | MOTMUL1     | slow out      |  multiplexer        |
|  17     | PC2         | MOTMUL2     | slow out      |                     |
|  18     | PC3         | MOTMUL EN   | slow out      |                     |
|  19     | PF2         | ADC2 in10   | ADC           | 5V in / 2           |
|  20     | (VSSA)      |             |               |                     |
|  21     | (VREF+)     |             |               |                     |
|  22     | (VDDA)      |             |               |                     |
|  23     | PA0         | ADC1 in1    | ADC           | External ADC inputs |
|  24     | PA1         | ADC1 in2    | ADC           |                     |
|  25     | PA2         | ADC1 in3    | ADC           |                     |
|  26     | PA3         | ADC1 in4    | ADC           |                     |
|  27     | (VSS)       |             |               |                     |
|  28     | (VDD)       |             |               |                     |
|  29     | PA4         | ADC2 in1    | ADC           | Vdrive / 10         |
|  30     | PA5         | SPI1 SCK    | AF            | In case of SPI TMC  |
|  31     | PA6         | SPI1 MISO   | AF            |                     |
|  32     | PA7         | SPI1 MOSI   | AF            |                     |
|  33     | PC4         | USART1 Tx   | AF            | External USART      |
|  34     | PC5         | USART1 Rx   | AF            |                     |
|  35     | PB0         | M7 STEP     | AF            |                     |
|  36     | PB1         | M7 EN       | slow out      |                     |
|  37     | PB2         | M7 DIR      | slow out      |                     |
|  38     | PE7         | M7 L0       | slow in PU    |                     |
|  39     | PE8         | M7 L1       | slow in PU/out|                     |
|  40     | PE9         | M6 STEP     | AF            |                     |
|  41     | PE10        | M6 L1       | slow in PU/out|                     |
|  42     | PE11        | M6 L0       | slow in PU    |                     |
|  43     | PE12        | M6 DIR      | slow out      |                     |
|  44     | PE13        | M6 EN       | slow out      |                     |
|  45     | PE14        | OUT2        | slow in PU/out| external GPIO       |
|  46     | PE15        | OUT1        | slow in PU/out|                     |
|  47     | PB10        | USART3 Tx   | AF            | USART for 4 of TMC  |
|  48     | PB11        | OUT0        | slow in PU/out|                     |
|  49     | (VSS)       |             |               |                     |
|  50     | (VDD)       |             |               |                     |
|  51     | PB12        | SCRN DCRS   | slow out      | screen commands     |
|  52     | PB13        | SCRN SCK    | AF            | SPI for screen      |
|  53     | PB14        | SCRN MISO   | AF            |                     |
|  54     | PB15        | SCRN MOSI   | AF            |                     |
|  55     | PD8         | SCRN RST    | slow out      | reset screen        |
|  56     | PD9         | SCRN CS     | slow out      | activate screen     |
|  57     | PD10        | M5 L1       | slow in PU/out|                     |
|  58     | PD11        | M5 L0       | slow in PU    |                     |
|  59     | PD12        | M5 STEP     | AF            |                     |
|  60     | PD13        | M5 DIR      | slow out      |                     |
|  61     | PD14        | M5 EN       | slow out      |                     |
|  62     | PD15        | M4 L1       | slow in PU/out|                     |
|  63     | PC6         | M4 STEP     | AF            |                     |
|  64     | PC7         | M4 L0       | slow in PU    |                     |
|  65     | PC8         | M4 DIR      | slow out      |                     |
|  66     | PC9         | M4 EN       | slow out      |                     |
|  67     | PA8         | USBpu       | slow out PP   |  USB DP pullup      |
|  68     | PA9         | BTN1        | slow in PU    |  buttons to operate |
|  69     | PA10        | BTN2 (SDA)  | slow in PU/AF |  with screen        |
|  70     | PA11        | USB DM      | AF            | USB                 |
|  71     | PA12        | USB DP      | AF            |                     |
|  72     | PA13        | SWDIO       | dflt          | dbg/flash           |
|  73     | PF6         | BTN3 (SCL)  | slow in PU/AF | (possible I2C2)     |
|  74     | (VSS)       |             |               |                     |
|  75     | (VDD)       |             |               |                     |
|  76     | PA14        | SWCLK       | dflt          | debug/sew           |
|  77     | PA15        | M3 STEP     | AF            |                     |
|  78     | PC10        | M3 L0       | slow in PU    |                     |
|  79     | PC11        | M3 L1       | slow in PU/out|                     |
|  80     | PC12        | M3 DIR      | slow out      |                     |
|  81     | PD0         | CAN Rx      | AF            | CAN bus             |
|  82     | PD1         | CAN Tx      | AF            |                     |
|  83     | PD2         | M3 EN       | slow out      |                     |
|  84     | PD3         | BTN4        | slow in PU    |                     |
|  85     | PD4         | BTN5        | slow in PU    |                     |
|  86     | PD5         | BTN6        | slow in PU    |                     |
|  87     | PD6         | M2 L1       | slow in PU    |                     |
|  88     | PD7         | M2 L0       | slow in PU/out|                     |
|  89     | PB3         | USART2 Tx   | AF            | USART for 4 of TMC  |
|  90     | PB4         | M2 DIR      | slow out      |                     |
|  91     | PB5         | M2 STEP     | AF            |                     |
|  92     | PB6         | M2 EN       | slow out      |                     |
|  93     | PB7         | M1 L1       | slow in PU/out|                     |
|  94     | (BOOT0)     |             |               | boot mode           |
|  95     | PB8         | M1 STEP     | AF            |                     |
|  96     | PB9         | M1 L0       | slow in PU    |                     |
|  97     | PE0         | M1 DIR      | slow out      |                     |
|  98     | PE1         | M1 EN       | slow out      |                     |
|  99     | (VSS)       |             |               |                     |
| 100     | (VDD)       |             |               |                     |

### Some comments.
_DIAG_ input used to detect problems with TMC drivers (multiplexed by _MUL_ outputs).
Each motor have next control signals: _EN_ - enable driver, _DIR_ - rotation direction, _STEP_ - microstepping clock signal, _L0_ - zero end-switch, _L1_ - max end-switch (or Cable Select for SPI-based TMC).
_MOTMUL_ - external multiplexer or other GPIO signals (e.g. to manage of 64 stepper motors).
_ADC1 in_ - four external ADC signals (0..3.3V).
_SPI1_ used to manage TMC drivers in case of SPI-based.
_USART1_ - to connect external something (master or slave).
_OUT_ - external GPIO.
_USART2 Tx_ used to manage USART-based TMC (numbers 0-3).
_USART3 Tx_ used to manage USART-based TMC (numbers 4-7).
_SCRN_ - control signals for SPI TFT screen.
_BTN_ - external button, keypad, joystick etc. Two of them could be connected to I2C devices.
_USB_ and _CAN_ used as main device control buses.
_SW_ used as debugging/sewing; also (I remember about USB pullup only after end of PCB design) _SWDIO_ used as USB pullup (so the device have no USB in debug mode - when BTN0 and BTN1 are pressed at start).

### Sorted by ports (with AF numbers).

|**Pin #**|**Pin name **| **function**| **settings**  |**comment **         |
|---------|-------------|-------------|---------------|---------------------|
|  23     | PA0         | ADC1 in1    | ADC           | External ADC inputs |
|  24     | PA1         | ADC1 in2    | ADC           |                     |
|  25     | PA2         | ADC1 in3    | ADC           |                     |
|  26     | PA3         | ADC1 in4    | ADC           |                     |
|  29     | PA4         | ADC2 in1    | ADC           | Vdrive / 10         |
|  30     | PA5         | SPI1 SCK    | AF5           | In case of SPI TMC  |
|  31     | PA6         | SPI1 MISO   | AF5           |                     |
|  32     | PA7         | SPI1 MOSI   | AF5           |                     |
|  67     | PA8         | BTN0        | slow in PU    | Buttons/joystick    |
|  68     | PA9         | BTN1        | slow in PU    |  to operate with    |
|  69     | PA10        | BTN2 (SDA)  | slow in PU/AF4|  screen             |
|  70     | PA11        | USB DM      | USB           | USB                 |
|  71     | PA12        | USB DP      | USB           |                     |
|  72     | PA13        | SWDIO/USBpu | dflt/slow out | USB pullup or dbg   |
|  76     | PA14        | SWCLK       | dflt          | debug/sew           |
|  77     | PA15        | M3 STEP     | AF1 (T2ch1)   |                     |
|  35     | PB0         | M7 STEP     | AF2 (T3ch3)   |                     |
|  36     | PB1         | M7 EN       | slow out      |                     |
|  37     | PB2         | M7 DIR      | slow out      |                     |
|  89     | PB3         | USART2 Tx   | AF7           | USART for 4 of TMC  |
|  90     | PB4         | M2 DIR      | slow out      |                     |
|  91     | PB5         | M2 STEP     | AF10 (T17ch1) |                     |
|  92     | PB6         | M2 EN       | slow out      |                     |
|  93     | PB7         | M1 L1       | slow in PU/out|                     |
|  95     | PB8         | M1 STEP     | AF1 (T16ch1)  |                     |
|  96     | PB9         | M1 L0       | slow in PU    |                     |
|  47     | PB10        | USART3 Tx   | AF7           | USART for 4 of TMC  |
|  48     | PB11        | OUT0        | slow in PU/out|                     |
|  51     | PB12        | SCRN DCRS   | slow out      | screen commands     |
|  52     | PB13        | SCRN SCK    | AF5           | SPI for screen      |
|  53     | PB14        | SCRN MISO   | AF5           |                     |
|  54     | PB15        | SCRN MOSI   | AF5           |                     |
|  15     | PC0         | MOTMUL0     | slow out      | external motors     |
|  16     | PC1         | MOTMUL1     | slow out      |  multiplexer        |
|  17     | PC2         | MOTMUL2     | slow out      |                     |
|  18     | PC3         | MOTMUL EN   | slow out      |                     |
|  33     | PC4         | USART1 Tx   | AF7           | External USART      |
|  34     | PC5         | USART1 Rx   | AF7           |                     |
|  63     | PC6         | M4 STEP     | AF4 (T8ch1)   |                     |
|  64     | PC7         | M4 L0       | slow in PU    |                     |
|  65     | PC8         | M4 DIR      | slow out      |                     |
|  66     | PC9         | M4 EN       | slow out      |                     |
|  78     | PC10        | M3 L0       | slow in PU    |                     |
|  79     | PC11        | M3 L1       | slow in PU/out|                     |
|  80     | PC12        | M3 DIR      | slow out      |                     |
|   7     | PC13        | M0 L1       | slow in PU/out| enable motor        |
|   8     | PC14        | M0 L0       | slow in PU    | direction of motor  |
|   9     | PC15        | M0 DIR      | slow out      | limit-switch 0      |
|  81     | PD0         | CAN Rx      | AF7           | CAN bus             |
|  82     | PD1         | CAN Tx      | AF7           |                     |
|  83     | PD2         | M3 EN       | slow out      |                     |
|  84     | PD3         | BTN4        | slow in PU    |                     |
|  85     | PD4         | BTN5        | slow in PU    |                     |
|  86     | PD5         | BTN6        | slow in PU    |                     |
|  87     | PD6         | M2 L1       | slow in PU    |                     |
|  88     | PD7         | M2 L0       | slow in PU/out|                     |
|  55     | PD8         | SCRN RST    | slow out      | reset screen        |
|  56     | PD9         | SCRN CS     | slow out      | activate screen     |
|  57     | PD10        | M5 L1       | slow in PU/out|                     |
|  58     | PD11        | M5 L0       | slow in PU    |                     |
|  59     | PD12        | M5 STEP     | AF2 (T4ch1)   |                     |
|  60     | PD13        | M5 DIR      | slow out      |                     |
|  61     | PD14        | M5 EN       | slow out      |                     |
|  62     | PD15        | M4 L1       | slow in PU/out|                     |
|  97     | PE0         | M1 DIR      | slow out      |                     |
|  98     | PE1         | M1 EN       | slow out      |                     |
|   1     | PE2         | DIAG        | slow in PU    | `diag` output of TMC|
|   2     | PE3         | MUL0        | slow out      | multiplexer address |
|   3     | PE4         | MUL1        | slow out      |  for DIAG input     |
|   4     | PE5         | MUL2        | slow out      |                     |
|   5     | PE6         | MUL EN      | slow out      | enable mul.         |
|  38     | PE7         | M7 L0       | slow in PU    |                     |
|  39     | PE8         | M7 L1       | slow in PU/out|                     |
|  40     | PE9         | M6 STEP     | AF2 (T1ch1)   |                     |
|  41     | PE10        | M6 L1       | slow in PU/out|                     |
|  42     | PE11        | M6 L0       | slow in PU    |                     |
|  43     | PE12        | M6 DIR      | slow out      |                     |
|  44     | PE13        | M6 EN       | slow out      |                     |
|  45     | PE14        | OUT2        | slow in PU/out| external GPIO       |
|  46     | PE15        | OUT1        | slow in PU/out|                     |
|  19     | PF2         | ADC2 in10   | ADC           | 5V in / 2           |
|  73     | PF6         | BTN3 (SCL)  | slow in PU/AF4| (possible I2C2)     |
|  10     | PF9         | M0 STEP     | AF3 (T15ch1)  | clock of motor      |
|  11     | PF10        | M0 EN       | slow out      | l-s 0 or CS of SPI  |


## DMA usage

* ADC1 - DMA1_ch1
* ADC2 - DMA2_ch1
* USART2 (PDN-UART for drivers 0..3) - DMA1_ch6 (Rx), DMA1_ch7 (Tx)
* USART3 (PDN-UART for drivers 4..7) - DMA1_ch3 (Rx), DMA1_ch2 (Tx)
* SPI2 (screen) - DMA1_ch4 (Rx), DMA1_ch5 (Tx) [or may be dedicated to USART1]

# Other stepper drivers connection

## DRV8825
Solder jumpers E2, B and A to connect ~RST and ~SLP to 3.3V.

Microstepping selection produced by soldering H2 (bit0), G2 (bit1) and/or C (bit2).