# syscall-write-trace

### eBPF Example: Tracing Plaintext at the `write()` Syscall Boundary

This example demonstrates how to use **Linux eBPF** to intercept the `write()` system call and extract plaintext data **before the kernel performs any buffering, encryption, or processing**.
It uses a simple TCP client/server pair to generate predictable network writes and an eBPF tracepoint program to observe them.

This example is part of the **wolfSSL eBPF observability suite**, designed to help developers understand how plaintext flows through the system and how eBPF can be used to debug, monitor, or study application behavior without modifying application code.

---

# 📌 **Problem**

When debugging network applications, especially TLS applications, developers often want to inspect:

* What plaintext is being written
* What data is being sent to the network
* Whether buffers contain what we expect
* Whether the application or kernel is modifying data
* Whether the problem is at the app layer, kernel layer, or crypto layer

However, once an application calls:

```
write(fd, buffer, count)
```

the kernel:

* does **not** expose the plaintext
* may buffer or coalesce writes
* may encrypt data (TLS offload, QUIC, etc.)
* hides memory from tools
* provides no visibility into the user buffer

Traditional debugging tools (tcpdump, Wireshark, strace) **cannot see the plaintext** before encryption or kernel processing.

This creates a visibility gap.

---

# 🎯 **Solution**

We attach an **eBPF tracepoint** to:

```
tracepoint/syscalls/sys_enter_write
```

This gives us:

* access to the syscall arguments
* the calling process’s PID/TID
* the file descriptor
* the byte count
* the raw user pointer to the data
* ability to read the plaintext with `bpf_probe_read_user()`

The eBPF program:

1. Filters events only from a target process (`client-tcp`)
2. Copies up to 255 bytes of user-space buffer safely
3. Sends them to user-space via a perf buffer
4. The userspace loader prints ASCII and hex output

This provides **perfect visibility** into the plaintext leaving the application.

---

# 🧩 **Architecture**

```
   TCP Client App (user space)
         |
         | 1. call write(fd, buf, count)
         v
 ┌──────────────────────────────┐
 │ tracepoint: sys_enter_write  │  ← eBPF hook runs BEFORE write executes
 │ eBPF program:                │
 │  - filters process name       │
 │  - reads buffer from user mem │
 │  - emits event via perf buf   │
 └───────────────┬──────────────┘
                 |
                 | 2. event (plaintext)
                 v
        Userspace Loader (write_tracer)
        --------------------------------
        - loads BPF program
        - attaches tracepoint
        - opens perf buffer
        - prints plaintext
                 |
                 | 3. human-readable output
                 v
              Terminal
```

---

# 🔍 **Detailed Walkthrough**

## 1. The Sample Applications

### `server-tcp.c`

A simple TCP echo server on port 11111:

* waits for a connection
* receives a message
* prints it
* echoes back a canned response
* loops

### `client-tcp.c`

A matching TCP client:

* prompts user for input
* writes it to the server
* prints server response

These programs provide **predictable write() calls** for tracing.

---

## 2. The eBPF Program: `write_tracer.bpf.c`

Hooks:

```
tracepoint/syscalls/sys_enter_write
```

Key details:

### ✔ Event Filtering

Checks the process name (`comm`) to avoid tracing all processes.

### ✔ Safe Memory Access

Uses:

```
bpf_probe_read_user()
```

to copy user memory safely, limited to 255 bytes (verifier-friendly bound).

### ✔ Perf Buffer Emission

Writes events to a ring buffer consumed by user-space.

### ✔ Struct of event data

Contains:

* PID, TID
* FD
* count
* process name
* captured data

---

## 3. The Userspace Loader: `write_tracer.c`

It:

1. Raises RLIMIT_MEMLOCK
2. Loads `write_tracer.bpf.o`
3. Attaches the tracepoint
4. Opens perf buffer
5. Pretty-prints events:

```
=== WRITE SYSCALL INTERCEPTED ===
Process: client-tcp (PID: 1234)
FD: 3
Count: 13 bytes
Data: "hello world!"
Hex: 68 65 6c ...
```

This gives full plaintext visibility.

---

# 🚀 **How to Build**

Dependencies (Ubuntu):

```bash
sudo apt install clang llvm libbpf-dev libelf-dev zlib1g-dev build-essential
```

Then:

```bash
make
```

This compiles:

* TCP client/server
* eBPF program (`write_tracer.bpf.o`)
* userspace loader (`write_tracer`)

---

# ▶️ **How to Run**

### 1. Terminal #1 — Start the tracer

```bash
sudo ./write_tracer
```

### 2. Terminal #2 — Start the TCP server

```bash
./server-tcp
```

### 3. Terminal #3 — Run client and type message

```bash
./client-tcp 127.0.0.1
```

Tracer output:

```
=== WRITE SYSCALL INTERCEPTED ===
Process: client-tcp
File Descriptor: 3
Write Count: 13 bytes
Data (first 13 bytes): hello world!
```

---

# 🎁 **Benefits of This Example**

### ✔ Shows how to read plaintext before kernel/network/TLS processing

### ✔ Demonstrates safe buffer access in eBPF

### ✔ Demonstrates filtering (PID, process name)

### ✔ Teaches core eBPF concepts: tracepoints, perf buffers, verifier constraints

### ✔ Foundation for more advanced examples (TLS plaintext, handshake tracing)

### ✔ Helps wolfSSL developers debug TLS behavior

### ✔ Useful for application developers integrating wolfSSL

---

# ⚙️ **Nitty-Gritty Details**

### 1. Why tracepoint instead of kprobe?

Tracepoints are:

* stable
* argument offsets fixed
* preferred for syscall entry tracing

Allows verifier to analyze program more easily.

### 2. Why use process name filtering?

Without it, the tracer prints:

* output from bash
* systemd
* everything reading/writing

Filtering avoids noise.

### 3. Why limit buffer to 255 bytes?

Verifier restrictions require fixed bounded copy sizes.
A 255-byte buffer is safe and sufficient for demos.

### 4. Why use perf buffer instead of ringbuf?

Perf buffer is more compatible with older kernels (e.g., Ubuntu LTS).
Perfect for examples.

### 5. Why use simple TCP client/server?

Consistent, predictable write() calls make tracing easy to demo.
