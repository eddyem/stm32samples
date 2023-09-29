BINARY      := tsys01
# MCU code
MCU         := F042x6
# change this linking script depending on particular MCU model,
LDSCRIPT    := stm32f042x6.ld

DEFINES := -DUSARTNUM=1 -DI2CPINS=67 -DUSB2_16

include ../makefile.f0
include ../../makefile.stm32
