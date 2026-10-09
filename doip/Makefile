# DoIP over TLS Examples Makefile
#
#   make
#   make check                          build, then run the pair on loopback
#   make WOLFSSL_INSTALL_DIR=/opt/wolfssl
#
# Nothing unusual is needed from wolfSSL: TLS 1.3 with ECDSA is on by default.
#
CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS   = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS     = -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

OPTIMIZE    = -Os
DEBUG_FLAGS = -g -DDEBUG

CFLAGS += $(OPTIMIZE)

# ISO 13400-2:2012 is 0x02, :2019 is 0x03. 0x03 to match the TLS port these
# examples use, which the 2019 edition is what added; both ends have to agree,
# the version being part of the synchronisation pattern. Override to match
# your target.
DOIP_VERSION ?= 0x03
CFLAGS += -DDOIP_PROTOCOL_VERSION=$(DOIP_VERSION)

# 3496 is the registered TCP_DATA_TLS port. The check target uses a high port
# instead so a run cannot collide with a real DoIP entity on the machine --
# both are above 1024, so neither needs privilege.
CHECK_PORT ?= 13496

TARGETS = doip-gateway doip-tester

.PHONY: all debug clean check

all: $(TARGETS)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

doip-gateway: doip-gateway.c doip.c doip.h
	$(CC) -o $@ doip-gateway.c doip.c $(CFLAGS) $(LIBS)

doip-tester: doip-tester.c doip.c doip.h
	$(CC) -o $@ doip-tester.c doip.c $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS) *.o *.log

# Runs the gateway in the background, then the tester against it. Two passes:
# an authenticated tester reads the VIN, and one with no certificate is
# refused. Both outcomes are expected, so both count as a pass.
check: $(TARGETS)
	@echo "--- header parser selftest ---"
	@out=$$(./doip-tester --selftest) && echo "$$out" && \
	  printf '%s' "$$out" | grep -q 'parser selftest'
	@echo "--- authenticated tester ---"
	@./doip-gateway $(CHECK_PORT) > gw.log 2>&1 & \
	 gw=$$!; sleep 1; \
	 out=$$(./doip-tester $(CHECK_PORT) 2>&1); \
	 echo "$$out"; \
	 kill $$gw 2>/dev/null; wait $$gw 2>/dev/null; \
	 printf '%s' "$$out" | grep -q 'doip-tester: PASS' || \
	   { echo "--- gw.log (the gateway's side) ---"; cat gw.log; false; }
	@echo "--- authenticated tester denied at routing activation ---"
	@./doip-gateway $(CHECK_PORT) --deny-auth > gw-denyauth.log 2>&1 & \
	 gw=$$!; sleep 1; \
	 out=$$(./doip-tester $(CHECK_PORT) 2>&1); \
	 echo "$$out"; \
	 kill $$gw 2>/dev/null; wait $$gw 2>/dev/null; \
	 printf '%s' "$$out" | grep -q 'REFUSED (missing authentication)' || \
	   { echo "--- gw-denyauth.log (the gateway's side) ---"; cat gw-denyauth.log; false; }
	@echo "--- tester with no certificate ---"
	@./doip-gateway $(CHECK_PORT) > gw-nocert.log 2>&1 & \
	 gw=$$!; sleep 1; \
	 out=$$(./doip-tester $(CHECK_PORT) --no-cert 2>&1); \
	 echo "$$out"; \
	 kill $$gw 2>/dev/null; wait $$gw 2>/dev/null; \
	 printf '%s' "$$out" | grep -q 'REFUSED by the peer after the handshake' || \
	   { echo "--- gw-nocert.log (the gateway's side) ---"; cat gw-nocert.log; false; }
	@echo "--- malformed generic header ---"
	@./doip-gateway $(CHECK_PORT) > gw-malformed.log 2>&1 & \
	 gw=$$!; sleep 1; \
	 out=$$(./doip-tester $(CHECK_PORT) --malformed 2>&1); \
	 echo "$$out"; \
	 kill $$gw 2>/dev/null; wait $$gw 2>/dev/null; \
	 printf '%s' "$$out" | grep -q 'malformed header rejected' || \
	   { echo "--- gw-malformed.log (the gateway's side) ---"; cat gw-malformed.log; false; }
	@echo "PASS: doip checks"
