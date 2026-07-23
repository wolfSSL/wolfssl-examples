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
	out=$$(./ed448_keys) && printf '%s' "$$out" | grep -q 'writing public key to ed448-pubkey.der (57 bytes)'
	out=$$(./ed448_pub) && printf '%s' "$$out" | grep -q 'Public Key: 57'
	out=$$(./ed448_sign) && printf '%s' "$$out" | grep -q 'Success'
	out=$$(./ed448_verify) && printf '%s' "$$out" | grep -q 'Success'
	@echo "PASS: pk-ed448 checks"
