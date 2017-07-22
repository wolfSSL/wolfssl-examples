# BTLE Examples Makefile
CC       = gcc
LIB_PATH = /usr/local
CFLAGS   = -Wall -I$(LIB_PATH)/include
LIBS     = -L$(LIB_PATH)/lib

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      =
DEBUG_FLAGS     = -g -DDEBUG
OPTIMIZE        = -Os

# Options
#CFLAGS+=$(DEBUG_FLAGS)
#CFLAGS+=$(OPTIMIZE)

# Static or Dynamic Library
#STATIC_LIB+=$(LIB_PATH)/lib/libwolfssl.a
LIBS+=$(DYN_LIB) -lm


.PHONY: clean all

all: ecc-client ecc-server

debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all


# build template
%.o: %.c
	gcc -c $< -o $@ $(CFLAGS)

ecc-server: ecc-server.o btle-sim.o $(STATIC_LIB)
	$(CC) $^ -o $@ $(LIBS)

ecc-client: ecc-client.o btle-sim.o $(STATIC_LIB)
	$(CC) $^ -o $@ $(LIBS)

clean:
	rm *.o
