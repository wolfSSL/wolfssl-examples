# PKCS7 Examples Makefile
CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS   = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
# only needed when linking wolfSSL statically; a shared libwolfssl built
# --with-libz already carries its own dependency on libz
ZLIB     =
PSA_LIB  = -lmbedcrypto
LIBS     = -L$(WOLFSSL_INSTALL_DIR)/lib -lm ${ZLIB}

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(WOLFSSL_INSTALL_DIR)/lib/libwolfssl.a
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os

# Options
#CFLAGS+=$(DEBUG_FLAGS)
CFLAGS+=$(OPTIMIZE)
#LIBS+=$(STATIC_LIB)
LIBS+=$(DYN_LIB)

ifneq ($(PSA),)
LIBS+=$(PSA_LIB)
CFLAGS+=-DUSE_PSA
endif

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
	rm -f $(TARGETS) signedData_attrs.der signedData_noattrs.der \
        signedFirmwarePkgData.der signedFirmwarePkgData_attrs.der \
        signedFirmwarePkgData_noattrs.der \
        signedData_EncryptedFPD_callback.der \
        signedData_EncryptedFPD_AES128_callback.der \
        signedEncryptedFPD_attrs.der signedEncryptedFPD_noattrs.der \
        signedCompressedFPD_attrs.der signedCompressedFPD_noattrs.der \
        signedEncryptedCompressedFPD_attrs.der signedEncryptedCompressedFPD_noattrs.der \
        signedData_detached_attrs.der signedData_detached_noattrs.der \
        signedData_cryptodev_attrs.der signedData_cryptodev_noattrs.der \
        envelopedDataKTRI.der envelopedDataKARI.der \
        envelopedDataPWRI.der envelopedDataORI.der envelopedDataKEKRI.der \
        authEnvelopedDataKARI.der authEnvelopedDataKTRI.der \
        authEnvelopedDataORI.der authEnvelopedDataPWRI.der encryptedData.der \
        authEnvelopedDataKEKRI.der compressedData.der \
        smime-created.p7s stream.log

# Redirect then grep (no pipe) so signedData-stream's nonzero exit fails the
# recipe, there is no SIGPIPE on the dump, and it works under BSD make too.
check: signedData-stream
	./signedData-stream content.txt > stream.log
	grep -q 'Successfully verified SignedData bundle' stream.log
	@echo "PASS: pkcs7-signeddata-stream checks"
