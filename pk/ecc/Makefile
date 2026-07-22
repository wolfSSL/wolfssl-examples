CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS=-I$(WOLFSSL_INSTALL_DIR)/include -Wall
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))

all: $(TARGETS)

%: %.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f $(TARGETS) *.der *.x963 

check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: ecc_keys ecc_pub ecc_sign ecc_sign_deterministic ecc_verify
	./ecc_keys | grep -q 'storing public key into ecc-public.x963 (65 bytes)'
	./ecc_pub | grep -q 'Public Key Qy: 32'
	./ecc_sign | grep -q 'Success'
	./ecc_sign_deterministic | grep -q 'Success'
	./ecc_verify | grep -q 'Success'
	@echo "PASS: pk-ecc checks"
