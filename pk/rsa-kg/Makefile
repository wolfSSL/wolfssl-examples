CC=gcc
CFLAGS=-Wall
LIBS= -lwolfssl -lm

all: rsa-kg-sv rsa-kg

rsa-kg-sv.o: rsa-kg-sv.c rsa-key.h
	$(CC) -c -o $@ $< $(CFLAGS)

rsa-kg-sv: rsa-kg-sv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

rsa-kg.o: rsa-kg.c
	$(CC) -c -o $@ $< $(CFLAGS)

rsa-kg: rsa-kg.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o rsa-kg-sv rsa-kg
