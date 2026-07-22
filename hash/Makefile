# Hash Examples Makefile
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

# Options
#CFLAGS+=$(DEBUG_FLAGS)
CFLAGS+=$(OPTIMIZE)
#LIBS+=$(STATIC_LIB)
LIBS+=$(DYN_LIB)

# build targets
SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))

.PHONY: clean all check

all: $(TARGETS)

debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

# build template
%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)

check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: sha256-hash sha512-hash sha3-256-hash hash-file sha256-hash-string sha256-hash-oneshot-string sha3-256-hash-oneshot-string
	./sha256-hash input.txt | grep -q '75294625788129796c09fcbf313ea16e2883356e322adc2f956b37dbdc10b6a7'
	./sha512-hash input.txt | grep -q 'ead56209da2dfb3562263aadc57d9382f0f7cb579ebb6dbf2f20bfd3cb68aaaad422f6ce6f1a88ec6c326edcf8456f650579b6e20eb39f3bb444bee8b65615ed'
	./sha3-256-hash input.txt | grep -q '0704c6ca55e7e5c706b543f07da1daed8149c838549096df6a52dac5f95f2fe0'
	./hash-file SHA256 input.txt | grep -q '75294625788129796c09fcbf313ea16e2883356e322adc2f956b37dbdc10b6a7'
	./sha256-hash-string | grep -q 'd4476d30fd94c746eb38d8a1b3931aa81d1e485be5a6362f47598017a91cb5d2'
	./sha256-hash-oneshot-string | grep -q 'd4476d30fd94c746eb38d8a1b3931aa81d1e485be5a6362f47598017a91cb5d2'
	./sha3-256-hash-oneshot-string | grep -q '10d69e59ac10b1d81755733f323bdadca3e04e4b17df72f5b343d6da701a4225'
	@echo "PASS: hash checks"
