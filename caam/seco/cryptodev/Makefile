# SECO Examples Makefile
WOLFSSL_PATH ?= /usr/local
CRYPTODEV_DIR ?= /usr/include
ZLIB_DIR ?= /usr
CFLAGS   = -O -Wall -I$(WOLFSSL_PATH)/include -I$(HSM_DIR)/include -I$(CRYPTODEV_DIR)
LIBS     = -L$(WOLFSSL_PATH)/lib -L$(ZLIB_DIR)/lib -lm -lz -lpthread

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
	$(CC) -o $@ $< $(CFLAGS) $(LIBS) $(STATIC_LIB) $(HSM_DIR)/lib/hsm_lib.a $(HSM_DIR)/lib/seco_nvm_manager.a

clean:
	rm -f $(TARGETS)
