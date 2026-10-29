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

# Every file the four programs open, so an incomplete tree regenerates instead
# of failing a check: the guard used to look at the root and the SECC leaf
# only, and an interrupted generation or one deleted file left the rest of the
# hierarchy in place for it to accept.
V2G_ARTIFACTS = certs/v2g-root.pem certs/secc.pem certs/secc-chain.pem \
                certs/secc.key certs/contract.pem certs/contract-chain.pem \
                certs/contract.key certs/provisioning.pem \
                certs/provisioning.key certs/other-provisioning.key

# The SECC leaf is deliberately short-lived (60 days), so "certs exist" is not
# the same as "certs are usable": a tree left over from a previous month makes
# every handshake fail on an expired certificate. Regenerate when any artifact
# is missing or empty, or when the leaf is not currently valid -- test -s, not
# -f, because an interrupted openssl leaves a zero length file behind.
check: $(TARGETS)
	@need=; \
	 for f in $(V2G_ARTIFACTS) ; do \
	   test -s $$f || need="$$need $$f" ; \
	 done; \
	 if test -n "$$need" ; then \
	   echo "regenerating the V2G hierarchy (missing:$$need)"; \
	   ./generate_v2g_certs.sh; \
	 elif ! openssl x509 -in certs/secc.pem -checkend 0 >/dev/null 2>&1; then \
	   echo "regenerating the V2G hierarchy (the SECC leaf is not valid)"; \
	   ./generate_v2g_certs.sh; \
	 fi
	@for f in $(V2G_ARTIFACTS) ; do \
	   test -s $$f || { echo "$$f still missing after generation" >&2; \
	     exit 1; }; \
	 done
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
