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

check: ed448_keys ed448_pub ed448_sign ed448_verify
	./ed448_keys | grep -q 'writing public key to ed448-pubkey.der (57 bytes)'
	./ed448_pub | grep -q 'Public Key: 57'
	./ed448_sign | grep -q 'Success'
	./ed448_verify | grep -q 'Success'
	@echo "PASS: pk-ed448 checks"
