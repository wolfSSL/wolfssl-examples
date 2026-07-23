CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS= -I$(WOLFSSL_INSTALL_DIR)/include -Wall
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm

all: rsa-kg-sv rsa-kg

rsa-kg-sv.o: rsa-kg-sv.c rsa-key.h
	$(CC) -c -o $@ $< $(CFLAGS)

rsa-kg-sv: rsa-kg-sv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

rsa-kg.o: rsa-kg.c
	$(CC) -c -o $@ $< $(CFLAGS)

rsa-kg: rsa-kg.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o rsa-kg-sv rsa-kg rsa-public.der rsa-private.der

# Both binaries now return nonzero on any failure, so the exit code is the check
# -- no grep of a line that is printed before the write can succeed or fail.
check: rsa-kg-sv rsa-kg
	./rsa-kg-sv -load-key >/dev/null
	./rsa-kg -bits 2048 >/dev/null
	@echo "PASS: pk-rsa-kg checks"
