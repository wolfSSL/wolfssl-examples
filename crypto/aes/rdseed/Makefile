CC ?= gcc
WOLFSSL_INSTALL_DIR ?= /usr/local

CPPFLAGS ?= -I$(WOLFSSL_INSTALL_DIR)/include
CFLAGS ?= -O2 -Wall -Wextra -Werror
CFLAGS += -mrdseed
LDFLAGS ?=
LDLIBS ?= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm

all: aesgcm-rdseed

aesgcm-rdseed: aesgcm-rdseed.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o aesgcm-rdseed

.PHONY: all clean check

check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: aesgcm-rdseed
	./aesgcm-rdseed | grep -q 'Decrypted'
	@echo "PASS: crypto-aes-rdseed checks"
