CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR=/usr/local
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm

all: aes-file-encrypt aescfb-file-encrypt aesctr-file-encrypt aesgcm-file-encrypt aesgcm-file-minimal aesgcm-oneshot aesgcm-minimal

aes-file-encrypt: aes-file-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

aescfb-file-encrypt: aescfb-file-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aesctr-file-encrypt: aesctr-file-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aesgcm-file-encrypt: aesgcm-file-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aesgcm-oneshot: aesgcm-oneshot.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aesgcm-minimal: aesgcm-minimal.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aesgcm-file-minimal: aesgcm-file-minimal.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o aes-file-encrypt aescfb-file-encrypt aesctr-file-encrypt aesgcm-file-encrypt aesgcm-file-minimal text* aesgcm-oneshot aesgcm-minimal

check: aesgcm-minimal aesgcm-oneshot aesgcm-file-encrypt
	out=$$(./aesgcm-minimal) && printf '%s' "$$out" | grep -q 'Decrypted: 73696e676c6520626c6f636b206d736700'
	out=$$(./aesgcm-oneshot) && printf '%s' "$$out" | grep -q 'Decrypted: 090909090909090909090909090909090909090909090909090909090909090909'
	out=$$(./aesgcm-file-encrypt -t 256) && printf '%s' "$$out" | grep -q 'Pass: The files are identical.'
	@echo "PASS: crypto-aes checks"
