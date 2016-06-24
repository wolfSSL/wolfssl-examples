CC=gcc
CFLAGS=-Wall
LIBS=-lwolfssl

all: client-tcp client-tls client-tls-nonblocking client-tls-ecdhe server-tls-ecdhe client-tls-resume client-callback memory-tls server-tcp server-tls server-tls-nonblocking server-tls-threaded server-callback

client-tcp: client-tcp.o
	$(CC) -o $@ $^ $(CFLAGS)

client-tls: client-tls.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-tls-nonblocking: client-tls-nonblocking.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-tls-ecdhe: client-tls-ecdhe.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-tls-ecdhe: server-tls-ecdhe.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-tls-resume: client-tls-resume.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-callback: client-callback.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

memory-tls: memory-tls.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -lpthread

server-tcp: server-tcp.o
	$(CC) -o $@ $^ $(CFLAGS)

server-tls: server-tls.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-tls-nonblocking: server-tls-nonblocking.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-tls-threaded: server-tls-threaded.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -lpthread

server-callback: server-callback.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o client-tcp client-tls client-tls-nonblocking client-tls-ecdhe server-tls-ecdhe client-tls-resume client-callback memory-tls server-tcp server-tls server-tls-nonblocking server-tls-threaded server-callback
