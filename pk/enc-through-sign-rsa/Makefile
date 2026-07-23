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
	rm -f $(TARGETS) encryptedAesKey

check: rsa-private-encrypt-app rsa-public-decrypt-app
	./rsa-private-encrypt-app >/dev/null
	out=$$(./rsa-public-decrypt-app) && printf '%s' "$$out" | grep -q 'Thisismyfakeaeskeythatis32bytes!'
	@echo "PASS: pk-enc-through-sign-rsa checks"
