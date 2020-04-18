Development board for TMC2130/DRV8825 stepper driver modules
============================================================

Stepper control over CAN bus, RS-485 and USB.

Pinout
======

PA0  - AIN0 (12V voltage control)                     AIN
PA1  - AIN1 (5V voltage control)                      AIN
PA3  - STEP                                           timer
PA4  - DIR                                            PP
PA5  - SCK - CFG1 - microstepping1                    SPI/PP
PA6  - MISO - CFG0 - ~RST                             SPI/PP
PA7  - MOSI - CFG1 - microstepping0                   SPI/PP
PA8  - Tx|Rx (RS485 direction)                        PP
PA9  - Tx (RS485)                                     USART
PA10 - Rx (RS485)                                     USART
PA11 - DM (USB)                                       USB
PA12 - DP (USB)                                       USB
PA13 - SWDIO (st-link)                                SWD
PA14 - SWCLK (st-link)                                SWD

PB0  - ESW0                                           PUin
PB1  - ESW1 (limit switches or other inputs)          PUin
PB2  - ESW2                                           PUin
PB8  - CAN_Rx (CAN)                                   CAN
PB9  - CAN_Tx (CAN)                                   CAN
PB10 - ESW3                                           PUin
PB12 - brdaddr0                                       PUin
PB13 - brdaddr1 (bits of board address switch)        PUin
PB14 - brdaddr2                                       PUin
PB15 - brdaddr3                                       PUin

PC13 - CFG6 - ~EN                                     PP
PC14 - CFG3 - ~CS - microstepping2                    PP
PC15 - ~SLEEP                                         PP

PF0 - VIO_on (turn ON Vdd of driver 4988 or 2130)     OD
PF1 - ~FAULT (~fault output of 8825)                  FLin

RS-485
======

The same protocol as USB, but 1st symbol should be BRDADDR


CAN
===

Data format: big-endian. For example 0x03 0x04 0x05 0x0a means 0x0304050a.
Messages with variable width.
IN messages have ID = 0x70 | (devNo<<1), devNo - number, selected by jumpers @ board.
OUT messages have ID=IN+1.
zeros byte of data is command. All other - data.


TODO
====

Add linecoding_handler to change RS-485 speed due to USB connection settings?
