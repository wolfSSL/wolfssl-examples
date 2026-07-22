CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR=/usr/local
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

all:pkcs12-example pkcs12-create-example

pkcs12-example: pkcs12-example.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

pkcs12-create-example: pkcs12-create-example.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o pkcs12-example pkcs12-create-example output.p12

check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: pkcs12-example pkcs12-create-example
	# capture first: grep -q closes the pipe on the long DER dump and pipefail then sees the binary's SIGPIPE
	out=$$(./pkcs12-example 2>&1); grep -q 'return value of parsing pkcs12 = 0 SUCCESS' <<< "$$out"
	out=$$(./pkcs12-create-example 2>&1); grep -q 'Printing PKCS12 DER file to output.p12' <<< "$$out"
	@echo "PASS: crypto-pkcs12 checks"
