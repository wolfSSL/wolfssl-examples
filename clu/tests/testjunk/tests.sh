#Test utility functionality
#!/bin/bash
PAT="cd ~/"
CYASSL="wolfssl"
ENC="-e"
DEC="-d"
HASH="-h"
BENCH="-b"
AES="aes-cbc-"
AES2="aes-ctr-"
DES="3des-cbc-"
CAM="camellia-cbc-"
M="-md5"
SHA="-sha"
SHA2="-sha256"
SHA3="-sha384"
SHA5="-sha512"
BLAKE="-blake2b"
zero=0
OPTS="cyassl/cyassl/options.h"

function crypto() {
    cd $currentDir
    COUNTER=8
    SIZEF=1
    SIZET=1

    until [ $COUNTER -lt 1 ]; do
        echo Creating File of Size $SIZEF...
        IN=$(mktemp /tmp/input.XXXXXXXXXX) || { echo "Failed to create temp file"; exit 1; }
        OUT=$(mktemp /tmp/output.XXXXXXXXXX) || { echo "Failed to create temp file"; exit 1; }
        KEY=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 10 | head -n 1)
        RANDF=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $SIZEF | head -n 1)
        RANDT=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $SIZET | head -n 1)

        echo $RANDF  >> $IN

        echo $CYASSL $ENC $1 size: $SIZEF bytes
        $CYASSL $ENC $1 -i $IN -o $OUT -p $KEY
        $CYASSL $DEC $1 -i $OUT -o $IN -p $KEY
        echo $CYASSL $ENC $1 -i $RANDT
        $CYASSL $ENC $1 -i $RANDT -o $OUT -p $KEY
        $CYASSL $DEC $1 -i $OUT -o $IN -p $KEY

        rm $OUT
        rm $IN

        let COUNTER-=1
        let SIZEF+=100
        let SIZET+=10
    done
}

function hashing() {
    cd $currentDir #return to current directory before generating files
    COUNTER=8
    SIZEF=1
    SIZET=1

    until [ $COUNTER -lt 1 ]; do
        echo Creating File of Size $SIZEF...
        IN=$(mktemp /tmp/input.XXXXXXXXXX) || { echo "Failed to create temp file"; exit 1; }
        OUT=$(mktemp /tmp/output.XXXXXXXXXX) || { echo "Failed to create temp file"; exit 1; }
        RANDF=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $SIZEF | head -n 1)
        RANDT=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $SIZET | head -n 1)
        echo $RANDF >> $IN

        echo $CYASSL -h $1 size: $SIZEF bytes
        $CYASSL -h $1 -i $IN -o $OUT
        echo $CYASSL -h $1 -i $RANDT
        $CYASSL -h $1 -i $RANDT -o $OUT

        rm $IN
        rm $OUT

        let COUNTER-=1
        let SIZEF+=100
        let SIZET+=10
    done
}
currentDir=$(pwd) #save our current directory
crypto $AES\128
crypto $AES\192
crypto $AES\256
cd ~              #change to home directory
grep -q "CYASSL_AES_COUNTER" $OPTS && if [[ $? -eq $zero ]]; then
    crypto $AES2\128
    crypto $AES2\192
    crypto $AES2\256
fi

crypto $DES\56
crypto $DES\112
crypto $DES\168
cd ~              #change to home directory
grep -q "HAVE_CAMELLIA" $OPTS && if [[ $? -eq $zero ]]; then
    crypto $CAM\128
    crypto $CAM\192
    crypto $CAM\256
fi

hashing $M
hashing $SHA
hashing $SHA2

cd ~              #change to home directory
grep -q "CYASSL_SHA384" $OPTS && if [[ $? -eq $zero ]]; then
    hashing $SHA3
fi
cd ~              #change to home directory
grep -q "CYASSL_SHA512" $OPTS && if [[ $? -eq $zero ]]; then
    hashing $SHA5
fi
cd ~              #change to home directory
grep -q "HAVE_BLAKE2" $OPTS && if [[ $? -eq $zero ]]; then
    hashing $BLAKE
fi
cd $currentDir
$CYASSL -b -t 5 -a
