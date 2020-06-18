CC=gcc
CFLAGS=-Wall
LIBS=

OPENSSL_DIR=/usr/local/opt
OPENSSL_FLAGS=-I$(OPENSSL_DIR)/openssl/include
WOLFSSL_FLAGS=
OPENSSL_LIB=-L$(OPENSSL_DIR)/openssl/lib -lcrypto -lssl
WOLFSSL_LIB=-lwolfssl

all:wolfsigtest opensigtest eccsiglentest

opensigtest:CFLAGS+=$(OPENSSL_FLAGS)
opensigtest:opensigtest.o
	$(CC) -o $@ $(LIBS) $^ $(CFLAGS) $(OPENSSL_LIB) 

wolfsigtest:CFLAGS+=$(WOLFSSL_FLAGS)
wolfsigtest:wolfsigtest.o
	$(CC) -o $@ $(LIBS) $^ $(CFLAGS) $(WOLFSSL_LIB) 

eccsiglentest:CFLAGS+=$(WOLFSSL_FLAGS)
eccsiglentest:eccsiglentest.o
	$(CC) -o $@ $(LIBS) $^ $(CFLAGS) $(WOLFSSL_LIB)

.PHONY: clean all

clean:
	rm -f *.o wolfsigtest opensigtest eccsiglentest
