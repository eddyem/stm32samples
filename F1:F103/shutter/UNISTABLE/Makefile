BINARY      := shutter
# MCU code
MCU         ?= F103x6
# change this linking script depending on particular MCU model,
LDSCRIPT    ?= stm32f103x6.ld
DEFINES := -DUSB1_16 -DSTM32F10X_LD
INC_DIR := ../../inc

include ../../makefile.f1
include ../../../makefile.stm32
