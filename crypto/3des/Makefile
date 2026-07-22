CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR=/usr/local
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

3des-file-encrypt: 3des-file-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

.PHONY: clean check

clean:
	rm -f *.o 3des-file-encrypt

check: 3des-file-encrypt
	printf 'ciphertestpw\n' | script -qec './3des-file-encrypt -e 168 -i in.txt -o out.enc' /dev/null | grep -q 'Success: 3DES encrypt complete'
	@echo "PASS: crypto-3des checks"
