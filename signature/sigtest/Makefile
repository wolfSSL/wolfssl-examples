CC=gcc
CFLAGS=-Wall
LIBS=

OPENSSL_FLAGS=-I/usr/local/ssl/include
WOLFSSL_FLAGS=
OPENSSL_LIB=-lcrypto -lssl
WOLFSSL_LIB=-lwolfssl

all:wolfsigtest opensigtest

opensigtest:CFLAGS+=$(OPENSSL_FLAGS)
opensigtest:opensigtest.o
	$(CC) -o $@ $(LIBS) $(OPENSSL_LIB) $^ $(CFLAGS)

wolfsigtest:CFLAGS+=$(WOLFSSL_FLAGS)
wolfsigtest:wolfsigtest.o
	$(CC) -o $@ $(LIBS) $(WOLFSSL_LIB) $^ $(CFLAGS)

.PHONY: clean all

clean:
	rm -f *.o wolfsigtest opensigtest
