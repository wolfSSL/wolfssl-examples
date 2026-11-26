#!/bin/bash

# fail out if commands fail
set -e

rm -rf build


# place wolfssl directory into the example directory
if [ ! -d wolfssl ]; then
    echo "Cloning wolfssl..."
    git clone --depth=1 git@github.com:wolfssl/wolfssl &> /dev/null
else
    echo "Using exiting wolfssl directory"
fi


# patch CAAM driver files for ECC/Blob use with wolfssl/IDE/MCUEXPRESSO/RT1170/fsl_caam_c.patch and wolfssl/IDE/MCUEXPRESSO/RT1170/fsl_caam_h.patch
echo "Patching CAAM driver..."
cd devices/MIMXRT1176/drivers/
if ! patch --dry-run -sfR < ../../../wolfssl/IDE/MCUEXPRESSO/RT1170/fsl_caam_c.patch &> /dev/null; then
    echo "    patching fsl_caam.c"
    patch < ../../../wolfssl/IDE/MCUEXPRESSO/RT1170/fsl_caam_c.patch &> /dev/null
else
    echo "    existing fsl_caam.c patch in place"
fi
if ! patch --dry-run -sfR < ../../../wolfssl/IDE/MCUEXPRESSO/RT1170/fsl_caam_h.patch &> /dev/null; then
    echo "    patching fsl_caam.h"
    patch < ../../../wolfssl/IDE/MCUEXPRESSO/RT1170/fsl_caam_h.patch &> /dev/null
else
    echo "    existing fsl_caam.h patch in place"
fi
cd ../../../

# mkdir build && cd build
mkdir build && cd build

# copy over example linker files from boards/evkmimxrt1170/rtos_examples/freertos_hello/cm7/armgcc/ 
cp ../boards/evkmimxrt1170/rtos_examples/freertos_hello/cm7/armgcc/*.ld .

# use cmake to build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../tools/cmake_toolchain_files/armgcc.cmake  -DCMAKE_C_FLAGS=-I../wolfssl/IDE/MCUEXPRESSO/RT1170/ -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=debug
make

echo "Done building elf, located now in ./build/debug"
exit 0
