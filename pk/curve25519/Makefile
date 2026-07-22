CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS= -I$(WOLFSSL_INSTALL_DIR)/include -Wall
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

curve25519_test: curve25519_test.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o curve25519_test

check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: curve25519_test
	./curve25519_test | grep -q 'Curve25519 test success'
	@echo "PASS: pk-curve25519 checks"
