CC=gcc
CFLAGS=-Wall
LIBS= -lwolfssl

3des-file-encrypt: 3des-file-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

.PHONY: clean

clean:
	rm -f *.o 3des-file-encrypt
