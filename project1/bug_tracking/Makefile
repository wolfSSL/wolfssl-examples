DEPS = ../include/unp.h
CC=gcc
CFLAGS=-Wall -I ../include -DCYASSL_DTLS
OBJ = echoserver.o

#if you are on a sunOS (System V) machine, you'll need to uncomment
#the next line.

#LIBS=-lsocket
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
all: server

server: server-dtls.c
		$(CC) -Wall -DCYASSL_DTLS -o server server-dtls.c -I ../include -lcyassl


.PHONY: clean

clean:
		-rm -f *.o server
