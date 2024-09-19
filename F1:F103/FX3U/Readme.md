A usefull thing made of chineese FX3U clone
===========================================

Works over RS-232 (default: 115200, 8N1) or CAN (default 250000 baud).

You can see pinout table in file `hardware.c`.

## Serial protocol (each string ends with '\n').

(TODO: add new)


```
commands format: parameter[number][=setter]
parameter [CAN idx] - help
--------------------------

    CAN bus commands:
canbuserr - print all CAN bus errors (a lot of if not connected)
cansniff - switch CAN sniffer mode

    Configuration:
bounce [14] - set/get anti-bounce timeout (ms, max: 1000)
canid [6] - set both (in/out) CAN ID / get in CAN ID
canidin [7] - get/set input CAN ID
canidout [8] - get/set output CAN ID
canspeed [5] - get/set CAN speed (bps)
dumpconf - dump current configuration
eraseflash [10] - erase all flash storage
saveconf [9] - save configuration
usartspeed [15] - get/set USART1 speed

    IN/OUT:
adc [4] - get raw ADC values for given channel
esw [12] - anti-bounce read inputs
eswnow [13] - read current inputs' state
led [16] - work with onboard LED
relay [11] - get/set relay state (0 - off, 1 - on)

    Other commands:
mcutemp [3] - get MCU temperature (*10degrC)
reset [1] - reset MCU
s - send CAN message: ID 0..8 data bytes
time [2] - get/set time (1ms, 32bit)
wdtest - test watchdog
error=badcmd
```

Value in square brackets is CAN bus command code.

## CAN bus protocol

All data in little-endian format!

BYTE -  MEANING

0, 1 - (uint16_t) - command code (value in square brackets upper);

2 - (uint8_t) - parameter number (e.g. ADC channel or X/Y channel number), 0..126 [ORed with 0x80 for setter], 127 means "no parameter";

3 - (uint8_t) - error code (only when device answers for requests);

4..7 - (int32_t) - data.

### CAN bus error codes

0 - `ERR_OK`       - all OK,

1 - `ERR_BADPAR`   - parameter is wrong,

2 - `ERR_BADVAL`   - value is wrong (e.g. out of range),

3 - `ERR_WRONGLEN` - wrong message length (for setter or for obligatory parameter number),

4 - `ERR_BADCMD`   - unknown command code,

5 - `ERR_CANTRUN`  - can't run given command due to bad parameters or other reason.


## MODBUS-RTU protocol

(TODO)