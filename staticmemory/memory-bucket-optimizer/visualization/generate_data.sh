#!/bin/bash

# Script to generate data files and plots for memory bucket optimization

# Set up directories
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
RESULTS_DIR="$SCRIPT_DIR/../results"

# Create data directory
mkdir -p "$SCRIPT_DIR/data"

# Function to extract allocation sizes and frequencies from memory logs
extract_allocation_data() {
    local memory_log="$1"
    local output_file="$2"
    
    # Extract allocation sizes and count occurrences
    grep "^Alloc:" "$memory_log" | sed -n 's/.*-> \([0-9]\+\).*/\1/p' | sort -n | uniq -c > "$output_file"
}

# Function to generate gnuplot script for allocation histogram
generate_allocation_histogram() {
    local data_file="$1"
    local output_file="$2"
    local title="$3"
    
    cat > "$SCRIPT_DIR/allocation_histogram.gp" << EOL
set terminal png size 800,600
set output "$output_file"
set title "$title"
set xlabel "Allocation Size (bytes)"
set ylabel "Frequency"
set style fill solid 0.5
set boxwidth 0.8
set grid
set logscale y
plot "$data_file" using 2:1 with boxes title "Allocation Sizes"
EOL
    
    gnuplot "$SCRIPT_DIR/allocation_histogram.gp"
}

# Function to generate gnuplot script for bucket optimization
generate_bucket_optimization() {
    local data_file="$1"
    local buckets_file="$2"
    local output_file="$3"
    local title="$4"
    
    # Extract bucket sizes from buckets file
    grep "^#define WOLFMEM_BUCKETS" "$buckets_file" | sed 's/#define WOLFMEM_BUCKETS //' | tr ',' '\n' > "$SCRIPT_DIR/data/bucket_sizes.txt"
    
    cat > "$SCRIPT_DIR/bucket_optimization.gp" << EOL
set terminal png size 800,600
set output "$output_file"
set title "$title"
set xlabel "Size (bytes)"
set ylabel "Count"
set grid
set style fill solid 0.5
set boxwidth 0.8
set key outside
plot "$data_file" using 2:1 with boxes title "Allocation Sizes", \\
     "$SCRIPT_DIR/data/bucket_sizes.txt" using 1:(0.5) with impulses lw 2 title "Bucket Sizes"
EOL
    
    gnuplot "$SCRIPT_DIR/bucket_optimization.gp"
}

# Function to generate gnuplot script for TLS operation comparison
generate_tls_comparison() {
    local output_file="$1"
    
    # Create data file for TLS operation comparison
    echo "# Operation TotalAllocs UniqueSizes LargestBucket TotalWaste" > "$SCRIPT_DIR/data/tls_comparison.txt"
    grep -v "Test Name" "$RESULTS_DIR/summary.csv" | sed 's/,/ /g' >> "$SCRIPT_DIR/data/tls_comparison.txt"
    
    cat > "$SCRIPT_DIR/tls_comparison.gp" << EOL
set terminal png size 800,600
set output "$output_file"
set title "TLS Operation Comparison"
set style data histogram
set style histogram cluster gap 1
set style fill solid 0.5 border -1
set boxwidth 0.9
set xtics rotate by -45
set grid
set key outside
plot "$SCRIPT_DIR/data/tls_comparison.txt" using 2:xtic(1) title "Total Allocs", \\
     "" using 3 title "Unique Sizes", \\
     "" using 4 title "Largest Bucket", \\
     "" using 5 title "Total Waste"
EOL
    
    gnuplot "$SCRIPT_DIR/tls_comparison.gp"
}

# Process each TLS operation
for test_name in tls12_google tls13_google tls12_cloudflare tls13_cloudflare; do
    memory_log="$RESULTS_DIR/${test_name}_memory.txt"
    buckets_file="$RESULTS_DIR/${test_name}_buckets.txt"
    
    if [ -s "$memory_log" ]; then
        # Extract allocation data
        extract_allocation_data "$memory_log" "$SCRIPT_DIR/data/${test_name}_alloc_data.txt"
        
        # Generate allocation histogram
        generate_allocation_histogram "$SCRIPT_DIR/data/${test_name}_alloc_data.txt" \
            "$SCRIPT_DIR/${test_name}_allocation_histogram.png" \
            "Allocation Size Distribution for ${test_name}"
        
        # Generate bucket optimization plot
        if [ -s "$buckets_file" ]; then
            generate_bucket_optimization "$SCRIPT_DIR/data/${test_name}_alloc_data.txt" \
                "$buckets_file" \
                "$SCRIPT_DIR/${test_name}_bucket_optimization.png" \
                "Bucket Optimization for ${test_name}"
        fi
    fi
done

# Generate TLS operation comparison
generate_tls_comparison "$SCRIPT_DIR/tls_comparison.png"

echo "Visualization completed. Results saved in $SCRIPT_DIR/"
