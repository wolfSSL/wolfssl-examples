CC=gcc
CFLAGS=-Wall -g
WOLFSSL_INSTALL_DIR=/usr/local
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

ascon-file-encrypt: ascon-file-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o ascon-file-encrypt

check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: ascon-file-encrypt
	printf 'ascon ci input\n' > in.txt
	printf 'ciphertestpw\n' | script -qec './ascon-file-encrypt -e -i in.txt -o out.enc' /dev/null | grep -q 'Success: Ascon encrypt complete'
	@echo "PASS: crypto-ascon checks"
