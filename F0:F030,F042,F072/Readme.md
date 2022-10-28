Samples for STM32F042-nucleo and chinese STM32F030-based devboard
=================================

This directory contains examples for F0 without any library

- **3steppersLB** - control independent moving of 3 stepper motors with encoders (by USB and CAN bus)
- **blink** - simple LED blink
- **canbus** - simplest CAN bus development board
- **CANbus4BTA** - (under development) device to replace old PEP-CAN @BTA
- **CANBUS_SSI** - work with BTA SSI azimutal encoder
- **CANbus_stepper** - (under development) stepper motor management over CAN-bus, USB and RS-485
- **Chiller** - simplest chiller controller
- **F0_testbrd** - sample code for STM32F0x2 checking via test board
- **htu21d_nucleo** - operaing with HTU-21D in STM32F042-nucleo
- **inc** - all encludes need
- **morze** - for STM32F030, echo data from USART1 on TIM3CH1 (PA6) as Morze code
- **NUCLEO_SPI** - tests of SSI over SPI
- **pl2303** - CDC template (emulation of PL2303) (*deprecated*)
- **PL2303_ringbuffer** - emulation of PL2303 with ring buffers on USB Rx/Tx (USE THIS instead of `pl2303`!!!)
- **QuadEncoder** - sample code for working with quadrature encoder (via timer)
- **Servo** - simple servo (like SG-90) controller
- **Snippets** - some small code snippets
- **Socket_fans** - fan manager
- **TM1637** - work with 7-segment LED indicators based on TM1637
- **tsys01_nucleo** - read two TSYS01 sensors using STM32F042
- **uart_blink** - code for STM32F030F4, echo data on USART1 and blink LEDS on PA4 and PA5
- **uart_blink_dma** - USART over DMA
- **uart_nucleo** - USART over DMA for STM32F042-nucleo
- **usbcan** - USB--CAN adapter using PL2303 emulation (*deprecated*)
- **usbcan_relay** - relay and some other management board over CAN bus
- **usbcan_ringbuffer** - USB--CAN adapter (USE THIS instead of `usbcan`!!!)
- **USBHID** - USB HID keyboard + mouse
- **USB_pl2303_snippet** - full minimal snippet of PL2303 emulation for STM32F072xB (*deprecated*)
