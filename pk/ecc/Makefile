CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS=-I$(WOLFSSL_INSTALL_DIR)/include -Wall
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))

all: $(TARGETS)

%: %.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f $(TARGETS) *.der *.x963 

check: ecc_keys ecc_pub ecc_sign ecc_sign_deterministic ecc_verify
	out=$$(./ecc_keys) && printf '%s' "$$out" | grep -q 'storing public key into ecc-public.x963 (65 bytes)'
	out=$$(./ecc_pub) && printf '%s' "$$out" | grep -q 'Public Key Qy: 32'
	out=$$(./ecc_sign) && printf '%s' "$$out" | grep -q 'Success'
	out=$$(./ecc_sign_deterministic) && printf '%s' "$$out" | grep -q 'Success'
	out=$$(./ecc_verify) && printf '%s' "$$out" | grep -q 'Success'
	@echo "PASS: pk-ecc checks"
