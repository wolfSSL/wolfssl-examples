CC      = gcc
CLANG   = clang
CFLAGS  = -O2 -g -Wall

# Auto-detect host arch, convert to BPF target name
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_M), x86_64)
    BPF_ARCH   := x86
    UAPI_PATH  := /usr/include/x86_64-linux-gnu
else ifeq ($(UNAME_M), aarch64)
    BPF_ARCH   := arm64
    UAPI_PATH  := /usr/include/aarch64-linux-gnu
else
    $(error Unsupported architecture: $(UNAME_M))
endif

BPF_CFLAGS = -O2 -g -target bpf -D__TARGET_ARCH_$(BPF_ARCH)

LIBBPF_LIBS = -lbpf -lelf -lz

TARGETS = client-tls server-tls wolfssl_uprobe wolfssl_uprobe.bpf.o

all: $(TARGETS)

# ===== TLS Programs =====
client-tls: client-tls.c
	$(CC) $(CFLAGS) $< -o $@ -lwolfssl

server-tls: server-tls.c
	$(CC) $(CFLAGS) $< -o $@ -lwolfssl

# ===== eBPF Program =====
wolfssl_uprobe.bpf.o: wolfssl_uprobe.bpf.c
	$(CLANG) $(BPF_CFLAGS) \
		-I/usr/include \
		-I$(UAPI_PATH) \
		-c $< -o $@

wolfssl_uprobe: wolfssl_uprobe.c wolfssl_uprobe.bpf.o
	$(CC) $(CFLAGS) $< -o $@ $(LIBBPF_LIBS)

clean:
	rm -f $(TARGETS) *.o *.log
