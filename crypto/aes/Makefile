CC=gcc
CFLAGS=-Wall
LIBS= -lwolfssl

aes-file-encrypt: aes-file-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

.PHONY: clean

clean:
	rm -f *.o aes-file-encrypt
