CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR=/usr/local
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm
GCM_KEY=77CF00EC060192530B5D06B6B426799B
ALIAS_MSG=input and output must be different files

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
	printf 'in place plaintext' > text.inplace && cp text.inplace text.inplace.orig && ln -f text.inplace text.inplace.lnk && ln -sf text.inplace text.inplace.sym
	out=$$(timeout 10 ./aesgcm-file-encrypt -e 256 -m 1 -k $(GCM_KEY) -v $(GCM_KEY) -i text.inplace -o text.inplace 2>&1); test $$? -eq 1 && printf '%s' "$$out" | grep -q '$(ALIAS_MSG)' && cmp text.inplace text.inplace.orig
	out=$$(timeout 10 ./aesgcm-file-encrypt -e 256 -m 1 -k $(GCM_KEY) -v $(GCM_KEY) -i text.inplace -o ./text.inplace 2>&1); test $$? -eq 1 && printf '%s' "$$out" | grep -q '$(ALIAS_MSG)' && cmp text.inplace text.inplace.orig
	out=$$(timeout 10 ./aesgcm-file-encrypt -e 256 -m 1 -k $(GCM_KEY) -v $(GCM_KEY) -i text.inplace -o text.inplace.lnk 2>&1); test $$? -eq 1 && printf '%s' "$$out" | grep -q '$(ALIAS_MSG)' && cmp text.inplace text.inplace.orig
	out=$$(timeout 10 ./aesgcm-file-encrypt -e 256 -m 1 -k $(GCM_KEY) -v $(GCM_KEY) -i text.inplace -o text.inplace.sym 2>&1); test $$? -eq 1 && printf '%s' "$$out" | grep -q '$(ALIAS_MSG)' && cmp text.inplace text.inplace.orig
	./aesgcm-file-encrypt -e 256 -m 1 -k $(GCM_KEY) -v $(GCM_KEY) -i text.inplace -o text.inplace.enc && cp text.inplace.enc text.inplace.enc.orig
	out=$$(timeout 10 ./aesgcm-file-encrypt -d 256 -m 1 -k $(GCM_KEY) -i text.inplace.enc -o text.inplace.enc 2>&1); test $$? -eq 1 && printf '%s' "$$out" | grep -q '$(ALIAS_MSG)' && cmp text.inplace.enc text.inplace.enc.orig
	out=$$(./aesgcm-file-encrypt -d 256 -m 1 -k X$(GCM_KEY) -i text.inplace.enc -o text.inplace.bad); test $$? -eq 1 && test ! -e text.inplace.bad && ! printf '%s' "$$out" | grep -q 'complete'
	@echo "PASS: crypto-aes checks"
