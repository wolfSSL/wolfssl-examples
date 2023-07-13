# PQ Stateful Hash-Based Signature Examples Makefile
CC       = gcc
LIB_PATH = /usr/local
HSS_INC  = <path to hss install>
CFLAGS   = -Wall -I$(LIB_PATH)/include -I$(HSS_INC)
LIBS     = -L$(LIB_PATH)/lib -lm

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(LIB_PATH)/lib/libwolfssl.a
HSS_LIB         = <path to hss_lib_thread.a>
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os

# Options
#CFLAGS+=$(DEBUG_FLAGS)
#CFLAGS+=$(OPTIMIZE)
LIBS+=$(STATIC_LIB)
#LIBS+=$(DYN_LIB)
LIBS+=$(HSS_LIB)

# build targets
SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))

.PHONY: clean all

all: $(TARGETS)

debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

# build template
lms_example: lms_example.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)
	rm -f lms_example.key
