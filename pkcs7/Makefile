# ECC Examples Makefile
CC       = gcc
LIB_PATH = /usr/local
CFLAGS   = -Wall -I$(LIB_PATH)/include
LIBS     = -L$(LIB_PATH)/lib -lwolfssl -lm -lz

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(LIB_PATH)/lib/libwolfssl.a
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os

# Options
#CFLAGS+=$(DEBUG_FLAGS)
CFLAGS+=$(OPTIMIZE)
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

clean:
	rm -f $(TARGETS) signedData_attrs.der signedData_noattrs.der \
        signedFirmwarePkgData.der signedFirmwarePkgData_attrs.der \
        signedFirmwarePkgData_noattrs.der \
        signedEncryptedFPD_attrs.der signedEncryptedFPD_noattrs.der \
        signedCompressedFPD_attrs.der signedCompressedFPD_noattrs.der \
        signedEncryptedCompressedFPD_attrs.der signedEncryptedCompressedFPD_noattrs.der \
        envelopedDataKTRI.der envelopedDataKARI.der \
        envelopedDataPWRI.der envelopedDataORI.der \
        authEnvelopedDataKARI.der authEnvelopedDataKTRI.der \
        authEnvelopedDataORI.der authEnvelopedDataPWRI.der encryptedData.der \
        compressedData.der
