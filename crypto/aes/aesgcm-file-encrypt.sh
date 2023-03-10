#!/bin/bash

# set -x

echo "aesgcm-file-encrypt tests"

keyStr=$(cat /dev/urandom | base64 | head -c 32)
echo "key = $keyStr"

ivStr=$(cat /dev/urandom | base64 | head -c 16)
echo "IV = $ivStr"


if [[ "$1" == "-b" ]] || [[ "$1" == "--bench" ]]; then

    # Define an array of file sizes to test
    file_sizes=("10M" "50M" "100M" "500M" "1G")

    # Define the output CSV file and write the header row
    output_file="aesgcm_times.csv"

    echo "System Information:" > "$output_file"
    echo "--------------------" >> "$output_file"
    echo "Hostname: $(hostname)" >> "$output_file"
    echo "Kernel Version: $(uname -r)" >> "$output_file"
    echo "Processor Type: $(uname -m)" >> "$output_file"
    echo "Operating System: $(lsb_release -d | awk '{print $2,$3,$4,$5}')" >> "$output_file"
    echo "CPU Info: $(lscpu | grep 'Model name' | cut -d':' -f2 | sed 's/^ //')" >> "$output_file"
    echo "Memory Info: $(free -h | grep 'Mem' | awk '{print $2}')" >> "$output_file"

    echo "File Size, Encryption, Decryption, Encryption EVP, Decryption EVP" > "$output_file"


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

        t_evpgcm_e="$( TIMEFORMAT="%R";time (./aesgcm-file-encrypt -e 256 -m 2 -k $keyStr -v $ivStr -i $text_file -o  text2cipher.evp.bin > /dev/null 2>&1) 2>&1 )"
        t_evpgcm_d="$( TIMEFORMAT="%R";time (./aesgcm-file-encrypt -d 256 -m 2 -k $keyStr -v $ivStr -i text2cipher.evp.bin -o text2cipher2text.evp.bin > /dev/null 2>&1) 2>&1 )"

        diff -s $text_file text2cipher2text.evp.bin > /dev/null
        if [ $? -eq 0 ]; then
          echo "Passed $t_evpgcm_e $t_evpgcm_d"
        else
          echo "Failed"
        fi

        echo "$size, $t_aesgcm_e,$t_aesgcm_d,$t_evpgcm_e,$t_evpgcm_d" >> "$output_file"
        rm text*
    done
else
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

exit 0
