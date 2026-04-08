BINARY      := as3935
# MCU code
MCU         ?= F103x6
# change this linking script depending on particular MCU model,
LDSCRIPT    ?= stm32f103x6.ld
DEFINES := -DSTM32F10X_MD

include ../makefile.f1
include ../../makefile.stm32

$(OBJDIR)/commproto.o: commproto.cpp $(VERSION_FILE)
