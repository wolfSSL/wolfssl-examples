# Fix n_fold unsigned underflow segfault in KRB5KDF with small constants
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append = " file://0001-fix-krb5kdf-n_fold-unsigned-underflow.patch"

# Disable the feature check for manual image configuration
require ${WOLFSSL_LAYERDIR}/inc/wolfssl-manual-config.inc

# Enable unit tests for wolfprovider replace default mode
require ${WOLFSSL_LAYERDIR}/inc/wolfprovider/wolfprovider-enable-replace-default-unittest.inc
require ${WOLFSSL_LAYERDIR}/inc/wolfprovider/wolfprovider-enable-unittest.inc
