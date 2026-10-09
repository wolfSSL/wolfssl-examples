# ISO 15118 (-20) TLS Examples Makefile
#
#   ./generate_v2g_certs.sh     build the V2G hierarchy first
#   make
#   make check                  run the pair, plus the refused case
#   make WOLFSSL_INSTALL_DIR=/opt/wolfssl
#
# The TLS examples need nothing unusual from wolfSSL: TLS 1.3 with ECDSA on
# secp256r1 is on by default. contract-install additionally needs
# --enable-x963kdf, and reports SKIP without it.
#
# See README.md for the option worth setting on the LIBRARY build,
# MAX_CHAIN_DEPTH -- it cannot be set from here.
#
CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS   = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS     = -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

OPTIMIZE    = -Os
DEBUG_FLAGS = -g -DDEBUG

CFLAGS += $(OPTIMIZE)

CHECK_PORT ?= 15118

TARGETS = secc-server evcc-client v2g-signature contract-install

.PHONY: all debug clean distclean check certs

all: $(TARGETS)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

certs:
	./generate_v2g_certs.sh

clean:
	rm -f $(TARGETS) *.o *.log

# Removes the generated hierarchy as well. Kept separate from clean so a
# rebuild does not silently invalidate a running session's certificates.
distclean: clean
	rm -rf certs

# The SECC leaf is deliberately short-lived (60 days), so "certs exist" is not
# the same as "certs are usable": a tree left over from a previous month makes
# every handshake fail on an expired certificate. Regenerate when the leaf is
# not currently valid rather than only when the root is missing.
check: $(TARGETS)
	@if ! test -f certs/v2g-root.pem || \
	    ! openssl x509 -in certs/secc.pem -checkend 0 >/dev/null 2>&1; then \
	  echo "regenerating the V2G hierarchy (missing or expired)"; \
	  ./generate_v2g_certs.sh; \
	fi
	@echo "--- signature over a fixture EXI fragment ---"
	@out=$$(./v2g-signature) && echo "$$out" && \
	  printf '%s' "$$out" | grep -q 'v2g-signature: PASS'
	@echo "--- contract certificate installation ---"
	@out=$$(./contract-install) && echo "$$out" && \
	  printf '%s' "$$out" | grep -qE 'contract-install: (PASS|SKIP)'
	@echo "--- EVCC with a contract certificate ---"
	@./secc-server $(CHECK_PORT) > secc.log 2>&1 & \
	 s=$$!; sleep 1; \
	 out=$$(./evcc-client $(CHECK_PORT) 2>&1); \
	 echo "$$out"; kill $$s 2>/dev/null; wait $$s 2>/dev/null; \
	 printf '%s' "$$out" | grep -q 'evcc-client: PASS' || \
	   { echo "--- secc.log (the station's side) ---"; cat secc.log; false; }
	@echo "--- EVCC with no contract certificate ---"
	@./secc-server $(CHECK_PORT) > secc-nocontract.log 2>&1 & \
	 s=$$!; sleep 1; \
	 out=$$(./evcc-client $(CHECK_PORT) --no-contract 2>&1); \
	 echo "$$out"; kill $$s 2>/dev/null; wait $$s 2>/dev/null; \
	 printf '%s' "$$out" | grep -q 'REFUSED' || \
	   { echo "--- secc-nocontract.log (the station's side) ---"; \
	     cat secc-nocontract.log; false; }
	@echo "PASS: iso15118 checks"
