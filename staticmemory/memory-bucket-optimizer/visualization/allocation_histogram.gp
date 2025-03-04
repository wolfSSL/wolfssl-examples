# Gnuplot script to visualize allocation sizes and frequencies

# Set the output file format
set terminal png size 800,600
set output "allocation_histogram.png"

# Set the title and labels
set title "Memory Allocation Sizes and Frequencies"
set xlabel "Allocation Size (bytes)"
set ylabel "Frequency"
set grid

# Set the style
set style fill solid 0.5
set boxwidth 0.8

# Plot the data
plot "allocation_data.txt" using 1:2 with boxes title "Allocation Frequency"
