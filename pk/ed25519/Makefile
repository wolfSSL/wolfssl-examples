CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS=-I$(WOLFSSL_INSTALL_DIR)/include -Wall
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm

SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))

all: $(TARGETS)

%: %.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f $(TARGETS) *.der

check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: ed25519_keys ed25519_pub ed25519_sign ed25519_verify
	./ed25519_keys | grep -q 'writing public key to ed25519-pubkey.der (32 bytes)'
	./ed25519_pub | grep -q 'Public Key: 32'
	./ed25519_sign | grep -q 'Success'
	./ed25519_verify | grep -q 'Success'
	@echo "PASS: pk-ed25519 checks"
