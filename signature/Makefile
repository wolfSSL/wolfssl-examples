CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR = /usr/local
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

all: signature

signature: signature.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o signature

check: signature
	out=$$(./signature Makefile) && printf '%s' "$$out" | grep -q 'ECC Signature Verification: Pass (0)'
	@echo "PASS: signature checks"
