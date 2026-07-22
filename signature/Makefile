CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR = /usr/local
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

all: signature

signature: signature.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o signature

check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: signature
	./signature Makefile | grep -q 'ECC Signature Verification: Pass (0)'
	@echo "PASS: signature checks"
