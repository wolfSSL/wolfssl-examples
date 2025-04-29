# Makefile for ECC signature verification comparison examples
CC=gcc
OPENSSL_CFLAGS=-Wall -I/usr/include/openssl
OPENSSL_LDFLAGS=-lssl -lcrypto
WOLFSSL_CFLAGS=-Wall -I/home/ubuntu/repos/wolfssl/
WOLFSSL_LDFLAGS=-L/home/ubuntu/repos/wolfssl/src/.libs -lwolfssl -lm

# Default target
all: ecc-verify-openssl-low ecc-verify-wolfssl

# Rule for building OpenSSL low-level example
ecc-verify-openssl-low: ecc-verify-openssl-low.c
	$(CC) -o $@ $< $(OPENSSL_CFLAGS) $(OPENSSL_LDFLAGS)

# Rule for building wolfSSL example
ecc-verify-wolfssl: ecc-verify-wolfssl.c
	$(CC) -o $@ $< $(WOLFSSL_CFLAGS) $(WOLFSSL_LDFLAGS)

# Clean target
clean:
	rm -f ecc-verify-openssl-low ecc-verify-wolfssl

.PHONY: all clean
