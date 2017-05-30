CC=gcc
#if you installed wolfssl to an alternate location use CFLAGS and LIBS to
#control your build:
#CFLAGS=-Wall -I/path/to/include
#LIBS=-L/path/to/lib -lwolfssl
#
#EXAMPLE:
#CFLAGS=-Wall -I/Users/khimes/work/testDir/wolf-install-dir-for-testing/include
#LIBS=-L/Users/khimes/work/testDir/wolf-install-dir-for-testing/lib -lwolfssl
#END EXAMPLE
#
# To use in place with wolfSSL repository:
#CFLAGS=-Wall -I../../wolfssl/ -g
#LIBS=-L../../wolfssl/src/.libs -lwolfssl -lm

CFLAGS=-Wall
LIBS=-lwolfssl



all: ed25519_cert

ed25519_cert: ed25519_cert.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o ed25519_cert *.der *.pem
