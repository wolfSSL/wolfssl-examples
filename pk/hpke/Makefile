CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS= -I$(WOLFSSL_INSTALL_DIR)/include -Wall
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

hpke_test: hpke_test.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o hpke_test

check: hpke_test
	out=$$(./hpke_test) && printf '%s' "$$out" | grep -q 'HPKE test success'
	@echo "PASS: pk-hpke checks"
