# Examples Makefile
CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
# example of a custom install location
#WOLFSSL_INSTALL_DIR = /Users/kalebhimes/work/testDir/wolf-install-dir-for-testing
CFLAGS   = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS     = -L$(WOLFSSL_INSTALL_DIR)/lib -lm

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(WOLFSSL_INSTALL_DIR)/lib/libwolfssl.a
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os

# Options
#CFLAGS+=$(DEBUG_FLAGS)
CFLAGS+=$(OPTIMIZE)
#LIBS+=$(STATIC_LIB) -ldl -lm
LIBS+=$(DYN_LIB)

# build targets
SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))

.PHONY: clean all check

all: $(TARGETS)

debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

# build template
%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)

check: test-cert-privkey-pair
	./test-cert-privkey-pair ../../certs/client-key.pem ../../certs/client-cert.pem | grep -q 'Congratulations, this private key and cert'
	! ./test-cert-privkey-pair ../../certs/client-cert.pem ../../certs/client-cert.pem >/dev/null 2>&1
	@echo "PASS: pk-test-cert-keypair checks"
