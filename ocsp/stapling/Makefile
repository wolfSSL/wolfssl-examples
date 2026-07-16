# Examples Makefile
CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS   = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS     = -L$(WOLFSSL_INSTALL_DIR)/lib

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(WOLFSSL_INSTALL_DIR)/lib/libwolfssl.a
DEBUG_FLAGS     = -g3 -O0 -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -O2

# Options
CFLAGS+=$(DEBUG_FLAGS)
#CFLAGS+=$(OPTIMIZE)
#LIBS+=$(STATIC_LIB)
LIBS+=$(DYN_LIB)

# build targets
SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))

.PHONY: clean all

all: $(TARGETS)

debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

# build template
%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

# the signer must be issued by server1's own issuer, or wolfSSL rejects the
# response with BAD_OCSP_RESPONDER (RFC 6960 delegated responder)
responder:
	openssl ocsp -index responder-certs/index.txt -port 22221 \
	    -rsigner responder-certs/ocsp-responder-int1-cert.pem \
		-rkey responder-certs/ocsp-responder-int1-key.pem \
		-CA client-certs/intermediate1-ca-cert.pem

clean:
	rm -f $(TARGETS)
