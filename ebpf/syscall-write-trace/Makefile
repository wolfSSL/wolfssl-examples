CC = gcc
CLANG = clang

CFLAGS = -O2 -g -Wall
BPF_CFLAGS = -O2 -g -target bpf -D__TARGET_ARCH_$(ARCH)

# Detect architecture for correct include path
ARCH := $(shell uname -m)
ifeq ($(ARCH),x86_64)
    ARCH_DIR = x86_64-linux-gnu
else ifeq ($(ARCH),aarch64)
    ARCH_DIR = aarch64-linux-gnu
else
    ARCH_DIR = x86_64-linux-gnu
endif

BPF_INCLUDES = -I/usr/include -I/usr/include/$(ARCH_DIR)
LIBBPF_LIBS = -lbpf -lelf -lz

TARGETS = write_tracer write_tracer.bpf.o client-tcp server-tcp

.PHONY: all clean help

all: $(TARGETS)

write_tracer.bpf.o: write_tracer.bpf.c
	$(CLANG) $(BPF_CFLAGS) $(BPF_INCLUDES) -c $< -o $@

write_tracer: write_tracer.c write_tracer.bpf.o
	$(CC) $(CFLAGS) write_tracer.c -lelf -lz -lbpf -o write_tracer

client-tcp: client-tcp.c
	$(CC) $(CFLAGS) client-tcp.c -o client-tcp

server-tcp: server-tcp.c
	$(CC) $(CFLAGS) server-tcp.c -o server-tcp

clean:
	rm -f *.o write_tracer client-tcp server-tcp

help:
	@echo "Targets:"
	@echo "  all       - Build syscall tracer and TCP demo apps"
	@echo "  clean     - Remove binaries"
	@echo ""
	@echo "Instructions:"
	@echo "  1. sudo ./write_tracer"
	@echo "  2. ./server-tcp"
	@echo "  3. ./client-tcp 127.0.0.1"
