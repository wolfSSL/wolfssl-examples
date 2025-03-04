# Memory Bucket Optimizer Visualization

This directory contains scripts and data files for visualizing memory bucket optimization results.

## Visualization Scripts

- `generate_data.sh`: Main script to generate data files and plots
- `allocation_histogram.gp`: Gnuplot script for allocation size histogram
- `bucket_optimization.gp`: Gnuplot script for bucket optimization visualization
- `tls_comparison.gp`: Gnuplot script for TLS operation comparison

## Generated Plots

- `tls12_google_allocation_histogram.png`: Allocation size distribution for TLS 1.2 connection to Google
- `tls13_google_allocation_histogram.png`: Allocation size distribution for TLS 1.3 connection to Google
- `tls12_cloudflare_allocation_histogram.png`: Allocation size distribution for TLS 1.2 connection to Cloudflare
- `tls13_cloudflare_allocation_histogram.png`: Allocation size distribution for TLS 1.3 connection to Cloudflare
- `tls12_google_bucket_optimization.png`: Bucket optimization for TLS 1.2 connection to Google
- `tls13_google_bucket_optimization.png`: Bucket optimization for TLS 1.3 connection to Google
- `tls12_cloudflare_bucket_optimization.png`: Bucket optimization for TLS 1.2 connection to Cloudflare
- `tls13_cloudflare_bucket_optimization.png`: Bucket optimization for TLS 1.3 connection to Cloudflare
- `tls_comparison.png`: Comparison of memory usage for different TLS operations

## Data Files

The `data` directory contains intermediate data files used for generating the plots.

## Usage

To generate all plots, run:

```bash
./generate_data.sh
```

This will process the memory logs and bucket configurations in the `results` directory and generate the plots in this directory.
