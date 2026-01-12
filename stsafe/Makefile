# STSAFE-A120 Test Makefile for Raspberry Pi
#
# Copyright 2025 wolfSSL Inc.

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g

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

# wolfSSL library flags
# For installed wolfSSL:
#LDFLAGS = -L$(WOLFSSL_DIR)/lib -lwolfssl -lm
# For local wolfSSL source (using .libs):
LDFLAGS = -L$(WOLFSSL_DIR)/src/.libs -lwolfssl -lm -Wl,-rpath,$(WOLFSSL_DIR)/src/.libs

# wolfSSL compile flags
# WOLFSSL_USER_SETTINGS tells wolfSSL to include user_settings.h
CFLAGS += -DWOLFSSL_USER_SETTINGS

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

# Test source files
TEST_SRC = stsafe_test.c
WOLFSSL_TEST_SRC = wolfssl_stsafe_test.c
WOLFSSL_FULL_TEST_SRC = wolfssl_stsafe_full_test.c

# All sources
ALL_SRC = $(STSELIB_CORE_SRC) $(STSELIB_API_SRC) $(STSELIB_SERVICES_SRC) \
          $(STSELIB_CERT_SRC) $(PLATFORM_SRC) $(PLATFORM_CRYPTO_SRC) $(TEST_SRC)

# wolfSSL STSAFE port source
WOLFSSL_STSAFE_SRC = $(WOLFSSL_DIR)/wolfcrypt/src/port/st/stsafe.c

# Targets
TARGET = stsafe_test
WOLFSSL_TARGET = wolfssl_stsafe_test
WOLFSSL_FULL_TARGET = wolfssl_stsafe_full_test

.PHONY: all clean test test-all info basic wolfssl wolfssl-full

all: $(TARGET) wolfssl wolfssl-full

# Full build with wolfSSL crypto support
$(TARGET): $(TEST_SRC) $(PLATFORM_SRC) $(PLATFORM_CRYPTO_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ \
		$(TEST_SRC) \
		$(PLATFORM_SRC) \
		$(PLATFORM_CRYPTO_SRC) \
		$(STSELIB_CORE_SRC) \
		$(STSELIB_API_SRC) \
		$(STSELIB_SERVICES_SRC) \
		$(STSELIB_CERT_SRC) \
		$(LDFLAGS)

# Basic build without wolfSSL (for initial testing without host sessions)
basic: $(TEST_SRC) $(PLATFORM_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) \
		$(TEST_SRC) \
		$(PLATFORM_SRC) \
		$(STSELIB_CORE_SRC) \
		$(STSELIB_API_SRC) \
		$(STSELIB_SERVICES_SRC) \
		$(STSELIB_CERT_SRC)

# wolfSSL crypto callback test build
# Note: We compile stsafe.c directly since it's not built into libwolfssl by default
# IMPORTANT: Use WOLFSSL_USE_OPTIONS_H to get options.h included for proper wolfSSL settings
# We add -DWOLFSSL_STSAFEA120 -DUSE_STSAFE_RNG_SEED to enable STSAFE in stsafe.c
WOLFSSL_STSAFE_FLAGS = -DWOLFSSL_USE_OPTIONS_H -DWOLFSSL_STSAFEA120 -DUSE_STSAFE_RNG_SEED -Wno-strict-prototypes
WOLFSSL_BASE_FLAGS = -Wall -Wextra -O2 -g
wolfssl: $(WOLFSSL_TEST_SRC) $(PLATFORM_SRC) $(PLATFORM_CRYPTO_SRC)
	$(CC) $(WOLFSSL_BASE_FLAGS) $(WOLFSSL_STSAFE_FLAGS) -I$(STSELIB_DIR) -I$(PLATFORM_DIR) -I$(WOLFSSL_DIR) \
		-c $(WOLFSSL_STSAFE_SRC) -o stsafe.o
	$(CC) $(WOLFSSL_BASE_FLAGS) $(WOLFSSL_STSAFE_FLAGS) -I$(STSELIB_DIR) -I$(PLATFORM_DIR) -I$(WOLFSSL_DIR) \
		-c $(PLATFORM_SRC) -o stse_platform_linux.o
	$(CC) $(WOLFSSL_BASE_FLAGS) $(WOLFSSL_STSAFE_FLAGS) -I$(STSELIB_DIR) -I$(PLATFORM_DIR) -I$(WOLFSSL_DIR) \
		-c $(PLATFORM_CRYPTO_SRC) -o stse_platform_crypto.o
	$(CC) $(WOLFSSL_BASE_FLAGS) $(WOLFSSL_STSAFE_FLAGS) -I$(STSELIB_DIR) -I$(PLATFORM_DIR) -I$(WOLFSSL_DIR) \
		-o $(WOLFSSL_TARGET) \
		$(WOLFSSL_TEST_SRC) \
		stse_platform_linux.o stse_platform_crypto.o stsafe.o \
		$(STSELIB_CORE_SRC) \
		$(STSELIB_API_SRC) \
		$(STSELIB_SERVICES_SRC) \
		$(STSELIB_CERT_SRC) \
		$(LDFLAGS)

test: $(TARGET)
	./$(TARGET)

test-wolfssl: wolfssl
	./$(WOLFSSL_TARGET)

# Full wolfSSL integration test with ECDH and benchmarks
wolfssl-full: $(WOLFSSL_FULL_TEST_SRC) $(PLATFORM_SRC) $(PLATFORM_CRYPTO_SRC)
	$(CC) $(WOLFSSL_BASE_FLAGS) $(WOLFSSL_STSAFE_FLAGS) -I$(STSELIB_DIR) -I$(PLATFORM_DIR) -I$(WOLFSSL_DIR) \
		-c $(WOLFSSL_STSAFE_SRC) -o stsafe.o
	$(CC) $(WOLFSSL_BASE_FLAGS) $(WOLFSSL_STSAFE_FLAGS) -I$(STSELIB_DIR) -I$(PLATFORM_DIR) -I$(WOLFSSL_DIR) \
		-c $(PLATFORM_SRC) -o stse_platform_linux.o
	$(CC) $(WOLFSSL_BASE_FLAGS) $(WOLFSSL_STSAFE_FLAGS) -I$(STSELIB_DIR) -I$(PLATFORM_DIR) -I$(WOLFSSL_DIR) \
		-c $(PLATFORM_CRYPTO_SRC) -o stse_platform_crypto.o
	$(CC) $(WOLFSSL_BASE_FLAGS) $(WOLFSSL_STSAFE_FLAGS) -I$(STSELIB_DIR) -I$(PLATFORM_DIR) -I$(WOLFSSL_DIR) \
		-o $(WOLFSSL_FULL_TARGET) \
		$(WOLFSSL_FULL_TEST_SRC) \
		stse_platform_linux.o stse_platform_crypto.o stsafe.o \
		$(STSELIB_CORE_SRC) \
		$(STSELIB_API_SRC) \
		$(STSELIB_SERVICES_SRC) \
		$(STSELIB_CERT_SRC) \
		$(LDFLAGS)

test-wolfssl-full: wolfssl-full
	./$(WOLFSSL_FULL_TARGET)

test-all: all
	@echo "=== Running stsafe_test ===" && ./$(TARGET)
	@echo ""
	@echo "=== Running wolfssl_stsafe_test ===" && ./$(WOLFSSL_TARGET)
	@echo ""
	@echo "=== Running wolfssl_stsafe_full_test ===" && ./$(WOLFSSL_FULL_TARGET)

clean:
	rm -f $(TARGET) $(WOLFSSL_TARGET) $(WOLFSSL_FULL_TARGET) *.o stsafe.o stse_platform_linux.o stse_platform_crypto.o

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
	@echo "  make basic    - Build without wolfSSL (basic tests only)"
	@echo "  make clean    - Clean build artifacts"
	@echo "  make info     - Show this configuration"
