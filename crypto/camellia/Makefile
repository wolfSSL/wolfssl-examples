CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR=/usr/local
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

camellia-encrypt: camellia-encrypt.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

.PHONY: clean check

clean:
	rm -f *.o camellia-encrypt

check: camellia-encrypt
	printf 'camellia ci input\n' > in.txt
	out=$$(printf 'ciphertestpw\n' | script -qec './camellia-encrypt -e 128 -i in.txt -o out.enc' /dev/null) && printf '%s' "$$out" | grep -q 'Success: Camellia encrypt complete'
	@echo "PASS: crypto-camellia checks"
