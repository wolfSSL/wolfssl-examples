#!/bin/sh

if [ -z $1 ]; then
    echo "Add path to wolfssl directory, i.e. ./build.sh ../../ "
    exit 1
fi

make SGX_MODE=HW SGX_PRERELEASE=1 SGX_WOLFSSL_LIB=${1}IDE/LINUX-SGX/ WOLFSSL_ROOT=${1} SGX_DEBUG=0 HAVE_WOLFSSL_TEST=1 HAVE_WOLFSSL_BENCHMARK=1 HAVE_WOLFSSL_SP=1

exit 0
