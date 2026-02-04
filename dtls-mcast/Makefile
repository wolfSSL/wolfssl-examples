# DTLS Multicast Examples Makefile
CC       = gcc

# build targets
TARGETS = mcast-peer

all: $(TARGETS)

mcast-peer: mcast-peer.c
	$(CC) -o $@ $< -L/usr/local/lib -lwolfssl

clean:
	rm -f $(TARGETS)

# Helper targets for running the example
run-node0:
	./mcast-peer 0

run-node1:
	./mcast-peer 1

run-node2:
	./mcast-peer 2
