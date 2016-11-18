CC=gcc
CFLAGS=-Wall
LIBS=-lwolfssl -lm
DEBUG_FLAGS=-g -DDEBUG
#CFLAGS+=$(DEBUG_FLAGS)

OS_DET=UNKNOWN
CPU_DET=UNKNOWN
ifeq ($(OS),Windows_NT)
    OS_DET=WIN32
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        CPU_DET=AMD64
    else
        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
            CPU_DET=AMD64
        endif
        ifeq ($(PROCESSOR_ARCHITECTURE),x86)
            CPU_DET=IA32
        endif
    endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        OS_DET=LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
        OS_DET=OSX
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
        CPU_DET=AMD64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
        CPU_DET=IA32
    endif
    ifneq ($(filter arm%,$(UNAME_P)),)
        CPU_DET=ARM
    endif
endif

# $(info $$OS_DET is [${OS_DET}])
# $(info $$CPU_DET is [${CPU_DET}])

ifeq ($(OS_DET),LINUX)
all: client-tcp client-tls client-tls-nonblocking client-tls-ecdhe server-tls-ecdhe client-tls-perf client-tls-resume client-callback memory-tls server-tcp server-tls server-tls-nonblocking server-tls-threaded server-tls-epoll-perf server-callback server-tls-epoll-threaded
else
all: client-tcp client-tls client-tls-nonblocking client-tls-ecdhe server-tls-ecdhe client-tls-resume client-callback memory-tls server-tcp server-tls server-tls-nonblocking server-tls-threaded server-callback
endif

client-tcp: client-tcp.o
	$(CC) -o $@ $^ $(CFLAGS)

client-tls: client-tls.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-tls-nonblocking: client-tls-nonblocking.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-tls-ecdhe: client-tls-ecdhe.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-tls-ecdhe: server-tls-ecdhe.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-tls-perf: client-tls-perf.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-tls-resume: client-tls-resume.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client-callback: client-callback.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

memory-tls: memory-tls.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -lpthread

server-tcp: server-tcp.o
	$(CC) -o $@ $^ $(CFLAGS)

server-tls: server-tls.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-tls-nonblocking: server-tls-nonblocking.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-tls-epoll-perf: server-tls-epoll-perf.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-tls-threaded: server-tls-threaded.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -lpthread

server-callback: server-callback.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

server-tls-epoll-threaded: server-tls-epoll-threaded.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -lpthread

.PHONY: clean all

clean:
	rm -f *.o client-tcp client-tls client-tls-nonblocking client-tls-ecdhe server-tls-ecdhe client-tls-perf client-tls-resume client-callback memory-tls server-tcp server-tls server-tls-nonblocking server-tls-threaded server-tls-epoll-perf server-callback server-tls-epoll-threaded
