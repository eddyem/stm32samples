BINARY		:= bme280
# MCU code
MCU			:= F303xb
# change this linking script depending on particular MCU model,
LDSCRIPT	:= stm32f303xB.ld
DEFINES := -DUSB1_16
LDADD := -lm

include ../makefile.f3
include ../../makefile.stm32
