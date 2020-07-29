CC=gcc
CFLAGS=-Wall
LIBS=

WOLFSSL_DIR := /usr/local
WOLFSSL_FLAGS=-I$(WOLFSSL_DIR)/include
WOLFSSL_LIB=-L$(WOLFSSL_DIR)/lib -lwolfssl

all:ecc_sign_verify

ecc_sign_verify:CFLAGS+=$(WOLFSSL_FLAGS)
ecc_sign_verify:ecc_sign_verify.o
	$(CC) -o $@ $^ $(LIBS) $(WOLFSSL_LIB) $(CFLAGS)

.PHONY: clean all

clean:
	rm -f *.o ecc_sign_verify
