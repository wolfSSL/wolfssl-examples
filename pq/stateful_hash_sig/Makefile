# PQ Stateful Hash-Based Signature Examples Makefile
#
# HSS_INC, HSS_LIB, XMSS_INC, XMSS_LIB, and WOLF_STATIC_LIB are only
# required if building with --with-libxmss or --with-liblms.
#
CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS   = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS     = -L$(WOLFSSL_INSTALL_DIR)/lib -lm

# option variables
WOLF_DYN_LIB    = -lwolfssl
WOLF_STATIC_LIB = $(WOLFSSL_INSTALL_DIR)/lib/libwolfssl.a

# These are only needed if building with the external
# integration (ext_lms, ext_xmss).
# HSS_INC  =  path to hash-sigs src dir
# XMSS_INC =  path to xmss-reference src dir
# HSS_LIB  =  path to hash-sigs hss_lib.a
# XMSS_LIB =  path to xmss-reference xmss_lib.a

# Options
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os
#CFLAGS+=$(DEBUG_FLAGS)
#CFLAGS+=$(OPTIMIZE)

# build targets
SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))

.PHONY: clean all

all: $(TARGETS)

debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

# build template
lms_example: lms_example.c
# If building with wc_lms (--enable-lms):
	$(CC) -o $@ $< $(CFLAGS) -DWOLFSSL_WC_LMS $(LIBS) $(WOLF_DYN_LIB)
# If building with ext_lms (--enable-lms --with-liblms=<path>):
#	$(CC) -o $@ $< $(CFLAGS) -I$(HSS_INC) $(LIBS) $(WOLF_STATIC_LIB) $(HSS_LIB)

xmss_example: xmss_example.c
# If building with wc_xmss (--enable-xmss):
	$(CC) -o $@ $< $(CFLAGS) -DWOLFSSL_WC_XMSS $(LIBS) $(WOLF_DYN_LIB)
# If building with ext_xmss (--enable-xmss --with-libxmss=<path>):
#	$(CC) -o $@ $< $(CFLAGS) -I$(XMSS_INC) $(LIBS) $(WOLF_STATIC_LIB) $(XMSS_LIB)

clean:
	rm -f $(TARGETS)
	rm -f lms_example.key
	rm -f xmss_example.key
