#!/bin/bash
# run_optimizer.sh
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
HOST="google.com"
PORT="443"
RESULTS_DIR="results"

# Print usage
usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -w, --wolfssl-dir <dir>  Path to wolfSSL directory (default: $WOLFSSL_DIR)"
    echo "  -h, --host <host>        Host to connect to (default: $HOST)"
    echo "  -p, --port <port>        Port to connect to (default: $PORT)"
    echo "  --help                   Show this help message"
    echo
    echo "Example:"
    echo "  $0 -h google.com -p 443"
    exit 1
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -w|--wolfssl-dir)
            WOLFSSL_DIR="$2"
            shift 2
            ;;
        -h|--host)
            HOST="$2"
            shift 2
            ;;
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        --help)
            usage
            ;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done

# Create results directory
mkdir -p "$RESULTS_DIR"

# Build wolfSSL with memory logging enabled
echo "Building wolfSSL with memory logging..."
cd "$WOLFSSL_DIR" || exit 1
./autogen.sh
./configure --enable-memorylog --enable-staticmemory
make

# Run the example client and collect memory logs
echo "Running example client..."
./examples/client/client -h "$HOST" -d -p "$PORT" -g > "../wolfssl-examples/staticmemory/memory-bucket-optimizer/$RESULTS_DIR/client_log.txt" 2>&1

# Extract memory allocation logs
cd "../wolfssl-examples/staticmemory/memory-bucket-optimizer" || exit 1
grep "^Alloc:" "$RESULTS_DIR/client_log.txt" > "$RESULTS_DIR/memory_log.txt"

# Run the memory bucket optimizer
echo "Running memory bucket optimizer..."
cd src || exit 1
make
./memory_bucket_optimizer "../../$RESULTS_DIR/memory_log.txt" > "../../$RESULTS_DIR/optimized_buckets.txt"

# Generate visualization plots
echo "Generating visualization plots..."
cd ../../visualization || exit 1
./generate_data.sh

echo "Optimization complete. Results saved in $RESULTS_DIR/"
echo "Optimized bucket configuration can be found in $RESULTS_DIR/optimized_buckets.txt"
echo "Visualization plots can be found in visualization/*.png"
