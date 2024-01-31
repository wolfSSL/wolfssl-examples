# PQ Stateful Hash-Based Signature Examples Makefile
#
# XMSS_INC, XMSS_LIB, and WOLF_STATIC_LIB are only required if
# building with --with-libxmss.
#
CC       = gcc
LIB_PATH = /usr/local
HSS_INC  =
XMSS_INC =
CFLAGS   = -Wall -I$(LIB_PATH)/include
LIBS     = -L$(LIB_PATH)/lib -lm

# option variables
WOLF_DYN_LIB    = -lwolfssl
WOLF_STATIC_LIB = $(LIB_PATH)/lib/libwolfssl.a
HSS_LIB         =
XMSS_LIB        =
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os

# Options
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
	$(CC) -o $@ $< $(CFLAGS) -I$(HSS_INC) $(LIBS) $(WOLF_STATIC_LIB) $(HSS_LIB)

xmss_example: xmss_example.c
# If building with  --enable-xmss=wolfssl:
	$(CC) -o $@ $< $(CFLAGS) -I$(XMSS_INC) $(LIBS) $(WOLF_STATIC_LIB) $(XMSS_LIB)
# If building with  --enable-xmss --with-libxmss=<path>:
#	$(CC) -o $@ $< $(CFLAGS) $(LIBS) $(WOLF_DYN_LIB)

xmss_example_verifyonly: xmss_example.c
	$(CC) -o $@ $< $(CFLAGS) -I$(XMSS_INC) -DWOLFSSL_XMSS_VERIFY_ONLY $(LIBS) $(WOLF_STATIC_LIB) $(XMSS_LIB)

clean:
	rm -f $(TARGETS)
	rm -f xmss_example_verifyonly
	rm -f lms_example.key
	rm -f xmss_example.key
