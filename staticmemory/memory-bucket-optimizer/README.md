# Memory Bucket Optimizer for wolfSSL

** Note that the staticmemory build of wolfSSL does not use less memory **
The staticmemory feature ends up using a bit more memory and is a simple sectioning up of a static buffer used dynamically instead of malloc/free. wolfSSL has the option for users to define custom XMALLOC/XFREE if wanting to use a different allocater.


The exact optimized configuration is a difficult problem (think traveling salesman problem), but this optimizer gives a good starting point. It uses a simple algorithm in that it fill the first half of bucket sizes with largest allocations and the second half based on max concurrent uses.

## Directory Structure

```
memory-bucket-optimizer/
├── optimizer/ # Source code for the optimizer
├── tester/ # Source code for testing configuration
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

4. Build and run tester (optional)

```
cd ~/wolfssl
./configure CPPFLAGS="-DWOLFSSL_NO_MALLOC -DWOLFSSL_DEBUG_MEMORY -DWOLFSSL_DEBUG_MEMORY_PRINT" --enable-staticmemory
make && sudo make install
cd tester && make
./memory_bucket_tester ../testwolfcrypt.log --buckets "289,832,1056,1072,1152,1616,1632,3160,4240" --dist "2,1,2,1,1,1,1,19,1" --buffer-size 74298
```

