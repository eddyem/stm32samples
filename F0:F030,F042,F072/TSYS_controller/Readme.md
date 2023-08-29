# Firmware for controllers of thermal sensors

Make regular scan of 8 sensors' pairs.
USART speed 115200. Code for ../../kicad/stm32

## Serial interface commands (ends with '\n'), small letter for only local processing:
- **0...7**  send message to Nth controller, not broadcast (after number should be CAN command)
- **@**  set/reset debug mode
- **a**  get raw ADC values
- **B**  send dummy CAN messages to broadcast address
- **b**  get/set CAN bus baudrate
- **c**  show coefficients for all thermosensors
- **D**  send dummy CAN messages to master (0) address
- **d**  get current CAN address of device
- **Ee** end temperature scan
- **Ff** turn sensors off
- **g**  group (sniffer) CAN mode (print to USB terminal all incoming CAN messages with alien IDs)
- **Hh** switch I2C to high speed (100kHz)
- **Ii** (re)init sensors
- **Jj** get MCU temperature
- **Kk** get values of U and I
- **Ll** switch I2C to low speed (default, 10kHz)
- **Mm** change master id to 0 (**m**) / broadcast (**M**)
- **N**  get build number
- **Oo** turn onboard diagnostic LEDs **O**n or **o**ff (both commands are local!)
- **P**  ping everyone over CAN
- **Qq** get system time
- **Rr** reinit I2C
- **s**  send CAN message (format: ID data[0..8], dec, 0x - hex, 0b - binary)
- **Tt** start single temperature measurement
- **u**  unique ID (default) CAN mode
- **Vv** very low speed
- **Xx** go into temperature scan mode
- **Yy** get sensors state over CAN (data format: 3 - state, 4,5 - presense mask [0,1], 6 - npresent, 7 - ntempmeasured
- **z**  check CAN status for errors

The command **M** allows to temporaly change master ID of all
controllers to broadcast ID. So all data they sent will be 
accessed @ any controller.

## PINOUT
- **I2C**: PB6 (SCL) & PB7 (SDA)
- **USART1**: PA9 (Tx) & PA10 (Rx)
- **CAN bus**: PB8 (Rx), PB9 (Tx)
- **USB bus**: PA11 (DM), PA12 (DP)
- **I2C multiplexer**: PB0..PB2 (0..2 address bits), PB12 (~EN)
- **sensors' power**: PB3 (in, overcurrent), PA8 (out, enable power)
- **signal LEDs**: PB10 (LED0), PB11 (LED1)
- **ADC inputs**: PA0 (V12/4.93), PA1 (V5/2), PA3 (I12 - 1V/A), PA6 (V3.3/2)
- **controller CAN address**: PA13..PA15 (0..2 bits), PB15 (3rd bit); 0 - master, other address - slave


## LEDS
- LED0 (nearest to sensors' connectors) - heartbeat
- LED1 (above LED0) - CAN bus OK

## CAN protocol
Variable data length: from 1 to 8 bytes.
First (number zero) byte of every sequence is command mark (0xA5) or data mark (0x5A).

## Commands
### Common commands
-    `CMD_PING`                (0)  request for PONG cmd
-    `CMD_START_MEASUREMENT`   (1)  start single temperature measurement
-    `CMD_SENSORS_STATE`       (2)  get sensors state
-    `CMD_START_SCAN`          (3)  run scan mode 
-    `CMD_STOP_SCAN`           (4)  stop scan mode
-    `CMD_SENSORS_OFF`         (5)  turn off power of sensors
-    `CMD_LOWEST_SPEED`        (6)  lowest I2C speed
-    `CMD_LOW_SPEED`           (7)  low I2C speed (10kHz)
-    `CMD_HIGH_SPEED`          (8)  high I2C speed (100kHz)
-    `CMD_REINIT_I2C`          (9)  reinit I2C with current speed
-    `CMD_CHANGE_MASTER_B`     (10) change master id to broadcast
-    `CMD_CHANGE_MASTER`       (11) change master id to 0
-    `CMD_GETMCUTEMP`          (12) MCU temperature value
-    `CMD_GETUIVAL`            (13) request to get values of V12, V5, I12 and V3.3
-    `CMD_GETUIVAL0`           (14) answer with values of V12 and V5
-    `CMD_GETUIVAL1`           (15) answer with values of I12 and V3.3
-    `CMD_REINIT_SENSORS`      (16) (re)init all sensors (discover all and get calibrated data)
-    `CMD_GETBUILDNO`          (17) get by CAN firmware build number (uint32_t, littleendian, starting from byte #4)
-    `CMD_SYSTIME`             (18) get system time in ms (uint32_t, littleendian, starting from byte #4)

### Dummy commands for test purposes
-    `CMD_DUMMY0` = 0xDA,
-    `CMD_DUMMY1` = 0xAD

### Commands data format
- byte 1 - Controller number
- byte 2 - Command received
- bytes 3..7 - data

### Thermal data format
- byte 3 - Sensor number (10*N + M, where N is multiplexer number, M - number of sensor in pair, i.e. 0,1,10,11,20,21...70,71)
- byte 4 - thermal data H
- byte 5 - thermal data L

### Sensors state data format
- byte 3 - Sstate value:
  -   `[SENS_INITING]`       = "init"
  -   `[SENS_RESETING]`      = "reset"
  -   `[SENS_GET_COEFFS]`    = "getcoeff"
  -   `[SENS_SLEEPING]`      = "sleep"
  -   `[SENS_START_MSRMNT]`  = "startmeasure"
  -   `[SENS_WAITING]`       = "waitresults"
  -   `[SENS_GATHERING]`     = "collectdata"
  -   `[SENS_OFF]`           = "off"
  -   `[SENS_OVERCURNT]`     = "overcurrent"
  -   `[SENS_OVERCURNT_OFF]` = "offbyovercurrent"
- byte 4 - `sens_present[0]` value
- byte 5 - `sens_present[1]` value
- byte 6 - `Nsens_present` value
- byte 7 - `Ntemp_measured` value

### MCU temperature data format
- byte 3 - data H
- byte 4 - data L

All temperature is in degrC/100!

### U and I data format
- byte 2 - type of data (`CMD_GETUIVAL0` - V12 and V5, `CMD_GETUIVAL1` - I12 and V3.3)

case CMD_GETUIVAL0

- bytes 3,4 - V12 H/L
- bytes 5,6 - V5 H/L

case CMD_GETUIVAL1

- bytes 3,4 - I12 H/L
- bytes 5,6 - V33 H/L

Voltage is in V/100, Current is in mA
