# Makefile for OpenSSL examples
CC=gcc
CFLAGS=-Wall -I/usr/include/openssl
LDFLAGS=-lssl -lcrypto

# List of all examples
EXAMPLES=ecc-verify-openssl

# Default target
all: $(EXAMPLES)

# Rule for building examples
ecc-verify-openssl: ecc-verify-openssl.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

# Clean target
clean:
	rm -f $(EXAMPLES)

.PHONY: all clean
