CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR=/usr/local
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

all:pkcs12-example pkcs12-create-example

pkcs12-example: pkcs12-example.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

pkcs12-create-example: pkcs12-create-example.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o pkcs12-example pkcs12-create-example output.p12
