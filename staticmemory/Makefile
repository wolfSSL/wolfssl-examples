# Static Memory Examples Makefile
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
	rm -f $(TARGETS) size.log debug.log

# Redirect then grep (no pipe) so a nonzero exit fails the recipe on its own and
# it works under BSD make too, where SHELLFLAGS pipefail is unavailable.
check: size-calculation debug-callback-example
	./size-calculation > size.log
	grep -q 'Checking out bucket 4 size of 25...ok' size.log
	./debug-callback-example > debug.log
	grep -q 'bucket size used was' debug.log
	@echo "PASS: staticmemory checks"
