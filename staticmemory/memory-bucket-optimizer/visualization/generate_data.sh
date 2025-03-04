#!/bin/bash

# Script to generate data files for gnuplot visualization

# Check if results directory exists
if [ ! -d "../verification_results" ]; then
    echo "Error: verification_results directory not found"
    exit 1
fi

# Create allocation data file
echo "# Allocation Size Frequency" > allocation_data.txt
grep -A 100 "Allocation Sizes and Frequencies:" ../verification_results/tls12_google_buckets.txt | grep -A 100 "Size" | grep -v "Size" | grep -v "----" | grep -v "^$" | grep -v "Optimized" | awk '{print $1, $2}' > allocation_data.txt

# Create bucket data file
echo "# Bucket Size Count Waste Dist" > bucket_data.txt
grep -A 100 "Optimized Bucket Sizes and Distribution:" ../verification_results/tls12_google_buckets.txt | grep -A 100 "Size" | grep -v "Size" | grep -v "----" | grep -v "^$" | grep -v "WOLFMEM" | awk '{print $1, $2, $3, $4}' > bucket_data.txt

# Create TLS comparison data file
echo "# TLS Operation Total_Allocs Unique_Sizes Largest_Bucket Total_Waste" > tls_comparison.txt
cat ../verification_results/summary.csv | grep -v "Test Name" | sed 's/,/ /g' > tls_comparison.txt

# Create sample memory usage over time data file (this would be replaced with actual data)
echo "# Time Default_Config Optimized_Config" > memory_usage_over_time.txt
echo "0 0 0" >> memory_usage_over_time.txt
echo "1 1000 800" >> memory_usage_over_time.txt
echo "2 2000 1600" >> memory_usage_over_time.txt
echo "3 3000 2400" >> memory_usage_over_time.txt
echo "4 4000 3200" >> memory_usage_over_time.txt
echo "5 5000 4000" >> memory_usage_over_time.txt

# Create sample memory heatmap data file (this would be replaced with actual data)
echo "# BucketSize Operation MemoryUsage" > memory_heatmap.txt
echo "16 1 1000" >> memory_heatmap.txt
echo "32 1 2000" >> memory_heatmap.txt
echo "64 1 3000" >> memory_heatmap.txt
echo "128 1 4000" >> memory_heatmap.txt
echo "16 2 800" >> memory_heatmap.txt
echo "32 2 1600" >> memory_heatmap.txt
echo "64 2 2400" >> memory_heatmap.txt
echo "128 2 3200" >> memory_heatmap.txt
echo "16 3 600" >> memory_heatmap.txt
echo "32 3 1200" >> memory_heatmap.txt
echo "64 3 1800" >> memory_heatmap.txt
echo "128 3 2400" >> memory_heatmap.txt
echo "16 4 400" >> memory_heatmap.txt
echo "32 4 800" >> memory_heatmap.txt
echo "64 4 1200" >> memory_heatmap.txt
echo "128 4 1600" >> memory_heatmap.txt

# Generate the plots
gnuplot allocation_histogram.gp
gnuplot bucket_optimization.gp
gnuplot tls_comparison.gp
gnuplot memory_usage_over_time.gp
gnuplot memory_heatmap.gp

echo "Data files and plots generated successfully"
