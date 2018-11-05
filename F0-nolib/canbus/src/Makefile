BINARY		= canbus
BOOTPORT	?= /dev/ttyUSB0
BOOTSPEED	?= 57600
# MCU FAMILY
FAMILY		= F0
# MCU code
MCU			= F042x6
# hardware definitions
DEFS		+= -DUSARTNUM=1
#DEFS		+= -DCHECK_TMOUT
DEFS		+= -DEBUG
# change this linking script depending on particular MCU model,
# for example, if you have STM32F103VBT6, you should write:
LDSCRIPT	= stm32f042k.ld

INDEPENDENT_HEADERS=

FP_FLAGS	?= -msoft-float
ASM_FLAGS	= -mthumb -mcpu=cortex-m0 -march=armv6-m -mtune=cortex-m0
ARCH_FLAGS	= $(ASM_FLAGS) $(FP_FLAGS)

###############################################################################
# Executables
OPREFIX		?= /opt/bin/arm-none-eabi
#PREFIX		?= /usr/x86_64-pc-linux-gnu/arm-none-eabi/gcc-bin/7.3.0/arm-none-eabi
PREFIX		?= $(OPREFIX)

RM			:= rm -f
RMDIR		:= rmdir
CC			:= $(PREFIX)-gcc
LD			:= $(PREFIX)-gcc
AR			:= $(PREFIX)-ar
AS			:= $(PREFIX)-as
OBJCOPY		:= $(OPREFIX)-objcopy
OBJDUMP		:= $(OPREFIX)-objdump
GDB			:= $(OPREFIX)-gdb
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

INC_DIR ?= ../../inc

INCLUDE 	:= -I$(INC_DIR)/F0 -I$(INC_DIR)/cm
LIB_DIR		:= $(INC_DIR)/ld

###############################################################################
# C flags
CFLAGS		+= -O2 -g -MD -D__thumb2__=1
CFLAGS		+= -Wall -Werror -Wextra -Wshadow -Wimplicit-function-declaration
CFLAGS		+= -Wredundant-decls $(INCLUDE)
# -Wmissing-prototypes -Wstrict-prototypes
CFLAGS		+= -fno-common -ffunction-sections -fdata-sections

###############################################################################
# Linker flags
LDFLAGS		+= --static -nostartfiles
#--specs=nano.specs
LDFLAGS		+= -L$(LIB_DIR)
LDFLAGS		+= -T$(LDSCRIPT)
LDFLAGS		+= -Wl,-Map=$(OBJDIR)/$(BINARY).map
LDFLAGS		+= -Wl,--gc-sections

###############################################################################
# Used libraries
LDLIBS		+= -Wl,--start-group -lc -lgcc -Wl,--end-group
LDLIBS		+= $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

DEFS		+= -DSTM32$(FAMILY) -DSTM32$(MCU)

#.SUFFIXES: .elf .bin .hex .srec .list .map .images
#.SECONDEXPANSION:
#.SECONDARY:

ELF		:= $(OBJDIR)/$(BINARY).elf
LIST	:= $(OBJDIR)/$(BINARY).list
BIN		:= $(BINARY).bin
HEX		:= $(BINARY).hex

all: bin list

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
	$(LD) $(LDFLAGS) $(ARCH_FLAGS) $(OBJS) $(LDLIBS) -o $(ELF)

clean:
	@echo "  CLEAN"
	$(RM) $(OBJS) $(DEPS) $(ELF) $(HEX) $(LIST) $(OBJDIR)/*.map *.d
	@rmdir $(OBJDIR) 2>/dev/null || true


flash: $(BIN)
	@echo "  FLASH  $(BIN)"
	$(STFLASH) write $(BIN) 0x8000000

boot: $(BIN)
	@echo "  LOAD $(BIN) through bootloader"
	$(STBOOT) -b$(BOOTSPEED) $(BOOTPORT) -w $(BIN)

gentags:
	CFLAGS="$(CFLAGS) $(DEFS)" geany -g $(BINARY).c.tags *[hc] 2>/dev/null

.PHONY: clean flash boot gentags
