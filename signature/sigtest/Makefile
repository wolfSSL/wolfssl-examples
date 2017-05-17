CC=clang
CFLAGS=-Wall
LIBS=

OPENSSL_FLAGS=
WOLFSSL_FLAGS=
OPENSSL_LIB=-lcrypto -lssl
WOLFSSL_LIB=-lwolfssl

all:wolfsigtest opensigtest

opensigtest:opensigtest.o
	$(CC) -o $@ $(LIBS) $(OPENSSL_LIB) $^ $(CFLAGS) $(OPENSSL_FLAGS)

wolfsigtest:wolfsigtest.o
	$(CC) -o $@ $(LIBS) $(WOLFSSL_LIB) $^ $(CFLAGS) $(WOLFSSL_FLAGS)

.PHONY: clean all

clean:
	rm -f *.o wolfsigtest opensigtest
