CC = gcc

WOLFSSL_INSTALL_DIR = /usr/local

WOLFSSL_CFLAGS = -Wextra -Werror -Wall -I$(WOLFSSL_INSTALL_DIR)/include
WOLFSSL_LIBS   = -L$(WOLFSSL_INSTALL_DIR)/lib -lm -lwolfssl

DEBUG_FLAGS    = -g -DDEBUG

all: ml_kem

ml_kem: ml_kem.c
	$(CC) -o $@ $^ $(WOLFSSL_CFLAGS) $(WOLFSSL_LIBS) $(DEBUG_FLAGS)

.PHONY: clean all check

clean:
	rm -f *.o ml_kem

check: ml_kem
	out=$$(./ml_kem) && printf '%s' "$$out" | grep -q 'Shared secrets match'
	@echo "PASS: pq-ml-kem checks"
