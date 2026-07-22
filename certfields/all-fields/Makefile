CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local

CFLAGS=-I$(WOLFSSL_INSTALL_DIR)/include -Wall
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

app: main.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o app

check: app
	./app ../../certs/client-cert.der RSA | grep -q 'CN = www.wolfssl.com (15)'
	! ./app ../../certs/client-key.pem RSA >/dev/null 2>&1
	@echo "PASS: certfields-all-fields checks"
