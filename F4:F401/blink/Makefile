BINARY		= blink
BOOTPORT	?= /dev/ttyUSB0
BOOTSPEED	?= 115200
# MCU FAMILY
FAMILY		?= F4
# MCU code
MCU			?= F407xx
# or __ARM_ARCH_7EM__
ARMARCH = __ARM_ARCH_7M__=1
# change this linking script depending on particular MCU model,
LDSCRIPT	?= stm32f407xg.ld
# debug
#DEFS		= -DEBUG

INDEPENDENT_HEADERS=

FP_FLAGS	?= -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
ASM_FLAGS	?= -mthumb -mcpu=cortex-m4 -mthumb-interwork -mlittle-endian -march=armv7e-m
ARCH_FLAGS	= $(ASM_FLAGS) $(FP_FLAGS) -D $(ARMARCH)

# autoincremental version & build date
VERSION_FILE = version.inc
ifeq ($(shell test -e $(VERSION_FILE) && echo -n yes), yes)
    NEXTVER := $(shell expr $$(awk '/#define BUILD_NUMBER/' $(VERSION_FILE) | tr -cd "[0-9]") + 1)
else
    NEXTVER := "1"
endif

BUILDDATE := $(shell date +%Y-%m-%d)

###############################################################################
# Executables
#PREFIX		?= arm-none-eabi
# gcc from arm web site 
PREFIX		?= /opt/bin/arm-none-eabi
TOOLCHLIB	?= /opt/arm-none-eabi/lib
RM			:= rm -f
RMDIR		:= rmdir
CC			:= $(PREFIX)-gcc
# don't replace ld with gcc: the binary size would be much greater!!
LD			:= $(PREFIX)-ld
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
OBJDIR 		:= mk
SRC			:= $(wildcard *.c)
OBJS		:= $(addprefix $(OBJDIR)/, $(SRC:%.c=%.o))
STARTUP		:= $(OBJDIR)/startup.o
MAP			:= $(OBJDIR)/$(BINARY).map
OBJS 		+= $(STARTUP)
# dependencies: we need them to recompile files if their headers-dependencies changed
DEPS		:= $(OBJS:.o=.d)

INC_DIR ?= ../inc

INCLUDE 	:= -I$(INC_DIR)/Fx -I$(INC_DIR)/cm
LIB_DIR		:= $(INC_DIR)/ld

###############################################################################
# C flags
CFLAGS		+= -g3 -gdwarf-2
CFLAGS		+= -O2 -D__thumb2__=1 -MD
CFLAGS		+= -Wall -Werror -Wextra -Wshadow 
CFLAGS		+= -fshort-enums -ffunction-sections -fdata-sections -flto
#CFLAGS		+= -fno-common -ffunction-sections -fdata-sections -fno-stack-protector
CFLAGS		+=  $(ARCH_FLAGS)

###############################################################################
# Linker flags
#LDFLAGS		+= -nostartfiles --static -nostdlib  -specs=nosys.specs -specs=nano.specs
LDFLAGS		+= $(ARCH_FLAGS)
LDFLAGS		+= -specs=nano.specs -flto
LDFLAGS		+= -L$(LIB_DIR)
#LDFLAGS		+= -L$(TOOLCHLIB)
LDFLAGS		+= -T$(LDSCRIPT)
LDFLAGS		+= -Wl,-Map=$(MAP),--cref -Wl,--gc-sections

###############################################################################
# Used libraries
#LDLIBS		+=  -lc $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

DEFS		+= -DSTM32$(FAMILY) -DSTM32$(MCU)

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
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDE) -o $@ -c $<

$(VERSION_FILE): *.[ch]
	[ -f $(VERSION_FILE) ] || echo -e "#define BUILD_NUMBER \"0\"\n#define BUILD_DATE \"none\"" > $(VERSION_FILE)
	@echo "  Generate version: $(NEXTVER) for date $(BUILDDATE)"
	@sed -i "s/#define BUILD_NUMBER.*/#define BUILD_NUMBER \"$(NEXTVER)\"/" $(VERSION_FILE)
	@sed -i "s/#define BUILD_DATE.*/#define BUILD_DATE \"$(BUILDDATE)\"/" $(VERSION_FILE)

#$(OBJDIR)/proto.o: proto.c $(VERSION_FILE)

$(OBJDIR)/%.o: %.c
	@echo "  CC      $<"
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDE) -o $@ -c $<

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
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $(ELF)

size: $(ELF)
	$(SIZE) $(ELF)

clean:
	@echo "  CLEAN"
	$(RM) $(OBJS) $(DEPS) $(ELF) $(HEX) $(LIST) $(MAP)
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

openocd:
	openocd -f openocd.cfg
dbg:
	arm-none-eabi-gdb $(ELF) -ex 'target remote localhost:3333' -ex 'monitor reset halt'

.PHONY: size clean flash boot dfuboot openocd dbg
