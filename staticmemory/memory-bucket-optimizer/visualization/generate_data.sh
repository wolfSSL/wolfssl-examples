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


# Script to generate data files and plots for memory bucket optimization

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="$SCRIPT_DIR/../results"

# Check if wolfSSL directory is provided
if [ -n "$1" ]; then
    WOLFSSL_DIR="$1"
    echo "Using wolfSSL directory: $WOLFSSL_DIR"
fi

# Create data directory
mkdir -p "$SCRIPT_DIR/data"

# Function to extract allocation sizes and frequencies from memory logs
extract_allocation_data() {
    local memory_log="$1"
    local output_file="$2"
    
    # Extract allocation sizes and count occurrences
    grep "^Alloc:" "$memory_log" | sed -n 's/.*-> \([0-9]\+\).*/\1/p' | sort -n | uniq -c > "$output_file"
}

# Process each TLS operation
for test_name in tls12_google tls13_google tls12_cloudflare tls13_cloudflare; do
    memory_log="$RESULTS_DIR/${test_name}_memory.txt"
    buckets_file="$RESULTS_DIR/${test_name}_buckets.txt"
    
    if [ -s "$memory_log" ]; then
        # Extract allocation data
        extract_allocation_data "$memory_log" "$SCRIPT_DIR/data/${test_name}_alloc_data.txt"
        
        # Create allocation histogram gnuplot script
        cat > "$SCRIPT_DIR/${test_name}_histogram.gp" << EOL
set terminal png size 800,600
set output "${test_name}_allocation_histogram.png"
set title "Allocation Size Distribution for ${test_name}"
set xlabel "Allocation Size (bytes)"
set ylabel "Frequency"
set style fill solid 0.5
set boxwidth 0.8
set grid
set logscale y
plot "data/${test_name}_alloc_data.txt" using 2:1 with boxes title "Allocation Sizes"
EOL
        
        # Run gnuplot for allocation histogram
        (cd "$SCRIPT_DIR" && gnuplot "${test_name}_histogram.gp")
        
        # Generate bucket optimization plot if bucket file exists
        if [ -s "$buckets_file" ]; then
            # Extract bucket sizes from buckets file
            grep "^#define WOLFMEM_BUCKETS" "$buckets_file" | sed 's/#define WOLFMEM_BUCKETS //' | tr ',' '\n' > "$SCRIPT_DIR/data/${test_name}_bucket_sizes.txt"
            
            # Create bucket optimization gnuplot script
            cat > "$SCRIPT_DIR/${test_name}_bucket_opt.gp" << EOL
set terminal png size 800,600
set output "${test_name}_bucket_optimization.png"
set title "Bucket Optimization for ${test_name}"
set xlabel "Size (bytes)"
set ylabel "Count"
set grid
set style fill solid 0.5
set boxwidth 0.8
set key outside
plot "data/${test_name}_alloc_data.txt" using 2:1 with boxes title "Allocation Sizes", \\
     "data/${test_name}_bucket_sizes.txt" using 1:(0.5) with impulses lw 2 title "Bucket Sizes"
EOL
            
            # Run gnuplot for bucket optimization
            (cd "$SCRIPT_DIR" && gnuplot "${test_name}_bucket_opt.gp")
        fi
    fi
done

# Create data file for TLS operation comparison
echo "# Operation TotalAllocs UniqueSizes LargestBucket TotalWaste" > "$SCRIPT_DIR/data/tls_comparison.txt"
grep -v "Test Name" "$RESULTS_DIR/summary.csv" | sed 's/,/ /g' >> "$SCRIPT_DIR/data/tls_comparison.txt"

# Create TLS operation comparison gnuplot script
cat > "$SCRIPT_DIR/tls_comparison.gp" << EOL
set terminal png size 800,600
set output "tls_comparison.png"
set title "TLS Operation Comparison"
set style data histogram
set style histogram cluster gap 1
set style fill solid 0.5 border -1
set boxwidth 0.9
set xtics rotate by -45
set grid
set key outside
plot "data/tls_comparison.txt" using 2:xtic(1) title "Total Allocs", \\
     "" using 3 title "Unique Sizes", \\
     "" using 4 title "Largest Bucket", \\
     "" using 5 title "Total Waste"
EOL

# Run gnuplot for TLS operation comparison
(cd "$SCRIPT_DIR" && gnuplot "tls_comparison.gp")

echo "Visualization completed. Results saved in $SCRIPT_DIR/"
