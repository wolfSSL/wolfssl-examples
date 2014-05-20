DEPS = ../include/unp.h
CC=gcc
CFLAGS=-Wall -I ../include
OBJ = echoserver.o

#if you are on a sunOS (System V) machine, you'll need to uncomment
#the next line.

#LIBS=-lsocket
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

%.0: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server-udp: server-udp.c
	$(CC) -Wall -o server-udp server-udp.c -I ../include -lcyassl	

server-dtls: server-dtls.c
	$(CC) -Wall -o server-dtls server-dtls.c -I ../include -lcyassl

.PHONY: clean

clean:
		-rm -f *.o server-udp server-dtls
