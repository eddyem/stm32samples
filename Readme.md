# Repository Overview

The `stm32samples` repository is a collection of low-level, bare-metal code examples and complete
projects for various STM32 microcontroller series (F0, F1, F3, F4, G0). It uses CMSIS header files
from `modm-io/cmsis-header-stm32` for register definitions.

## Project Structure by MCU Series

### STM32F0 (F030, F042, F072)
- **3steppersLB** – Controls 3 stepper motors with encoders via USB/CAN bus
- **blink** – Simple LED toggling
- **CANbus4BTA** – CAN replacement for PEP-CAN
- **CANBUS_SSI** – Works with BTA SSI azimuth encoder
- **Chiller** – Basic chiller temperature controller
- **F0_testbrd** – Tests STM32F0x2 on custom board
- **htu21d_nucleo** – Reads HTU-21D sensor on Nucleo-F042K
- **morze** – Echoes USART1 data as Morse code on TIM3CH1 (PA6)
- **NUCLEO_SPI** – SSI over SPI on Nucleo-F042K
- **PL2303_ringbuffer** – USB CDC ACM with ring buffers
- **QuadEncoder** – Quadrature encoder via timer
- **Servo** – Controls SG-90 style servos
- **Socket_fans** – Manages multiple fans
- **TM1637** – Drives 7‑segment displays
- **tsys01_nucleo** – Reads two TSYS01 sensors
- **uart_blink & uart_blink_dma** – USART echo with DMA
- **usbcan_relay** – Relay board over CAN bus
- **usbcan_ringbuffer** – USB‑CAN adapter with ring buffers
- **USBHID** – USB HID keyboard + mouse

### STM32F1 (F103) 
- **BMP180, BMP280** – I²C temperature/pressure sensors
- **Canon_managing_device** – Controls Canon lenses
- **canUART** – UART‑CAN bridge
- **DHT22_DHT11** – Humidity sensors using timer+DMA
- **DS18** – DS18x20 temperature sensors
- **F1_testbrd** – Test board for LQFP48 packages
- **I2Cscan** – I²C scanner with register R/W
- **LED_Screen** – Drives 32×16 LED matrices (P10)
- **MAX7219_screen** – Controls 8×8 LED matrices
- **RGB_LED_Screen** – HUB75E RGB panel with 8.8.4 color
- **SevenCDCs** – Seven USB‑CDC ACM instances on one MCU (joke project)
- **shutter** – Bistable shutter via USB/external signal
- **Tetris** – Tetris on HUB75E panel
- **USB_HID** – USB HID mouse+keyboard
- **ws2815** – Rainbow effect on WS2815 strips

### STM32F3 (F303)
- **ADC** – Works with ADC peripheral
- **blink** – Toggles LEDs on PB0/PB1
- **CANusb** – USB‑CAN adapter
- **floatPrintf** – Converts floats to strings
- **Multistepper** – Controls up to 64 multiplexed steppers
- **NitrogenFlooding** – Automatic nitrogen flooding
- **PL2303** – PL2303 emulator (old)
- **Seven_CDCs** – Seven USB CDC ACM interfaces
- **usart1fullDMA** – USART1 Rx/Tx DMA with match detection
- **USB_template_CDC** – USB‑CDC template

### STM32F4 (F401)
- **blink** – Simple LED toggling

### STM32G0 (G070, G0B1)
- **blink** – Basic LED blink
- **RTC** – Real‑time clock
- **flash** – Flash memory operations
- **g0b1** – USB‑CDC working on G0B1
- **i2c** – I²C operations
- **usart** – USART communication

### Other Directories
- **deprecated** – Archive of older, no‑longer‑maintained code
- **snippets** – Small reusable code fragments
- **testboard** – KiCad 8 design files

## Development Environment

- **Toolchain**: arm-none-eabi‑gcc with `-flto` optimisations
- **Build**: Structured Makefiles per series
- **Debug**: OpenOCD configuration files provided
- **Libraries**: No third‑party libraries; only CMSIS‑SVD headers

## Project Maturity

- **Alpha**: USB template for F303, MLX90640 all‑sky camera
- **Stable**: LED blink, UART echo, sensor interfaces, USB‑CDC
- **Draft**: CANbus4BTA, MLX90640multi
- **Deprecated**: Older code in `deprecated/`

## Summary

This repository is a valuable resource for learning low‑level STM32 programming without vendor
libraries. It covers practical applications:
- **Industrial control**: Stepper motors, CAN bus, relay boards
- **Sensor interfacing**: Temperature, pressure, humidity, encoders
- **Human‑machine interfaces**: LED matrices, 7‑segment displays, USB HID
- **Communication**: UART, SPI, I²C, USB‑CDC, USB‑HID, CAN
- **Prototyping**: Test boards for F0, F1, F3, G0

For the latest updates, check the repository on GitHub.

## License

This repository is provided under the GNU General Public License v3 or later.
Full text of the GPLv3 is available at <https://www.gnu.org/licenses/gpl-3.0.html>.
