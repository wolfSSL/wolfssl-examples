#!/bin/bash

# set -x

echo "aesgcm-file-encrypt tests"
echo "./aesgcm-file-encrypt -b to run benchmark after building wolfSSL with"
echo "./configure --enable-aesgcm-stream && sudo make install"

# Define the output CSV file
output_file="aesgcm_times.csv"
output_file_header="aesgcm_times_header.csv"
partition="/dev/nvme0n1p8"

keyStr=$(cat /dev/urandom | base64 | head -c 32)
echo "key = $keyStr"

ivStr=$(cat /dev/urandom | base64 | head -c 16)
echo "IV = $ivStr"

if [[ "$1" == "-b" ]] || [[ "$1" == "--bench" ]]; then
  cp "$output_file" "$output_file.back"
  echo "File-size,Buffer-size,AES-256-GCM-enc,AES-256-GCM-dec" > "$output_file"

  buffer_sizes=("32" "64" "128" "256" "512" "1024" "2048" "4096" "8192" "12288" "20480" "1073741824")

  for b_size in "${buffer_sizes[@]}"
  do
    rm aesgcm-file-encrypt text*
    make clean
    make CPPFLAGS="-DMAX_BUFFER_SIZE=$b_size -DMIN_BUFFER_SIZE=32" aesgcm-file-encrypt

    # Define an array of file sizes to test

    file_sizes=("10M" "50M" "100M" "500M" "1G")

    echo "System Information:" > "$output_file_header"
    echo "--------------------" >> "$output_file_header"
    echo "Hostname: $(hostname)" >> "$output_file_header"
    echo "Kernel Version: $(uname -r)" >> "$output_file_header"
    echo "Processor Type: $(uname -m)" >> "$output_file_header"
    echo "Operating System: $(lsb_release -d | awk '{print $2,$3,$4,$5}')" >> "$output_file_header"
    echo "CPU Info: $(lscpu | grep 'Model name' | cut -d':' -f2 | sed 's/^ //')" >> "$output_file_header"
    echo "Memory Info: $(free -h | grep 'Mem' | awk '{print $2}')" >> "$output_file_header"
    echo "The page size is: $(sudo blockdev --getbsz $partition) bytes." >> "$output_file_header"
    echo "The sector size of the disk is: $(sudo blockdev --getss $partition) bytes." >> "$output_file_header"

    # Loop over each file size and measure performance
    for size in "${file_sizes[@]}"
    do
      # Generate a test file of the given size
      text_file=text_"$size".bin
      dd if=/dev/urandom of=$text_file bs=$size count=1 conv=fsync

      t_aesgcm_e="$( TIMEFORMAT="%R";time (./aesgcm-file-encrypt -e 256 -m 1 -k $keyStr -v $ivStr -i $text_file -o  text2cipher.bin > /dev/null 2>&1) 2>&1 )"
      t_aesgcm_d="$( TIMEFORMAT="%R";time (./aesgcm-file-encrypt -d 256 -m 1 -k $keyStr -v $ivStr -i text2cipher.bin -o text2cipher2text.bin > /dev/null 2>&1) 2>&1 )"

      diff -s $text_file text2cipher2text.bin > /dev/null
      if [ $? -eq 0 ]; then
        echo "Passed $t_aesgcm_e $t_aesgcm_d"
      else
        echo "Failed"
      fi
      echo "$size,$b_size,$t_aesgcm_e,$t_aesgcm_d" >> "$output_file"
      rm text*
    done
  done
else
    rm aesgcm-file-encrypt text*
    make clean
    make CPPFLAGS="-DMAX_BUFFER_SIZE=4096" aesgcm-file-encrypt
    # test a smaller file size (default option)
    dd if=/dev/urandom bs=1024 count=1 | head -c 1022 > text.bin
    res=$(./aesgcm-file-encrypt -e 256 -m 1 -k $keyStr -v $ivStr -i text.bin -o  text2cipher.bin )
    echo $res
    res=$(./aesgcm-file-encrypt -d 256 -m 1 -k $keyStr -v $ivStr -i text2cipher.bin -o text2cipher2text.bin )
    echo $res

    diff -s text.bin text2cipher2text.bin > /dev/null
    if [ $? -eq 0 ]; then
      echo "Passed"
    else
      echo "Failed"
    fi
    rm text*
fi

if true; then
  input_file="$output_file"
  output_file_sorted="data.csv"

  awk -F, 'function to_bytes(x) {
             split(x, a, /[^0-9.]+/);
             return a[1] * (index("KMGT", substr(x, length(a[1]) + 1)) * 1024 ^ (index("KMGT", substr(x, length(a[1]) + 1)) - 1));
           }
           NR == 1 { gsub(",", " "); print $0; next }
           { line = $1; for (i = 2; i <= NF; i++) line = line "," $i; print to_bytes($1) "," line }' "$input_file" |
           sort -t, -k2,2n -k3,3n | awk -F, 'NR == 1 { print $0 } NR > 1 { $1=""; $0=substr($0, 2); print }' > "$output_file_sorted"

   sed -i 's/1073741824/1G/g' "$output_file_sorted"
  ./plot_data.gp
fi


exit 0
