# Memory Bucket Optimizer Visualization

This directory contains scripts for visualizing memory allocation patterns and bucket optimization results.

## Visualization Scripts

1. `allocation_histogram.gp`: Generates a histogram of allocation sizes and frequencies
2. `bucket_optimization.gp`: Visualizes bucket sizes and waste
3. `tls_comparison.gp`: Compares memory usage across different TLS operations
4. `memory_usage_over_time.gp`: Tracks memory usage over time
5. `memory_heatmap.gp`: Visualizes memory usage patterns

## Usage

```bash
# Generate all visualization plots
./generate_data.sh

# Generate a specific plot
gnuplot allocation_histogram.gp
```

## Output Files

The scripts generate PNG image files:

- `allocation_histogram.png`: Histogram of allocation sizes
- `bucket_optimization.png`: Bucket sizes and waste
- `tls_comparison.png`: Comparison of TLS operations
- `memory_usage_over_time.png`: Memory usage over time
- `memory_heatmap.png`: Memory usage heatmap

## Data Files

The scripts use data files generated from the memory bucket optimizer results:

- `allocation_sizes.txt`: Allocation sizes and frequencies
- `bucket_sizes.txt`: Bucket sizes and waste
- `tls_comparison.txt`: Memory usage for different TLS operations
- `memory_usage.txt`: Memory usage over time
- `memory_heatmap.txt`: Memory usage patterns

## Requirements

- gnuplot 5.2 or later
- bash 5.0 or later
