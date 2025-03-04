# Memory Bucket Optimizer Visualization

This directory contains gnuplot scripts to visualize the memory allocation patterns and optimization results from the Memory Bucket Optimizer tool.

## Scripts

- `allocation_histogram.gp`: Visualizes allocation sizes and frequencies
- `bucket_optimization.gp`: Visualizes bucket sizes and waste
- `tls_comparison.gp`: Compares memory usage for different TLS operations
- `memory_usage_over_time.gp`: Visualizes memory usage over time for different bucket configurations
- `memory_heatmap.gp`: Creates a heatmap of memory usage by bucket size and operation
- `generate_data.sh`: Generates data files for gnuplot

## Usage

To generate the visualization plots:

```bash
cd visualization
./generate_data.sh
```

This will generate the following plots:

- `allocation_histogram.png`: Histogram of allocation sizes and frequencies
- `bucket_optimization.png`: Visualization of bucket sizes and waste
- `tls_comparison.png`: Comparison of memory usage for different TLS operations
- `memory_usage_over_time.png`: Memory usage over time for different bucket configurations
- `memory_heatmap.png`: Heatmap of memory usage by bucket size and operation

## Data Files

The scripts generate the following data files:

- `allocation_data.txt`: Allocation sizes and frequencies
- `bucket_data.txt`: Bucket sizes, counts, waste, and distribution
- `tls_comparison.txt`: Memory usage comparison for different TLS operations
- `memory_usage_over_time.txt`: Memory usage over time for different bucket configurations
- `memory_heatmap.txt`: Memory usage by bucket size and operation
