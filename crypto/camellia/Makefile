CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR=/usr/local
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

camellia-encrypt: camellia-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

.PHONY: clean check

clean:
	rm -f *.o camellia-encrypt

check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: camellia-encrypt
	printf 'camellia ci input\n' > in.txt
	printf 'ciphertestpw\n' | script -qec './camellia-encrypt -e 128 -i in.txt -o out.enc' /dev/null | grep -q 'Success: Camellia encrypt complete'
	@echo "PASS: crypto-camellia checks"
