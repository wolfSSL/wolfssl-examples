# `tls-uprobe-trace` – Tracing `wolfSSL_write()` and `wolfSSL_read()` with eBPF

This example demonstrates how to use **eBPF uprobes** to observe **plaintext TLS data** inside applications that use **wolfSSL**.
Because decryption happens *inside the process* (not in the kernel), traditional tools like `tcpdump` or kprobes **cannot see decrypted SSL/TLS plaintext**.
Uprobes make it possible to attach to userspace functions such as `wolfSSL_write()` and `wolfSSL_read()` and extract:

* plaintext being written (encrypted later by TLS)
* plaintext being read (already decrypted by TLS)
* PID/TID
* SSL* internal pointer
* byte count + sample of the buffer
* process name

This example includes:

✔️ TLS client
✔️ TLS server
✔️ eBPF probe attached to `wolfSSL_write`
✔️ eBPF probe attached to `wolfSSL_read` (uretprobe)
✔️ Perf buffer event delivery to user space
✔️ Architecture-portable code (x86_64 & ARM64)
✔️ Symbol lookup (no hardcoding offsets)

---

## 1. Problem

When using TLS, all application data is encrypted before it leaves the process:

```
cleartext → wolfSSL_write() → encrypt → kernel → network
network → kernel → decrypt → wolfSSL_read() → cleartext
```

This means:

* Kernel tools **cannot see decrypted payloads**
* tcpdump/wireshark **see only ciphertext**
* syscall tracing (e.g., `sys_enter_write`) **sees ciphertext**
* security & observability tools cannot inspect application-level data

For debugging, auditing, or educational purposes, developers often need a way to inspect:

* what plaintext the application *actually sent*
* what plaintext it *actually received*

---

## 2. Solution: eBPF Uprobes on wolfSSL

eBPF uprobes allow attaching to **any userspace function**.
By attaching to:

* `wolfSSL_write()` → entry probe
* `wolfSSL_read()` → entry + return probes

we can safely extract plaintext buffers from user-space memory *before encryption* or *after decryption*.

This example uses:

* `trace_wolfssl_write_enter`
* `trace_wolfssl_read_enter`
* `trace_wolfssl_read_exit`

The eBPF program reads arguments from registers (`PT_REGS_PARM1`, etc.), copies user-space buffers via `bpf_probe_read_user()`, and emits structured events to a perf buffer.

---

## 3. Benefits

### ✔️ Observe decrypted TLS data

See exactly what `client-tls` and `server-tls` send and receive.

### ✔️ No modification to wolfSSL or application

Zero instrumentation changes required.

### ✔️ Low overhead & safe

eBPF verifier guarantees memory safety; uprobes only execute on function entry/exit.

### ✔️ Works with any application using wolfSSL

Examples reuse standard wolfSSL examples with no changes.

### ✔️ Foundation for deeper wolfSSL observability

Future variants can trace:

* handshake events
* cipher suite negotiation
* session resumption
* key export callbacks
* user-defined I/O callbacks

---

## 4. Architecture

```
        +---------------------------+
        |   TLS Application         |
        |  (client-tls / server-tls)|
        +-------------+-------------+
                      | TLS API
                      v
              wolfSSL_write() / wolfSSL_read()
                      |
        -------- uprobe / uretprobe ---------
                      |
              eBPF programs (BPF bytecode)
                      |
              perf event ring buffer
                      |
             userspace loader (wolfssl_uprobe)
                      |
              human-readable terminal output
```

### Files included:

| File                                | Purpose                                      |
| ----------------------------------- | -------------------------------------------- |
| `client-tls.c`                      | TLS client using wolfSSL                     |
| `server-tls.c`                      | TLS server using wolfSSL                     |
| `wolfssl_uprobe.c`                  | eBPF loader + perf buffer consumer           |
| `wolfssl_uprobe.bpf.c`              | eBPF program (uprobes on wolfSSL_write/read) |
| `Makefile`                          | Build everything cleanly for x86 or ARM      |
| `../../certs/ca-cert.pem`           | CA certificate (shared example)             |
| `../../certs/server-cert.pem`       | Server certificate (shared example)         |
| `../../certs/server-key.pem`        | Server key (shared example)                 |

---

## 5. Building

### Requirements

```bash
sudo apt install clang llvm libbpf-dev libelf-dev zlib1g-dev make
```

wolfSSL must be installed (shared library):

```bash
sudo make install
sudo ldconfig
```

### Build everything

```bash
make
```

This produces:

* `client-tls`
* `server-tls`
* `wolfssl_uprobe`
* `wolfssl_uprobe.bpf.o`

---

## 6. Running the Example

### 1. Start the eBPF tracer

(must run as root)

```bash
sudo ./wolfssl_uprobe
```

You should see:

```
Successfully loaded and attached eBPF programs!
Monitoring wolfSSL_write() and wolfSSL_read() calls...
```

### 2. Start the TLS server

```bash
./server-tls
```

### 3. Start the TLS client

```bash
./client-tls 127.0.0.1
Message for server: hello world!
```

### Expected tracer output

#### From `wolfSSL_write`:

```
=== wolfSSL_write() INTERCEPTED ===
Process: client-tls (PID: 2312)
Write Count: 12 bytes
Data: hello world!
=====================================
```

#### From `wolfSSL_read`:

```
=== wolfSSL_read() INTERCEPTED ===
Process: client-tls (PID: 2312)
Bytes Read: 25
Decrypted Data: I hear ya fa shizzle!
=====================================
```

---

## 7. Notes & Limitations

* Tracing is implemented using **uprobes**, so only plaintext inside userspace is visible (by design).
* The example currently filters to process name `client-tls` to reduce noise.
* Return probe (`uretprobe`) only fires once TLS read returns to user space.

---

## 8. Cleanup

```bash
make clean
```
