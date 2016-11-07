CC=gcc
CFLAGS=-Wall
LIBS=-lwolfssl

all: client-tcp client-tls client-tls-nonblocking client-tls-ecdhe server-tls-ecdhe client-tls-perf client-tls-resume client-callback memory-tls server-tcp server-tls server-tls-nonblocking server-tls-threaded server-tls-epoll-perf server-callback server-tls-epoll-threaded

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

client-tls-perf: client-tls-perf.o
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

server-tls-epoll-perf: server-tls-epoll-perf.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-tls-threaded: server-tls-threaded.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -lpthread

server-callback: server-callback.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-tls-epoll-threaded: server-tls-epoll-threaded.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -lpthread

.PHONY: clean all

clean:
	rm -f *.o client-tcp client-tls client-tls-nonblocking client-tls-ecdhe server-tls-ecdhe client-tls-perf client-tls-resume client-callback memory-tls server-tcp server-tls server-tls-nonblocking server-tls-threaded server-tls-epoll-perf server-callback server-tls-epoll-threaded
