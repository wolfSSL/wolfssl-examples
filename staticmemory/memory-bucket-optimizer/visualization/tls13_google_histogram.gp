set terminal png size 800,600
set output "tls13_google_allocation_histogram.png"
set title "Allocation Size Distribution for tls13_google"
set xlabel "Allocation Size (bytes)"
set ylabel "Frequency"
set style fill solid 0.5
set boxwidth 0.8
set grid
set logscale y
plot "data/tls13_google_alloc_data.txt" using 2:1 with boxes title "Allocation Sizes"
