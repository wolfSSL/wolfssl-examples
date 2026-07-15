CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS+=-I$(WOLFSSL_INSTALL_DIR)/include
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm

all: keywrap

keywrap: keywrap.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o keywrap
