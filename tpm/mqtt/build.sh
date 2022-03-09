#!/usr/bin/bash

set -e

REPO_DIR=../../..
INST_DIR=$(dirname $(realpath $0))/inst

if [ "$1" != "" ]; then
    REPO_DIR=$1
fi

if [ "$2" != "" ]; then
    INST_DIR=$2
fi

echo "Building wolfSSL with prefix $INST_DIR"
pushd ${REPO_DIR}/wolfssl
./autogen.sh
./configure --prefix=$INST_DIR --enable-wolftpm --enable-debug
make install
popd

echo "Building wolfTPM with prefix $INST_DIR"
pushd ${REPO_DIR}/wolftpm
./autogen.sh
./configure --prefix=$INST_DIR --enable-swtpm --enable-debug
make install
popd

echo "Building wolfMQTT with prefix $INST_DIR"
pushd ${REPO_DIR}/wolfmqtt
./autogen.sh
./configure --prefix=$INST_DIR --enable-debug
make install
popd
