set terminal png size 800,600
set output "tls13_cloudflare_bucket_optimization.png"
set title "Bucket Optimization for tls13_cloudflare"
set xlabel "Size (bytes)"
set ylabel "Count"
set grid
set style fill solid 0.5
set boxwidth 0.8
set key outside
plot "/home/ubuntu/repos/wolfssl-examples/staticmemory/memory-bucket-optimizer/visualization/data/tls13_cloudflare_alloc_data.txt" using 2:1 with boxes title "Allocation Sizes", \
     "data/bucket_sizes.txt" using 1:(0.5) with impulses lw 2 title "Bucket Sizes"
