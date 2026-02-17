# wolfHSM DTLS Client Example
#
# Usage:
#   make download_repos    # Clone wolfSSL and wolfHSM repos
#   make all               # Build everything (wolfSSL server, wolfHSM server, client)
#   make run_hsm_server    # Start wolfHSM server
#   make run_dtls_server   # Start wolfSSL DTLS server
#   make run_client        # Run the DTLS client
#   make clean             # Clean build artifacts
#   make clean_repos       # Remove cloned repositories

BIN = wh_dtls_client

WOLFSSL_DIR ?= ./wolfssl
WOLFHSM_DIR ?= ./wolfhsm
WOLFHSM_PORT_DIR = $(WOLFHSM_DIR)/port/posix
WOLFHSM_SERVER_DIR = $(WOLFHSM_DIR)/examples/posix/wh_posix_server

PROJECT_DIR = .
BUILD_DIR = $(PROJECT_DIR)/Build

# Compiler settings
CC = gcc
CSTD = -std=c99
CFLAGS_EXTRA = -Werror -Wall -Wextra -ffunction-sections -fdata-sections
CFLAGS = $(CSTD) $(CFLAGS_EXTRA)
DEF = -D_POSIX_C_SOURCE=200809L -DWOLFSSL_USER_SETTINGS -DWOLFHSM_CFG
DEF += -DWC_USE_DEVID=0x5748534D

INC = -I$(PROJECT_DIR) -I$(WOLFSSL_DIR) -I$(WOLFHSM_DIR) -I$(WOLFHSM_PORT_DIR)

# Linker settings
LDFLAGS = -Wl,--gc-sections
LIBS = -lc -lm

# Source files (wolfCrypt, wolfSSL, wolfHSM, port, project)
WOLFCRYPT_SRC := $(wildcard $(WOLFSSL_DIR)/wolfcrypt/src/*.c)
SRC_C = $(filter-out %/evp.c %/misc.c,$(WOLFCRYPT_SRC))
WOLFSSL_SRC := $(wildcard $(WOLFSSL_DIR)/src/*.c)
SRC_C += $(filter-out %/bio.c %/conf.c %/pk.c %/ssl_asn1.c %/ssl_bn.c %/ssl_certman.c %/ssl_crypto.c %/ssl_load.c %/ssl_misc.c %/ssl_p7p12.c %/ssl_sess.c %/ssl_sk.c %/x509.c %/x509_str.c,$(WOLFSSL_SRC))
SRC_C += $(wildcard $(WOLFHSM_DIR)/src/*.c)
SRC_C += $(wildcard $(WOLFHSM_PORT_DIR)/*.c)
SRC_C += $(wildcard $(PROJECT_DIR)/*.c)

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
	@echo "Build complete. Run 'make help' for usage instructions."

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

# Build wolfSSL
# Note: The DTLS client uses its own user_settings.h to build wolfSSL statically,
# so this configure is only for the wolfSSL example server/client binaries.
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

# Build wolfHSM POSIX server
# Note: The wolfHSM server Makefile expects WOLFSSL_DIR relative to its location
# Server is at ./wolfhsm/examples/posix/wh_posix_server/
# wolfssl is at ./wolfssl/
# So from server: ../../../../wolfssl
build_wolfhsm_server: check_repos
	@echo "=== Building wolfHSM server ==="
	@if [ ! -f "$(WOLFHSM_SERVER_DIR)/Build/wh_posix_server.elf" ]; then \
		$(MAKE) -C $(WOLFHSM_SERVER_DIR) clean || true; \
		$(MAKE) -C $(WOLFHSM_SERVER_DIR) WOLFSSL_DIR=../../../../wolfssl -j; \
	else \
		echo "wolfHSM server already built, skipping"; \
	fi

# Build DTLS client
build_app: $(BUILD_DIR) $(BUILD_DIR)/$(BIN).elf
	@echo "DTLS client built: $(BUILD_DIR)/$(BIN).elf"

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
	$(WOLFHSM_SERVER_DIR)/Build/wh_posix_server.elf --type tcp \
		--key $(WOLFSSL_DIR)/certs/ecc-client-key.der \
		--id 1 --client 12

run_dtls_server: all
	@echo "Starting wolfSSL DTLS server..."
	@echo "Press Ctrl+C to stop"
	@echo ""
	cd $(WOLFSSL_DIR) && ./examples/server/server -u -v 4 \
		-c ./certs/server-ecc.pem \
		-k ./certs/ecc-key.pem \
		-A ./certs/client-ecc-cert.pem \
		-p 11111 -i

run_client: all
	$(BUILD_DIR)/$(BIN).elf 127.0.0.1

# Clean build artifacts
clean:
	@echo "Cleaning build files"
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
