CC = gcc

WOLFSSL_INSTALL_DIR = /usr/local

WOLFSSL_CFLAGS = -Wextra -Werror -Wall -I$(WOLFSSL_INSTALL_DIR)/include
WOLFSSL_LIBS   = -L$(WOLFSSL_INSTALL_DIR)/lib -lm -lwolfssl

DEBUG_FLAGS    = -g -DDEBUG

COMMON = ../mlkem-cryptocb.c

all: client-tls13-mlkem server-tls13-mlkem

client-tls13-mlkem: client-tls13-mlkem.c $(COMMON)
	$(CC) -o $@ $^ $(WOLFSSL_CFLAGS) $(WOLFSSL_LIBS) $(DEBUG_FLAGS)

server-tls13-mlkem: server-tls13-mlkem.c $(COMMON)
	$(CC) -o $@ $^ $(WOLFSSL_CFLAGS) $(WOLFSSL_LIBS) $(DEBUG_FLAGS)

.PHONY: clean all

clean:
	rm -f *.o client-tls13-mlkem server-tls13-mlkem
