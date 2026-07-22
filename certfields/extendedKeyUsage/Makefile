CC=gcc -fsanitize=address
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS=-Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

run: test.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o run

check: run
	ASAN_OPTIONS=detect_leaks=0 ./run | grep -q 'extKeyUsage OID:'
	@echo "PASS: certfields-extendedkeyusage checks"
