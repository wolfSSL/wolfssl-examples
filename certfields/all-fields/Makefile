CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local

CFLAGS=-I$(WOLFSSL_INSTALL_DIR)/include -Wall
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

app: main.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o app
