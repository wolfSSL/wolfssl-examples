## wolfSSL eBPF Examples

This directory contains **eBPF-based observability examples** demonstrating how Linux eBPF can be used to monitor system calls, user-space wolfSSL operations, network activity, and TLS behavior **without modifying application code**.

These examples are designed for:

* **debugging**
* **education**
* **performance tracing**
* **understanding TLS internals**
* **research and experimentation**

All examples require:

* Linux kernel with eBPF support (4.19+ recommended)
* clang/LLVM (for compiling `.bpf.c` programs)
* libbpf and libelf
* root privileges
* A recent installation of wolfSSL if running TLS examples