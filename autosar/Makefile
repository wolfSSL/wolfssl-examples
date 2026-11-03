# AUTOSAR CSM Examples Makefile
#
# Requires wolfSSL built and installed with --enable-autosar.
#
#   make                 build every example
#   make check           build and run every example
#   make REDIRECT=1      build against a libwolfssl that was itself configured
#                        with the key redirection macros (see README.md --
#                        the macros go into the library, not the application)
#   make WOLFSSL_INSTALL_DIR=/opt/wolfssl
#
# The user_settings/ subdirectory is a separate build that compiles the wolfSSL
# sources directly instead of linking an installed library; see its README.
#
# The MAC cases in csm-errors compile in automatically when libwolfssl was built
# with --enable-autosar-cmac; nothing extra is needed here. csm-secoc needs that
# option outright and reports SKIP without it, and csm-she-provision likewise
# needs --enable-she=standard. csm-cryptocb needs --enable-cryptocb.
#
CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS   = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS     = -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

# option variables
STATIC_LIB      = $(WOLFSSL_INSTALL_DIR)/lib/libwolfssl.a
DEBUG_FLAGS     = -g -DDEBUG
OPTIMIZE        = -Os

CFLAGS+=$(OPTIMIZE)

# Key input redirection is a compile-time property of the wolfSSL library.
# These must match the macros libwolfssl was configured with, or the example
# will report that the wrong keystore slot was used.
ifdef REDIRECT
CFLAGS += -DREDIRECTION_CONFIG=0x03 \
          -DREDIRECTION_IN1_KEYID=1 -DREDIRECTION_IN1_KEYELMID=0x01 \
          -DREDIRECTION_IN2_KEYID=4 -DREDIRECTION_IN2_KEYELMID=0x05
endif

TARGETS = csm-basic csm-key-redirection csm-jobs csm-errors csm-secoc \
          csm-threads csm-stream csm-she-provision csm-cryptocb

.PHONY: all debug clean check

all: $(TARGETS)

debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

# only csm-threads needs it; harmless elsewhere but kept target-specific
csm-threads: EXTRA_LIBS = -lpthread

%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS) $(EXTRA_LIBS)

clean:
	rm -f $(TARGETS) *.o

check: $(TARGETS)
	out=$$(./csm-basic) && \
		printf '%s' "$$out" | grep -qE 'csm-basic: (PASS|SKIP)'
	out=$$(./csm-key-redirection) && \
		printf '%s' "$$out" | grep -qE 'csm-key-redirection: (PASS|SKIP)'
	out=$$(./csm-jobs) && \
		printf '%s' "$$out" | grep -qE 'csm-jobs: (PASS|SKIP)'
	out=$$(./csm-errors) && \
		printf '%s' "$$out" | grep -qE 'csm-errors: (PASS|SKIP)'
	out=$$(./csm-secoc) && \
		printf '%s' "$$out" | grep -qE 'csm-secoc: (PASS|SKIP)'
	out=$$(./csm-threads) && \
		printf '%s' "$$out" | grep -qE 'csm-threads: (PASS|SKIP)'
	out=$$(./csm-stream) && \
		printf '%s' "$$out" | grep -qE 'csm-stream: (PASS|SKIP)'
	out=$$(./csm-she-provision) && \
		printf '%s' "$$out" | grep -qE 'csm-she-provision: (PASS|SKIP)'
	out=$$(./csm-cryptocb) && \
		printf '%s' "$$out" | grep -qE 'csm-cryptocb: (PASS|SKIP)'
	@echo "PASS: autosar checks"
