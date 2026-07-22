CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS= -I$(WOLFSSL_INSTALL_DIR)/include -Wall
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

hpke_test: hpke_test.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o hpke_test

check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: hpke_test
	./hpke_test | grep -q 'HPKE test success'
	@echo "PASS: pk-hpke checks"
