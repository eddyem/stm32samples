# IR All-Sky Camera Documentation

## Project Overview

This firmware implements an **infrared all-sky camera system** based on up to five **MLX90640**
thermal imaging sensors (32×24 pixels each), designed for **cloud monitoring** at the Special
Astrophysical Observatory of the Russian Academy of Sciences. The system automates sky quality
assessment for 0.5-meter telescopes of the "Astro-M" complex.

The device continuously captures thermal images from multiple sensors (Zenith, East, South, West,
North), processes environmental data from a **BMP280** pressure/humidity/temperature sensor, and
controls **PWM heaters** to prevent dew formation on optics.

**Repository:** [https://github.com/eddyem/stm32samples/tree/master/F3%3AF303/MLX90640-allsky](https://github.com/eddyem/stm32samples/tree/master/F3%3AF303/MLX90640-allsky)

---

## Hardware Requirements

### Microcontroller
- **STM32F303** series (tested on F303)
- 72 MHz system clock (HSE) or 48 MHz (HSI)

### Sensors
| Sensor | Interface | Purpose |
|--------|-----------|---------|
| MLX90640 (up to 5) | I²C (400 kHz) | Thermal imaging (32×24 pixels, -40..+300°C) |
| BMP280 / BME280 | SPI | Ambient temperature, pressure, humidity |
| NTC thermistors (4) | ADC (12-bit) | Heater feedback / backup temperature |

### Outputs
- **DAC output** (PA5) – Onboard heater control (0–3.3V)
- **PWM channels** (TIM3) – External heaters (PA6, PA7, PB0, PB1)
- **UART1** – Main debug/auxiliary interface
- **USB** (CDC class) – Secondary interface


### Pin Assignment (STM32F303)

| Pin | Function | Notes |
|-----|----------|-------|
| PA0–PA3 | NTC1–NTC4 | ADC inputs |
| PA4 | DAC_OUT1 | Onboard heater |
| PA5 | ADC_IN2 | DAC feedback |
| PA6, PA7 | PWM CH1, CH2 | TIM3_CH1, TIM3_CH2 (external heaters) |
| PA9, PA10 | UART1 TX/RX | Controlling interface |
| PA11, PA12 | USB DP/DM | CDC virtual COM |
| PA13, PA14 | SWD | Programming/debug |
| PA15 | USB DP pullup | USB pullup management for development board |
| PB0, PB1 | PWM CH3, CH4 | TIM3_CH3, CH4 (sky-quality + T-proportional) |
| PB3, PB4, PB5 | SPI SCK, MISO, MOSI | BMP280 interface |
| PB6, PB7 | I²C1 SCL/SDA | MLX90640 bus |
| PB9 | SPI CS | BMP280 chip select |

---

## Firmware Architecture

The firmware is **event-driven** with a **state machine** for MLX90640 data acquisition and a
**command parser** for bidirectional communication over USB and UART.

### Core Modules

| File | Responsibility |
|------|----------------|
| `main.c` | Main loop, SysTick, initialization, state machine orchestration |
| `adc.c/h` | ADC setup (DMA, median filtering), NTC reading, Vdd measurement |
| `i2c.c/h` | I²C low-level + DMA, multi-word transfers, bus scanning |
| `mlxproc.c/h` | MLX90640 state machine (5 sensors), parameter caching, image processing |
| `mlx90640.c/h` | MLX90640 calibration math (from Melexis datasheet) |
| `BMP280.c/h` | SPI-based BMP280/BME280 driver |
| `heater.c/h` | PWM heater control, dew-point avoidance logic |
| `commproto.cpp/h` | Command parser (USB/UART), ASCII maps, binary image export |
| `usb_dev.c/h` | USB CDC stack (bulk endpoints, ring buffers) |
| `usart.c/h` | UART driver with DMA and ring buffers |
| `spi.c/h` | SPI driver for BMP280 |
| `ringbuffer.c/h` | Circular buffers |
| `strfunc.c/h` | Number/string conversions, hexdumps |

### Data Flow

```
MLX90640 sensors (I²C DMA)
       ↓
mlxproc.c (state machine, 5× parameter loading)
       ↓
mlx90640.c (pixel-by-pixel temperature calculation)
       ↓
user requests via USB/UART (ASCII map, binary, temperature map)

BMP280 (SPI)
       ↓
Temperature, Pressure, Humidity, Dew point
       ↓
Heater logic (auto or setpoint) → PWM outputs

ADC (DMA + median filter)
       ↓
NTC temperatures, MCU temperature, Vdd
```

---

## MLX90640 Processing

### State Machine (`mlxproc.c`)

The firmware manages up to **5 sensors** (addresses 0x10..0x14 left-shifted for I²C). The state
machine cycles through:

1. **NOTINIT** – Read calibration parameters (832 words) from each sensor over I²C DMA.
2. **WAITPARAMS** – Wait for DMA completion, calculate parameters via `get_parameters()`.
3. **WAITSUBPAGE** – Poll `REG_STATUS` for `NEWDATA` flag, skip subpage 0.
4. **READSUBPAGE** – Read image data (832 words) via DMA, process with `process_image()`.

**Key features:**
- Automatic sensor exclusion after `MLX_MAX_ERRORS` (default 11) consecutive errors.
- `Tlastimage[]` timestamps for each sensor (used by "cartoon mode" and timeout recovery).
- Supports changing sensor I²C addresses at runtime (`mlx_sethwaddr()`).

### Temperature Calculation (`mlx90640.c`)

Implementation follows **Melexis MLX90640 Datasheet (Section 11)**:
- Supply voltage compensation (`kVdd`, `vdd25`)
- Ambient temperature (`KvPTAT`, `KtPTAT`, `vPTAT25`)
- Gain normalization
- Offset correction (per-pixel `offset[]`, `kta[]`, `kv[]`)
- TGC (temperature gradient compensation)
- Pixel sensitivity (`alpha[]`) + KsTa, KsTo, range detection (CT3, CT4)
- **Extended temperature range** (up to 400°C) via `alphacorr[]`

Output: **float array of 768 temperatures** (°C) per sensor.

---

## Environmental & Heater Control

### BMP280/BME280 Readings
- **Temperature** (°C)
- **Pressure** (Pa, also converted to hPa and mmHg)
- **Humidity** (%)
- **Dew point** (°C) – calculated via Magnus formula.

Measurements are triggered every `ENV_MEAS_PERIOD` (10 seconds) in forced mode.

### Heater Modes

| Mode | Activation | Behavior |
|------|------------|----------|
| **Auto** (`autoheater=1`) | Humidity > 90% | Sets holding temperature = ambient + dew_over_delta (7°C) + 0.5°C (min 5°C) |
| **Hold** (`setheater N`) | Manual command | Maintains N°C ±5°C hysteresis using PWM |
| **Off** | Default | All heaters disabled |

**PWM channels:**
- `ch0`, `ch1` – External heaters (controlled identically for symmetry)
- `ch2` – Proportional to ambient temperature: `(T_ambient + 20) × 2` %, clamped 0–100
- `ch3` – Proportional to `(35°C – T_ambient + T_sky) × 2.5` %, clamped 0–100

**Anti-overheating:** If any NTC exceeds setpoint by >5°C, PWM is reduced to `PWM_HOLD_VAL` (10%).

---

## Command Protocol

The device presents a **virtual COM port** (CDC) over USB and a second UART interface. Both share
the same command set.

### Command Syntax
```
<command> [parameter] [= value]
```
- Commands are **case-sensitive** (lowercase).
- Parameters are **decimal** (or hex with `0x`, binary with `b`).
- Use `help` for full list.
- Lines end with `\n` (newline).

### Core Commands

#### MLX90640 Image Commands

| Command | Description | Example |
|---------|-------------|---------|
| `ascii n` | Draw ASCII-art thermal map (16 grayscale levels) | `ascii 0` |
| `tempmap n` | Print numeric temperature grid (°C) | `tempmap 1` |
| `binary n` | Dump raw float array (768×4 bytes) | `binary 2` |
| `acqtime n` | Show last image acquisition timestamp (ms) | `acqtime 3` |
| `cartoon` | Toggle live ASCII animation over USB | `cartoon` |

#### Sensor Management

| Command | Description | Example |
|---------|-------------|---------|
| `listids` | List active I²C addresses of all sensors | `listids` |
| `mlxaddr n [= addr]` | Get/set I²C address for sensor n (0–4) | `mlxaddr 2 = 0x15` |
| `mlxdump n` | Dump all calibration parameters for sensor n | `mlxdump 0` |
| `state` | Show MLX90640 state machine status | `state` |
| `mlxpause` / `mlxcont` / `mlxstop` | Control acquisition | – |

#### Environment

| Command | Description | Example |
|---------|-------------|---------|
| `environ` | Print T, P, H, dew point, Tsky | `environ` |
| `bmereinit` | Reinitialize BMP280 | – |
| `ntc [n]` | Get NTC temperature (°C) for channel n (0–3) | `ntc 1` |

#### Heater Control

| Command | Description | Example |
|---------|-------------|---------|
| `autoheater [= 0/1]` | Enable/disable automatic dew avoidance | `autoheater = 1` |
| `setheater [= N]` | Set holding temperature (°C) | `setheater = 25` |
| `clearheater` | Disable holding, turn off heaters | – |
| `pwm [n] [= val]` | Get/set PWM duty cycle (0–100%) for channel n | `pwm 0 = 50` |

#### System & Debug

| Command | Description | Example |
|---------|-------------|---------|
| `adc [n]` | Get raw ADC value (0–4095) for channel n | `adc 4` |
| `dac [= val]` | Get/set DAC output (0–4095) | `dac = 2048` |
| `mcutemp` | Show MCU internal temperature (°C) | – |
| `mcuvdd` | Show Vdd (V) | – |
| `time` | Show SysTick counter (ms) | – |
| `reset` | Software reset | – |
| `sendstr = text` | Forward string to other interface | `sendstr = "hello"` |

#### Raw I²C (for debugging)

| Command | Description | Example |
|---------|-------------|---------|
| `iicscan` | Scan I²C bus, report found addresses | – |
| `iicaddr [= addr]` | Get/set current I²C target address (non-shifted) | `iicaddr = 0x33` |
| `iicspeed [= n]` | Set I²C speed: 0=10k,1=100k,2=400k,3=1M,4=2M | – |
| `readreg reg [= n]` | Read n 16-bit registers | `readreg 0x8000` |
| `writedata = v1 v2 ...` | Write 16-bit values | `writedata = 0x1234 0x5678` |
| `hwaddr addr` | Change hardware I²C address of MLX90640 (non-shifted) | `hwaddr 0x15` |

### Output Formats

#### ASCII Thermal Map (`ascii`)
```
RANGE=12.345
MIN=-5.67
MAX=6.78
 .':;+*oxX#&%B$@
 .':;+*oxX#&%B$@
 ...
```
Uses 16‑character grayscale ramp: space (coldest) → `@` (hottest).

#### Numeric Map (`tempmap`)
```
-2.3 1.2 3.4 ...
 5.6 7.8 9.0 ...
...
```

#### Binary Image (`binary n`)
```
BINARY0=<768×4 bytes raw float array>ENDIMAGE\n
```
Little-endian IEEE 754 `float` values. Exact size: 3072 bytes per image.

#### Environment Output
```
TEMPERATURE=22.34
SKYTEMPERATURE=-15.67
PRESSURE_HPA=1013.25
PRESSURE_MM=759.98
HUMIDITY=65.20
TEMP_DEW=15.60
T_MEASUREMENT=12345678
```


### Sensors location

| N | ID   | Cardinal direction |
|---|------|--------------------|
| 0 | 0x10 | Zenith             |
| 1 | 0x11 | East               |
| 2 | 0x12 | South              |
| 3 | 0x13 | West               |
| 4 | 0x14 | North              |
---------------------------------


---

## Building & Flashing

### Prerequisites
- ARM GCC toolchain (`arm-none-eabi-gcc`)
- Make
- [st-flash](https://github.com/texane/stlink),
       [stm32flash](https://sourceforge.net/projects/stm32flash/) or
       [OpenOCD](https://openocd.sourceforge.io) (for flashing MCU)

### Build
```bash
git clone --depth=1 https://github.com/eddyem/stm32samples.git
cd stm32samples/F3:F303/MLX90640-allsky
make
```

### Configuration
- `EBUG` flag in Makefile enables debug output (slower, but verbose).
- `NOCAN` – define if CAN peripheral not used (increases USB buffer).
- `BUILD_NUMBER` and `BUILD_DATE` in `version.inc` are auto-generated.

### Flashing
```bash
make flash # for flashing over st-link
# make boot - for flashing over USART1 bootloader
# make dfuboot - for flashing over USB-DFU
```
or manually (over st-link):
```bash
openocd -f interface/stlink.cfg -f target/stm32f3x.cfg -c "program build/firmware.elf verify reset exit"
```

### udev Rule (Linux)
Create `/etc/udev/rules.d/99-USBiinterface.rules`:
```
ACTION=="add", ENV{USB_IDS}=="0483:5740", ATTRS{interface}=="?*", PROGRAM="/bin/bash -c \"ls /dev | grep $attr{interface} | wc -l \"", SYMLINK+="$attr{interface}%c", MODE="0666", GROUP="tty"
```
After replug, device appears as `/dev/ir-allsky0`.

---

## Calibration & Tuning

### NTC Thermistors
The firmware includes a **lookup table (LUT)** for NTC with B=3950, R25=1000Ω, covering -10..+59°C.
If a sensor reads `<5 ADC` → short circuit; `>4090 ADC` → open circuit.

To calibrate your NTCs, modify `Rlut[]` in `adc.c`.

### MLX90640
Calibration data is **per-sensor** and stored in EEPROM. The firmware reads it automatically on
startup.
No user calibration is required – the factory data is used.

### Vdd Measurement
Internal Vrefint (1.2V typical) and factory calibration value `*VREFINT_CAL_ADDR` are used:
```
Vdd = (VREFINT_CAL * 3.3) / ADC_VREF
```

### MCU Temperature
Internal sensor (ADC1_IN16) uses factory calibration `TEMP30_CAL_ADDR` and `TEMP110_CAL_ADDR`.

---

## Troubleshooting

### No USB device detected
- Check `USBPU_ON()` in `main.c` – some boards need pull-up control, others constant.
- Verify PA11/PA12 are configured as AF14.
- Try different USB cable / port.

### I²C communication errors
- Reduce speed: `iicspeed = 1` (100 kHz).
- Check pull-up resistors (2.2k–10kΩ on SCL/SDA).
- Use `iicscan` to verify sensor addresses.

### BMP280 not responding
- Check SPI wiring (PB3–PB5, PB9 CS).
- Try software reset: `bmereinit`.
- Verify chip ID: should be 0x58 (BMP280) or 0x60 (BME280).

### Heater not working
- Check `autoheater` status: `autoheater`.
- Verify NTC readings: `ntc`.
- For manual mode: `setheater = 25`, observe `pwm`.

### Cartoon mode stops updating
- Cartoon mode works **only over USB**, not UART.
- If no new images, check `state` – may be paused or in error.

### Watchdog resets
- IWDG period is 2 seconds. Long operations (e.g., full calibration dump) may trigger reset.  
- Disable in debug builds (remove `IWDG_REFRESH` calls or undefine `EBUG`).

---

## Performance & Limitations

| Parameter | Value |
|-----------|-------|
| Image update rate (5 sensors) | ~1–2 Hz (depends on I²C speed) |
| RAM usage | ~39 kB (image buffers + parameters) |
| Flash usage | ~26 kB |
| Maximum sensors | 5 (limited by I²C address space 0x10–0x14) |
| Temperature range | -40..+300°C (MLX90640) |
| Accuracy | ±1°C (typical after calibration) |

---

## License

All source files are licensed under **GNU General Public License v3.0** unless stated otherwise.  
The BMP280 driver includes MIT-licensed code from sheinz (2016).

---

## Credits & References

- **Author:** Edward V. Emelianov <edward.emelianoff@gmail.com>
- **MLX90640 Datasheet:** Melexis (Document REV 10)
- **BMP280 Datasheet:** Bosch Sensortec
- **STM32F303 Reference Manual:** STMicroelectronics (RM0316)

For questions, bug reports, or contributions, please use the GitHub issue tracker.
