CC=gcc
CFLAGS=-Wall
LIBS= -lwolfssl

pkcs12-example: pkcs12-example.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o pkcs12-example
