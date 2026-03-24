# Makefile for wolfCrypt SRAM PUF bare-metal example
#
# Target: Cortex-M (tested on NUCLEO-H563ZI)
# Usage:  make            - build puf_example.elf and puf_example.hex
#         make clean      - remove build artifacts
#         make PUF_TEST=0 - build for real hardware SRAM (no test mode)
#
# Copyright (C) 2006-2026 wolfSSL Inc.

# Toolchain
TOOLCHAIN ?= arm-none-eabi-
CC = $(TOOLCHAIN)gcc
LD = $(TOOLCHAIN)gcc
AR = $(TOOLCHAIN)ar
OBJCOPY = $(TOOLCHAIN)objcopy
OBJDUMP = $(TOOLCHAIN)objdump
SIZE = $(TOOLCHAIN)size
NM = $(TOOLCHAIN)nm

# wolfSSL root (relative to this directory)
WOLFSSL_ROOT ?= ../../wolfssl

# Build output
BUILD_DIR = ./Build
BIN = puf_example

# Architecture
ARCHFLAGS = -mcpu=cortex-m33 -mthumb -mabi=aapcs

# Compiler flags
CFLAGS = $(ARCHFLAGS) -std=gnu99 -Wall -Os
CFLAGS += -ffunction-sections -fdata-sections -fno-builtin
CFLAGS += -DWOLFSSL_USER_SETTINGS
CFLAGS += -I. -I$(WOLFSSL_ROOT)

# Linker flags
LDFLAGS = $(ARCHFLAGS)
LDFLAGS += --specs=nosys.specs --specs=nano.specs
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(BIN).map
LDFLAGS += -T./linker.ld

# Math lib
LIBS = -lm

# Source files
SRC_C  = main.c
SRC_C += startup.c
SRC_C += stm32.c

# wolfCrypt sources needed for PUF
SRC_C += $(WOLFSSL_ROOT)/wolfcrypt/src/puf.c
SRC_C += $(WOLFSSL_ROOT)/wolfcrypt/src/sha256.c
SRC_C += $(WOLFSSL_ROOT)/wolfcrypt/src/kdf.c
SRC_C += $(WOLFSSL_ROOT)/wolfcrypt/src/hmac.c
SRC_C += $(WOLFSSL_ROOT)/wolfcrypt/src/hash.c
SRC_C += $(WOLFSSL_ROOT)/wolfcrypt/src/memory.c
SRC_C += $(WOLFSSL_ROOT)/wolfcrypt/src/wc_port.c
SRC_C += $(WOLFSSL_ROOT)/wolfcrypt/src/error.c
SRC_C += $(WOLFSSL_ROOT)/wolfcrypt/src/misc.c
SRC_C += $(WOLFSSL_ROOT)/wolfcrypt/src/logging.c
SRC_C += $(WOLFSSL_ROOT)/wolfcrypt/src/random.c
SRC_C += $(WOLFSSL_ROOT)/wolfcrypt/src/sp_int.c
SRC_C += $(WOLFSSL_ROOT)/wolfcrypt/src/sha3.c

# Object files
FILENAMES_C = $(notdir $(SRC_C))
OBJS_C = $(addprefix $(BUILD_DIR)/, $(FILENAMES_C:.c=.o))
vpath %.c $(dir $(SRC_C))

# Targets
.PHONY: all clean

all: $(BUILD_DIR) $(BUILD_DIR)/$(BIN).hex
	@echo ""
	$(SIZE) $(BUILD_DIR)/$(BIN).elf

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	@echo "Compiling: $(notdir $<)"
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/$(BIN).elf: $(OBJS_C)
	@echo "Linking: $(notdir $@)"
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo ""
	$(NM) -n $@ > $(BUILD_DIR)/$(BIN).sym
	$(OBJDUMP) -S $@ > $(BUILD_DIR)/$(BIN).disasm

$(BUILD_DIR)/$(BIN).hex: $(BUILD_DIR)/$(BIN).elf
	@echo "Generating HEX: $(notdir $@)"
	$(OBJCOPY) -O ihex $< $@

clean:
	rm -f $(BUILD_DIR)/*.elf $(BUILD_DIR)/*.hex $(BUILD_DIR)/*.map
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.sym $(BUILD_DIR)/*.disasm
