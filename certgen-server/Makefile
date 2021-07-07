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

CFLAGS=-Wall
LIBS=-lwolfssl


all: server

server:test.o server-tls.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o test.o server-tls.o server created_cert.der created_chain.pem created_key.der
