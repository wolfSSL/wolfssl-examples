# Memory Bucket Optimizer for wolfSSL Static Memory

This tool analyzes memory allocation patterns from wolfSSL operations and recommends optimal static memory bucket configurations to minimize wasted memory.

## Software Bill of Materials (SBOM)

| Component | Version | License | Purpose |
|-----------|---------|---------|---------|
| wolfSSL | 5.6.6 | GPLv2 | TLS/SSL library with static memory support |
| gnuplot | 5.4+ | gnuplot license | Data visualization |
| gcc | 9.4+ | GPLv3 | C compiler |
| GNU Make | 4.2+ | GPLv3 | Build system |

## Stack Components

1. **Memory Bucket Optimizer**
   - Analyzes memory allocation logs
   - Recommends optimal bucket sizes
   - Generates visualization plots

2. **wolfSSL Integration**
   - Uses wolfSSL's static memory feature
   - Configures memory buckets via WOLFMEM_BUCKETS and WOLFMEM_DIST
   - Requires wolfSSL built with --enable-staticmemory

3. **Visualization Tools**
   - gnuplot scripts for data visualization
   - Memory usage analysis plots
   - Bucket optimization charts

## Building and Running

1. Build wolfSSL with static memory and logging:
   ```bash
   cd ../../../wolfssl
   ./autogen.sh
   ./configure --enable-staticmemory --enable-memorylog
   make
   ```

2. Build the memory bucket optimizer:
   ```bash
   cd ../wolfssl-examples/staticmemory/memory-bucket-optimizer
   make
   ```

3. Run the optimizer:
   ```bash
   ./run_optimizer.sh
   ```

4. Generate visualization plots:
   ```bash
   cd visualization
   ./generate_data.sh
   ```

## Directory Structure

```
memory-bucket-optimizer/
├── src/                    # Source code
├── visualization/          # Visualization scripts
├── examples/              # Example applications
├── Makefile              # Build system
├── run_optimizer.sh      # Main script
└── README.md             # This file
```

## Usage

1. **Basic Usage**
   ```bash
   ./run_optimizer.sh
   ```

2. **Optimize for Multiple TLS Operations**
   ```bash
   ./optimize_multiple.sh
   ```

3. **Verify Optimization Results**
   ```bash
   ./verify_optimization.sh
   ```

## Output

The tool generates:
1. Optimized bucket configurations (WOLFMEM_BUCKETS and WOLFMEM_DIST)
2. Memory usage analysis plots
3. Comparison of different TLS operations
4. Memory allocation pattern visualizations

## Example Output

```c
/* Optimized bucket configuration */
#define WOLFMEM_BUCKETS 16,22,30,40,86,133,184,256,344,512,864,1248,1812,3128,5518,8368
#define WOLFMEM_DIST    2,3,7,7,7,7,7,7,7,7,7,7,8,8,8,8
```

## License

This example is part of wolfSSL examples and is licensed under the same terms as wolfSSL.
See the LICENSE file in the wolfSSL root directory for details.
