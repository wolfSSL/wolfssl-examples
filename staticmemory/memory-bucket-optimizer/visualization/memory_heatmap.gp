# Gnuplot script to create a heatmap of memory usage

# Set the output file format
set terminal png size 800,600
set output "memory_heatmap.png"

# Set the title and labels
set title "Memory Usage Heatmap by Bucket Size and Operation"
set xlabel "Bucket Size (bytes)"
set ylabel "TLS Operation"
set cblabel "Memory Usage (bytes)"

# Set the style for heatmap
set view map
set palette defined (0 "blue", 1 "green", 2 "yellow", 3 "red")
set cbrange [0:5000]
set yrange [0.5:4.5]
set xtics rotate by -45
set grid

# Plot the data
set datafile separator " "
plot "memory_heatmap.txt" using 1:2:3 with image title ""
