BINARY      := PL2303
# MCU code
MCU         ?= F103x6
# change this linking script depending on particular MCU model,
LDSCRIPT    ?= stm32f103x6.ld
DEFINES := -DUSB1_16 -DSTM32F10X_LD

include ../makefile.f1
include ../../makefile.stm32
