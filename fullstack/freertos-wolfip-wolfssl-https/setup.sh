#!/bin/bash

# Script to setup FreeRTOS environment for wolfSSL examples
set -e

FREERTOS_REPO="https://github.com/FreeRTOS/FreeRTOS.git"
FREERTOS_KERNEL_REPO="https://github.com/FreeRTOS/FreeRTOS-Kernel.git"
echo "Setting up FreeRTOS simulation environment..."

# Create directories if they don't exist
mkdir -p freertos
cd freertos

# Clone FreeRTOS repositories if they don't exist
if [ ! -d "FreeRTOS" ]; then
    git clone --depth=1 $FREERTOS_REPO
fi

# Clone wolfSSL and wolfIP if they don't exist
cd ../../../
if [ ! -d "wolfssl" ]; then
    git clone --depth=1 https://github.com/wolfSSL/wolfssl.git
    cd wolfssl
    ./autogen.sh
    ./configure
    make
    cd ..
fi

if [ ! -d "wolfip" ]; then
    git clone --depth=1 https://github.com/wolfSSL/wolfip.git
    cd wolfip
    ./autogen.sh
    ./configure
    make
    cd ..
fi

cd wolfssl-examples/fullstack/freertos-wolfip-wolfssl-https/freertos

if [ ! -d "FreeRTOS-Kernel" ]; then
    git clone --depth=1 $FREERTOS_KERNEL_REPO
fi

echo "FreeRTOS repositories cloned successfully"

# Create basic directory structure for our project
mkdir -p ../src
mkdir -p ../include
mkdir -p ../build

echo "Directory structure created"
echo "Setup complete!"
