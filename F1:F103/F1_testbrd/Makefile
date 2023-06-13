BINARY      := pl2303
# MCU code
MCU         ?= F103x6
# change this linking script depending on particular MCU model,
LDSCRIPT    ?= stm32f103x6.ld
DEFINES     := -DSTM32F10X_LD -DUSB1_16 

include ../makefile.f1
include ../../makefile.stm32
