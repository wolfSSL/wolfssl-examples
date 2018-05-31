CC=gcc
CFLAGS=-Wall
LIBS=-lwolfssl

all:eccsign

eccsign:eccsign.o
	$(CC) -o $@ $(LIBS) $^ $(CFLAGS)

.PHONY: clean all

clean:
	rm -f *.o eccsign
