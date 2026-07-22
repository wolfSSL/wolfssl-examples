CC = gcc

WOLFSSL_INSTALL_DIR = /usr/local

WOLFSSL_CFLAGS = -Werror -Wall -I$(WOLFSSL_INSTALL_DIR)/include -DUSE_WOLFSSL
WOLFSSL_LIBS   = -L$(WOLFSSL_INSTALL_DIR)/lib -lm -lwolfssl

# empty = system OpenSSL; pointing this at /usr/local would shadow openssl/*.h with wolfSSL's opensslextra compat headers
OPENSSL_INSTALL_DIR ?=
ifneq ($(OPENSSL_INSTALL_DIR),)
OPENSSL_CFLAGS = -Werror -Wall -I$(OPENSSL_INSTALL_DIR)/include
OPENSSL_LIBS   = -L$(OPENSSL_INSTALL_DIR)/lib -lm -lssl -lcrypto
else
OPENSSL_CFLAGS = -Werror -Wall
OPENSSL_LIBS   = -lm -lssl -lcrypto
endif

DEBUG_FLAGS    = -g -DDEBUG

all: wolfssl_acert openssl_acert

wolfssl_acert: test_x509_acert.c
	$(CC) -o $@ $^ $(WOLFSSL_CFLAGS) $(WOLFSSL_LIBS)

openssl_acert: test_x509_acert.c
	$(CC) -o $@ $^ $(OPENSSL_CFLAGS) $(OPENSSL_LIBS)

.PHONY: clean all check

clean:
	rm -f *.o wolfssl_acert openssl_acert

check: wolfssl_acert
	./wolfssl_acert -f certs/acert.pem -k certs/acert_pubkey.pem | grep -q 'info: acert_do_test: good'
	./wolfssl_acert -f certs/acert_ietf.pem -k certs/acert_ietf_pubkey.pem | grep -q 'info: acert_do_test: good'
	@echo "PASS: x509-acert checks"
