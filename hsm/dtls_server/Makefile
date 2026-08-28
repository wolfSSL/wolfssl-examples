# Makefile for TLS/DTLS Server using wolfHSM for crypto operations
#
# This example demonstrates a server that offloads all cryptographic
# operations to a wolfHSM server running on the POSIX transport with
# DMA support. By default, DTLS (UDP) mode is used.
#
# Usage:
#   make download_repos    # Clone wolfSSL and wolfHSM repos
#   make all               # Build everything (wolfSSL, wolfHSM server, DTLS server)
#   make run_hsm_server    # Start wolfHSM server
#   make run_dtls_server   # Start wolfSSL DTLS server (this example)
#   make run_client        # Run the wolfSSL DTLS client
#   make clean             # Clean build artifacts
#   make clean_repos       # Remove cloned repositories

BIN = wh_server

WOLFSSL_DIR ?= ./wolfssl
WOLFHSM_DIR ?= ./wolfhsm
WOLFHSM_PORT_DIR = $(WOLFHSM_DIR)/port/posix
WOLFHSM_SERVER_DIR = $(WOLFHSM_DIR)/examples/posix/wh_posix_server

PROJECT_DIR = .
CONFIG_DIR = $(PROJECT_DIR)/config
BUILD_DIR = $(PROJECT_DIR)/Build

# Compiler settings
CC = gcc
CSTD = -std=c99
CFLAGS_EXTRA = -Werror -Wall -Wextra -ffunction-sections -fdata-sections
CFLAGS = $(CSTD) $(CFLAGS_EXTRA)

# Defines
DEF = -D_POSIX_C_SOURCE=200809L -DWOLFSSL_USER_SETTINGS -DWOLFHSM_CFG
DEF += -DWOLFHSM_CFG_DMA

# Includes
INC = -I$(PROJECT_DIR) -I$(CONFIG_DIR) -I$(WOLFSSL_DIR) -I$(WOLFHSM_DIR) -I$(WOLFHSM_PORT_DIR)

# Linker settings (platform-specific: darwin uses -dead_strip, others use --gc-sections)
OS_NAME := $(shell uname -s | tr A-Z a-z)
ifeq ($(OS_NAME),darwin)
LDFLAGS = -Wl,-dead_strip
else
LDFLAGS = -Wl,--gc-sections
endif
LIBS = -lc -lm

# Source files (wolfCrypt, wolfSSL, wolfHSM, port, project)
SRC_C = $(wildcard $(WOLFSSL_DIR)/wolfcrypt/src/*.c)
SRC_C += $(wildcard $(WOLFSSL_DIR)/src/*.c)
SRC_C += $(wildcard $(WOLFHSM_DIR)/src/*.c)
SRC_C += $(wildcard $(WOLFHSM_PORT_DIR)/*.c)
SRC_C += $(PROJECT_DIR)/server.c $(PROJECT_DIR)/server_io.c

# Debug support
ifeq ($(DEBUG),1)
    CFLAGS += -ggdb -g3
    LDFLAGS += -ggdb -g3
    DEF += -DWOLFHSM_CFG_DEBUG
endif

# Object files
FILENAMES_C = $(notdir $(SRC_C))
OBJS_C = $(addprefix $(BUILD_DIR)/, $(FILENAMES_C:.c=.o))
vpath %.c $(dir $(SRC_C))

# Phony targets
.PHONY: all download_repos build_wolfssl build_wolfhsm_server build_app run_hsm_server run_dtls_server run_client clean clean_repos

# Default target
all: check_repos build_wolfssl build_wolfhsm_server build_app
	@echo "Build complete. Run 'make run_hsm_server', 'make run_dtls_server', 'make run_client' in separate terminals."

# Clone repositories
download_repos:
	@echo "=== Cloning repositories ==="
	@if [ ! -d "$(WOLFSSL_DIR)" ]; then \
		git clone --depth 1 https://github.com/wolfssl/wolfssl.git $(WOLFSSL_DIR); \
	else \
		echo "wolfssl already exists, skipping clone"; \
	fi
	@if [ ! -d "$(WOLFHSM_DIR)" ]; then \
		git clone --depth 1 https://github.com/wolfssl/wolfhsm.git $(WOLFHSM_DIR); \
	else \
		echo "wolfhsm already exists, skipping clone"; \
	fi

# Check that repos exist
check_repos:
	@if [ ! -d "$(WOLFSSL_DIR)" ] || [ ! -d "$(WOLFHSM_DIR)" ]; then \
		echo "Error: Repositories not found. Run 'make download_repos' first."; \
		exit 1; \
	fi

# Build wolfSSL (for example client)
build_wolfssl: check_repos
	@echo "=== Building wolfSSL ==="
	@if [ ! -f "$(WOLFSSL_DIR)/examples/server/server" ]; then \
		cd $(WOLFSSL_DIR) && \
		./autogen.sh && \
		./configure --enable-dtls --enable-dtls13 --enable-ecc && \
		make -j; \
	else \
		echo "wolfSSL already built, skipping"; \
	fi

# Build wolfHSM POSIX server (with DMA for this example)
# Note: The wolfHSM server Makefile expects WOLFSSL_DIR relative to its location
# Server is at ./wolfhsm/examples/posix/wh_posix_server/
# wolfssl is at ./wolfssl/
# So from server: ../../../../wolfssl
build_wolfhsm_server: check_repos
	@echo "=== Building wolfHSM server ==="
	@if [ ! -f "$(WOLFHSM_SERVER_DIR)/Build/wh_posix_server.elf" ]; then \
		$(MAKE) -C $(WOLFHSM_SERVER_DIR) clean || true; \
		$(MAKE) -C $(WOLFHSM_SERVER_DIR) WOLFSSL_DIR=../../../../wolfssl DMA=1 -j; \
	else \
		echo "wolfHSM server already built, skipping"; \
	fi

# Build DTLS server
build_app: $(BUILD_DIR) $(BUILD_DIR)/$(BIN).elf
	@echo "DTLS server built: $(BUILD_DIR)/$(BIN).elf"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	@echo "Compiling: $(notdir $<)"
	$(CC) $(CFLAGS) $(DEF) $(INC) -c -o $@ $<

$(BUILD_DIR)/$(BIN).elf: $(OBJS_C)
	@echo "Linking: $(notdir $@)"
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Convenience targets for running each component in separate terminals

run_hsm_server: all
	@echo "Starting wolfHSM server..."
	@echo "Press Ctrl+C to stop"
	@echo ""
	$(WOLFHSM_SERVER_DIR)/Build/wh_posix_server.elf --type dma

run_dtls_server: all
	@echo "Starting wolfSSL DTLS server (wolfHSM crypto offload)..."
	@echo "Press Ctrl+C to stop"
	@echo ""
	$(BUILD_DIR)/$(BIN).elf -p 11111 -A $(WOLFSSL_DIR)/certs/client-cert.pem

run_client: all
	cd $(WOLFSSL_DIR) && ./examples/client/client -u -v 4 -h 127.0.0.1 -p 11111

# Clean build artifacts
clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR)
	@# Clean wolfHSM server build
	@if [ -d "$(WOLFHSM_SERVER_DIR)" ]; then \
		$(MAKE) -C $(WOLFHSM_SERVER_DIR) clean 2>/dev/null || true; \
	fi
	@# Clean wolfSSL build
	@if [ -f "$(WOLFSSL_DIR)/Makefile" ]; then \
		$(MAKE) -C $(WOLFSSL_DIR) clean 2>/dev/null || true; \
	fi

clean_repos: clean
	@echo "Removing cloned repositories"
	rm -rf $(WOLFSSL_DIR) $(WOLFHSM_DIR)
