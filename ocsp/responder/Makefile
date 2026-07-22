# Makefile for OCSP Responder Examples
#
# wolfSSL must be built with:
#   ./configure --enable-ocsp --enable-ocsp-responder
#   make && sudo make install

CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS   = -Wall -Wextra -g -I$(WOLFSSL_INSTALL_DIR)/include
LIBS     = -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

# Uncomment for static linking:
# LIBS   = $(WOLFSSL_INSTALL_DIR)/lib/libwolfssl.a -lm -lpthread

SRC      = $(wildcard *.c)
TARGETS  = $(patsubst %.c, %, $(SRC))

all: $(TARGETS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)

.PHONY: check

check: ocsp-request-response
	./ocsp-request-response | grep -q '=== Example complete ==='
	@echo "PASS: ocsp-responder checks"
