#!/bin/bash

set -e

WOLFSSL_TFM_WORKDIR=${WOLFSSL_TFM_WORKDIR:="/tmp"}
TFM_GIT_URL=https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git
TFM_COMMIT_ID=f07cc31545bbba3bad1806ed078c3aee3a09dc52
TRUSTED_FIRMWARE_DIR=${TRUSTED_FIRMWARE_DIR:="${WOLFSSL_TFM_WORKDIR}/wolfssl_tfm/tfm"}

WOLFSSL_DIR=${WOLFSSL_DIR:="${WOLFSSL_TFM_WORKDIR}/wolfssl_tfm/wolfssl"}

TEST_REPO_GIT_URL=https://git.trustedfirmware.org/TF-M/tf-m-tests.git
TEST_REPO_TAG=TF-Mv1.5.0
TEST_REPO_DIR=${TEST_REPO_DIR:="${WOLFSSL_TFM_WORKDIR}/wolfssl_tfm/wolfssl_test_repo"}

download_trusted_firmware_m() {
    echo "downloading trusted firmware-m source in ${TRUSTED_FIRMWARE_DIR}..."
    if [ -d "${TRUSTED_FIRMWARE_DIR}" ]
    then
        echo "${TRUSTED_FIRMWARE_DIR} exists, skipping src dowload.."
        return
    fi

    mkdir -p "${TRUSTED_FIRMWARE_DIR}"
    git clone "${TFM_GIT_URL}" "${TRUSTED_FIRMWARE_DIR}"
    (cd "${TRUSTED_FIRMWARE_DIR}" && git checkout "${TFM_COMMIT_ID}")
}

download_wolfssl_src() {
    echo "downloading WolfSSL source in ${WOLFSSL_DIR}..."
    if [ -d "${WOLFSSL_DIR}" ]
    then
        echo "${WOLFSSL_DIR} exists, skipping src dowload.."
        return
    fi
    mkdir -p "${WOLFSSL_DIR}"
    curl --location https://api.github.com/repos/wolfssl/wolfssl/tarball/master | \
    tar --directory="${WOLFSSL_DIR}" --strip-components=1 -x -z
}

download_tfm_repo_test_src() {
    echo "downloading tfm_test_repo in ${TEST_REPO_DIR}..."
    if [ -d "${TEST_REPO_DIR}" ]
    then
        echo "${TEST_REPO_DIR} exists, skipping src dowload.."
        return
    fi

    mkdir -p "${TEST_REPO_DIR}"
    git clone --depth 1 --branch "${TEST_REPO_TAG}"\
        "${TEST_REPO_GIT_URL}" "${TEST_REPO_DIR}"

    echo "applying wolfssl_patch to ${TEST_REPO_DIR}..."
    cp ./0001-WolfSSL-TLS-1.3-client-server-PSA-demo.patch "${TEST_REPO_DIR}"
    (cd "${TEST_REPO_DIR}" && \
         git apply ./0001-WolfSSL-TLS-1.3-client-server-PSA-demo.patch)
}

compile_tfm() {
    # restart from scratch if build dir already exists
    if [ -d "${TRUSTED_FIRMWARE_DIR}/build" ]
    then
        rm -rf "${TRUSTED_FIRMWARE_DIR}/build"
    fi

    (cd "${TRUSTED_FIRMWARE_DIR}" && \
         mkdir build && \
         cd build && \
         cmake .. -DTFM_PLATFORM=stm/nucleo_l552ze_q \
               -DTFM_TOOLCHAIN_FILE=../toolchain_GNUARM.cmake \
               -G"Unix Makefiles" \
               -DNS=ON \
               -DCMAKE_BUILD_TYPE=RelWithDebInfo \
               -DTEST_S=OFF \
               -DTEST_NS=OFF \
               -DTFM_TEST_REPO_PATH="${TEST_REPO_DIR}" \
               -DWOLFSSL_DEMO=ON \
               -DWOLFSSL_ROOT_PATH="${WOLFSSL_DIR}"\
               -DTFM_NS_REG_TEST=ON)
    (cd "${TRUSTED_FIRMWARE_DIR}/build" && cmake --build . -- install && ./postbuild.sh)
}

compile_wolfssl() {
    (cd "${WOLFSSL_DIR}" && \
         ./autogen.sh && \
         CFLAGS="-mcpu=cortex-m33 -Os --specs=nano.specs -fdata-sections -ffunction-sections -fno-builtin -fshort-enums -funsigned-char -mthumb -nostdlib -Wno-error=redundant-decls -Wno-error=switch-enum \
         -DNO_WOLFSSL_DIR -DWOLFSSL_NO_SOCK -DNO_WRITEV -DWOLFSSL_USER_IO -DNO_SHA512 -DNO_SHA224 -DNO_SHA -DNO_ERROR_STRINGS -DNO_FILESYSTEM -DBENCH_EMBEDDED -DWOLFSSL_SMALL_STACK" \
               ./configure \
               --host=arm-none-eabi \
               --disable-examples \
               --disable-rsa \
               --disable-chacha \
               --disable-poly1305 \
               --disable-dh \
               --disable-md5 \
               --disable-sha512 \
               --disable-sha224 \
               --disable-sha \
               --disable-sha384 \
               --disable-pwdbased \
               --disable-pkcs12 \
               --disable-tlsv12 \
               --disable-crypttests \
               --disable-benchmark \
               --enable-pkcallbacks \
               --enable-psa \
               --with-psa-include="${TRUSTED_FIRMWARE_DIR}/interface/include" && \
         make)
}

flash_tfm() {
    (cd "${TRUSTED_FIRMWARE_DIR}/build" && \
         ./regression.sh && \
         ./TFM_UPDATE.sh )
}

download_trusted_firmware_m
download_wolfssl_src
download_tfm_repo_test_src
compile_wolfssl
compile_tfm

echo "WolfSSL TF-M example built."
echo "To flash on the board run:"
echo "cd ${TRUSTED_FIRMWARE_DIR}/build && ./regression.sh && ./TFM_UPDATE.sh"

# flash_tfm
