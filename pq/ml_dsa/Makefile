CC = gcc

WOLFSSL_INSTALL_DIR = /usr/local

WOLFSSL_CFLAGS = -Wextra -Werror -Wall -I$(WOLFSSL_INSTALL_DIR)/include
WOLFSSL_LIBS   = -L$(WOLFSSL_INSTALL_DIR)/lib -lm -lwolfssl

DEBUG_FLAGS    = -g -DDEBUG

all: ml_dsa_test

ml_dsa_test: ml_dsa.c
	$(CC) -o $@ $^ $(WOLFSSL_CFLAGS) $(WOLFSSL_LIBS) $(DEBUG_FLAGS)

.PHONY: clean all check

clean:
	rm -f *.o ml_dsa_test

check: ml_dsa_test
	./ml_dsa_test -c 2 | grep -q 'info: verify message good'
	./ml_dsa_test -c 3 | grep -q 'info: verify message good'
	./ml_dsa_test -c 5 -m wolfssl-examples CI | grep -q 'info: verify message good'
	./ml_dsa_test -p | grep -q 'ML-DSA-87'
	@echo "PASS: pq-ml-dsa checks"
