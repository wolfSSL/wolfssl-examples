CC=gcc
CFLAGS=-Wall
LIBS= -lwolfssl -lm

all: aes-file-encrypt aescfb-file-encrypt aesctr-file-encrypt aesgcm-file-encrypt

aes-file-encrypt: aes-file-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

aescfb-file-encrypt: aescfb-file-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aesctr-file-encrypt: aesctr-file-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aesgcm-file-encrypt: aesgcm-file-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o aes-file-encrypt aescfb-file-encrypt aesctr-file-encrypt aesgcm-file-encrypt text*
