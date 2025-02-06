Management of 8 independend steppers
====================================

Eighth stepper could be changed to 8 dependent multiplexed. Based on STM32F303VDT6.

# Pinout

(all GPIO outs are push-pull if not mentioned another)

### Sorted by pin number

| Pin #   | Pin name    | function    | settings      | comment             |
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
**DIAG** input used to detect problems with TMC drivers (multiplexed by **MUL** outputs).

Each motor have next control signals: **EN** - enable driver, **DIR** - rotation direction, **STEP** - microstepping clock signal, **L0** - zero end-switch,
**L1** - max end-switch (or Cable Select for SPI-based TMC).

**MOTMUL** - external multiplexer or other GPIO signals (e.g. to manage of 64 stepper motors).

**ADC1 in** - four external ADC signals (0..3.3V).

**SPI1** used to manage TMC drivers in case of SPI-based.

**USART1** - to connect external something (master or slave).

**OUT** - external GPIO.

**USART2 Tx** used to manage USART-based TMC (numbers 0-3).

**USART3 Tx** used to manage USART-based TMC (numbers 4-7).

**SCRN** - control signals for SPI TFT screen.

**BTN** - external button, keypad, joystick etc. Two of them could be connected to I2C devices.

**USB** and **CAN** used as main device control buses.

**SW** used as debugging/flashing.

### Sorted by ports (with AF numbers).

| Pin #   | Pin name    | function    | settings      | comment             |
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


# Stepper drivers connection

After changing stepper type by soldering/desoldering appropriate jumpers, don't forget to change their types in settings
(and save settings in MSU flash memory after checking it with `dumpconf`).

Connection diagram for soldering jumpers shown on reverse side of PCB. 

Jumpers **A**, **B** and **C** allow to short the contacts of driver's pins 6 (CLK / DCO / ~SLEEP), 5 (PDN_UART / SDO / ~RESET) and 4
(SPREAD / CS / M2), respectively. 

Jumper **D1** connects MCU UART output with jumper **E1**, **D2** connects MCU MISO with same contact of **E1**.
Jumpers **E1** and **E2** allows to pullup pin 5 of driver to be connected with 3.3V or UART/MISO.

Jumper **F1** allows to connect pin 4 to CS line. **F2** connects pin 4 to 3.3V.

Jumper **G1** connects pin 3 (MS2 / SCK / M1) to SCK line. **G2** connects this pin to 3.3V.

Jumper **H1** connects pin pin 2 (MS1 / SDI / M0) to MOSI line  **H2** connects it to 3.3V.

So, depending on driver type and its mode you can solder these jumpers so, than driver can work over PDN-UART, SPI or configuration
pins can select microstepping directly.


## DRV8825 and other simplest drivers
Solder jumpers **E2**, **B** and **A** to connect **~RST** and **~SLP** to 3.3V.

Microstepping selection produced by soldering **H2** (bit0), **G2** (bit1) and/or **C** (bit2).

## TMC2209 and other PDN-UART based
Solder jumpers **D1** and **E1** to connect MCU UART2/UART3 to PDN-UART pin of driver. Solder jumpers **G2**/**H2** according 
address table (X - soldered, O - not soldered):

| Address | G2    | H2    |
|:-------:|:-----:|:-----:|
| 0       | **O** | **O** |
| 1       | **O** | **X** |
| 2       | **X** | **O** |
| 3       | **X** | **X** |
| 4       | **O** | **O** |
| 5       | **O** | **X** |
| 6       | **X** | **O** |
| 7       | **X** | **X** |

## TMC2130 and other SPI based
(not negotiated yet)

Solder jumpers **D2** and **E1** to connect MISO, **F1** to connect CS, **G1** for SCK and **H1** for MOSI.

# Protocol

## Errors
Error codes are in brackets:

- **OK**       (0) - all OK;
- **BADPAR**   (1) - bad parameter (`N`) value (e.g. greater than max available or absent);
- **BADVAL**   (2) - bad setter value (absent, text or over range);
- **WRONGLEN** (3) - (only for CAN bus, you can't meet in text protocol) wrong CAN message length;
- **BADCMD**   (4) - unknown command, in case of USB proto user will see full command list instead of this error;
- **CANTRUN**  (5) - can't run required function.

## USB

Used simple text protocol each string should contain one command and ends with `'\n'`.
Uppercase N at the end of command means parameter number (No of motor, button, ADC channel etc).
Getters and action commands have simplest format `command[N]`: some commands have no parameter,
other have two variants - with and without parameter; wariant without parameter will act on all
parameters (e.g. getter for all buttons) - in this case `N` enclosed in square brackets.
Setters have format `command[N] = value`. The `value` is integer number, so in case floating values
you need to multiply it by 1000 and write result. So, voltage and temperature are measured in millivolts
and thousandths of degC respectively.

Some commands (especially that have no CAN analogue) have non-standard format described later.
They are not intended to be used in automatic control systems, only in manual mode.

Answer for every (excluding special) setter and getter in case of success is `command[N]=value`
showing current value for getter or new value for setter.
In case of error you will see one of error mesages (message `OK` with errcode=0 usually not shown). 
Also you can get such answers:

- `BADCMD` - wrong command (not in dictionary);
- `BADARGS` - bad arguments format (e.g. for command `cansend` etc);
- `FAIL` - if something wrong and parser got impermissible error code.

In following list letter `G` means "getter", `S` - "setter", no of them - custom USB-only command.
The number in brackets is CAN command code. The asterisk following command means that it isn't implemented yet (and maybe
will be never implemented).

### absposN (35) GS
Absolute stepper position in steps, setter just changes current value. E.g. you want to set current position 
as zero (be carefull: `gotoz` will zero position again on a zero-point limit switch). Maximum absolute value is `maxstepsN`.
### accelN (17) GS
Stepper acceleration/deceleration on ramp (steps/s^2), only positive. Maximum value is `ACCELMAXSTEPS` from `flash.h`.
### adcN (4) G
ADC value (N=0..3).
### buttonN (5) G 
Given (N=0..6) buttons' state. If number is right, returns two strings: `buttonN=state` (where `state` is
`none`, `press`, `hold` or `release`) and `buttontmN=time` (where `time` is time from start of last event).

!!!NOTE: Button numbering starts from 0, not 1 as shown on PCB!!!
### canerrcodes
Print last CAN errcodes.
Print "No errors" or last error code.
### canfilter DATA
CAN bus hardware filters, format: bank# FIFO# mode(M/I) num0 [num1 [num2 [num3]]] .
By default have two filters allowing to listen any CAN ID:

    Filter 0, FIFO0 in MASK mode: ID=0x01, MASK=0x01
    Filter 1, FIFO1 in MASK mode: ID=0x00, MASK=0x01

### canflood ID DATA
Send or clear (if empty) flood message: ID byte0 ... byteN.
On empy message return `NO ID given, send nothing!` (and stops flooding), or `Message parsed OK`.
### canfloodT N
Flood period, N in milliseconds (N >= 0ms).
### canid [ID]
Get or set CAN ID of device. Default CAN ID is "1".
### canignore [ID]
Software ignore list (max 10 IDs), negative to delete all, non-negative to add next. 
### canincrflood ID
Send incremental flood message with given ID. Message have uint32_t little endian type and increments
for each packet starting from 0. Empty command stops flooding (`canflood` too).
### canpause
Pause filtered IN packets displaying. Returns `Pause CAN messages`.
### canreinit
Reinit CAN with last settings. Returns `OK`.
### canresume
Resume filtered IN packets displaying, returns `RESUME CAN messages`.
### cansend ID [data] 
Send data over CAN with given ID. If `data` is omitted, send empty message.
In case of absence of `ACK` you can get message `CAN bus is off, try to restart it`.
### canspeed N
CAN  bus speed (in kbps, 50 <= N <= 1500)
In case of setter, store new speed value in global parameters (and if you call `saveconf` later, it will be saved in flash memory).
As getter show string like `canspeed=250kbps`.
### canstat 
Get CAN bus status: values of registers `CAN->MSR`, `CAN->TSR`, `CAN->RF0R` and `CAN->RF1F`.
### diagn[N] (37) G
DIAG state of motor N (or all, in this case motor 0 is LSB of answer etc.). One means "all OK", zero is error state.
### drvtypeN (45) GS 
Nth driver type (0 - only step/dir, 1 - UART, 2 - SPI, 3 - reserved). This parameter is taken from `.drvtype` bits of `motflags` settings parameter.
### dumperr
Dump error codes. Returns:

    Error codes:
    0 - OK
    1 - BADPAR
    2 - BADVAL
    3 - WRONGLEN
    4 - BADCMD
    5 - CANTRUN

### dumpcmd
Dump command codes. Returns all list of CAN bus command codes.
###  dumpconf
Dump current configuration. Returns a lot of information both common (like CAN bus speed, ID and so on) and
for each motor driver. You can call independeng getters for each of this parameter.
###  dumpmotN
Dump configuration of Nth motor only.
### dumpmotflags 
Dump motor flags' bits (for `motflagsN`) and reaction to limit switches (`eswreact`) values:

    Motor flags:
    bit0 - 0: reverse - invert motor's rotation
    bit1 - 1: [reserved]
    bit2 - 2: [reserved]
    bit3 - 3: donthold - clear motor's power after stop
    bit4 - 4: eswinv - inverse end-switches (1->0 instead of 0->1)
    bit5 - 5: [reserved]
    bit6 - 6,7: drvtype - driver type (0 - only step/dir, 1 - UART, 2 - SPI, 3 - reserved)
    End-switches reaction:
    0 - ignore both end-switches
    1 - ignore ESW1, ESW0 stops only when negative mowing
    2 - stop @ esw in any moving direction
    3 - stop only when moving in given direction (e.g. to minus @ESW0)

### dumpstates 
Dump motors' state codes (for getter `stateN`):

    Motor's state codes:
    0 - relax
    1 - acceleration
    2 - moving
    3 - moving at lowest speed
    4 - deceleration
    5 - stalled (not used here!)
    6 - error

### emstop[N] (29)
Emergency stop Nth motor or all (if `N` absent). Returns `OK` or error text.
### eraseflash [=N] (38)
Erase flash data storage (full or only N'th page of it). Use this option only if you have problems
when try to save current configuration.
### eswN (6) G
Get end-switches state. Return two bits (for limit switches 0 and 1): 1 means "active", 0 - "passive". 
If you use SPI-based driver, only one switch available, so return will be 1-bit.
### eswreactN (24) GS
End-switches (limit-switches) reaction: 0 - ignore both limits; 
1 - ignore ESW1, stop on ESW0 only when moving to negative direction;
2 - stop on any limit switch independently from direction;
3 - stop only on switch corresponding to moving direction (i.e. ESW0 for negative and ESW1 for positive). 
You can modify this values on-the-fly (but only when steppers aren't moving). This can be usefull,
for example, to rotate filter turret into given position using switch 1 to both as limit switch and position stopper.
But even in state 0 (ignore) active state of both switches estimates as error and you won't be able to move motor.
### gotoN (26) GS
Move motor to given absolute position.
### gotozN (32)
Find zero position & refresh counters. The motor would rotate in reverse direction until limit switch 0 acts or amount of
steps (parameter `maxsteps` of configuration) is exhausted.
### gpioconfN* GS
GPIO configuration (0 - PUin, 1 - PPout, 2 - ODout), N=0..2.
### gpio[N] (12) GS
GPIO values, N=0..2. Without `N` run for all GPIOs (each bit is state: 0/1, started from LSB which is zero channel).
By default GPIOs are pulled up inputs, so setters won't do any effect. To reconfigure them as outputs use gpioconf when (if)
it will be available.
### help
Show this help.
### maxspeedN (18) GS
Maximal motor speed (steps per sec). Depends on current microstep configuration. As speed depends on Nth motor's timer settings,
you can't give any value you want. Speed recalculated through ARR value:

    ARR = 26MHz / microsteps / (speed - 1).

Minimal value of ARR is 99 (260kHz), maximal is 0xffff (600Hz). And after that actual speed value calculated:
    
    speed = 26MHz / microsteps / (ARR + 1).

And it can't be greater than 0xffff. So if your motor really can make, let's say, 32000 steps per second, you can run it only
for microstepping not more than 2.
### maxstepsN (21) GS
Maximal steps amount (from zero ESW) for given motor. This is limiting positive position and amount of steps to make when
searching of zero limit switch.
### mcut (7) G
Temperature of MCU in thousandths of degC.
### mcuvdd (8) G
MCU Vdd value.
### microstepsN (16) GS
Microsteps settings (2^0..2^9). Read `maxspeed` about relationship of maximal speed and microsteppings.
### minspeedN (19) GS
Minimal motor speed (steps per sec), this is the starting speed of motor's ramp and default speed for `relslow` command,
so don't try to make it too large.
### motcurrentN (46) GS
Maximal motor current (1..32 for 1/32..32/32 of max current) for TMC drivers. In case of simplest drivers like DRV8825
you should set current using varistor on driver's board.
### motflagsN (23) GS 
Nth motor flags. To understand bit fields of these flags use `dumpmotflags`.
### motmul * (36) GS
external multiplexer status (<0 - disable, 0..7 - enable and set address).
### motno (44) GS
Motor number for next `pdn` commands (if you want to run custom commands by hands or in a batch).
### motreinit (25)
Re-init motors after configuration changed. Some changes will run this automatically.
Answer - "OK" or error text.
### pdnN (43) GS 
Read/write TMC2209 (and other UART-based drivers) registers over uart @ motor number `motno`.
For `pdnN=X` `N` is register number, `X` is data to write into it. Due to protocol's particulars
you can't work with registers with address more than 126 (0x7e).
### ping (1) 
Echo given command back. For CAN bus return original packet, for USB - given argumemt and parameter (but checking parameter).
### relposN (27) GS
Relative (with ramp) move for given amount of steps (get remaining steps for getter). 
### relslowN (28) GS 
Like `relpos` but with constant slowest speed (you can change it with `minspeed`).
### reset (9)
Software reset.
### saveconf (13)
Save current configuration into MCU flash memory.
Answer - "OK" or error text.
### screen*  GS
Enable (1) or disable (0) screen.
### speedlimit (20) G
Get limiting speed for current microsteps setting.
### stateN (33) G
Get Nth motor state (`dumpstates`: 0-relax, 1-accel, 2-move, 3-mvslow, 4-decel, 5-stall, 6-err).
### stopN (30) 
Stop Nth motor with deceleration (moving by ramp).
Answer - "OK" or error text.
### time (10) G
Get time from start (ms).
### tmcbus * GS 
TMC control bus (0 - USART, 1 - SPI), unuseful command; use `motflags` instead.
### udata* (39) GS
Data by usart in slave mode (text strings, '\\n'-terminated).
### usartstatus* (40)
GS status of USART1 (0 - off, 1 - master, 2 - slave).
### vdrive (41) G 
Approximate voltage on Vdrive input, millivolts.
### vfive (42) G
Approximate voltage on 5V bus, millivolts.

## CAN bus
Default CAN ID is 1.
Protocol use variable length incoming messages: from 2 bytes for simplest commands to all 8 bytes for common.
Bytes (in time order):

- **0**, **1** - little-endian command code. 
- **2** - parameter number (0..126), 127 means "no parameter", elder bit (0x80) is a setter flag (without it 
all rest part of message ignored).
- **3** - (only in packets from controller) error code.
- **4**...**8** - little-endian data.

Command codes described in USB answer for `dumpcmd`:

    1 - ping
    4 - adc
    5 - button
    6 - esw
    7 - mcut
    8 - mcuvdd
    9 - reset
    10 - time
    12 - gpio
    13 - saveconf
    16 - microsteps
    17 - accel
    18 - maxspeed
    19 - minspeed
    20 - speedlimit
    21 - maxsteps
    23 - motflags
    24 - eswreact
    25 - motreinit
    26 - goto
    27 - relpos
    28 - relslow
    29 - emstop 
    30 - stop
    32 - gotoz
    33 - state
    35 - abspos
    36 - motmul
    37 - diagn
    38 - eraseflash
    41 - vdrive
    42 - vfive
    43 - pdn
    44 - motno
    45 - drvtype
    46 - motcurrent

Error codes (`dumperr`):

    0 - OK
    1 - BADPAR
    2 - BADVAL
    3 - WRONGLEN
    4 - BADCMD
    5 - CANTRUN

