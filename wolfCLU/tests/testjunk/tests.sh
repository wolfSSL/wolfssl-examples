#Test utility functionality
#!/bin/bash

CURR_PATH=`pwd`
echo "CURR_PATH = $CURR_PATH"

PAT="cd $CURR_PATH/../../"

WOLFSSL="wolfssl"
ENC="-encrypt"
DEC="-decrypt"
HASH="-hash"
BENCH="-bench"
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
OPTS="wolfssl/wolfssl/options.h"
INPUT="-in"
OUTPUT="-out"
PASSWD="-pwd"

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

        echo $WOLFSSL $ENC $1 size: $SIZEF bytes
        $WOLFSSL $ENC $1 $INPUT $IN $OUTPUT $OUT $PASSWD $KEY
        $WOLFSSL $DEC $1 $INPUT $OUT $OUTPUT $IN $PASSWD $KEY
        echo $WOLFSSL $ENC $1 $INPUT $RANDT
        $WOLFSSL $ENC $1 $INPUT $RANDT $OUTPUT $OUT $PASSWD $KEY
        $WOLFSSL $DEC $1 $INPUT $OUT $OUTPUT $IN $PASSWD $KEY

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

        echo $WOLFSSL $HASH $1 size: $SIZEF bytes
        $WOLFSSL $HASH $1 $INPUT $IN $OUTPUT $OUT
        echo $WOLFSSL $HASH $1 $INPUT $RANDT
        $WOLFSSL $HASH $1 $INPUT $RANDT $OUTPUT $OUT

        rm $IN
        rm $OUT

        let COUNTER-=1
        let SIZEF+=100
        let SIZET+=10
    done
}

crypto $AES\128
crypto $AES\192
crypto $AES\256
cd $CURR_PATH/../../              #change to home directory
grep -q "WOLFSSL_AES_COUNTER" $OPTS && if [[ $? -eq $zero ]]; then
    crypto $AES2\128
    crypto $AES2\192
    crypto $AES2\256
fi

crypto $DES\56
crypto $DES\112
crypto $DES\168
cd $CURR_PATH/../../              #change to home directory
grep -q "HAVE_CAMELLIA" $OPTS && if [[ $? -eq $zero ]]; then
    crypto $CAM\128
    crypto $CAM\192
    crypto $CAM\256
fi

hashing $M
hashing $SHA
hashing $SHA2


cd $CURR_PATH/../../              #change to home directory
grep -q "WOLFSSL_SHA384" $OPTS && if [[ $? -eq $zero ]]; then
    hashing $SHA3
fi
cd $CURR_PATH/../../              #change to home directory
grep -q "WOLFSSL_SHA512" $OPTS && if [[ $? -eq $zero ]]; then
    hashing $SHA5
fi
cd $CURR_PATH/../../              #change to home directory
grep -q "HAVE_BLAKE2" $OPTS && if [[ $? -eq $zero ]]; then
    hashing $BLAKE
fi
cd $CURR_PATH
$WOLFSSL -b -t 5 -a
