# ECC Examples Makefile
CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS   = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS     = -L$(WOLFSSL_INSTALL_DIR)/lib -lm

# option variables
DYN_LIB         = -lwolfssl -pthread
STATIC_LIB      = $(WOLFSSL_INSTALL_DIR)/lib/libwolfssl.a
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

.PHONY: clean all check

all: $(TARGETS)

debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

# ecc-export-Qx-Qy reads the .der ecc-key-export writes, so keep that order
check: all
	out=$$(./ecc-key-decode    ) && printf '%s' "$$out" | grep -q "Success"
	out=$$(./ecc-params        ) && printf '%s' "$$out" | grep -q "Gy: 32"
	out=$$(./ecc-sign          ) && printf '%s' "$$out" | grep -q "Firmware Signature 9: Ret 0"
	out=$$(./ecc-stack         ) && printf '%s' "$$out" | grep -q "stack used ="
	out=$$(./ecc-verify        ) && printf '%s' "$$out" | grep -q "hash_firmware_verify: 0"
	out=$$(./ecc-verify-minimal) && printf '%s' "$$out" | grep -q "wc_ecc_verify_hash: ret=0, is_valid_sig=1"
	out=$$(./ecc-key-export    ) && printf '%s' "$$out" | grep -q "ECC Public Key Exported to ./ECC_SECP256K1_pub.pem"
	out=$$(./ecc-export-Qx-Qy ECC_SECP256K1.der qxqy.raw) && printf '%s' "$$out" | grep -q "Exported Qx and Qy"
	@echo "PASS: ecc checks"

# build template
%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)
	rm -f ECC*.der
	rm -f ECC*.pem
