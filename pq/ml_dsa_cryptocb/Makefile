CC = gcc

WOLFSSL_INSTALL_DIR = /usr/local

WOLFSSL_CFLAGS = -Wextra -Werror -Wall -I$(WOLFSSL_INSTALL_DIR)/include
WOLFSSL_LIBS   = -L$(WOLFSSL_INSTALL_DIR)/lib -lm -lwolfssl

DEBUG_FLAGS    = -g -DDEBUG

all: ml_dsa_cryptocb

ml_dsa_cryptocb: ml_dsa_cryptocb.c
	$(CC) -o $@ $^ $(WOLFSSL_CFLAGS) $(WOLFSSL_LIBS) $(DEBUG_FLAGS)

.PHONY: clean all check

clean:
	rm -f *.o ml_dsa_cryptocb

check: ml_dsa_cryptocb
	out=$$(./ml_dsa_cryptocb) && printf '%s' "$$out" | grep -q 'via crypto callback'
	@echo "PASS: pq-ml-dsa-cryptocb checks"
