# make debug adds -DEBUG -Werror
# make ADDEFS="additional defs"
BINARY		= canstepper
BOOTPORT	?= /dev/ttyUSB0
BOOTSPEED	?= 115200
# MCU FAMILY
FAMILY		?= F0
# MCU code (STM32F072xx)
MCU			?= F072xB
# change this linking script depending on particular MCU model,
LDSCRIPT	?= stm32f0728.ld
DEFS		= ${ADDEFS} -DVERSION=\"0.0.1\" -DUSARTNUM=1
TARGET := RELEASE

FP_FLAGS	?= -msoft-float
ASM_FLAGS	?= -mthumb -mcpu=cortex-m0 -march=armv6-m -mtune=cortex-m0
ARCH_FLAGS	= $(ASM_FLAGS) $(FP_FLAGS)

###############################################################################
# Executables
#PREFIX		?= arm-none-eabi
# gcc from arm web site
PREFIX		?= /opt/bin/arm-none-eabi
RM			:= rm -f
RMDIR		:= rmdir
CC			:= $(PREFIX)-gcc
LD			:= $(PREFIX)-gcc
AR			:= $(PREFIX)-ar
AS			:= $(PREFIX)-as
SIZE 		:= $(PREFIX)-size
OBJCOPY		:= $(PREFIX)-objcopy
OBJDUMP		:= $(PREFIX)-objdump
GDB			:= $(PREFIX)-gdb
STFLASH		:= $(shell which st-flash)
STBOOT		:= $(shell which stm32flash)
DFUUTIL		:= $(shell which dfu-util)

###############################################################################
# Source files
OBJDIR 		= mk
SRC			:= $(wildcard *.c)
OBJS		:= $(addprefix $(OBJDIR)/, $(SRC:%.c=%.o))
STARTUP		= $(OBJDIR)/startup.o
OBJS 		+= $(STARTUP)
# dependencies: we need them to recompile files if their headers-dependencies changed
DEPS		:= $(OBJS:.o=.d)

INC_DIR ?= ../../inc

INCLUDE 	:= -I$(INC_DIR)/Fx -I$(INC_DIR)/cm
LIB_DIR		:= $(INC_DIR)/ld

###############################################################################
# C flags
CFLAGS		+= -O2 -g -MD -D__thumb2__=1
CFLAGS		+= -Wall -Wextra -Wshadow
CFLAGS		+= -fno-common -ffunction-sections -fdata-sections

###############################################################################
# Linker flags
LDFLAGS		+= --static -nostartfiles --specs=nano.specs
LDFLAGS		+= -L$(LIB_DIR)
LDFLAGS		+= -T$(LDSCRIPT)
LDFLAGS     += -Wl,-Map=$(OBJDIR)/$(BINARY).map
LDFLAGS     += -Wl,--gc-sections

###############################################################################
# Used libraries
LDLIBS      += -Wl,--start-group -lc -lgcc -Wl,--end-group
LDLIBS		+= $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)
DEFS		+= -DSTM32$(FAMILY) -DSTM32$(MCU)

ELF		:= $(OBJDIR)/$(BINARY).elf
LIST	:= $(OBJDIR)/$(BINARY).list
BIN		:= $(BINARY).bin
HEX		:= $(BINARY).hex

all: $(OBJDIR)/RELEASE bin list size
release: all

debug: CFLAGS += -DEBUG -Werror -W -Wimplicit-function-declaration
debug: $(OBJDIR)/DEBUG bin list size

$(OBJDIR)/DEBUG:
	@rm -rf $(OBJDIR)
	@mkdir $(OBJDIR)
	@> $(OBJDIR)/DEBUG
	@echo "TARGET: DEBUG"
	echo "CFLAGS += -DEBUG -Werror -W -Wimplicit-function-declaration" > $(OBJDIR)/CFLAGS
$(OBJDIR)/RELEASE:
	@rm -rf $(OBJDIR)
	@mkdir $(OBJDIR)
	@> $(OBJDIR)/RELEASE
	@echo "TARGET: RELEASE"
	echo "" > $(OBJDIR)/CFLAGS

elf: $(ELF)
bin: $(BIN)
hex: $(HEX)
list: $(LIST)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
-include $(OBJDIR)/CFLAGS
endif

$(OBJDIR):
	mkdir $(OBJDIR)

$(STARTUP): $(INC_DIR)/startup/vector.c
	@echo "  CC   startup"
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDE) $(ARCH_FLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.c
	@echo "  CC	  $<"
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDE) $(ARCH_FLAGS) -o $@ -c $<

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
	@echo "  LD	  $(ELF)"
	$(LD) $(LDFLAGS) $(ARCH_FLAGS) $(OBJS) $(LDLIBS) -o $(ELF)

size: $(ELF)
	$(SIZE) $(ELF)

clean:
	@echo "  CLEAN"
	@$(RM) $(HEX)
	@$(RM) -rf $(OBJDIR) 2>/dev/null || true


flash: $(BIN)
	@echo "  FLASH  $(BIN)"
	$(STFLASH) --reset write $(BIN) 0x8000000

boot: $(BIN)
	@echo "  LOAD $(BIN) through bootloader"
	$(STBOOT) -b$(BOOTSPEED) $(BOOTPORT) -w $(BIN)

dfuboot: $(BIN)
	@echo "  LOAD  $(BIN) THROUGH DFU"
	$(DFUUTIL) -a0 -D $(BIN) -s 0x08000000

.PHONY: clean flash boot
