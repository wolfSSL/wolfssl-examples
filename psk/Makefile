# PSK Examples Makefile
CC       = gcc
LIB_PATH = /usr/local
CFLAGS   = -Wall -I$(LIB_PATH)/include
LIBS     = -L$(LIB_PATH)/lib -lm

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(LIB_PATH)/lib/libwolfssl.a
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os

# Options
# For debug (./configure --enable-debug --disable-shared)
# Uncomment DEBUG_FLAGS and STATIC_LIB (comment DYN_LIB)
#CFLAGS+=$(DEBUG_FLAGS)
CFLAGS+=$(OPTIMIZE)
#LIBS+=$(STATIC_LIB)
LIBS+=$(DYN_LIB)


all: client-tcp client-psk client-psk-nonblocking client-psk-resume client-psk-tls13-multi-id server-tcp server-psk server-psk-nonblocking server-psk-threaded client-psk-bio-custom server-psk-tls13-multi-id

client-tcp: client-tcp.o
	$(CC) -o $@ $^ $(CFLAGS)

client-psk: client-psk.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-psk-nonblocking: client-psk-nonblocking.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-psk-resume: client-psk-resume.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-psk-tls13-multi-id: client-psk-tls13-multi-id.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-tcp: server-tcp.o
	$(CC) -o $@ $^ $(CFLAGS)

server-psk: server-psk.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-psk-nonblocking: server-psk-nonblocking.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-psk-threaded: server-psk-threaded.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -lpthread

client-psk-bio-custom: client-psk-bio-custom.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-psk-tls13-multi-id: server-psk-tls13-multi-id.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o client-tcp client-psk client-psk-nonblocking client-psk-resume client-psk-tls13-multi-id server-tcp server-psk server-psk-nonblocking server-psk-threaded client-psk-bio-custom server-psk-tls13-multi-id
