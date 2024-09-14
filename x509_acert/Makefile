CC = gcc

WOLFSSL_INSTALL_DIR = /usr/local
OPENSSL_INSTALL_DIR = /usr/local

WOLFSSL_CFLAGS = -Werror -Wall -I$(WOLFSSL_INSTALL_DIR)/include -DUSE_WOLFSSL
WOLFSSL_LIBS   = -L$(WOLFSSL_INSTALL_DIR)/lib -lm -lwolfssl

OPENSSL_CFLAGS = -Werror -Wall -I$(OPENSSL_INSTALL_DIR)/include
OPENSSL_LIBS   = -L$(OPENSSL_INSTALL_DIR)/lib64/ -lm -lssl -lcrypto

DEBUG_FLAGS    = -g -DDEBUG

all: wolfssl_acert openssl_acert

wolfssl_acert: test_x509_acert.c
	$(CC) -o $@ $^ $(WOLFSSL_CFLAGS) $(WOLFSSL_LIBS)

openssl_acert: test_x509_acert.c
	$(CC) -o $@ $^ $(OPENSSL_CFLAGS) $(OPENSSL_LIBS)

.PHONY: clean all

clean:
	rm -f *.o wolfssl_acert openssl_acert
