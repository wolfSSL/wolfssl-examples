CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS= -I$(WOLFSSL_INSTALL_DIR)/include -Wall
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm

all: dh-pg-ka

dh-pg-ka.o: dh-pg-ka.c dh-params.h
	$(CC) -c -o $@ $< $(CFLAGS)

dh-pg-ka: dh-pg-ka.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

.PHONY: clean

clean:
	rm -f *.o dh-pg-ka
