#!/usr/bin/gnuplot

# Function to get the number of lines in the file
filelines(file) = system(sprintf("awk 'END {print NR}' %s", file))

# Get the number of lines in the data file
datafile = "data.csv"
nlines = int(filelines(datafile)) - 1
# you can adjust the number of rows you want to ply
chunk_size = 12

# Set the terminal type
set term png

# Set the title, style, and key settings
set title "wolfSSL"
set style fill solid 1.00 border lt -1
set style histogram clustered gap 4 title textcolor lt -1
set datafile missing '-'
set style data histograms

# Set x-axis settings
set xtics border in scale 0,0 nomirror rotate by -45  autojustify
set xtics norangelimit
set xtics ()
set format y '%.0s%c'

set lmargin at screen 0.15
set rmargin at screen 0.90

#set bmargin at screen 0.10
set tmargin at screen 0.90

# Set legend position
#set key outside below
set key autotitle columnhead noenhanced

# Set grid settings
set grid mxtics mytics
set grid x y

# Set titles for x and y axes
set xlabel "FileSize-BufferSize (Bytes)"
set ylabel "Total time (Seconds)"

# Uncomment and set xrange and yrange if necessary
# set xrange [lower:upper]
# set yrange [lower:upper]

# Loop through the data file, plotting 12 rows at a time

do for [i=0:int((nlines-1)/chunk_size)] {
    # Set the output file name
    set output sprintf("output_%03d.png", i + 1)

    # Calculate start and end rows for the current chunk
    start = 0 + i * chunk_size
    end = start + chunk_size - 1

    # Plot the data from the CSV file for the current chunk
    plot for [col=3:*] for [filename in datafile] filename using col:xticlabels(strcol(1)."-".strcol(2)) every ::start::end with linespoints
}

