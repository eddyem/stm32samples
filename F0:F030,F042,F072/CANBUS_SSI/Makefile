BINARY      := canstepper
# MCU code
MCU         ?= F072xB
# change this linking script depending on particular MCU model,
LDSCRIPT    ?= stm32f0728.ld
DEFINES     := -DVERSION=\"0.0.1\" -DUSARTNUM=1

include ../makefile.f0
include ../../makefile.stm32
