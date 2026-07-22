CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR=/usr/local
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

all:pkcs12-example pkcs12-create-example

pkcs12-example: pkcs12-example.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

pkcs12-create-example: pkcs12-create-example.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o pkcs12-example pkcs12-create-example output.p12

check: pkcs12-example pkcs12-create-example
	./pkcs12-example | grep -q 'return value of parsing pkcs12 = 0 SUCCESS'
	./pkcs12-create-example | grep -q 'Printing PKCS12 DER file to output.p12'
	@echo "PASS: crypto-pkcs12 checks"
