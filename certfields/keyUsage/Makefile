CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS=-Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

run: test.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o run

check: run
	out=$$(./run) && printf '%s' "$$out" | grep -q 'cert can be used for'
	@echo "PASS: certfields-keyusage checks"
