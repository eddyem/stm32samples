BINARY		= blink
BOOTPORT	?= /dev/ttyUSB0
BOOTSPEED	?= 115200
# MCU FAMILY
FAMILY		?= F1
# MCU code
MCU			?= F103xB
#density (stm32f10x.h, lines 70-84)
DENSITY 	?= MD
DEFS		= -DEBUG
# change this linking script depending on particular MCU model,
# for example, if you have STM32F103VBT6, you should write:
LDSCRIPT	?= ld/stm32f103xB.ld
#LDSCRIPT = STM32F103C8.ld

INDEPENDENT_HEADERS=

FP_FLAGS	?= -msoft-float
ASM_FLAGS	?= -mthumb -mcpu=cortex-m3 
#-mfix-cortex-m3-ldrd
ARCH_FLAGS	= $(ASM_FLAGS) $(FP_FLAGS)

###############################################################################
# Executables
PREFIX		?= arm-none-eabi

RM			:= rm -f
RMDIR		:= rmdir
CC			:= $(PREFIX)-gcc
LD			:= $(PREFIX)-ld
AR			:= $(PREFIX)-ar
AS			:= $(PREFIX)-as
SIZE 		:= $(PREFIX)-size
OBJCOPY		:= $(PREFIX)-objcopy
OBJDUMP		:= $(PREFIX)-objdump
GDB			:= $(PREFIX)-gdb
STFLASH		:= $(shell which st-flash)
STBOOT		:= $(shell which stm32flash)

###############################################################################
# Source files
OBJDIR 		= mk
LDSCRIPT	?= $(BINARY).ld
SRC			:= $(wildcard *.c)
OBJS		:= $(addprefix $(OBJDIR)/, $(SRC:%.c=%.o))
STARTUP		= $(OBJDIR)/startup.o
OBJS 		+= $(STARTUP)
DEPS		:= $(OBJS:.o=.d)

INC_DIR ?= ../inc

INCLUDE 	:= -I$(INC_DIR)/Fx -I$(INC_DIR)/cm
LIB_DIR		:= $(INC_DIR)/ld

###############################################################################
# C flags
CFLAGS		+= -O0 -g
# -MD -D__thumb2__=1
#CFLAGS		+= -Wall -Werror -Wextra -Wshadow -Wimplicit-function-declaration
#CFLAGS		+= -Wredundant-decls
# -Wmissing-prototypes -Wstrict-prototypes
CFLAGS		+= -fno-common 
#-ffunction-sections -fdata-sections

###############################################################################
# Linker flags
LDFLAGS		+= -nostartfiles
# --static
#--specs=nano.specs
LDFLAGS		+= -L$(LIB_DIR)
LDFLAGS		+= -T$(LDSCRIPT)
#LDFLAGS		+= -Wl,-Map=$(OBJDIR)/$(BINARY).map
#LDFLAGS		+= -Wl,--gc-sections

###############################################################################
# Used libraries
#LDLIBS		+= -Wl,--start-group -lc -lgcc -Wl,--end-group 
#LDLIBS		+= $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

DEFS		+= -DSTM32$(FAMILY) -DSTM32$(MCU) -DSTM32F10X_$(DENSITY)

#.SUFFIXES: .elf .bin .hex .srec .list .map .images
#.SECONDEXPANSION:
#.SECONDARY:

ELF		:= $(OBJDIR)/$(BINARY).elf
LIST	:= $(OBJDIR)/$(BINARY).list
BIN		:= $(BINARY).bin
HEX		:= $(BINARY).hex

all: bin list size

elf: $(ELF)
bin: $(BIN)
hex: $(HEX)
list: $(LIST)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

$(OBJDIR):
	mkdir $(OBJDIR)

$(STARTUP): $(INC_DIR)/startup/vector.c
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDE) $(ARCH_FLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.c
	@echo "  CC      $<"
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDE) $(ARCH_FLAGS) -o $@ -c $<

#$(OBJDIR)/%.d: %.c $(OBJDIR)
#	$(CC) -MM -MG $< | sed -e 's,^\([^:]*\)\.o[ ]*:,$(@D)/\1.o $(@D)/\1.d:,' >$@

$(BIN): $(ELF)
	@echo "  OBJCOPY $(BIN)"
	$(OBJCOPY) -Obinary $(ELF) $(BIN)

$(HEX): $(ELF)
	@echo "  OBJCOPY $(HEX)"
	$(OBJCOPY) -Oihex $(ELF) $(HEX)

$(LIST): $(ELF)
	@echo "  OBJDUMP $(LIST)"
	$(OBJDUMP) -S $(ELF) > $(LIST)

$(ELF): $(OBJDIR) $(OBJS)
	@echo "  LD      $(ELF)"
	$(LD) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $(ELF)

size: $(ELF)
	$(SIZE) $(ELF)    

clean:
	@echo "  CLEAN"
	$(RM) $(OBJS) $(DEPS) $(ELF) $(HEX) $(LIST) $(OBJDIR)/*.map
	@rmdir $(OBJDIR) 2>/dev/null || true


flash: $(BIN)
	@echo "  FLASH  $(BIN)"
	$(STFLASH) write $(BIN) 0x8000000

boot: $(BIN)
	@echo "  LOAD $(BIN) through bootloader"
	$(STBOOT) -b$(BOOTSPEED) $(BOOTPORT) -w $(BIN)

dfuboot: $(BIN)
	@echo "  LOAD  $(BIN) THROUGH DFU"
	$(DFUUTIL) -a0 -D $(BIN) -s 0x08000000

.PHONY: clean flash boot
