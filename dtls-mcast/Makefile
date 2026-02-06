# DTLS Multicast Examples Makefile
CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS   = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS     = -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm

# build targets
TARGETS = mcast-peer

all: $(TARGETS)

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
