#!/bin/bash
GREEN='\e[0;32m'
RED='\e[0;31m'
NC='\e[0m'
ZERO=0
fail=0
i=0
total=0
 
function hashtest() {
    fail=0
    i=0
    total=0
    
    # -r options causes the "\" to be read
    while read -r md; do
        #if $i < 10
        if [[ $i -lt 10 ]]; then
            FILE="./byte000$i.dat"
        #if $i < 100 
        elif [[ $i -lt 100 ]]; then
            FILE="./byte00$i.dat"
        elif [[ $i -lt 1000 ]]; then
            FILE="./byte0$i.dat"
        else 
            break
        fi   
#takes last $3 characters off
#        md="${md:0:-$3}"
        COUNTER=0
        while [ $COUNTER -lt $3 ]; do
            md="${md%?}"
            COUNTER=$[COUNTER+1]
        done
#converts to lowercase
#        md=${md,,}
        md="$(tr [A-Z] [a-z] <<< "$md")"
        cipher="$(wolfssl -h $2 -i $FILE)"
        echo "$cipher                   $md"
        #compare result of hash to line in file byte-hashes.sha1
        if test "$cipher" != "$md"; then
            fail=$[fail+1]
        fi
        total=$[total+1]
        i=$[i+1]
    done < $1
    
    if [ $fail = $ZERO ]; then
        echo -e "${GREEN}All $total $2 Tests Passed${NC}"
    else
        echo -e "${RED}$fail/$total $2 Tests Failed${NC}"
    fi
}
echo Testing...
hashtest ./byte-hashes.sha1 sha 3
hashtest ./byte-hashes.md5 md5 2
