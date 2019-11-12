CC=gcc
CFLAGS=-Wall
LIBS= -lwolfssl -lm

all: dh-pg-ka

dh-pg-ka.o: dh-pg-ka.c dh-params.h
	$(CC) -c -o $@ $< $(CFLAGS)

dh-pg-ka: dh-pg-ka.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

.PHONY: clean

clean:
	rm -f *.o dh-pg-ka
