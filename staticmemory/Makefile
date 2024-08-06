# ECC Examples Makefile
CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS   = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
ZLIB     =
#ZLIB    += -lz
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

.PHONY: clean all

all: $(TARGETS)

debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

# build template
%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)
