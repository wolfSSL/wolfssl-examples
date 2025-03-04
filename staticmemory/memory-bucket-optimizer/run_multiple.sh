#!/bin/bash
# run_multiple.sh
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

# Test configurations
declare -A TESTS=(
    ["tls12_google"]="google.com:443"
    ["tls13_google"]="google.com:443:-v 4"
    ["tls12_cloudflare"]="cloudflare.com:443"
    ["tls13_cloudflare"]="cloudflare.com:443:-v 4"
)

# Create results directory
mkdir -p "$RESULTS_DIR"

# Build wolfSSL with memory logging enabled
echo "Building wolfSSL with memory logging..."
cd "$WOLFSSL_DIR" || exit 1
./autogen.sh
./configure --enable-memorylog --enable-staticmemory
make

# Run tests for each configuration
for test_name in "${!TESTS[@]}"; do
    IFS=':' read -r host port extra_args <<< "${TESTS[$test_name]}"
    
    echo "Running test: $test_name"
    echo "Host: $host, Port: $port, Extra args: $extra_args"
    
    # Run the example client
    ./examples/client/client -h "$host" -d -p "$port" $extra_args -g > \
        "$RESULTS_DIR/${test_name}_log.txt" 2>&1
    
    # Extract memory allocation logs
    cd "$SCRIPT_DIR" || exit 1
    grep "^Alloc:" "$RESULTS_DIR/${test_name}_log.txt" > "$RESULTS_DIR/${test_name}_memory.txt"
    
    # Run the memory bucket optimizer
    cd "$SCRIPT_DIR/src" || exit 1
    ./memory_bucket_optimizer "$RESULTS_DIR/${test_name}_memory.txt" > \
        "$RESULTS_DIR/${test_name}_buckets.txt"
    
    # Return to wolfSSL directory for next test
    cd "$WOLFSSL_DIR" || exit 1
done

# Generate visualization plots
cd "$SCRIPT_DIR/visualization" || exit 1
./generate_data.sh "$WOLFSSL_DIR"

echo "All tests completed. Results saved in $RESULTS_DIR/"
echo "Visualization plots can be found in visualization/*.png"
