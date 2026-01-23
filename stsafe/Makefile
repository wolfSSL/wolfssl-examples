# STSAFE-A120 Test Makefile for Raspberry Pi
#
# Copyright 2025 wolfSSL Inc.

CC = gcc
CFLAGS = -Wall -Wextra -O0 -g -Wno-strict-prototypes -DWOLFSSL_USER_SETTINGS

# wolfSSL configuration
# Option 1: Use installed wolfSSL (after make install)
#WOLFSSL_DIR = /usr/local
# Option 2: Use local wolfSSL source
WOLFSSL_DIR = $(HOME)/wolfssl

# STSELib paths
STSELIB_DIR = $(HOME)/STSELib
PLATFORM_DIR = ./platform

# Include paths
# Note: Current directory (.) must come first so our user_settings.h is found
INCLUDES = -I. \
           -I$(STSELIB_DIR) \
           -I$(PLATFORM_DIR) \
           -I$(WOLFSSL_DIR)

# Build directory for artifacts
BUILD_DIR = build

# wolfSSL source files - build directly from source
WOLFSSL_SRC = $(wildcard $(WOLFSSL_DIR)/src/*.c)
WOLFSSL_CRYPT_SRC = $(wildcard $(WOLFSSL_DIR)/wolfcrypt/src/*.c)
WOLFSSL_PORT_SRC = $(WOLFSSL_DIR)/wolfcrypt/src/port/st/stsafe.c
WOLFSSL_ALL_SRC = $(WOLFSSL_SRC) $(WOLFSSL_CRYPT_SRC) $(WOLFSSL_PORT_SRC)

# wolfSSL static library
WOLFSSL_LIB = $(BUILD_DIR)/libwolfssl.a

# Additional linker flags
LDFLAGS = -L$(BUILD_DIR) -lwolfssl -lm

# STSELib source files
STSELIB_CORE_SRC = $(STSELIB_DIR)/core/stse_device.c \
                   $(STSELIB_DIR)/core/stse_frame.c \
                   $(STSELIB_DIR)/core/stse_generic_typedef.c \
                   $(STSELIB_DIR)/core/stse_platform.c \
                   $(STSELIB_DIR)/core/stse_session.c

STSELIB_API_SRC = $(wildcard $(STSELIB_DIR)/api/*.c)
STSELIB_SERVICES_SRC = $(wildcard $(STSELIB_DIR)/services/stsafea/*.c) \
                       $(wildcard $(STSELIB_DIR)/services/stsafel/*.c)
STSELIB_CERT_SRC = $(wildcard $(STSELIB_DIR)/certificate/*.c)

# Platform source files
PLATFORM_SRC = $(PLATFORM_DIR)/stse_platform_linux.c

# Platform crypto source (wolfSSL implementation)
# Uncomment when building with wolfSSL crypto support:
PLATFORM_CRYPTO_SRC = $(PLATFORM_DIR)/stse_platform_crypto_wolfssl.c

# Targets
STSAFE_TARGET = stsafe_test
WOLFSSL_TARGET = wolfssl_stsafe_test
WOLFSSL_FULL_TARGET = wolfssl_stsafe_full_test
ALL_TARGETS = $(STSAFE_TARGET) $(WOLFSSL_TARGET) $(WOLFSSL_FULL_TARGET)

.PHONY: all clean test-all info

all: $(WOLFSSL_LIB) $(ALL_TARGETS)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build wolfSSL static library
$(WOLFSSL_LIB): $(WOLFSSL_ALL_SRC) | $(BUILD_DIR)
	@echo "Building wolfSSL library..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $(WOLFSSL_ALL_SRC)
	mv *.o $(BUILD_DIR)/
	ar rcs $@ $(BUILD_DIR)/*.o
	@echo "wolfSSL library built successfully"

# Generic rule for all test targets - each target depends on its corresponding .c file
$(ALL_TARGETS): %: %.c $(WOLFSSL_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< \
		$(PLATFORM_SRC) $(PLATFORM_CRYPTO_SRC) \
		$(STSELIB_CORE_SRC) $(STSELIB_API_SRC) $(STSELIB_SERVICES_SRC) $(STSELIB_CERT_SRC) \
		$(LDFLAGS)

test-all: all
	@echo "=== Running stsafe_test ===" && ./$(STSAFE_TARGET)
	@echo ""
	@echo "=== Running wolfssl_stsafe_test ===" && ./$(WOLFSSL_TARGET)
	@echo ""
	@echo "=== Running wolfssl_stsafe_full_test ===" && ./$(WOLFSSL_FULL_TARGET)

clean:
	rm -rf $(BUILD_DIR) $(ALL_TARGETS)

# Show configuration
info:
	@echo "STSELib directory: $(STSELIB_DIR)"
	@echo "wolfSSL directory: $(WOLFSSL_DIR)"
	@echo "Platform directory: $(PLATFORM_DIR)"
	@echo "Include paths: $(INCLUDES)"
	@echo "Linker flags: $(LDFLAGS)"
	@echo ""
	@echo "Build targets:"
	@echo "  make          - Build all test executables"
	@echo "  make test-all - Build and run all tests"
	@echo "  make clean    - Clean build artifacts"
	@echo "  make info     - Show this configuration"
