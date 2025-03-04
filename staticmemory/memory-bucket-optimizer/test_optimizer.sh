#!/bin/bash
#
# Copyright (C) 2025 wolfSSL Inc.
#
# This file is part of wolfSSL.
#
# wolfSSL is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# wolfSSL is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA


# Script to test the memory bucket optimizer with different TLS operations

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Check if wolfSSL directory is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <wolfssl_dir>"
    echo "Example: $0 ~/repos/wolfssl"
    exit 1
else
    WOLFSSL_DIR="$1"
fi

# Check if wolfSSL directory exists
if [ ! -d "$WOLFSSL_DIR" ]; then
    echo "Error: wolfSSL directory not found at $WOLFSSL_DIR"
    echo "Please provide a valid wolfSSL directory."
    exit 1
fi

# Set up directories
RESULTS_DIR="$SCRIPT_DIR/results"

# Create results directory
mkdir -p "$RESULTS_DIR"

# Build wolfSSL with memory logging enabled
cd "$WOLFSSL_DIR" || exit 1
./autogen.sh && ./configure --enable-memorylog --enable-staticmemory && make

# Function to run a test and collect memory usage data
run_test() {
    local test_name="$1"
    local host="$2"
    local port="$3"
    local extra_args="$4"
    
    echo "Running test: $test_name"
    echo "Host: $host, Port: $port, Extra args: $extra_args"
    
    # Run the example client with memory logging
    $WOLFSSL_DIR/examples/client/client -h "$host" -d -p "$port" -g $extra_args > "$RESULTS_DIR/${test_name}_output.txt" 2>&1
    
    # Extract memory allocation logs
    grep "^Alloc:" "$RESULTS_DIR/${test_name}_output.txt" > "$RESULTS_DIR/${test_name}_memory.txt"
    
    # Run the memory bucket optimizer
    cd $SCRIPT_DIR/src || exit 1
    ./memory_bucket_optimizer "$RESULTS_DIR/${test_name}_memory.txt" > "$SCRIPT_DIR/$RESULTS_DIR/${test_name}_buckets.txt"
    
    # Return to script directory
    cd $SCRIPT_DIR || exit 1
}

# Run tests for different TLS operations
run_test "tls12_google" "google.com" "443" ""
run_test "tls13_google" "google.com" "443" "-v 4"
run_test "tls12_cloudflare" "cloudflare.com" "443" ""
run_test "tls13_cloudflare" "cloudflare.com" "443" "-v 4"

# Generate a summary of the results
echo "Generating summary..."
echo "Test Name,Total Allocs,Unique Sizes,Largest Bucket,Total Waste" > "$RESULTS_DIR/summary.csv"

for result_file in "$RESULTS_DIR"/*_buckets.txt; do
    test_name=$(basename "$result_file" _buckets.txt)
    alloc_file="$RESULTS_DIR/${test_name}_memory.txt"
    
    total_allocs=$(grep -c "^Alloc:" "$alloc_file" || echo "0")
    unique_sizes=$(grep "Found .* unique allocation sizes" "$result_file" | awk '{print $2}' || echo "0")
    largest_bucket=$(grep -A 3 "Optimized Bucket Sizes" "$result_file" | tail -n 1 | awk '{print $1}' || echo "0")
    total_waste=$(grep -A 3 "Optimized Bucket Sizes" "$result_file" | tail -n 1 | awk '{print $3}' || echo "0")
    
    echo "$test_name,$total_allocs,$unique_sizes,$largest_bucket,$total_waste" >> "$RESULTS_DIR/summary.csv"
done

echo "All tests completed. Results saved in $RESULTS_DIR/"
echo "Summary saved to $RESULTS_DIR/summary.csv"

# Compare the different bucket configurations
echo "Comparing bucket configurations for different TLS operations..."
echo

for result_file in "$RESULTS_DIR"/*_buckets.txt; do
    test_name=$(basename "$result_file" _buckets.txt)
    buckets=$(grep "^#define WOLFMEM_BUCKETS" "$result_file" | sed 's/#define WOLFMEM_BUCKETS //' || echo "")
    dist=$(grep "^#define WOLFMEM_DIST" "$result_file" | sed 's/#define WOLFMEM_DIST //' || echo "")
    
    echo "$test_name:"
    echo "  WOLFMEM_BUCKETS: $buckets"
    echo "  WOLFMEM_DIST: $dist"
    echo
done
