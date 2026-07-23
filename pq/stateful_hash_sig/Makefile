# PQ Stateful Hash-Based Signature Examples Makefile
#
CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS   = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS     = -L$(WOLFSSL_INSTALL_DIR)/lib -lm

# option variables
WOLF_DYN_LIB    = -lwolfssl

# Options
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os
#CFLAGS+=$(DEBUG_FLAGS)
#CFLAGS+=$(OPTIMIZE)

# build targets
SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))

.PHONY: clean all check

all: $(TARGETS)

debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

# build template
# Requires wolfSSL built with --enable-lms:
lms_example: lms_example.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS) $(WOLF_DYN_LIB)

# Requires wolfSSL built with --enable-xmss:
xmss_example: xmss_example.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS) $(WOLF_DYN_LIB)

clean:
	rm -f $(TARGETS)
	rm -f lms_example.key
	rm -f xmss_example.key

check: lms_example xmss_example
	out=$$(./lms_example 1 5 1) && printf '%s' "$$out" | grep -q 'finished'
	out=$$(./xmss_example XMSS-SHA2_10_256 10) && printf '%s' "$$out" | grep -q 'finished'
	@echo "PASS: pq-stateful-hash-sig checks"
