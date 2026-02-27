## Makefile for TLS/DTLS Server using wolfHSM for crypto operations
##
## This example demonstrates a server that offloads all cryptographic
## operations to a wolfHSM server running on the POSIX transport with
## DMA support. By default, DTLS (UDP) mode is used.
##
## Usage:
##   1. Build: make DEBUG=1
##   2. Start the wolfHSM server: cd ../../posix/wh_posix_server && ./Build/wh_posix_server.elf --type dma
##   3. Run this server: ./Build/wh_server.elf
##   4. Connect with a client

## Project name - sets output filename
BIN = wh_server

## Important directories
PROJECT_DIR         ?= .
CONFIG_DIR          ?= $(PROJECT_DIR)/config

# wolfSSL and wolfHSM directories (relative to this Makefile)
WOLFSSL_DIR         ?= ../../../../wolfssl
WOLFHSM_DIR         ?= ../../..
WOLFHSM_PORT_DIR    ?= $(WOLFHSM_DIR)/port/posix

# Output directory for build files
BUILD_DIR           ?= $(PROJECT_DIR)/Build

## Includes
INC =   -I$(PROJECT_DIR) \
        -I$(CONFIG_DIR) \
        -I$(WOLFSSL_DIR) \
        -I$(WOLFHSM_DIR) \
        -I$(WOLFHSM_PORT_DIR)

## Defines
# POSIX requires C source be defined before any header
DEF += -D_POSIX_C_SOURCE=200809L

# Library configuration defines for user-supplied settings
DEF += -DWOLFSSL_USER_SETTINGS -DWOLFHSM_CFG

# Enable DMA transport by default (matches server --type dma)
DEF += -DWOLFHSM_CFG_DMA

## Architecture flags
ARCHFLAGS ?=

## Compiler and linker flags
ASFLAGS ?= $(ARCHFLAGS)
CFLAGS_EXTRA ?= -Wextra
CFLAGS ?= $(ARCHFLAGS) -Wno-cpp -std=c99 -Wall -Werror $(CFLAGS_EXTRA)
LDFLAGS ?= $(ARCHFLAGS)
LIBS = -lc -lm

# Platform-specific linker flags for dead code stripping
OS_NAME := $(shell uname -s | tr A-Z a-z)
ifeq ($(OS_NAME),darwin)
    LDFLAGS += -Wl,-dead_strip
else
    LDFLAGS += -Wl,--gc-sections
endif

## Makefile options

# Set to @ to suppress command echo
CMD_ECHO ?=

# Debug build
ifeq ($(DEBUG),1)
    DBGFLAGS = -ggdb -g3 -O0
    CFLAGS += $(DBGFLAGS)
    LDFLAGS += $(DBGFLAGS)
    DEF += -DWOLFHSM_CFG_DEBUG
endif

# Verbose debug output
ifeq ($(DEBUG_VERBOSE),1)
    DBGFLAGS = -ggdb -g3 -O0
    CFLAGS += $(DBGFLAGS)
    LDFLAGS += $(DBGFLAGS)
    DEF += -DWOLFHSM_CFG_DEBUG -DWOLFHSM_CFG_DEBUG_VERBOSE
endif

# Address sanitizer
ifeq ($(ASAN),1)
    CFLAGS += -fsanitize=address
    LDFLAGS += -fsanitize=address
endif

## Source files

# wolfCrypt source files
SRC_C += $(wildcard $(WOLFSSL_DIR)/wolfcrypt/src/*.c)

# wolfSSL TLS source files
SRC_C += $(wildcard $(WOLFSSL_DIR)/src/*.c)

# wolfHSM source files
SRC_C += $(wildcard $(WOLFHSM_DIR)/src/*.c)

# wolfHSM POSIX port/HAL code
SRC_C += $(wildcard $(WOLFHSM_PORT_DIR)/*.c)

# Project source files
SRC_C += $(PROJECT_DIR)/server.c
SRC_C += $(PROJECT_DIR)/server_io.c

## Automated processing

FILENAMES_C = $(notdir $(SRC_C))
OBJS_C = $(addprefix $(BUILD_DIR)/, $(FILENAMES_C:.c=.o))
vpath %.c $(dir $(SRC_C))

## Makefile Targets

.PHONY: all build clean help

all: build

build: $(BUILD_DIR) $(BUILD_DIR)/$(BIN).elf

$(BUILD_DIR):
	$(CMD_ECHO) mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	@echo "Compiling: $(notdir $<)"
	$(CMD_ECHO) $(CC) $(CFLAGS) $(DEF) $(INC) -c -o $@ $<

$(BUILD_DIR)/$(BIN).elf: $(OBJS_C)
	@echo "Linking: $(notdir $@)"
	$(CMD_ECHO) $(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR)

help:
	@echo "TLS/DTLS Server with wolfHSM Crypto Offload"
	@echo ""
	@echo "Options:"
	@echo "  DEBUG=1         - Enable debug build with symbols"
	@echo "  DEBUG_VERBOSE=1 - Enable verbose debug output"
	@echo "  ASAN=1          - Enable address sanitizer"
	@echo ""
	@echo "Example:"
	@echo "  make DEBUG=1"
