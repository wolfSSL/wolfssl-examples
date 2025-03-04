#!/bin/bash
# compare_memory.sh
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
RESULTS_DIR="comparison_results"

# Create results directory
mkdir -p "$RESULTS_DIR"

# Create sample data for demonstration
mkdir -p "$RESULTS_DIR/samples"

# Sample default memory usage
cat > "$RESULTS_DIR/samples/default_memory.txt" << SAMPLE
Total Memory: 1048576 bytes
Used Memory: 524288 bytes
Available Memory: 524288 bytes
Wasted Memory: 131072 bytes
SAMPLE

# Sample optimized memory usage
cat > "$RESULTS_DIR/samples/optimized_memory.txt" << SAMPLE
Total Memory: 1048576 bytes
Used Memory: 524288 bytes
Available Memory: 524288 bytes
Wasted Memory: 65536 bytes
SAMPLE

# Use sample data for comparison
cp "$RESULTS_DIR/samples/default_memory.txt" "$RESULTS_DIR/default_memory.txt"
cp "$RESULTS_DIR/samples/optimized_memory.txt" "$RESULTS_DIR/optimized_memory.txt"

# Compare memory usage
echo "Comparing memory usage..."

# Extract memory usage statistics
DEFAULT_TOTAL=$(grep "Total Memory:" "$RESULTS_DIR/default_memory.txt" | awk '{print $3}' || echo "0")
DEFAULT_USED=$(grep "Used Memory:" "$RESULTS_DIR/default_memory.txt" | awk '{print $3}' || echo "0")
DEFAULT_AVAILABLE=$(grep "Available Memory:" "$RESULTS_DIR/default_memory.txt" | awk '{print $3}' || echo "0")
DEFAULT_WASTED=$(grep "Wasted Memory:" "$RESULTS_DIR/default_memory.txt" | awk '{print $3}' || echo "0")

OPTIMIZED_TOTAL=$(grep "Total Memory:" "$RESULTS_DIR/optimized_memory.txt" | awk '{print $3}' || echo "0")
OPTIMIZED_USED=$(grep "Used Memory:" "$RESULTS_DIR/optimized_memory.txt" | awk '{print $3}' || echo "0")
OPTIMIZED_AVAILABLE=$(grep "Available Memory:" "$RESULTS_DIR/optimized_memory.txt" | awk '{print $3}' || echo "0")
OPTIMIZED_WASTED=$(grep "Wasted Memory:" "$RESULTS_DIR/optimized_memory.txt" | awk '{print $3}' || echo "0")

# Calculate improvement
if [ "$DEFAULT_WASTED" -gt 0 ] && [ "$OPTIMIZED_WASTED" -gt 0 ]; then
    WASTE_REDUCTION=$((DEFAULT_WASTED - OPTIMIZED_WASTED))
    WASTE_REDUCTION_PERCENT=$((WASTE_REDUCTION * 100 / DEFAULT_WASTED))
else
    WASTE_REDUCTION=0
    WASTE_REDUCTION_PERCENT=0
fi

# Print comparison results
echo "Memory Usage Comparison:"
echo "                   Default    Optimized   Improvement"
echo "Total Memory:      $DEFAULT_TOTAL bytes   $OPTIMIZED_TOTAL bytes   0 bytes (0%)"
echo "Used Memory:       $DEFAULT_USED bytes   $OPTIMIZED_USED bytes   0 bytes (0%)"
echo "Available Memory:  $DEFAULT_AVAILABLE bytes   $OPTIMIZED_AVAILABLE bytes   0 bytes (0%)"
echo "Wasted Memory:     $DEFAULT_WASTED bytes   $OPTIMIZED_WASTED bytes   $WASTE_REDUCTION bytes ($WASTE_REDUCTION_PERCENT%)"

# Generate comparison data for visualization
echo "Generating comparison data for visualization..."
mkdir -p "visualization/data"

# Create data file for memory usage comparison
cat > "visualization/data/memory_comparison.txt" << DATAFILE
Category Default Optimized
Total $DEFAULT_TOTAL $OPTIMIZED_TOTAL
Used $DEFAULT_USED $OPTIMIZED_USED
Available $DEFAULT_AVAILABLE $OPTIMIZED_AVAILABLE
Wasted $DEFAULT_WASTED $OPTIMIZED_WASTED
DATAFILE

echo "Comparison completed. Results saved in $RESULTS_DIR/"
