CC=gcc
#if you installed wolfssl to an alternate location use CFLAGS and LIBS to
#control your build:

#EXAMPLE: set WOLF_INSTALL_DIR to point to your install location like so:
# WOLF_INSTALL_DIR=/Users/username/work/testDir/wolf-install-dir-for-testing
#END EXAMPLE

WOLF_INSTALL_DIR=/usr/local

# ECC Examples Makefile
CC       = gcc
LIB_PATH = /usr/local
CFLAGS   = -Wall -I$(LIB_PATH)/include
LIBS     = -L$(LIB_PATH)/lib -lm

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(LIB_PATH)/lib/libwolfssl.a
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os

# Options you can enable/disable.
CFLAGS+=$(DEBUG_FLAGS)
#CFLAGS+=$(OPTIMIZE)
#LIBS+=$(STATIC_LIB)
LIBS+=$(DYN_LIB)

all: gen_dual_keysig_root_cert gen_dual_keysig_server_cert gen_rsa_dilithium_dual_keysig_root_cert gen_rsa_dilithium_dual_keysig_server_cert gen_rsa_falcon_dual_keysig_root_cert gen_rsa_falcon_dual_keysig_server_cert gen_ecdsa_dilithium_dual_keysig_root_cert gen_ecdsa_dilithium_dual_keysig_server_cert gen_ecdsa_falcon_dual_keysig_root_cert gen_ecdsa_falcon_dual_keysig_server_cert


gen_dual_keysig_root_cert: gen_dual_keysig_cert.c
	$(CC) -o $@ gen_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_ROOT_CERT

gen_dual_keysig_server_cert: gen_dual_keysig_cert.c
	$(CC) -o $@ gen_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_SERVER_CERT

gen_rsa_dilithium_dual_keysig_root_cert: gen_rsa_dilithium_dual_keysig_cert.c
	$(CC) -o $@ gen_rsa_dilithium_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_ROOT_CERT

gen_rsa_dilithium_dual_keysig_server_cert: gen_rsa_dilithium_dual_keysig_cert.c
	$(CC) -o $@ gen_rsa_dilithium_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_SERVER_CERT

gen_rsa_falcon_dual_keysig_root_cert: gen_rsa_falcon_dual_keysig_cert.c
	$(CC) -o $@ gen_rsa_falcon_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_ROOT_CERT

gen_rsa_falcon_dual_keysig_server_cert: gen_rsa_falcon_dual_keysig_cert.c
	$(CC) -o $@ gen_rsa_falcon_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_SERVER_CERT

gen_ecdsa_dilithium_dual_keysig_root_cert: gen_ecdsa_dilithium_dual_keysig_cert.c
	$(CC) -o $@ gen_ecdsa_dilithium_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_ROOT_CERT

gen_ecdsa_dilithium_dual_keysig_server_cert: gen_ecdsa_dilithium_dual_keysig_cert.c
	$(CC) -o $@ gen_ecdsa_dilithium_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_SERVER_CERT

gen_ecdsa_falcon_dual_keysig_root_cert: gen_ecdsa_falcon_dual_keysig_cert.c
	$(CC) -o $@ gen_ecdsa_falcon_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_ROOT_CERT

gen_ecdsa_falcon_dual_keysig_server_cert: gen_ecdsa_falcon_dual_keysig_cert.c
	$(CC) -o $@ gen_ecdsa_falcon_dual_keysig_cert.c $(CFLAGS) $(CPPFLAGS) $(LIBS) -DGEN_SERVER_CERT

.PHONY: clean all

clean:
	rm -f gen_*_root_cert
	rm -f gen_*_server_cert
	rm -f *.der
	rm -f *.pem
