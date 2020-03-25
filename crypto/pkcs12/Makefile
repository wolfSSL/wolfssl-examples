CC=gcc
CFLAGS=-Wall
LIBS= -lwolfssl

all:pkcs12-example pkcs12-create-example

pkcs12-example: pkcs12-example.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

pkcs12-create-example: pkcs12-create-example.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o pkcs12-example pkcs12-create-example output.p12
