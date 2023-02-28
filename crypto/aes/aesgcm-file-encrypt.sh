#!/bin/bash

# set -x

dd if=/dev/urandom bs=1024 count=1 | head -c 1022 > text.bin

echo "aesgcm-file-encrypt tests"
keyStr=$(cat /dev/urandom | base64 | head -c 32)
echo "key = $keyStr"

ivStr=$(cat /dev/urandom | base64 | head -c 16)
echo "IV = $ivStr"


res=$(./aesgcm-file-encrypt -e 256 -m 1 -k $keyStr -v $ivStr -i text.bin -o  text2cipher.bin)
echo $res
res=$(./aesgcm-file-encrypt -d 256 -m 1 -k $keyStr -v $ivStr -i text2cipher.bin -o text2cipher2text.bin )
echo $res

diff -s text.bin text2cipher2text.bin > /dev/null
if [ $? -eq 0 ]; then
  echo "Passed"
else
  echo "Failed"
fi

res=$(./aesgcm-file-encrypt -e 256 -m 2 -k $keyStr -v $ivStr -i text.bin -o  text2cipher.evp.bin)
echo $res
res=$(./aesgcm-file-encrypt -d 256 -m 2 -k $keyStr -v $ivStr -i text2cipher.evp.bin -o text2cipher2text.evp.bin )
echo $res

diff -s text.bin text2cipher2text.evp.bin > /dev/null
if [ $? -eq 0 ]; then
  echo "Passed"
else
  echo "Failed"
fi
