# DTLS Multicast Examples Makefile
CC       = gcc

# Path to wolfSSL source directory (adjust as needed)
WOLFSSL_DIR = ../../wolfssl

CFLAGS   = -Wall -Wextra -I$(WOLFSSL_DIR) -I$(WOLFSSL_DIR)/wolfssl
LIBS     = $(WOLFSSL_DIR)/src/.libs/libwolfssl.a -lm -lpthread

# option variables
DEBUG_FLAGS     = -g3 -DDEBUG -O0
OPTIMIZE        = -Os

# Options - uncomment DEBUG_FLAGS for debugging
#CFLAGS+=$(DEBUG_FLAGS)
CFLAGS+=$(OPTIMIZE)

# build targets
TARGETS = mcast-peer

.PHONY: clean all debug

all: $(TARGETS)

debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

mcast-peer: mcast-peer.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)

# Helper targets for running the example
run-node0:
	./mcast-peer 0

run-node1:
	./mcast-peer 1

run-node2:
	./mcast-peer 2
