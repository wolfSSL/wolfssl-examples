Example use cases when compiling wolfSSL with --enable-caam=seco

## Setup Envirnment

Setup the location to SECO HSM and NVM library, cryptodev header (if wolfSSL is built to look for it), wolfSSL library path, and libz install path. The following is examples of setting the envirnment variables:

export HSM_DIR=/home/user/imx-seco-libs/export/usr
export CRYPTODEV_DIR=/home/user/build-xwayland/sysroots-components/aarch64/cryptodev-linux/usr/include
export WOLFSSL_PATH=/home/user/wolfssl-install
export ZLIB_DIR=/home/user/zlib-aarch64-install

## Building wolfSSL
This is an example configure for building wolfSSL
./configure --host=aarch64-poky-linux --with-libz=$ZLIB_DIR --with-seco=$HSM_DIR --enable-caam=seco --enable-cmac --enable-aesgcm --enable-aesccm --enable-keygen CPPFLAGS="-DHAVE_AES_ECB -I$CRYPTODEV_DIR" --enable-devcrypto=seco --enable-curve25519 --enable-static --prefix=$WOLFSSL_PATH

## Additional cryptodev-linux Examples
Examples for use in conjunction with SECO are in the cryptodev directory
