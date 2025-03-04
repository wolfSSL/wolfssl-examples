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
plot "data/tls_comparison.txt" using 2:xtic(1) title "Total Allocs", \
     "" using 3 title "Unique Sizes", \
     "" using 4 title "Largest Bucket", \
     "" using 5 title "Total Waste"
