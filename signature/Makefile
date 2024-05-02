CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR = /usr/local
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

all: signature

signature: signature.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o signature
