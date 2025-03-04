# Memory Bucket Optimizer Source Code

This directory contains the source code for the Memory Bucket Optimizer tool.

## Software Bill of Materials (SBOM)

| Component | Description | License |
|-----------|-------------|---------|
| Memory Bucket Optimizer | Core optimization tool | GPLv2 |
| GCC | C compiler | GPLv3 |

## Stack Components

The Memory Bucket Optimizer tool analyzes memory allocation patterns in wolfSSL and recommends optimal static memory bucket configurations to minimize wasted memory. The stack components include:

1. **Memory Bucket Optimizer**: The core optimization tool
2. **GCC**: The C compiler used to build the tool

## Building

To build the Memory Bucket Optimizer tool, run:

```bash
cd ~/repos/wolfssl-examples/staticmemory/memory-bucket-optimizer
make
```

## Usage

To use the Memory Bucket Optimizer tool, run:

```bash
./src/memory_bucket_optimizer <memory_log_file> > <output_file>
```

For example:

```bash
./src/memory_bucket_optimizer results/tls13_google_memory.txt > results/tls13_google_buckets.txt
```

This will analyze the memory allocation patterns in the memory log file and generate optimized bucket configurations in the output file.

## Algorithm

The memory bucket optimizer uses the following algorithm:

1. Parse memory allocation logs to identify allocation sizes and frequencies
2. Sort allocation sizes from smallest to largest
3. Select the most frequent allocation sizes as bucket sizes
4. Assign distribution values based on allocation frequency
5. Calculate memory waste and optimize for minimal overhead
