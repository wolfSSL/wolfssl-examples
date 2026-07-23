CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS=-I$(WOLFSSL_INSTALL_DIR)/include -Wall
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm

SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))

all: $(TARGETS)

%: %.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f $(TARGETS) 

check: sign_and_verify
	out=$$(./sign_and_verify) && printf '%s' "$$out" | grep -q 'Successfully validated signature'
	@echo "PASS: pk-ed25519-gen checks"
