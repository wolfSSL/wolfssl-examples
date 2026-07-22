CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS=-Wall
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

all: certverify

certverify: certverify.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o certverify

check: certverify
	./certverify | grep -q 'Verified certificate'
	@echo "PASS: certstore checks"
