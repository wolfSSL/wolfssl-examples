# RPi5 (Cortex-A76 aarch64) — enable ARM assembly and LKCAPI registration
EXTRA_OECONF:append:raspberrypi5 = " --enable-linuxkm-lkcapi-register=all-kconfig"

# QEMU (aarch64) — enable LKCAPI registration for virtual machine testing
EXTRA_OECONF:append:qemuarm64 = " --enable-linuxkm-lkcapi-register=all-kconfig"

# Sign the module with the kernel's own key to prevent "module verification failed" taint
require ${WOLFSSL_LAYERDIR}/inc/wolfssl-linuxkm/wolfssl-linuxkm-sign-module.inc
