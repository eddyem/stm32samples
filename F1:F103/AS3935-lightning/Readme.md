# AS3935-based Lightning Detector

## Overview

This project implements a lightning detection system using two AS3935 Franklin Lightning Sensors interfaced
with an STM32F103 microcontroller. The device communicates over USB CDC (virtual serial port) and exposes a
text-based command protocol for configuration, monitoring, and data extraction.
In the next version it is possible to add RS-232 or RS-485 as an alternative to USB.

Key features:
- Dual AS3935 sensors on single SPI channel.
- On-chip ADC for internal MCU temperature and voltage supply monitoring (more features may be added in the
  future).
- Persistent configuration storage in internal Flash memory.
- USB CDC interface with automatic detection of lightning events.
- Extensive command set for real-time tuning and status readout.

---

## Hardware Connections

| Pin    | Configuration | Function |
|--------|-----------|----------|
| PA0    | GPIO      | INT0 - External interrupt from sensor 0 (pull-down) |
| PA1    | GPIO      | INT1 - External interrupt from sensor 1 (pull-down) |
| PA2    | GPIO      | CS0 - SPI chip select for sensor 0 |
| PA3    | GPIO      | CS1 - SPI chip select for sensor 1 |
| PA5    | SPI1 SCK  | SPI clock |
| PA6    | SPI1 MISO | SPI Master In / Slave Out |
| PA7    | SPI1 MOSI | SPI Master Out / Slave In |
| PA9    | USART1 Tx | (unused in this version) |
| PA10   | USART1 Rx | (unused) |
| PA15   | GPIO      | USB D+ pull-up control (1.5 kΩ) |
| PC13   | GPIO      | On-board LED (active low) |

The two AS3935 sensors share the SPI bus but have separate chip select lines and interrupt pins.
If necessary, it can be expanded to include more sensors.

---

## USB CDC Communication

The device appears as a virtual COM port when connected to a host computer. Communication parameters (baud
rate, parity, etc.) are ignored — the underlying USB bulk transport guarantees reliable delivery.

- **Line coding**: default sent by host (e.g., 115200 8N1) is accepted but not used.
- **Ring buffers**: 1024 bytes each for TX and RX.
- **Command terminator**: every command must end with a newline (`\n`). The response is also terminated by a
  newline, except for multi-line outputs (help, dumpconf, SPI).

When the host opens the virtual COM port (DTR asserted), the device becomes ready. If the port is closed, the
`CDCready` flag is cleared and any outgoing data is discarded.

---

## Command Protocol

### General Syntax

```
<command> [<channel>] [ = <value> ]
```

- **command** — case-sensitive name (exactly as listed, all lowercase).
- **channel** — numeric index of the sensor (0 or 1). Required for sensor-specific commands; omitted for
global commands.
- **value** — decimal integer, hexadecimal (`0x` prefix), binary (`0b` prefix), or a string (for commands
like `setiface`). For the `SPI` command, the value is a sequence of hex bytes and/or quoted text.
- **whitespaces** around `=` and between tokens is ignored.

There are two modes:

1. **Getter** — if `=` is absent, the current value is returned:
   ```
   command<channel> = <current_value>
   ```
   No `OK` response follows.

2. **Command** — also don't need `=`, but returns `OK` or error code.

3. **Setter** — if `=` is present, the value after `=` is assigned. The returned value is text describing
   error code (`OK` etc.). A getter-style response is **not** returned after a successful set; only `OK`
   appears.

For channel-dependent commands the response includes the channel number, e.g.:
```
gain0 = 5
```

Global commands (no channel) use the same format without a number:
```
restonstart = 1
```

Commands that are purely actions (e.g., `clearstat`, `resetdef`) only take a channel number and do not accept
an `=` sign. On success they reply `OK`.

### Error Codes

If a command cannot be executed, one of the following error strings is returned instead of `OK`:

| Error String | Meaning |
|--------------|---------|
| `BADCMD`     | Unknown command |
| `BADPAR`     | Invalid channel number |
| `BADVAL`     | Invalid value for the setter (out of range or malformed) |
| `WRONGLEN`   | Buffer overflow or line too long |
| `CANTRUN`    | SPI communication failure, sensor not responding, or illegal state |
| `BUSY`       | SPI or ring buffer busy - retry later |
| `OVERFLOW`   | Input string exceeded maximum length (256 characters) |

### Command Reference

All commands are defined in the source file `commproto.cpp` using macro `COMMAND_TABLE`.

#### Sensor Configuration Commands

These commands read or write parameters of a specific AS3935 sensor. They require the channel number as the
first argument. For details, please refer to the sensor's technical documentation.

| Command      | Description | Value Range | Default |
|--------------|-------------|-------------|---------|
| `displco`    | Display on IRQ pin: 0=nothing, 1=TRCO, 2=SRCO, 3=LCO | 0 - 3 | 0 |
| `gain`       | Amplifier gain (AFE_GB) | 0 - 31 | 18 |
| `lco_fdiv`   | Antenna LCO frequency divider | 0 - 3 | 0 |
| `maskdist`   | Mask disturbers (1 = mask, 0 = allow) | 0 or 1 | 0 |
| `minnumlig`  | Minimum lightning number (0→1, 1→5, 2→9, 3→16) | 0 - 3 | 0 |
| `nflev`      | Noise floor level | 0 - 7 | 2 |
| `srej`       | Spike rejection | 0 - 15 | 2 |
| `tuncap`     | Tune capacitor setting (n·8 pF) | 0 - 15 | 0 |
| `wdthres`    | Watchdog threshold | 0 - 15 | 2 |


**Syntax examples:**

```
gain 0 = 12          set gain of sensor 0 to 12
gain 1               read gain of sensor 1 → gain1 = 5
displco 0 = 1        configure sensor 0 IRQ pin to output TRCO clock
displco 0            read display mode → displco0 = 1
```

#### Sensor Action & Status Commands

These commands do not take a value after `=`, only a channel number.

| Command      | Description |
|--------------|-------------|
| `clearstat`  | Clear the lightning statistics (last 15 min) |
| `distance`   | Read estimated distance to the lightning in km (0-63, 63=out of range) |
| `energy`     | Read energy of the last lightning (24-bit value) |
| `intcode`    | Read the last interrupt code: 1=Noise, 4=Disturber, 8=Lightning |
| `iscalib`    | Check if both RCO and TRCO calibrations are done (returns 1 if done) |
| `resetdef`   | Reset the sensor to factory default settings |
| `wakeup`     | Wake up the sensor and run RCO calibration |

**Examples:**

```
wakeup 0            wake up sensor 0, auto-calibrate → OK
iscalib 0           read calibration status → iscalib0 = 1
energy 0            read energy → energy0 = 123456
distance 1          read distance → distance1 = 12
intcode 0           read interrupt flags → intcode0 = 8
clearstat 0         clear statistics → OK
resetdef 0          reset to defaults → OK
```

#### Global (Non-Sensor) Commands

These commands do not require a channel number.

| Command | Description |
|---------|-------------|
| `dumpconf` | Dump the full current configuration (stored in RAM until saved) |
| `eraseflash` | Erase the entire Flash configuration storage |
| `help`  | Print list of all commands and a repository URL |
| `mcureset` | Software reset of the MCU |
| `mcutemp` | MCU internal temperature in tenths of °C (e.g., 287 = 28.7℃) |
| `readconf` | Read sensor configuration from the chip and update RAM for given channel |
| `restonstart` | Restore sensor parameters at start-up (0 or 1) |
| `saveconf` | Save the current RAM configuration to Flash |
| `setiface` | Get/set the USB interface name string (max 16 characters) |
| `time`  | Current uptime in milliseconds |
| `vdd`   | Supply voltage VDD in hundredths of a volt (e.g., 330 = 3.30 V) |

**Examples:**

```
mcutemp               → mcutemp = 287
vdd                   → vdd = 330
setiface = MyDetector   set interface name to "MyDetector"
setiface              → setiface = MyDetector
restonstart = 1         enable automatic restore at power-up
saveconf                save current configuration to Flash
dumpconf                print all stored and current parameters
readconf 0              read sensor 0 parameters into RAM (for later save)
```

#### SPI Low-Level Command

The `SPI` command allows arbitrary bidirectional SPI transactions to a selected sensor.

**Syntax:**
```
SPI <channel> = <hexdata>
```

`<hexdata>` is a sequence of bytes expressed as space- or comma-separated hexadecimal numbers (without `0x`
prefix), optionally with quoted text strings. Anything inside double quotes is transmitted as raw ASCII.

**Example:**
```
SPI 0 = 00 01 "hello" 02
```
This sends the bytes `0x00, 0x01, 'h','e','l','l','o', 0x02` to sensor 0 and returns the received bytes as a
hex dump.

**Response format:**
```
SPI0 = 
00 01 68 65 6c 6c 6f 02
```

---

## Lightning Interrupts

The main loop continuously monitors the INT pins (PA0, PA1). When a rising edge is detected and the
corresponding `DISPLCO` setting is `0` (not used as clock output), the code reads the interrupt register:

- Disturber and noise events are reported as:

  ```
  INTERRUPT0=NOICE,DISTURBER
  ```
- A lightning event additionally triggers `lightning_info()`, which prints the energy and distance:

  ```
  INTERRUPT0=LIGHTNING
  energy0 = 123456
  distance0 = 12
  ```

If the pin is configured as a clock output (`displco` != 0), no interrupt processing occurs. You can mask
disturber interrupts by command `maskdist n = 0`, in this case only `NOICE` and `LIGHTNING` will be
monitored.

---

## Configuration Storage

User settings are stored in the internal Flash of the STM32F103. The Flash area reserved for configuration
starts at a fixed address (defined in the linker script) and has a limited capacity. The structure
`user_conf` contains:

- `userconf_sz` — magic cookie (size of the structure).
- `iInterface` + `iIlength` — USB interface name (stored as two bytes per character for Unicode).
- `spars[]` — per-sensor parameters (gain, WDTH, NF_LEV, SREJ, MIN_NUM_LIG, MASK_DIST, LCO_FDIV, TUN_CAP).
- `flags.restore` — if 1, all parameters are automatically applied after reset.

**Mechanism:**

- Each time `saveconf` is issued, a new copy of the configuration is appended to the Flash storage. A binary
  search at boot finds the last valid entry.
- If the storage becomes full, the next `saveconf` will first erase the entire storage and then write the
  current configuration.
- `eraseflash` erases the whole configuration area; `restonstart` controls whether the stored configuration
  is uploaded to sensors at start-up.

**Capacity:** 

Approximately `(FLASH_SIZE * 1024 - offset) / sizeof(user_conf)` configurations can be stored. For a typical
64 KB device, this is hundreds of entries.

---

## ADC and Internal Sensors

The STM32’s ADC continuously samples the internal temperature sensor (channel 16) and the internal reference
voltage (channel 17) in scan mode with DMA circular buffer. A median filter of 9 samples is applied to each
channel.

- **`mcutemp`** returns temperature in tenths of °C, computed as:  

  `T = 25 + (V_25 - Vsense)/Avg_Slope`, with `V_25=1.45 V` and `Avg_Slope=4.3 mV/°C`.
- **`vdd`** calculates the supply voltage using the internal 1.2 V reference:  

  `Vdd = 1.2 * 4096 / (ADC reading of Vrefint)`.

These commands do not require a sensor channel.

---

## Examples

### Typical Workflow

1. Connect the device to a USB port and open a terminal at any baud rate.
2. Type `help` to see the available commands.
3. Wake up the sensors and calibrate:

   ```
   wakeup 0
   wakeup 1
   ```
4. Configure gains, thresholds and other values if need:

   ```
   gain 0 = 10
   wdthres 1 = 3
   ```
5. Verify settings:

   ```
   dumpconf
   ```
6. Enable persistent configuration:

   ```
   restonstart = 1
   saveconf
   ```

7. Monitor lightning events — the device will print `INTERRUPT...` messages automatically. If need, you can
   clear statistics periodically or call other setters/getters; for example, reduce the sensitivity or
   lightning strike threshold during a severe thunderstorm and restore these values ​​after it ends

### Reading Sensor Status
Any time you can read sensor status (to get noice information or other data):

```
intcode 0
energy 0
distance 0
```

### Saving and Restoring Configuration

```
dumpconf        # see current settings
saveconf        # write to flash
mcureset        # reboot (only if something goes wrong)
```

After reset, the configuration is automatically loaded (if `restonstart` was 1).

---

## How to distinguish between identical device

The device uses standard USB VID/PID, which are shared for free use by ST. Therefore, it may be difficult to
determine which `/dev/ttyACMx` this particular device corresponds to.

You can set custom interface name (`setiface=...`). After storing in flash, reconnect or reset device and
run `lsusb -v` for given device. At `iInterface` field you will see stored name. This field can be used to
create human-readable symlinks in `/dev`.

For automatical creation of symlink in `/dev` to your `/dev/ttyACMx`, add this udev-script to
`/etc/udev/rules.d/99-usbids.rules`

```
ACTION=="add", DRIVERS=="usb", ENV{USB_IDS}="%s{idVendor}:%s{idProduct}"
ACTION=="add", ENV{USB_IDS}=="0483:5740", ATTRS{interface}=="?*", PROGRAM="/bin/bash -c \"ls /dev | grep $attr{interface} | wc -l \"", SYMLINK+="$attr{interface}%c", MODE="0666", GROUP="tty"
```

---

## Build Information

Typing `help` you will see project repository URL, build number and build date at first string.

The code is distributed under GPLv3.
