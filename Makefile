all:	server-udp

#if you are on a sunOS (System V) machine, you'll need to uncomment
#the next line.

#LIBS=-lsocket

server-udp:	server-udp.c
	$(CC) -o server-udp server-udp.c $(LIBS)

.PHONY: clean

clean:
		-rm -f *.o server-udp
