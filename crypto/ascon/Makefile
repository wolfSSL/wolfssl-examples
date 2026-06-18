CC=gcc
CFLAGS=-Wall -g
WOLFSSL_INSTALL_DIR=/usr/local
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

ascon-file-encrypt: ascon-file-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o ascon-file-encrypt
