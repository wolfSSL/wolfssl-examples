CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS=-Wall -I$(WOLFSSL_INSTALL_DIR)/include -g
#LIBS= -lwolfssl -lm
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm

all: srp srp_gen

srp.o: srp.c srp_params.h srp_store.h
	$(CC) -c -o $@ srp.c $(CFLAGS)

srp_gen.o: srp_gen.c srp_params.h
	$(CC) -c -o $@ srp_gen.c $(CFLAGS)

srp: srp.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

srp_gen: srp_gen.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.der *.x963 *.o srp srp_gen

check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: srp srp_gen
	./srp wolfssl password | grep -q 'Client verified server proof'
	./srp_gen wolfssl password | grep -q 'static const byte verifier['
	@echo "PASS: pk-srp checks"
