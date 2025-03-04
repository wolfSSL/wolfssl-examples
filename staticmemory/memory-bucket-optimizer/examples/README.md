# Memory Bucket Optimizer Examples

This directory contains examples demonstrating how to use optimized memory bucket configurations with wolfSSL's static memory feature.

## Examples

1. `example_application.c`: Basic example showing how to use static memory with optimized bucket configurations
2. `tls_example.c`: Example demonstrating optimized bucket configurations for TLS 1.2 and TLS 1.3

## Building

```bash
make
```

## Running

1. Basic Example:
   ```bash
   ./example_application
   ```

2. TLS Example:
   ```bash
   ./tls_example
   ```

## Requirements

- wolfSSL built with `--enable-staticmemory`
- GNU Make
- GCC

## Notes

- The examples demonstrate different bucket configurations optimized for specific use cases
- Memory usage statistics are provided to show the effectiveness of the optimizations
- Both examples handle the case where static memory is not enabled in wolfSSL
