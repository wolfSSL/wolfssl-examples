CC=gcc
CFLAGS=-Wall -I../../wolfssl -g -DDEBUG
LIBS=-lwolfssl -lm -L../../wolfssl/src/.libs

# For debug (./configure --enable-debug --disable-shared)
#LIBS=/usr/local/lib/libwolfssl.a
#CFLAGS+=-g -DDEBUG


all: client-tcp client-psk client-psk-nonblocking client-psk-resume client-psk-tls13-multi-id server-tcp server-psk server-psk-nonblocking server-psk-threaded client-psk-bio-custom server-psk-tls13-multi-id

client-tcp: client-tcp.o
	$(CC) -o $@ $^ $(CFLAGS)

client-psk: client-psk.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-psk-nonblocking: client-psk-nonblocking.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-psk-resume: client-psk-resume.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-psk-tls13-multi-id: client-psk-tls13-multi-id.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-tcp: server-tcp.o
	$(CC) -o $@ $^ $(CFLAGS)

server-psk: server-psk.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-psk-nonblocking: server-psk-nonblocking.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-psk-threaded: server-psk-threaded.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -lpthread

client-psk-bio-custom: client-psk-bio-custom.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-psk-tls13-multi-id: server-psk-tls13-multi-id.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o client-tcp client-psk client-psk-nonblocking client-psk-resume client-psk-tls13-multi-id server-tcp server-psk server-psk-nonblocking server-psk-threaded client-psk-bio-custom server-psk-tls13-multi-id
