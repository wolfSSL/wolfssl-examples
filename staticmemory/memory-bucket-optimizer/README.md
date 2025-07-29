# Memory Bucket Optimizer for wolfSSL

** Note that the staticmemory build of wolfSSL does not use less memory **
The staticmemory feature ends up using a bit more memory and is a simple sectioning up of a static buffer used dynamically instead of malloc/free. wolfSSL has the option for users to define custom XMALLOC/XFREE if wanting to use a different allocater.


This tool analyzes memory allocation patterns in wolfSSL and recommends optimal static memory bucket configurations to minimize wasted memory.

## Overview

When wolfSSL is built with the `--enable-staticmemory` option, it uses a static memory management system with memory buckets. The size and distribution of these buckets can significantly impact memory usage efficiency. This tool helps optimize these bucket configurations for specific TLS operations.

## Directory Structure

```
memory-bucket-optimizer/
├── optimizer/ # Source code for the optimizer
└── README.md               # This file
```

## Prerequisites

- wolfSSL (built with `CPPFLAGS="-DWOLFSSL_DEBUG_MEMORY -DWOLFSSL_DEBUG_MEMORY_PRINT` and `--enable-staticmemory`)
- GCC compiler
- gnuplot (for visualization)

## Building

```bash
make
```

## Usage

### Basic Usage

1. Build wolfSSL with memory logging enabled:

```bash
cd ~/wolfssl
./configure CPPFLAGS="-DWOLFSSL_DEBUG_MEMORY -DWOLFSSL_DEBUG_MEMORY_PRINT" --enable-staticmemory
make
sudo make install
```

2. Run the application linked to wolfssl:

```bash
./wolfcrypt/test/testwolfcrypt &> testwolfcrypt.log
```

This will run the application with memory log output.

3. Run the log through the optimizer

```bash
cd optimizer
make
./memory_bucket_optimizer testwolfcrypt.log
```

