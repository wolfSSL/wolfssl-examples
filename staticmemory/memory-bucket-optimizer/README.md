# Memory Bucket Optimizer for wolfSSL Static Memory

This tool analyzes memory allocation patterns from wolfSSL operations and recommends optimal static memory bucket configurations to minimize wasted memory.

## Overview

wolfSSL can be built with the `--enable-staticmemory` option, which uses a buffer divided into chunks of memory (buckets) that can be checked out dynamically. The size of these memory buckets are configurable, and the overhead of each bucket is wasted memory. This tool helps find a set of memory buckets with the least amount of wasted overhead.

## Stack Components

| Component | Version | Description | License |
|-----------|---------|-------------|---------|
| wolfSSL | 5.6.3+ | TLS/SSL and crypto library | GPLv2 |
| Memory Bucket Optimizer | 1.0.0 | Memory optimization tool | GPLv2 |
| gnuplot (optional) | 5.2+ | Plotting utility | GPLv2 |
| gcc | 9.0+ | C compiler | GPLv3 |
| bash | 5.0+ | Shell scripting | GPLv3 |

## Building

```bash
# Build wolfSSL with memory logging enabled
cd ../wolfssl
./configure --enable-memorylog --enable-staticmemory
make

# Build the memory bucket optimizer
cd ../wolfssl-examples/staticmemory/memory-bucket-optimizer
make
```

## Usage

### Basic Usage

```bash
# Run the optimizer with default settings
./run_optimizer.sh

# Run the optimizer with custom host and port
./run_optimizer.sh -h example.com -p 443
```

### Testing Multiple TLS Operations

```bash
# Run tests for different TLS operations
./run_multiple.sh
```

### Visualizing Results

```bash
# Generate visualization plots
cd visualization
./generate_data.sh
```

## Example Output

```
Found 78 unique allocation sizes

Allocation Sizes and Frequencies:
Size    Count
----    -----
4       4
5       2
...
8368    2

Optimized Bucket Sizes and Distribution:
Size    Count   Wasted  Dist
----    -----   ------  ----
16      2       4.00    2
22      3       10.00   3
...
8368    8       818.00  8

WOLFMEM_BUCKETS and WOLFMEM_DIST Macros:
#define WOLFMEM_BUCKETS 16,22,30,40,86,133,184,256,344,512,864,1248,1812,3128,5518,8368
#define WOLFMEM_DIST 2,3,7,7,7,7,7,7,7,7,7,7,8,8,8,8
```

## Directory Structure

```
memory-bucket-optimizer/
├── Makefile                  # Main Makefile
├── README.md                 # This file
├── examples/                 # Example applications
│   ├── Makefile              # Examples Makefile
│   ├── example_application.c # Basic example
│   └── tls_example.c         # TLS example
├── run_multiple.sh           # Script to run multiple tests
├── run_optimizer.sh          # Main script to run the optimizer
├── src/                      # Source code
│   ├── Makefile              # Source Makefile
│   └── memory_bucket_optimizer.c # Main optimizer code
├── test_operations.sh        # Script to test different TLS operations
└── visualization/            # Visualization scripts
    ├── allocation_histogram.gp    # Allocation histogram plot
    ├── bucket_optimization.gp     # Bucket optimization plot
    ├── generate_data.sh           # Data generation script
    ├── memory_heatmap.gp          # Memory usage heatmap
    ├── memory_usage_over_time.gp  # Memory usage over time plot
    └── tls_comparison.gp          # TLS comparison plot
```

## Software Bill of Materials (SBOM)

| Component | Version | Source | License | Purpose |
|-----------|---------|--------|---------|---------|
| wolfSSL | 5.6.3+ | https://github.com/wolfSSL/wolfssl | GPLv2 | TLS/SSL and crypto library |
| Memory Bucket Optimizer | 1.0.0 | This repository | GPLv2 | Memory optimization tool |
| gnuplot | 5.2+ | http://www.gnuplot.info/ | GPLv2 | Visualization of memory usage |
| gcc | 9.0+ | https://gcc.gnu.org/ | GPLv3 | Compilation of C code |
| bash | 5.0+ | https://www.gnu.org/software/bash/ | GPLv3 | Shell scripting |
| make | 4.0+ | https://www.gnu.org/software/make/ | GPLv3 | Build automation |

## Algorithm

The memory bucket optimizer uses the following algorithm:

1. Parse memory allocation logs from wolfSSL operations
2. Identify unique allocation sizes and their frequencies
3. Sort allocation sizes from smallest to largest
4. Calculate optimal bucket sizes to minimize waste
5. Generate `WOLFMEM_BUCKETS` and `WOLFMEM_DIST` macros

## License

This project is licensed under the GPL v2 License - see the LICENSE file for details.

## References

- [wolfSSL Static Memory Documentation](https://www.wolfssl.com/documentation/manuals/wolfssl/chapter02.html#static-memory)
- [wolfSSL Memory Logging](https://www.wolfssl.com/documentation/manuals/wolfssl/chapter02.html#memory-use)
