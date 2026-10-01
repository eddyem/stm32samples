# A useful thing made of a Chinese FX3U clone

Device works over RS-232 (default 115200 8N1), CAN bus (default 250 kbit/s), or MODBUS-RTU (default
9600 8N1).

Full pinout table is available in `hardware.c`.

The firmware help output also prints the build number and build date from `version.inc`.

## Startup diagnostics

On power-up or reset the firmware prints a startup banner over USART1:

```
START
IWDGRSTF=1    # reset occurred due to independent watchdog
SFTRSTF=1     # software reset (NVIC_SystemReset)
PORRSTF=1     # power-on / power-down reset
PINRSTF=1     # reset via NRST pin
```

Only the flags that were actually set are printed. After printing, all reset flags are cleared.

## Hardware notes

### Inputs (X)

X8 is not a screw terminal — it is the on-board "Prog" pushbutton (PB2).
X9 is absent. Bits of the `inchannels` mask correspond to these positions.

| Ch  | Pin  | Notes |
|-----|------|-------|
| X0  | PB13 | |
| X1  | PB14 | |
| X2  | PB11 | |
| X3  | PB12 | |
| X4  | PE15 | |
| X5  | PB10 | |
| X6  | PE13 | |
| X7  | PE14 | |
| X8  | PB2  | on-board "Prog" button |
| X9  | —    | absent |
| X10 | PE11 | |
| X11 | PE12 | |
| X12 | PE9  | |
| X13 | PE10 | |
| X14 | PE7  | |
| X15 | PE8  | |

### Outputs (Y)

Y8 and Y9 are absent.

| Ch  | Pin  |
|-----|------|
| Y0  | PC9  |
| Y1  | PC8  |
| Y2  | PA8  |
| Y3  | PA0  |
| Y4  | PB3  |
| Y5  | PD12 |
| Y6  | PB15 |
| Y7  | PA7  |
| Y8  | —    |
| Y9  | —    |
| Y10 | PA6  |
| Y11 | PA2  |

### On-board LED

"RUN" LED is on PD10. **Active low**: `led` returns the logical state (`1` when the LED is on,
`0` when off).

### ADC channels

| № | Enum         | Pin / source | Meaning |
|---|--------------|--------------|---------|
| 0 | `ADC_CH_0`   | PA1 / adc1   | voltage input, up to 11 V |
| 1 | `ADC_CH_1`   | PA3 / adc3   | voltage input, up to 11 V |
| 2 | `ADC_CH_2`   | PC4 / adc14  | voltage input |
| 3 | `ADC_CH_3`   | PC5 / adc15  | current input |
| 4 | `ADC_CH_4`   | PC0 / adc10  | current input, 0..20 mA |
| 5 | `ADC_CH_5`   | PC1 / adc11  | current input |
| 6 | `ADC_POT0`   | PC2 / adc12  | right on-board potentiometer |
| 7 | `ADC_POT1`   | PC3 / adc13  | left on-board potentiometer |
| 8 | `ADC_CH_TSEN`| internal     | MCU temperature sensor |
| 9 | `ADC_CH_VDD` | internal     | Vdd reference |

Each channel is sampled continuously in scan mode via DMA into a circular buffer of `9 ×
ADC_CHANNELS` values. The getter returns the median of the last 9 samples per channel. Reported raw
values are 12-bit (0..4095).

The `mcutemp` command returns MCU temperature in `°C × 10` (as `int32_t`).

## Runtime parameters

### Watchdog

IWDG prescaler `/4` (LSI ≈ 40 kHz) with reload 1250 → about **125 ms** watchdog timeout. Refreshed
in every main-loop iteration, in DMA/send wait loops and during flash writes. Any hang longer than
that triggers a reset.

### CAN timeouts

- Mailbox wait inside `CAN_send()`: `SEND_TIMEOUT_MS / 10` = **10 ms**.
- High-level send loops (command reply, ESW notifications): up to `SEND_TIMEOUT_MS` = **100 ms**.
If a message cannot be queued, `error=canbusy` is printed to USART.

### Buffer sizes

| Subsystem    | Macro                | Value | Notes |
|--------------|----------------------|-------|-------|
| USART input  | `UARTBUFSZI`         | 196   | longer lines are dropped; firmware prints `USART IN buffer overflow!` |
| USART output | `UARTBUFSZO`         | 256   | |
| CAN RX queue | `CAN_INMESSAGE_SIZE` | 8     | extra messages are dropped silently |
| Modbus RX    | `MODBUSBUFSZI`       | 68    | |
| Modbus TX    | `MODBUSBUFSZO`       | 64    | |

## Serial protocol

Every command is a string terminated by `\n`. General syntax:

```
command[number][=value]
```

- `command` — command name (letters/digits);
- `number` — optional parameter number, 0..127;
- `=value` — optional setter value.

Numbers are parsed by `getnum()` and accept decimal (`123`), hexadecimal (`0x7B`), octal (`0173`)
and binary (`0b1111011`). Signed values (`getint()`) allow a leading `-`.

Values in parentheses after a flag command is its bit number in the whole `uint32_t`. E.g. to reset
flag `f_relay_inverted` you can call `f_relay_inverted=0` or `flags2=0`.

```
commands format: parameter[number][=setter]
parameter [CAN idx] - help
--------------------------

CAN bus commands:
canbuserr - print all CAN bus errors (a lot of if not connected)
cansniff - switch CAN sniffer mode
s - send CAN message: ID 0..8 data bytes

Configuration:
bounce [14] - set/get anti-bounce timeout (ms, max: 1000)
canid [6] - set both (in/out) CAN ID / get in CAN ID
canidin [7] - get/set input CAN ID
canidout [8] - get/set output CAN ID
canspeed [5] - get/set CAN speed (bps)
dumpconf - dump current configuration
eraseflash [10] - erase all flash storage
f_relay_inverted (2) - inverted state between relay and inputs
f_send_esw_can (0) - change of IN will send status over CAN with `canidin`
f_send_relay_can (1) - change of IN will send also CAN command to change OUT with `canidout`
f_send_relay_modbus (3) - change of IN will send also MODBUS command to change OUT with `modbusidout` (only for master!)
flags [17] - set/get configuration flags (as one U32 without parameter or Nth bit with)
modbusid [20] - set/get modbus slave ID (1..247) or set it master (0)
modbusidout [21] - set/get modbus slave ID (0..247) to send relay commands
modbusspeed [22] - set/get modbus speed (1200..115200)
saveconf [9] - save configuration
usartspeed [15] - get/set USART1 speed

IN/OUT:
adc [4] - get raw ADC value for the given channel (0..9)
esw [12] - anti-bounce read inputs
eswnow [13] - read current inputs' state
led [16] - work with onboard LED
relay [11] - get/set relay state (0 - off, 1 - on)

Other commands:
inchannels [18] - get u32 with bits set on supported IN channels
mcutemp [3] - get MCU temperature (*10degrC)
modbus - send modbus request with format "slaveID fcode regaddr nregs [N data]", to send zeros you can omit rest of 'data'
modbusraw - send RAW modbus request (will send up to 62 bytes + calculated CRC)
outchannels [19] - get u32 with bits set on supported OUT channels
reset [1] - reset MCU
time [2] - get/set time (1ms, 32bit)
wdtest - test watchdog
```

Value in square brackets is the CAN bus command code (see below).

### Notes on specific commands

#### `bounce`

`bouncetime` (default 50 ms) is not a classical debounce delay — it is the **per-input sampling
interval**. Once an input has been sampled, it will not be re-read until `bouncetime` milliseconds
have elapsed. Worst-case reaction time for an input change is therefore up to `bouncetime` ms;
changes within that window are ignored (which is the debounce behaviour itself).

#### `s`

Send a CAN message. All numbers are space-separated; the first is the CAN ID (0..0x7FF), the
remaining 0..8 numbers are data bytes. Numbers may be given in any format accepted by `getnum()`.

```
s 0x123 0x11 0x22 0x33
s 291 1 2 3 4
```

On invalid arguments the firmware prints `error=badpar` / `error=badval` / `error=wronglen` and
sends nothing.

#### `cansniff`

When enabled, every received CAN frame is printed to USART in the format:

```
<time_ms> #<ID> <b0> <b1> ...
```

All fields are hexadecimal except `<time_ms>`. While messages are being received, the regular
periodic USART keep-alive is suppressed.

#### `adc`

Accepts a parameter number 0..9 (`ADC_CHANNELS - 1`). Numbers outside this range return
`error=badpar`.

#### `flags`

With a parameter number (0..`MAX_FLAG_BITNO` = 0..3): sets/reads the Nth bit only. Without a
parameter ("no par", 0x7F): operates on the whole `uint32_t`. Bits above `MAX_FLAG_BITNO` return
`error=badpar`.

### Default configuration

Flash storage is empty after flashing; on first boot `flashstorage_init()` returns `currentconfidx
= -1` and the firmware uses `USERCONF_INITIALIZER` from `flash.c`:

```
CANspeed    = 250000
CANIDin     = 1
CANIDout    = 2
usartspeed  = 115200
bouncetime  = 50
modbusID    = 1
modbusIDout = 2
modbusspeed = 9600
flags       = { sw_send_relay_inv = 1 }
```

After the first `saveconf`, these defaults are replaced by the stored record.

## CAN bus protocol

Default speed is 250 kbit/s. Default CAN IDs are 1 (input) and 2 (output) for a slave. **All
multi-byte data is little-endian.**

| Byte(s) | Meaning |
|---------|---------|
| 0, 1    | `uint16_t` command code (see table below) |
| 2       | `uint8_t` parameter number: 0..126, ORed with 0x80 for setter, 127 = "no parameter" |
| 3       | `uint8_t` error code (only in device answers) |
| 4..7    | `int32_t` data |

When the device receives a CAN packet addressed to its own ID or to ID = 0 ("broadcast"), it
performs the requested action and sends an answer (usually a getter reply). If the command cannot
be executed or carries bad data, the device returns the same packet with the error code inserted
into byte 3.

Getters may be requested by a 3-byte packet (command code + parameter). "No parameter" (0x7F) in
some commands means "all data" — e.g. get/set all relays or get all inputs.

### CAN bus error codes (byte 3 of the answer)

| Code | Name             | Meaning |
|------|------------------|---------|
| 0    | `ERR_OK`         | all OK |
| 1    | `ERR_BADPAR`     | wrong parameter |
| 2    | `ERR_BADVAL`     | value out of range |
| 3    | `ERR_WRONGLEN`   | wrong message length (for setter or where a parameter is required) |
| 4    | `ERR_BADCMD`     | unknown command code |
| 5    | `ERR_CANTRUN`    | cannot run the command (bad parameters or other reason) |

Bus-level errors (stuff/form/ack/bit/CRC, bus-off, error-passive, error-warning) are not reported
in byte 3. They are printed over USART by `CAN_printerr()` when the `canbuserr` printer is enabled:

```
Receive error counter: <n>
Transmit error counter: <n>
Last error code: <name>
[Bus off] [Passive error limit] [Error counter limit]
```

### CAN command codes

| Code | Enum | Text command |
|------|------|--------------|
| 0  | `CMD_PING`         | (ping) |
| 1  | `CMD_RESET`        | `reset` |
| 2  | `CMD_TIME`         | `time` |
| 3  | `CMD_MCUTEMP`      | `mcutemp` |
| 4  | `CMD_ADCRAW`       | `adc` |
| 5  | `CMD_CANSPEED`     | `canspeed` |
| 6  | `CMD_CANID`        | `canid` |
| 7  | `CMD_CANIDin`      | `canidin` |
| 8  | `CMD_CANIDout`     | `canidout` |
| 9  | `CMD_SAVECONF`     | `saveconf` |
| 10 | `CMD_ERASESTOR`    | `eraseflash` |
| 11 | `CMD_RELAY`        | `relay` |
| 12 | `CMD_GETESW`       | `esw` |
| 13 | `CMD_GETESWNOW`    | `eswnow` |
| 14 | `CMD_BOUNCE`       | `bounce` |
| 15 | `CMD_USARTSPEED`   | `usartspeed` |
| 16 | `CMD_LED`          | `led` |
| 17 | `CMD_FLAGS`        | `flags` |
| 18 | `CMD_INCHNLS`      | `inchannels` |
| 19 | `CMD_OUTCHNLS`     | `outchannels` |
| 20 | `CMD_MODBUSID`     | `modbusid` |
| 21 | `CMD_MODBUSIDOUT`  | `modbusidout` |
| 22 | `CMD_MODBUSSPEED`  | `modbusspeed` |

### Examples

All data in hex. Slave ID is omitted.

Get current time:
- request: `02 00 00`
- answer: `02 00 00 00 de ad be ef` — last four bytes are time in ms since power-up.

Set relay number 5:
- request: `0b 00 85 00 01 00 00 00`
- answer: `0b 00 05 00 01 00 00 00`

Set relays 0..3, reset the rest:
- request: `0b 00 ff 00 07 00 00 00`
- answer: `0b 00 7f 00 07 00 00 00`

Changing flags works like the text command: with a parameter number the Nth bit is changed, without
a parameter the whole `uint32_t` is replaced.

## MODBUS-RTU protocol

The device can operate as master or slave. Default format is 9600-8N1. **Big-endian**, as the
standard requires. Default slave ID is 1, and the "relay command" target ID is 2.

Set `modbusid=0` to enter master mode. In master mode the device no longer answers incoming modbus
requests, but instead parses incoming **responses** and prints them to USART.

The `modbus` command sends a formal modbus request in the format:

```
modbus = slaveID fcode regaddr nregs [N data]
```

All numbers are space-separated and parsed by `getnum()` (decimal / hex / octal / binary).
`slaveID` and `fcode` are one byte each; `regaddr` and `nregs` are two bytes little-endian; `N` is
one byte; `data` is N bytes. Optional data bytes are allowed only for "multiple" functions (0x0F,
0x10). For simple setters (0x05, 0x06) `nregs` is the two-byte value written to the slave.

```
modbus = 1 6 2 1             # slave 1, write register, register 2 (MR_LED), value 1
modbus = 1 0x0f 0 8 1 0xff   # slave 1, write coils, 8 coils, 1 byte of data
```

`modbusraw` does not validate the fields; it just sends the data (user should add CRC by himself).
Useful for testing unusual requests.

In master mode, flag `f_send_relay_modbus` makes the device send an "write coils" command with ID =
`modbusidout` every time the IN state changes. This lets you bind several devices: inputs of one
drive the outputs of another. If `modbusidout` is zero, a broadcast is sent (slaves do not reply to
broadcasts, they just perform the action).

### Implementation notes

- Modbus uses UART4 with DMA for both RX and TX. End of frame is detected by the IDLE interrupt.
- There is no 3.5-character silent-interval handling — any IDLE marks the end of a packet.
- Input buffer is 68 bytes (up to 67 data bytes), output buffer is 64 bytes (up to 64 bytes).
Enlarge `MODBUSBUFSZI` / `MODBUSBUFSZO` in `modbusrtu.h` if needed.
- Maximal modbus slave ID is 247.
- The device does **not reply** to broadcast requests (ID = 0).

### Slave registers

Holding registers: `[R]` = read-only, `[W]` = write-only, `[RW]` = read/write.

| № | Symbol            | Access | Meaning |
|---|-------------------|--------|---------|
| 0 | `MR_RESET`        | W      | reset MCU |
| 1 | `MR_TIME`         | RW     | MCU time in ms (`uint32_t`) |
| 2 | `MR_LED`          | RW     | on-board LED state |
| 3 | `MR_INCHANNELS`   | R      | `uint32_t` of available IN channels |
| 4 | `MR_OUTCHANNELS`  | R      | `uint32_t` of available OUT channels |

### Supported function codes

#### 01 — read coils
Read state of all relays. `regaddr` must be 0, `nregs` must be a multiple of 8 (in this hardware: 8 or 16). Answer contains `nregs / 8` bytes; bit 0 of the first data byte is relay 0.

Example — read all relays; only relay 10 active:
- request: `01 01 00 00 00 10`
- answer: `01 01 02 00 04`

Errors: `02` — non-zero `regaddr`; `03` — `nregs` not a multiple of 8 or too large.

#### 02 — read discrete inputs
Same semantics as "read coils", but for the IN channels.

Example — read first 8 INs; all 4 low-order inputs active:
- request: `01 02 00 00 00 08`
- answer: `01 02 01 0f`

#### 03 — read holding register
Reads one register at a time.

Example — read time:
- request: `01 03 00 01 00 01`
- answer: `01 03 04 01 53 15 00` — value `0x00155301` = 1397505 ms ≈ 1397.5 s.

Errors: `02` — bad `regaddr`; `03` — `regno != 1`.

#### 04 — read input register
Read `nregs` ADC channels starting at `regaddr`.

Example — read channels 5..8:
- request: `01 04 00 05 00 04`
- answer: `01 04 08 6c 08 21 00 33 00 41 00` — `0x086c` (2156) for channel 5, etc.

Errors: `02` — bad start channel; `03` — bad amount (zero or beyond last channel).

#### 05 — write coil
Changes a single relay state. `regaddr` — relay number, `nregs` — value (0 = off, non-zero = on).

Example - turn on coil 3:
- request: `01 05 00 03 00 01`
- answer: `01 05 00 03 00 01`

Errors: `02` — bad relay number.

#### 06 — write holding register
Writes to one register (`MR_RESET`, `MR_TIME` or `MR_LED`).

Example — turn LED on:
- request: `01 06 00 02 00 01`
- answer: `01 06 00 02 00 01`

Errors: `02` — bad register.

#### 0F — write multiple coils
Changes all relays at once. `regaddr` must be 0, `nregs` a multiple of 8, `N` = `(nregs + 7) / 8`.
Each data bit is a relay state.

Example — turn on relays 0..7:
- request: `01 0f 00 00 00 08 01 ff`
- answer: `01 0f 00 00 00 08`

Turn on all relays (0..7 and 10, 11):
- request: `01 0f 00 00 00 10 02 ff 0f`
- answer: `01 0f 00 00 00 10 56 c2`

Errors: `02` — non-zero `regaddr`; `03` — wrong amount; `07` — cannot change relays.

#### 10 — write multiple registers
Only `MR_TIME` can be written this way; `nregs` must be 1 and the data length 4 bytes.

Example — clear `Tms`:
- request: `01 10 00 01 00 01 04 00 00 00 00`
- answer: `01 10 00 01 00 01`

Errors: `02` — wrong register.

### Modbus exception codes

| Code | Name | Meaning |
|------|------|---------|
| 01 | `ME_ILLEGAL_FUNCION` | function code is not authorized for the slave |
| 02 | `ME_ILLEGAL_ADDRESS` | data address is not authorized |
| 03 | `ME_ILLEGAL_VALUE`   | data field value is not authorized |
| 04 | `ME_SLAVE_FAILURE`   | unrecoverable error |
| 05 | `ME_ACK`             | accepted, but processing takes a long time |
| 06 | `ME_SLAVE_BUSY`      | slave is busy |
| 07 | `ME_NACK`            | programming request cannot be performed |
| 08 | `ME_PARITY_ERROR`    | memory parity error |

## Limitations

- CAN RX queue holds 8 messages; extras are dropped silently.
- USART RX buffer holds 196 bytes; longer lines are discarded with an error message.
- Modbus does not implement the 3.5-character silent interval. Buffers are 67/64 bytes.
- The Modbus master does not implement retries, timeouts or a transaction queue — it just prints
incoming responses. Reliable exchange should be arranged by the host.
- CAN filters accept only the configured `CANIDin` plus ID 0 (broadcast); the "monitor" mode adds a
second, match-all filter.
- The IWDG is enabled in release builds. Any hang longer than ~125 ms triggers a reset.
- The `EBUG` build disables the IWDG and enables verbose `DBG(...)` messages.

## Short programming guide

### Adding a new value to flash storage

All stored values are described in `struct user_conf` (`flash.h`). You can add new fields, but keep
32-bit alignment in mind. Bit flags live in `union confflags_t`, which combines 32-bit and per-bit
access.

After adding a field:
1. Add a setter/getter (usually via `u32setget` or `flagsetget`).
2. Add a line in `dumpconf()` (`proto.c`).

The text protocol allows working with flags by their semantic name. To add a flag, edit `proto.c`:
- add a `static const char* S_f_...` constant with the flag name;
- add its address to the `bitfields[]` array **in the same order as the bits are defined in
`confflags_t`** (critical: `dumpconf` and `confflags` index this array by bit number);
- add an entry to the `text_cmd` enum;
- add a `funcdescr` entry to `funclist`;
- modify `confflags()` for setter/getter handling.

### Adding a new command

Base commands are processed in `canproto.c` and `proto.c`. `modbusproto.c` handles modbus-specific
commands.

To add a CAN/serial command:

1. Add an enum member in `canproto.h` (`CMD_...`). **This value is the numeric command code** on
the wire.
2. Add a string constant with the text command name in `proto.c`.
3. Add a `funcdescr` entry to `funclist`.
4. Implement the handler in `canproto.c` (returns one of `errcodes`, receives a `CAN_message *`).

**Important:** the `funclist[]` array in `canproto.c` is indexed by enum value — the entry for
`CMD_X` must be at array position `CMD_X`. Use designated initializers (`[CMD_X] = {...}`) as the
existing code does.

The `commonfunction` struct has fields `{fn, minval, maxval, datalen}`:
- `minval == maxval` disables range checking of the value (bytes 4..7) for setter commands;
- `datalen` is the minimal packet length in bytes that the handler requires.

The handler only sees a `CAN_message *`. The serial parser builds an equivalent packet from user
input: `[C C P 0 V0 V1 V2 V3]`, where `C` = command code (little-endian); `P` = parameter number
(or 0x7F if not specified), ORed with 0x80 in case of a setter; `Vx` = bytes of the user value
(little-endian).

For `uint32_t` configuration values use `u32setget`; for bit flags — `flagsetget`.

### Adding a serial-only command

If the command has no CAN equivalent, work purely in `proto.c`:
- add an entry to the `text_cmd` enum (negative indices are used in `funclist`);
- add a string constant and a `funcdescr` entry;
- implement the handler with signature `errcodes fn(const char *str, text_cmd cmd)`;
- register it in the `textfunctions[]` array.

### Working with modbus

Modbus-specific enums (`modbus_fcode`, `modbus_exceptions`) and structs (`modbus_request`,
`modbus_response`) are declared in `modbusrtu.h`. `data` fields hold bytes in wire order. For
requests without data (Fcode ≤ 6), `data` may be `NULL`.

High-level modbus slave handlers live in `modbusproto.c`. To add a new register, extend the
`modbus_registers` enum in `modbusproto.h` and handle the new value in `readreg()`, `writereg()` or
`writeregs()`. The main dispatch point is `parse_modbus_request()`.

---

## License

All source files are licensed under **GNU General Public License v3.0** unless stated otherwise. 
