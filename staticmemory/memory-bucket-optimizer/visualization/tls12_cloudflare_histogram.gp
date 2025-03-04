set terminal png size 800,600
set output "tls12_cloudflare_allocation_histogram.png"
set title "Allocation Size Distribution for tls12_cloudflare"
set xlabel "Allocation Size (bytes)"
set ylabel "Frequency"
set style fill solid 0.5
set boxwidth 0.8
set grid
set logscale y
plot "data/tls12_cloudflare_alloc_data.txt" using 2:1 with boxes title "Allocation Sizes"
