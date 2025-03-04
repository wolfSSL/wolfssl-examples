# Memory Bucket Optimizer for wolfSSL

This tool analyzes memory allocation patterns in wolfSSL and recommends optimal static memory bucket configurations to minimize wasted memory.

## Overview

When wolfSSL is built with the `--enable-staticmemory` option, it uses a static memory management system with memory buckets. The size and distribution of these buckets can significantly impact memory usage efficiency. This tool helps optimize these bucket configurations for specific TLS operations.

## Software Bill of Materials (SBOM)

| Component | Description | License |
|-----------|-------------|---------|
| Memory Bucket Optimizer | Core optimization tool | GPLv2 |
| wolfSSL | TLS/SSL library | GPLv2 |
| gnuplot | Plotting utility | GPLv2 |
| Bash Scripts | Automation scripts | GPLv2 |
| Example Applications | Demo applications | GPLv2 |

## Directory Structure

```
memory-bucket-optimizer/
├── src/                    # Source code for the optimizer
├── results/                # Results of optimization runs
├── examples/               # Example applications using optimized buckets
├── visualization/          # Visualization scripts and plots
├── run_multiple.sh         # Script to run tests for multiple TLS operations
├── compare_memory.sh       # Script to compare memory usage
└── README.md               # This file
```

## Prerequisites

- wolfSSL (built with `--enable-memorylog` and `--enable-staticmemory`)
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
cd ~/repos/wolfssl
./configure --enable-memorylog --enable-staticmemory && make
```

2. Run the optimizer:

```bash
./run_multiple.sh
```

This will:
- Run the example client with different TLS operations
- Collect memory allocation logs
- Generate optimized bucket configurations for each operation
- Create visualization plots

### Advanced Usage

To optimize for a specific TLS operation:

```bash
./src/memory_bucket_optimizer results/tls13_google_memory.txt > results/tls13_google_buckets.txt
```

To compare memory usage between default and optimized configurations:

```bash
./compare_memory.sh
```

## Visualization

The `visualization` directory contains scripts to generate plots:

- Allocation size histograms
- Bucket optimization plots
- TLS operation comparisons

To generate plots:

```bash
cd visualization
./generate_data.sh
```

## Example Applications

The `examples` directory contains example applications that demonstrate how to use the optimized bucket configurations in your wolfSSL applications.

## Testing with WOLFSSL_NO_MALLOC

To ensure that the optimized bucket configurations work correctly without falling back to system malloc, you can use the `test_with_no_malloc.sh` script:

```bash
./test_with_no_malloc.sh ~/repos/wolfssl tls13_google
```

This script:
- Builds wolfSSL with `CPPFLAGS=-DWOLFSSL_NO_MALLOC` and `--enable-staticmemory`
- Creates a test application with the optimized bucket configuration
- Runs the test application to verify that it works correctly without system malloc
- Confirms that the memory bucket optimization is effective for embedded systems

This testing is critical for embedded systems where dynamic memory allocation is not available or not desired.

## Algorithm

The memory bucket optimizer uses the following algorithm:

1. Parse memory allocation logs to identify allocation sizes and frequencies
2. Sort allocation sizes from smallest to largest
3. Select the most frequent allocation sizes as bucket sizes
4. Assign distribution values based on allocation frequency
5. Calculate memory waste and optimize for minimal overhead

## License

This project is licensed under the GPL v2.0 License - see the LICENSE file for details.
