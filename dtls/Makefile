# DTLS Examples Makefile
CC       = gcc
LIB_PATH = /usr/local
CFLAGS   = -Wall -I$(LIB_PATH)/include
LIBS     = -L$(LIB_PATH)/lib -lm

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(LIB_PATH)/lib/libwolfssl.a
DEBUG_FLAGS     = -g3 -DDEBUG -O0
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

# add the -pthread flag and -lpthread lib to any threaded or shared examples
%-threaded: CFLAGS+=-pthread
%-threaded: LIBS+=-lpthread
%-shared: CFLAGS+=-pthread
%-shared: LIBS+=-lpthread

# try to build the libevent server. "|| true" ignores the error return.
server-dtls13-event: server-dtls13-event.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS) -levent || true

# build template
%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)
