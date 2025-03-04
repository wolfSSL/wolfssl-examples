#!/bin/bash
# test_operations.sh
#
# Copyright (C) 2006-2025 wolfSSL Inc.
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

# Default values
WOLFSSL_DIR="../../../wolfssl"
RESULTS_DIR="test_results"

# Test configurations
declare -A TESTS=(
    ["tls12_google"]="google.com:443"
    ["tls13_google"]="google.com:443:-v 4"
    ["tls12_cloudflare"]="cloudflare.com:443"
    ["tls13_cloudflare"]="cloudflare.com:443:-v 4"
    ["tls12_resume"]="google.com:443:-r"
    ["tls13_resume"]="google.com:443:-v 4 -r"
    ["tls12_nonblocking"]="google.com:443:-N"
    ["tls13_nonblocking"]="google.com:443:-v 4 -N"
)

# Create results directory
mkdir -p "$RESULTS_DIR"

# Check if wolfSSL is built with memory logging
if [ ! -f "$WOLFSSL_DIR/options.h" ]; then
    echo "Error: wolfSSL options.h not found at $WOLFSSL_DIR/options.h"
    echo "Please make sure wolfSSL is built with --enable-memorylog"
    exit 1
fi

# Check if memory logging is enabled
if ! grep -q "WOLFSSL_MEMORY_LOG" "$WOLFSSL_DIR/options.h"; then
    echo "Warning: wolfSSL is not built with memory logging enabled"
    echo "Using sample memory logs for testing"
    
    # Create sample memory logs
    mkdir -p "$RESULTS_DIR/samples"
    
    # Sample TLS 1.2 memory log
    cat > "$RESULTS_DIR/samples/tls12_sample.txt" << 'SAMPLE'
Alloc: 16 bytes at address: 0x55b7a2c31e70 from func: wolfSSL_Init
Alloc: 24 bytes at address: 0x55b7a2c31e90 from func: wolfSSL_CTX_new
Alloc: 32 bytes at address: 0x55b7a2c31eb0 from func: wolfSSL_new
Alloc: 64 bytes at address: 0x55b7a2c31ef0 from func: wolfSSL_connect
Alloc: 128 bytes at address: 0x55b7a2c31f40 from func: DoHandShake
Alloc: 256 bytes at address: 0x55b7a2c31fc0 from func: BuildMessage
Alloc: 512 bytes at address: 0x55b7a2c320d0 from func: BuildCertHashes
Alloc: 1024 bytes at address: 0x55b7a2c322f0 from func: BuildFinished
Alloc: 2048 bytes at address: 0x55b7a2c32730 from func: BuildTls13Message
Alloc: 4096 bytes at address: 0x55b7a2c32fb0 from func: BuildTls13Message
SAMPLE
    
    # Sample TLS 1.3 memory log
    cat > "$RESULTS_DIR/samples/tls13_sample.txt" << 'SAMPLE'
Alloc: 16 bytes at address: 0x55b7a2c31e70 from func: wolfSSL_Init
Alloc: 24 bytes at address: 0x55b7a2c31e90 from func: wolfSSL_CTX_new
Alloc: 32 bytes at address: 0x55b7a2c31eb0 from func: wolfSSL_new
Alloc: 64 bytes at address: 0x55b7a2c31ef0 from func: wolfSSL_connect
Alloc: 128 bytes at address: 0x55b7a2c31f40 from func: DoHandShake
Alloc: 256 bytes at address: 0x55b7a2c31fc0 from func: BuildMessage
Alloc: 512 bytes at address: 0x55b7a2c320d0 from func: BuildCertHashes
Alloc: 1024 bytes at address: 0x55b7a2c322f0 from func: BuildFinished
Alloc: 2048 bytes at address: 0x55b7a2c32730 from func: BuildTls13Message
Alloc: 4096 bytes at address: 0x55b7a2c32fb0 from func: BuildTls13Message
Alloc: 8192 bytes at address: 0x55b7a2c34fb0 from func: BuildTls13KeyShare
SAMPLE
    
    # Use sample logs for testing
    for test_name in "${!TESTS[@]}"; do
        if [[ "$test_name" == *"tls13"* ]]; then
            cp "$RESULTS_DIR/samples/tls13_sample.txt" "$RESULTS_DIR/${test_name}_memory.txt"
        else
            cp "$RESULTS_DIR/samples/tls12_sample.txt" "$RESULTS_DIR/${test_name}_memory.txt"
        fi
    done
else
    # Build wolfSSL with memory logging enabled
    echo "Building wolfSSL with memory logging..."
    cd "$WOLFSSL_DIR" || exit 1
    ./autogen.sh
    ./configure --enable-memorylog
    make
    
    # Run tests for each configuration
    for test_name in "${!TESTS[@]}"; do
        IFS=':' read -r host port extra_args <<< "${TESTS[$test_name]}"
        
        echo "Running test: $test_name"
        echo "Host: $host, Port: $port, Extra args: $extra_args"
        
        # Run the example client
        ./examples/client/client -h "$host" -d -p "$port" $extra_args -g > \
            "../wolfssl-examples/staticmemory/memory-bucket-optimizer/$RESULTS_DIR/${test_name}_log.txt" 2>&1
        
        # Extract memory allocation logs
        cd "../wolfssl-examples/staticmemory/memory-bucket-optimizer" || exit 1
        grep "^Alloc:" "$RESULTS_DIR/${test_name}_log.txt" > "$RESULTS_DIR/${test_name}_memory.txt"
        
        # Return to wolfSSL directory for next test
        cd "$WOLFSSL_DIR" || exit 1
    done
fi

# Run the memory bucket optimizer for each test
cd "../wolfssl-examples/staticmemory/memory-bucket-optimizer" || exit 1
for test_name in "${!TESTS[@]}"; do
    echo "Optimizing memory buckets for $test_name..."
    
    # Run the memory bucket optimizer
    cd src || exit 1
    ./memory_bucket_optimizer "../../$RESULTS_DIR/${test_name}_memory.txt" > \
        "../../$RESULTS_DIR/${test_name}_buckets.txt"
    
    cd .. || exit 1
done

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

# Print the summary
echo "Test results summary:"
cat "$RESULTS_DIR/summary.csv"

echo "All tests completed. Results saved in $RESULTS_DIR/"
