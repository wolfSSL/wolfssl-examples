all:	server-udp
all: 	server-dtls
	

#if you are on a sunOS (System V) machine, you'll need to uncomment
#the next line.

#LIBS=-lsocket

server-udp:	server-udp.c
	$(CC) -o server-udp server-udp.c $(LIBS)	

server-dtls:	server-dtls.c
	$(CC) -o server-dtls server-dtls.c $(LIBS) -lcyassl

.PHONY: clean

clean:
		-rm -f *.o server-udp server-dtls
