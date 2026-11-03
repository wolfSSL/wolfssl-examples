# Mbed TLS Compatibility Example Makefile
CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
# wolfPSA built with MBEDTLS_COMPAT=1
WOLFPSA_DIR = ../../wolfpsa
CFLAGS   = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS     = -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm
# The client uses only wolfPSA, so <mbedtls/ssl.h> resolves to wolfPSA
PSA_CFLAGS = -Wall -I$(WOLFPSA_DIR)/wolfpsa -I$(WOLFPSA_DIR)/build/include
PSA_LIBS   = $(WOLFPSA_DIR)/libwolfpsa.a -lm

TARGETS = client-tls-mbedtls
# ../tls/server-tls13 requires a client certificate, which this client does
# not send, so make check runs ../tls/server-tls built for each version
CHECK_SERVERS = check-server-tls12 check-server-tls13

all: $(TARGETS)

$(WOLFPSA_DIR)/libwolfpsa.a:
	@echo "$@ not found, build wolfPSA with make MBEDTLS_COMPAT=1"
	@false

%: %.c $(WOLFPSA_DIR)/libwolfpsa.a
	$(CC) -o $@ $< $(PSA_CFLAGS) $(PSA_LIBS)

check-server-tls12: ../tls/server-tls.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

check-server-tls13: ../tls/server-tls.c
	$(CC) -o $@ $< -DUSE_TLSV13 $(CFLAGS) $(LIBS)

check: $(TARGETS) $(CHECK_SERVERS)
	./check.sh

clean:
	rm -f $(TARGETS) $(CHECK_SERVERS)

.PHONY: all check clean
