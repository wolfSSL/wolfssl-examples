CC=gcc
CFLAGS=-Wall
LIBS= -lwolfssl

camellia-encrypt: camellia-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

.PHONY: clean

clean:
	rm -f *.o camellia-encrypt
