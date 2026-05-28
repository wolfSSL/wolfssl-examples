# Apply wolfSSL kernel randomness patches for FIPS DRBG integration
# Adds callback hooks to drivers/char/random.c and include/linux/random.h
# allowing wolfSSL kernel module (libwolfssl.ko) to register its FIPS-certified
# DRBG implementation with the kernel.

inherit wolfssl-kernel-random
WOLFSSL_KERNEL_RANDOM_PATCH = "6.12"

# Enable kernel FIPS mode and crypto subsystem
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append = " file://fips-crypto.cfg"
