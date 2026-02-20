BINARY		:= multiiface
# MCU code
MCU			:= F303xc
# change this linking script depending on particular MCU model,
LDSCRIPT	:= stm32f303xB.ld
# add define "-DSPIDMA" to use DMA for SSI encoder, in that case IF1 (USART3) would be interrupt-driven!
DEFINES := -DUSB1_16

include ../makefile.f3
include ../../makefile.stm32
