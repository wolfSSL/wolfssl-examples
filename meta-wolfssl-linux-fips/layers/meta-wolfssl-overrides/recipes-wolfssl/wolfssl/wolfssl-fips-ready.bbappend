# Configure wolfSSL FIPS Ready with libgcrypt, gnutls, and wolfProvider support
#
# These .inc files from meta-wolfssl enable the necessary wolfSSL configure
# options for each subsystem to use wolfSSL FIPS Ready as the crypto backend.

require ${WOLFSSL_LAYERDIR}/inc/wolfssl-fips-ready/wolfssl-enable-libgcrypt.inc
require ${WOLFSSL_LAYERDIR}/inc/wolfssl-fips-ready/wolfssl-enable-gnutls.inc
require ${WOLFSSL_LAYERDIR}/inc/wolfprovider/wolfssl-enable-wolfprovider-fips-ready.inc

# Fix for bundle missing stamp-h.in required by automake
do_configure_create_stamph() {
    if [ ! -f ${S}/stamp-h.in ]; then
        touch ${S}/stamp-h.in
    fi
}

addtask do_configure_create_stamph after do_patch before do_configure
