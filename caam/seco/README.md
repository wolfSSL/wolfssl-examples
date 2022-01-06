Example use cases when compiling wolfSSL with --enable-caam=seco

## Setup Envirnment

Setup the location to SECO HSM and NVM library, cryptodev header (if wolfSSL is built to look for it), wolfSSL library path, and libz install path. The following is examples of setting the envirnment variables:

export SECO_PATH=/home/user/imx-seco-libs/export/usr
export DEVCRYPTO_PATH=/home/user/build-xwayland/sysroots-components/aarch64/cryptodev-linux/usr
export WOLFSSL_PATH=/home/user/wolfssl-install
export ZLIB_PATH=/home/user/zlib-aarch64-install
