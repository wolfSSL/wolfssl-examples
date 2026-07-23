CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS+=-I$(WOLFSSL_INSTALL_DIR)/include
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm

all: keywrap

keywrap: keywrap.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean all check

clean:
	rm -f *.o keywrap

check: keywrap
	out=$$(./keywrap) && printf '%s' "$$out" | grep -q 'Decrypted Random Key: '
	@echo "PASS: crypto-keywrap checks"
