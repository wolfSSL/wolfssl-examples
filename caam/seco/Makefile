# SECO Examples Makefile
WOLFSSL_PATH ?= /usr/local
DEVCRYPTO_PATH ?= /usr
ZLIB_PATH ?= /usr
CFLAGS   = -O -Wall -I$(WOLFSSL_PATH)/include -I$(SECO_PATH)/include -I$(DEVCRYPTO_PATH)/include
LIBS     = -L$(WOLFSSL_PATH)/lib -L$(ZLIB_PATH)/lib -lm -lz -lpthread

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(WOLFSSL_PATH)/lib/libwolfssl.a

# build targets
SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))

.PHONY: clean all
all: $(TARGETS)

# build template
%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS) $(STATIC_LIB) $(SECO_PATH)/lib/hsm_lib.a $(SECO_PATH)/lib/seco_nvm_manager.a

clean:
	rm -f $(TARGETS)
