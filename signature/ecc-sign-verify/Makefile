CC=gcc
CFLAGS=-Wall
LIBS=

WOLFSSL_INSTALL_DIR := /usr/local
WOLFSSL_FLAGS=-I$(WOLFSSL_INSTALL_DIR)/include
WOLFSSL_LIB=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

all:ecc_sign_verify

ecc_sign_verify:CFLAGS+=$(WOLFSSL_FLAGS)
ecc_sign_verify:ecc_sign_verify.o
	$(CC) -o $@ $^ $(LIBS) $(WOLFSSL_LIB) $(CFLAGS)

.PHONY: clean all check

clean:
	rm -f *.o ecc_sign_verify

check: ecc_sign_verify
	./ecc_sign_verify | grep -q 'Successfully verified signature w/ ecc key size 521!'
	@echo "PASS: signature-ecc-sign-verify checks"
