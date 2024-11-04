CC = gcc

WOLFSSL_INSTALL_DIR = /usr/local

WOLFSSL_CFLAGS = -Wextra -Werror -Wall -I$(WOLFSSL_INSTALL_DIR)/include
WOLFSSL_LIBS   = -L$(WOLFSSL_INSTALL_DIR)/lib -lm -lwolfssl

DEBUG_FLAGS    = -g -DDEBUG

all: ml_dsa_test

ml_dsa_test: ml_dsa.c
	$(CC) -o $@ $^ $(WOLFSSL_CFLAGS) $(WOLFSSL_LIBS) $(DEBUG_FLAGS)

.PHONY: clean all

clean:
	rm -f *.o wolfssl_acert ml_dsa_test
