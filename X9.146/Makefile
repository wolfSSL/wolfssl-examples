# X9.146 Examples Makefile
#if you installed wolfssl to an alternate location use CFLAGS and LIBS to
#control your build:

#EXAMPLE: set WOLFSSL_INSTALL_DIR to point to your install location like so:
# WOLFSSL_INSTALL_DIR=/Users/username/work/testDir/wolf-install-dir-for-testing
#END EXAMPLE

WOLFSSL_INSTALL_DIR=/usr/local

# ECC Examples Makefile
CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS   = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS     = -L$(WOLFSSL_INSTALL_DIR)/lib -lm

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(WOLFSSL_INSTALL_DIR)/lib/libwolfssl.a
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os

# Options you can enable/disable.
CFLAGS+=$(DEBUG_FLAGS)
#CFLAGS+=$(OPTIMIZE)
#LIBS+=$(STATIC_LIB)
LIBS+=$(DYN_LIB)

all: gen_dual_keysig_root_cert gen_dual_keysig_server_cert gen_rsa_mldsa_dual_keysig_root_cert gen_rsa_mldsa_dual_keysig_server_cert gen_ecdsa_mldsa_dual_keysig_root_cert gen_ecdsa_mldsa_dual_keysig_server_cert


gen_dual_keysig_root_cert: gen_dual_keysig_cert.c
	$(CC) -o $@ gen_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_ROOT_CERT

gen_dual_keysig_server_cert: gen_dual_keysig_cert.c
	$(CC) -o $@ gen_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_SERVER_CERT

gen_rsa_mldsa_dual_keysig_root_cert: gen_rsa_mldsa_dual_keysig_cert.c
	$(CC) -o $@ gen_rsa_mldsa_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_ROOT_CERT

gen_rsa_mldsa_dual_keysig_server_cert: gen_rsa_mldsa_dual_keysig_cert.c
	$(CC) -o $@ gen_rsa_mldsa_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_SERVER_CERT

gen_ecdsa_mldsa_dual_keysig_root_cert: gen_ecdsa_mldsa_dual_keysig_cert.c
	$(CC) -o $@ gen_ecdsa_mldsa_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_ROOT_CERT

gen_ecdsa_mldsa_dual_keysig_server_cert: gen_ecdsa_mldsa_dual_keysig_cert.c
	$(CC) -o $@ gen_ecdsa_mldsa_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_SERVER_CERT

.PHONY: clean all check

clean:
	rm -f gen_*_root_cert
	rm -f gen_*_server_cert
	rm -f *.der
	rm -f *.pem

check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: gen_rsa_mldsa_dual_keysig_root_cert gen_rsa_mldsa_dual_keysig_server_cert
	openssl genpkey -algorithm rsa -pkeyopt rsa_keygen_bits:3072 -out ca-key.der -outform der
	openssl genpkey -algorithm rsa -pkeyopt rsa_keygen_bits:3072 -out server-key.der -outform der
	./gen_rsa_mldsa_dual_keysig_root_cert | grep -q 'SUCCESS!'
	./gen_rsa_mldsa_dual_keysig_server_cert | grep -q 'SUCCESS!'
	@echo "PASS: x9146 checks"
