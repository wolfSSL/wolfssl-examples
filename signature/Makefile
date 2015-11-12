CC=gcc
CFLAGS=-Wall
LIBS= -lwolfssl

all: signature

signature: signature.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o signature
