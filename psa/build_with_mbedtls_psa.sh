#!/bin/bash

# wolfSSL with PSA enabled using mbedtls PSA implementation
# This script must run from the wolfSSL root folder

set -e

MBEDTLS_COMMIT_ID=acc74b841307659eea0461275c7c0309874c87d7
MBEDTLS_DIR=${MBEDTLS_DIR:="/tmp/mbedtls"}
MBEDTLS_INCLUDE_DIR=${MBEDTLS_DIR}/build/include/
MBEDTLS_LIB_DIR=${MBEDTLS_DIR}/build/library/
WOLFSSL_INSTALL=${WOLFSSL_INSTALL:="/usr/local"}

# uncomment to enable debug build
#MBEDTLS_DEBUG="-DCMAKE_BUILD_TYPE=Debug"
#WOLFSSL_DEBUG="-g -ggdb -DDEBUG_WOLFSSL -DWOLFSSL_DEBUG_TLS"

if [ ! -d ./wolfssl ]
then
    echo "You need to run this script from inside the wolfSSL root folder"
    exit -1
fi

download_mbedtls_src() {
    echo "downloading mbedtls source in ${MBEDTLS_DIR}..."
    if [ -d "${MBEDTLS_DIR}" ]
    then
        echo "${MBEDTLS_DIR} exists, skipping src dowload.."
        return
    fi
    mkdir -p "${MBEDTLS_DIR}"
    curl --location https://github.com/ARMmbed/mbedtls/archive/${MBEDTLS_COMMIT_ID}.tar.gz | \
    tar --directory="${MBEDTLS_DIR}" --strip-components=1 -x -z
}

build_mbedtls() {
    echo "building mbedtls in ${MBEDTLS_DIR}/build..."
    mkdir -p "${MBEDTLS_DIR}/build"
    (cd "${MBEDTLS_DIR}/build" && cmake ${MBEDTLS_DEBUG} -DCMAKE_POSITION_INDEPENDENT_CODE=ON ../)
    (cd "${MBEDTLS_DIR}/build" && cmake --build .)
}

build_wolfssl() {
    echo "building wolfSSL with PSA enabled..."
    if [ ! -f ./configure ]
    then
        ./autogen.sh
    fi
    CFLAGS="-Wno-error=redundant-decls -Werror -Wswitch-enum -Wno-error=switch-enum -DWOLFSSL_PSA_GLOBAL_LOCK ${WOLFSSL_DEBUG}" \
          ./configure \
          --enable-psa --with-psa-include="${MBEDTLS_INCLUDE_DIR}" --enable-pkcallbacks\
          --disable-examples --disable-benchmark --disable-crypttests --prefix="${WOLFSSL_INSTALL}"
    make
}

download_mbedtls_src
build_mbedtls
build_wolfssl
