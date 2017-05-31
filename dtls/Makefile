CC=clang
CFLAGS=-DWOLFSSL_DTLS
LIBS=-lwolfssl -lpthread

all: client-udp client-dtls client-dtls-nonblocking client-dtls-resume client-dtls-shared server-udp server-dtls server-dtls-nonblocking server-dtls-threaded

client-udp: client-udp.o
	$(CC) -o $@ $^ $(CFLAGS)

client-dtls: client-dtls.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-dtls-nonblocking: client-dtls-nonblocking.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-dtls-resume: client-dtls-resume.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-dtls-shared: client-dtls-shared.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-udp: server-udp.o
	$(CC) -o $@ $^ $(CFLAGS)

server-dtls: server-dtls.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-dtls-nonblocking: server-dtls-nonblocking.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-dtls-threaded: server-dtls-threaded.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o client-udp client-dtls client-dtls-nonblocking client-dtls-resume client-dtls-shared server-udp server-dtls server-dtls-nonblocking server-dtls-threaded
