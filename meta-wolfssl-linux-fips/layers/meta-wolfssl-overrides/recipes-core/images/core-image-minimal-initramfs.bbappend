# Add wolfSSL FIPS kernel module to initramfs for early boot loading

PACKAGE_INSTALL:append = " wolfssl-linuxkm"

inherit wolfssl-initramfs

ROOTFS_POSTPROCESS_COMMAND += "wolfssl_initramfs_run_depmod; "
ROOTFS_POSTPROCESS_COMMAND += "wolfssl_initramfs_inject_after_loadmodules; "
