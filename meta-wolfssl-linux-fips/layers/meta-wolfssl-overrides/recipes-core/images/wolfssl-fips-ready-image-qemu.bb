SUMMARY = "QEMU image with wolfSSL FIPS Ready cryptography"
DESCRIPTION = "QEMU (aarch64) image based on core-image-base with wolfSSL FIPS Ready \
crypto backend powering libgcrypt, gnutls, and wolfProvider (replace-default mode). \
Use this to test the wolfSSL FIPS Ready integration without physical hardware."

LICENSE = "MIT"

inherit wolfssl-compatibility

python __anonymous() {
    virtual_provider = d.getVar('PREFERRED_PROVIDER_virtual/wolfssl') or ''
    wolfssl_provider = d.getVar('PREFERRED_PROVIDER_wolfssl') or ''

    valid_fips_providers = ('wolfssl-fips', 'wolfssl-fips-ready')

    if virtual_provider not in valid_fips_providers:
        bb.fatal("wolfssl-fips-ready-image-qemu requires PREFERRED_PROVIDER_virtual/wolfssl "
                 "to be one of %s. Current: '%s'. "
                 "Run 'make generate-fips-conf' and 'make fips-on' first."
                 % (valid_fips_providers, virtual_provider))

    if wolfssl_provider not in valid_fips_providers:
        bb.fatal("wolfssl-fips-ready-image-qemu requires PREFERRED_PROVIDER_wolfssl "
                 "to be one of %s. Current: '%s'. "
                 "Run 'make generate-fips-conf' and 'make fips-on' first."
                 % (valid_fips_providers, wolfssl_provider))
}

require recipes-core/images/core-image-base.bb

# NOTE: INITRAMFS_IMAGE and INITRAMFS_IMAGE_BUNDLE are set in conf/qemu-override.conf
# (they must be global so linux-yocto picks them up, not scoped to this image recipe)

# wolfSSL FIPS Ready core
IMAGE_INSTALL:append = " wolfssl wolfcrypttest wolfcryptbenchmark"

# FIPS Ready subsystems
IMAGE_INSTALL:append = " libgcrypt libgcrypt-ptest"
IMAGE_INSTALL:append = " gnutls gnutls-dev gnutls-bin gnutls-fips"
IMAGE_INSTALL:append = " wolfssl-gnutls-wrapper wolfssl-gnutls-wrapper-dev"
IMAGE_INSTALL:append = " wolfprovider openssl openssl-bin wolfprovidercmd wolfproviderenv"

# Test utilities
IMAGE_INSTALL:append = " ptest-runner bash make glibc-utils binutils ldd curl cryptsetup"
