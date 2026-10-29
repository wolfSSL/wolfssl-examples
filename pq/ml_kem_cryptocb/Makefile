CC = gcc

WOLFSSL_INSTALL_DIR = /usr/local

WOLFSSL_CFLAGS = -Wextra -Werror -Wall -I$(WOLFSSL_INSTALL_DIR)/include
WOLFSSL_LIBS   = -L$(WOLFSSL_INSTALL_DIR)/lib -lm -lwolfssl

DEBUG_FLAGS    = -g -DDEBUG

COMMON = ../mlkem-cryptocb.c

all: ml_kem_cryptocb

ml_kem_cryptocb: ml_kem_cryptocb.c $(COMMON)
	$(CC) -o $@ $^ $(WOLFSSL_CFLAGS) $(WOLFSSL_LIBS) $(DEBUG_FLAGS)

.PHONY: clean all check

clean:
	rm -f *.o ml_kem_cryptocb

check: ml_kem_cryptocb
	out=$$(./ml_kem_cryptocb) && printf '%s' "$$out" | grep -q 'via crypto callback'
	@echo "PASS: pq-ml-kem-cryptocb checks"
