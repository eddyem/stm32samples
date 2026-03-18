# USB-CAN-GPIO Adapter Command Protocol

Based on [USB-CAN](https://github.com/eddyem/stm32samples/tree/master/F0%3AF030%2CF042%2CF072/usbcan_ringbuffer).

Unlike original (only USB-CAN) adapter, in spite of it have GPIO contacts on board, this firmware allows to use both
USB-CAN and GPIO.

The old USB-CAN is available as earlier by /dev/USB-CANx, also you can see new device: /dev/USB-GPIOx.
New interface allows you to configure GPIO and use it's base functions.

---

# GPIO Interface Protocol for `/dev/USB-CANx`

---

## Overview
This firmware implements a versatile USB device with two CDC ACM interfaces:
- **ICAN** (interface 0) — for CAN bus communication
- **IGPIO** (interface 1) — for GPIO, USART, I2C, SPI, PWM control and monitoring

The device can be configured dynamically via text commands sent over the USB virtual COM ports. All settings can be saved to internal flash and restored on startup.

--- 

## General Command Format
- Commands are case-sensitive, up to 15 characters long.
- Parameters are separated by spaces or commas.
- Numeric values can be decimal, hexadecimal (`0x...`), octal (`0...`), or binary (`b...`).
- For `USART=...` command the part after `=` is interpreted according to the current `hexinput` mode (see `hexinput` command).
 Specific numeric values (as in `SPI` and `I2C` send) are hex-only without `0x` prefix.

---

## Global Commands
| Command | Description |
|---------|-------------|
| `canspeed [=value]` | Get or set CAN bus speed (kBaud, 10─1000). |
| `curcanspeed` | Show the actual CAN speed currently used by the interface. |
| `curpinconf` | Dump the *current* (working) pin configuration ─ may differ from saved config. |
| `dumpconf` | Dump the global configuration stored in RAM (including CAN speed, interface names, pin settings, USART/I2C/SPI parameters). |
| `eraseflash` | Erase the entire user configuration storage area. |
| `help` | Show help message. |
| `hexinput [=0/1]` | Set input mode: `0` ─ plain text, `1` ─ hex bytes + quoted text. Affects commands like `USART` and `sendcan`. |
| `iic=addr data...` | Write data over I2C. Address and data bytes are given in hex. Example: `iic=50 01 02` |
| `iicread=addr nbytes` | Read `nbytes` from I2C device at `addr` and display as hex. Both addr and nbytes are hex numbers. |
| `iicreadreg=addr reg nbytes` | Read `nbytes` from I2C device register `reg`. Also hex. |
| `iicscan` | Start scanning I2C bus; found addresses are printed asynchronously. |
| `mcutemp` | Show MCU internal temperature in ℃*10. |
| `mcureset` | Reset the microcontroller. |
| `PAx [= ...]` | Configure or read GPIOA pin `x` (0-15). See **Pin Configuration** below. |
| `PBx [= ...]` | Configure or read GPIOB pin `x`. |
| `pinout [=function1,function2,...]` | List all pins and their available alternate functions. If a space- or comma-separated list of functions is given, only pins supporting any of them are shown. |
| `pwmmap` | Show pins capable of PWM and indicate collisions (pins sharing the same timer channel). |
| `readconf` | Re-read configuration from flash into RAM. |
| `reinit` | Apply the current pin configuration to hardware. Must be issued after any pin changes. |
| `saveconf` | Save current RAM configuration to flash. |
| `sendcan=...` | Send data over the CAN USB interface. If `hexinput=1`, the argument is parsed as hex bytes with plain text in quotes; otherwise as plain text (a newline is appended). |
| `setiface=N [=name]` | Set or get the name of interface `N` (0 = CAN, 1 = GPIO). |
| `SPI=data...` | Perform SPI transfer: send hex bytes, receive and display the received bytes. |
| `time` | Show system time in milliseconds since start. |
| `USART[=...]` | If `=...` is given, send data over USART (text or hex depending on `hexinput`). Without argument, read and display any received USART data (as text or hex, according to USART's `TEXT`/`HEX` setting). |
| `vdd` | Show approximate supply voltage (Vdd, V*100). |

---

## Pin Configuration (Commands `PAx`, `PBx`)

Pins are configured with the syntax:
```
PAx = MODE PULL OTYPE FUNC MISC ...
```
or for a simple digital output:
```
PAx = 0   # set pin low
PAx = 1   # set pin high
```
or for PWM output:
```
PAx = value   # set PWM duty cycle (0-255)
```

### Available Keywords (in any order)

| Group   | Keyword  | Meaning |
|---------|----------|---------|
| **MODE** | `AIN`    | Analog input (ADC) |
|         | `IN`     | Digital input |
|         | `OUT`    | Digital output |
|         | `AF`     | Alternate function ─ automatically set when a function like `USART`, `SPI`, `I2C`, `PWM` is selected. |
| **PULL** | `PU`     | Pull-up enabled (GPIO-only group, don't affect on functions) |
|         | `PD`     | Pull-down enabled |
|         | `FL`     | Floating (no pull) |
| **OTYPE**| `PP`     | Push-pull output |
|         | `OD`     | Open-drain output |
| **FUNC** | `USART`  | Enable USART alternate function |
|         | `SPI`    | Enable SPI alternate function |
|         | `I2C`    | Enable I2C alternate function |
|         | `PWM`    | Enable PWM alternate function |
| **MISC** | `MONITOR`| Enable asynchronous monitoring of pin changes (for GPIO input or ADC) or USART incoming message. When the pin changes, its new value is sent over USB automatically. |
|         | `THRESHOLD n` | Set ADC threshold (0-4095). Only meaningful with `AIN`. A change larger than this triggers an async report (if `MONITOR` enabled on this pin). |
|         | `SPEED n` | Set interface speed/frequency (baud for USART, Hz for SPI, speed index for I2C). |
|         | `TEXT`    | USART operates in text mode (lines terminated by `\n`). |
|         | `HEX`     | USART operates in binary mode (data output as hex dump, data input by IDLE-separated portions if `MONITOR` enabled). |

### Notes
- `AF` is automatically set when any `FUNC` is selected; you do not need to type it explicitly.
- For `PWM`, the duty cycle can be set by assigning a number (0-255) directly, e.g. `PA1=128`.
- For `OUT`, assigning `0` or `1` sets/clears the pin.
- For `ADC` (`AIN`), `MONITOR` uses the `THRESHOLD` value; if not given, any change triggers a report.
- Conflicting configurations (e.g., two different USARTs on the same pins, or missing SCL/SDA for I2C) are detected and rejected with an error message.

After changing pin settings, you must issue the `reinit` command for the changes to take effect.

---

## USART

### Pin Assignment
USART1 can be used on:
- PA9 (TX), PA10 (RX)
- PB6 (TX), PB7 (RX)

USART2 on:
- PA2 (TX), PA3 (RX)

Both USARTs share the same DMA channels, so only one USART can be active at a time.

### Configuration via Pin Commands
When you set a pin to `FUNC_USART`, you can also specify:
- `SPEED n` ─ baud rate (default 9600)
- `TEXT` ─ enable line-oriented mode (data is buffered until `\n`)
- `HEX` ─ binary mode (data is sent/received as raw bytes; asynchronous output appears as hex dump)
- `MONITOR` ─ enable asynchronous reception; received data will be sent over USB automatically (as text lines or hex dump depending on mode)

Example:
```
PA9 = USART SPEED 115200 TEXT MONITOR
PA10 = USART
reinit
```
Now USART1 is active at 115200 baud, text mode, with monitoring. Any incoming line will be printed as `USART = ...`.

### Sending Data
```
USART=Hello, world!          # if hexinput=0, sends plain text
USART=48 65 6c 6c 6f         # if hexinput=1, sends 5 bytes
```
If `TEXT` mode is enabled, a newline is automatically appended to the transmitted string.

### Receiving Data
Without `=`, the command reads and displays any data waiting in the ring buffer, like:
```
USART = 48 65 6c 6c 6f
```
If in `TEXT` mode, only complete lines are returned. In `HEX` mode, all received bytes are shown as a hex dump.

If `MONITOR` disabled, but incoming data flow is too large for buffering between consequent `USART` calls,
some "old" data would be printed asynchroneously. 

---

## I2C

### Pin Assignment

I2C1 is available on any mix of: 
- PB6 (SCL), PB7 (SDA)
- PB10 (SCL), PB11 (SDA)

You must configure both SCL and SDA pins for I2C.

### Configuration via Pin Commands
- `I2C` ─ selects I2C alternate function
- `SPEED n` ─ speed index: 0=10kHz, 1=100kHz, 2=400kHz, 3=1MHz (default 100kHz if omitted)
- `MONITOR` is not used for I2C.

Example:
```
PB6 = I2C SPEED 2
PB7 = I2C
reinit
```
This sets up I2C at 400 kHz.

### Commands
- `iic=addr data...` ─ write bytes to device at 7-bit address `addr`. Address and data are given in hex, e.g. `iic=50 01 02 03`.
- `iicread=addr nbytes` ─ read `nbytes` (both numbers are hex) from device and display as hex dump.
- `iicreadreg=addr reg nbytes` ─ read `nbytes` from register `reg` (all numbers are hex).
- `iicscan` ─ start scanning all possible 7-bit addresses (1─127). As devices respond, messages like `foundaddr = 0x50` are printed asynchronously.

---

## SPI

### Pin Assignment

SPI1 can be used on any mix of:
- **PA5** (SCK), **PA6** (MISO), **PA7** (MOSI)
- **PB3** (SCK), **PB4** (MISO), **PB5** (MOSI)

All three pins are not strictly required; you may configure only SCK+MOSI (TX only) or SCK+MISO (RX only). The SPI peripheral will be set to the appropriate mode automatically.

### Configuration via Pin Commands
- `SPI` ─ selects SPI alternate function
- `SPEED n` ─ desired frequency in Hz (actual nearest prescaler will be chosen automatically)
- `CPOL` ─ clock polarity (1 = idle high)
- `CPHA` ─ clock phase (1 = second edge)
- `LSBFIRST` ─ LSB first transmission.

If `CPOL`/`CPHA` are not given, they default to 0 (mode 0). `LSBFIRST` defaults to MSB first.

Example:
```
PA5 = SPI SPEED 2000000 CPOL CPHA
PA6 = SPI
PA7 = SPI
reinit
```
Configures SPI at ~2 MHz, mode 3 (CPOL=1, CPHA=1), full duplex.

### Commands
- `SPI=data...` ─ send the given hex bytes, and display the received bytes. Works in full-duplex or write-only modes. Example:
    ```
    SPI=01 02 03
    ```
    will send three bytes and output the three bytes received simultaneously.

- `SPI=n` — reads `n` bytest of data (n should be decimal, binary or hex with prefixes `0b` or `0x`).

---

## PWM

### Pin Assignment
PWM is available on many pins; see the output of `pwmmap` (or `pinout=PWM`) for a complete list with possible conflicts (pins sharing the same timer channel). 
Conflicts are automatically detected ─ if you try to use two conflicting pins, one will be reset to default. 

### Configuration via Pin Commands
- `PWM` ─ selects PWM alternate function
- No additional parameters are needed; the duty cycle is set by writing a number directly to the pin.

Example:
```
PA1 = PWM
reinit
PA1=128   # set 50% duty cycle
```

### Reading PWM Value
```
PA1
```
returns the current duty cycle (0─255).

---

## Monitoring and Asynchronous Messages

When a pin is configured with `MONITOR` and is not in AF mode (i.e., GPIO input or ADC), any change in its state triggers an automatic USB message. The format is the same as the pin getter: `PAx = value`. For ADC, the value is the ADC reading; the message is sent only when the change exceeds the programmed `THRESHOLD` (if any).

USART monitoring (if enabled with `MONITOR`) sends received data asynchronously, using the same output format as the `USART` command.

I2C scan results are also printed asynchronously while scan mode is active.

---

## Saving Configuration

The entire configuration (interface names, CAN speed, pin settings, USART/I2C/SPI parameters) can be saved to flash with `saveconf`.
On startup, the last saved configuration is automatically loaded. The flash storage uses a simple rotating scheme, so many previous configurations are preserved until the storage area is erased with `eraseflash`.

---

## Error Codes
Most commands return one of the following status strings (if not silent):

| Code        | Meaning |
|-------------|---------|
| `OK`        | Command executed successfully. |
| `BADCMD`    | Unknown command. |
| `BADPAR`    | Invalid parameter. |
| `BADVAL`    | Value out of range or unacceptable. |
| `WRONGLEN`  | Message length incorrect. |
| `CANTRUN`   | Cannot execute due to current state (e.g., peripheral not configured). |
| `BUSY`      | Resource busy (e.g., USART TX busy). |
| `OVERFLOW`  | Input string too long. |

Commands that produce no direct output (e.g., getters) return nothing (silent) and only print the requested value.

---

# CAN Interface Protocol for `/dev/USB-CANx`

--- 

This part describes the simple text‑based protocol used to communicate with the CAN interface of the USB‑CAN‑GPIO adapter.
The interface appears as a standard CDC ACM virtual COM port (e.g. `/dev/ttyACM0` or `/dev/USB-CANx`). 
All commands are terminated by a newline character (`\n`). Numeric parameters can be entered in decimal, hexadecimal (prefix `0x`), octal (prefix `0`), or binary (prefix `b`).

---

## Received CAN Message Format

When a CAN message is received (and display is not paused), it is printed as:

```
<timestamp_ms> #<ID> <data0> <data1> ... <dataN-1>
```

- **`<timestamp_ms>`** – system time in milliseconds since start.
- **`<ID>`** – 11‑bit CAN identifier in hexadecimal (e.g. `0x123`).
- **`<dataX>`** – data bytes in hexadecimal, separated by spaces. If the message has no data, only the timestamp and ID are printed.

Example:
```
12345 #0x123 0xDE 0xAD 0xBE 0xEF
```

---

## Commands

### General
| Command | Description | Example |
|---------|-------------|---------|
| `?`     | Show a brief help message. | `?` |

### CAN Configuration & Control
| Command | Description | Example |
|---------|-------------|---------|
| `b [speed]` | Set CAN bus speed in kBaud (10–1000). Without argument, display the current speed. | `b 250` |
| `I`         | Re‑initialise the CAN controller with the last set speed. | `I` |
| `c`         | Show CAN status registers (MSR, TSR, RF0R, RF1R). | `c` |
| `e`         | Show the CAN error register (ESR) with a human‑readable description. | `e` |

### Sending Messages
| Command | Description | Example |
|---------|-------------|---------|
| `s ID [byte0 ... byte7]` | Send a CAN message with the given ID and up to 8 data bytes. Returns immediately after queuing. | `s 0x123 0xAA 0xBB` |
| `S ID [byte0 ... byte7]` | Same as `s`. | `S 0x100 0x01` |

### Flood (Periodic Transmission)
| Command | Description | Example |
|---------|-------------|---------|
| `F ID [byte0 ... byte7]` | Set a message to be transmitted repeatedly. The message is stored and sent every `t` milliseconds. | `F 0x200 0x55 0xAA` |
| `i`                      | Enable *incremental* flood mode. Sends a 4‑byte counter (increasing by 1 each time) using the ID from the last `F` command. | `i` |
| `t <ms>`                 | Set the flood period in milliseconds (default 5 ms). | `t 10` |

### Filtering
| Command | Description | Example |
|---------|-------------|---------|
| `f bank fifo mode num0 [num1 [num2 [num3]]]` | Configure a hardware filter. `bank` — filter bank number (0–27). `fifo` — FIFO assignment (0 or 1). `mode` — `I` for ID list mode, `M` for mask mode. `numX` — IDs or ID/mask pairs (for mask mode). | `f 0 1 I 0x123 0x456` – two IDs in list mode. `f 1 0 M 0x123 0x7FF` – ID 0x123 with mask 0x7FF (accept all). |
| `l`       | List all active filters with their configuration. | `l` |
| `a <ID>`  | Add an ID to the software ignore list (up to 10 IDs). Messages with this ID will not be printed. | `a 0x321` |
| `p`       | Print the current ignore list. | `p` |
| `d`       | Clear the entire ignore list. | `d` |
| `P`       | Pause/resume printing of incoming CAN messages. Toggles between paused and running. | `P` |

### LEDs & Misc
| Command | Description | Example |
|---------|-------------|---------|
| `o`     | Turn both LEDs off. | `o` |
| `O`     | Turn both LEDs on. | `O` |
| `T`     | Show the system time in milliseconds since start. | `T` |
| `R`     | Perform a software reset of the microcontroller. | `R` |

---

## Error Reporting

When an error occurs, the device may print one of the following messages:

| Message         | Meaning |
|-----------------|---------|
| `ERROR: ...`    | General error with a descriptive text. |
| `NAK`           | A transmitted message was not acknowledged (e.g., no receiver on the bus). |
| `FIFO overrun`  | A CAN FIFO overrun occurred – messages were lost. |
| `CAN bus is off`| The controller entered the bus‑off state; try to restart it with `I`. |

Additionally, the `e` command gives a detailed breakdown of the error register.

---

## Notes

- All settings (baud rate, filters, ignore list, flood message) are stored in RAM only and are lost after a reset or power‑off.
To make changes (only speed available) permanent, use the **GPIO interface** commands `saveconf`/`storeconf` after configuring CAN.
- The device can only handle one active USART at a time, but the CAN interface operates independently.
- The command parser is case‑sensitive for the single‑letter commands (they are expected in lower case, except where noted).

---

## How to distinguish between identical device

In the **GPIO interface** you can setup custom interface name (`setiface=...`) for both USB-CAN and USB-GPIO interfaces.
After storing them in flash, reconnect and `lsusb -v` for given device will show your saved names on `iInterface` fields.
These fields could be used to create human-readable symlinks in `/dev`.

To see symlink in `/dev` to your `/dev/ttyACMx` device based on `iInterface` field, add this udev-script to `/etc/udev/rules.d/usbids.rules`

```
ACTION=="add", DRIVERS=="usb", ENV{USB_IDS}="%s{idVendor}:%s{idProduct}"
ACTION=="add", ENV{USB_IDS}=="067b:2303", ATTRS{interface}=="?*", PROGRAM="/bin/bash -c \"ls /dev | grep $attr{interface} | wc -l \"", SYMLINK+="$attr{interface}%c", MODE="0666", GROUP="tty"
ACTION=="add", ENV{USB_IDS}=="0483:5740", ATTRS{interface}=="?*", PROGRAM="/bin/bash -c \"ls /dev | grep $attr{interface} | wc -l \"", SYMLINK+="$attr{interface}%c", MODE="0666", GROUP="tty"
```
