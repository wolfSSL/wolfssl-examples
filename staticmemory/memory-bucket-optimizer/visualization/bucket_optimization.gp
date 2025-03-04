# Gnuplot script to visualize bucket sizes and waste

# Set the output file format
set terminal png size 800,600
set output "bucket_optimization.png"

# Set the title and labels
set title "Memory Bucket Sizes and Waste"
set xlabel "Bucket Size (bytes)"
set ylabel "Waste (bytes)"
set y2label "Distribution"
set grid

# Set the style
set style fill solid 0.5
set boxwidth 0.8

# Enable y2 axis
set ytics nomirror
set y2tics

# Plot the data
plot "bucket_data.txt" using 1:3 with boxes title "Waste" axes x1y1, \
     "bucket_data.txt" using 1:4 with linespoints title "Distribution" axes x1y2
