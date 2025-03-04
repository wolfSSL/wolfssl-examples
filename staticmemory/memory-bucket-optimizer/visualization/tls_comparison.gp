# Gnuplot script to compare different TLS operations

# Set the output file format
set terminal png size 800,600
set output "tls_comparison.png"

# Set the title and labels
set title "Memory Usage Comparison for Different TLS Operations"
set xlabel "TLS Operation"
set ylabel "Memory Usage"
set grid

# Set the style
set style data histogram
set style histogram cluster gap 1
set style fill solid 0.5 border -1
set boxwidth 0.9

# Plot the data
plot "tls_comparison.txt" using 2:xtic(1) title "Total Allocations", \
     "" using 3 title "Unique Sizes", \
     "" using 5 title "Total Waste"
