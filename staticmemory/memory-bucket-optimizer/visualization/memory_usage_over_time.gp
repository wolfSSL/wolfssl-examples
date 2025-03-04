# Gnuplot script to visualize memory usage over time

# Set the output file format
set terminal png size 800,600
set output "memory_usage_over_time.png"

# Set the title and labels
set title "Memory Usage Over Time for Different Bucket Configurations"
set xlabel "Time (s)"
set ylabel "Memory Usage (bytes)"
set grid

# Set the style
set style data linespoints
set key outside

# Plot the data
plot "memory_usage_over_time.txt" using 1:2 title "Default Configuration", \
     "memory_usage_over_time.txt" using 1:3 title "Optimized Configuration"
