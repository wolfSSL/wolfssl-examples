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

check: ed25519_keys ed25519_pub ed25519_sign ed25519_verify
	out=$$(./ed25519_keys) && printf '%s' "$$out" | grep -q 'writing public key to ed25519-pubkey.der (32 bytes)'
	out=$$(./ed25519_pub) && printf '%s' "$$out" | grep -q 'Public Key: 32'
	out=$$(./ed25519_sign) && printf '%s' "$$out" | grep -q 'Success'
	out=$$(./ed25519_verify) && printf '%s' "$$out" | grep -q 'Success'
	@echo "PASS: pk-ed25519 checks"
