#!/bin/bash
#
# configure.sh - Initialize Yocto build directory and write configuration
#
# Called by 'make configure'. Safe to re-run — managed config block is replaced.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
LAYERS_DIR="${SCRIPT_DIR}/layers"
POKY_DIR="${LAYERS_DIR}/poky"

if [ ! -d "${POKY_DIR}" ]; then
    echo "Error: poky not found. Run ./setup.sh first."
    exit 1
fi

# --------------------------------------------------------------------------
# Initialize build directory if needed
# --------------------------------------------------------------------------

if [ ! -d "${BUILD_DIR}/conf" ]; then
    echo "[INIT] Initializing build directory..."
    source "${POKY_DIR}/oe-init-build-env" "${BUILD_DIR}" > /dev/null 2>&1
fi

# --------------------------------------------------------------------------
# Write bblayers.conf
# --------------------------------------------------------------------------

echo "[CONFIG] Writing bblayers.conf..."

cat > "${BUILD_DIR}/conf/bblayers.conf" <<BBLAYERS
# POKY_BBLAYERS_CONF_VERSION is increased each time build/conf/bblayers.conf
# changes incompatibly
POKY_BBLAYERS_CONF_VERSION = "2"

BBPATH = "\${TOPDIR}"
BBFILES ?= ""

BBLAYERS ?= " \\
  ${LAYERS_DIR}/poky/meta \\
  ${LAYERS_DIR}/poky/meta-poky \\
  ${LAYERS_DIR}/meta-openembedded/meta-oe \\
  ${LAYERS_DIR}/meta-openembedded/meta-python \\
  ${LAYERS_DIR}/meta-openembedded/meta-networking \\
  ${LAYERS_DIR}/meta-openembedded/meta-multimedia \\
  ${LAYERS_DIR}/meta-raspberrypi \\
  ${LAYERS_DIR}/meta-wolfssl \\
  ${LAYERS_DIR}/meta-network-overrides \\
  "
BBLAYERS

# --------------------------------------------------------------------------
# Write RPi5 config block in local.conf
# --------------------------------------------------------------------------

echo "[CONFIG] Writing RPi5 configuration to local.conf..."

CONFIG_MARKER="# ==== meta-wolfssl-linux-fips managed config ===="

# Remove any existing managed config block
sed -i "/${CONFIG_MARKER}/,/${CONFIG_MARKER} END/d" "${BUILD_DIR}/conf/local.conf"

# Append fresh config block
cat >> "${BUILD_DIR}/conf/local.conf" <<'LOCALCONF'
# ==== meta-wolfssl-linux-fips managed config ====
# This block is managed by 'make configure'. To change, edit configure.sh
# and re-run 'make configure'.

MACHINE = "raspberrypi5"

# ---- Image features ----
IMAGE_FEATURES += "ssh-server-openssh"

# ---- Display / GPU ----
MACHINE_FEATURES:append = " vc4graphics"
VC4DTBO = "vc4-kms-v3d"

# ---- Early HDMI console ----
# Force HDMI output and route kernel console to framebuffer
RPI_EXTRA_CONFIG = "hdmi_force_hotplug=1\nhdmi_group=1\nhdmi_mode=16\ndisable_overscan=1"
CMDLINE_DEBUG = "console=tty1"
SERIAL_CONSOLES:append = " 115200;tty1"

# ---- Wi-Fi and Bluetooth ----
DISTRO_FEATURES:append = " wifi bluetooth"
IMAGE_INSTALL:append = " linux-firmware-rpidistro-bcm43455 bluez5 wpa-supplicant network-config"
LICENSE_FLAGS_ACCEPTED:append = " synaptics-killswitch"

# ---- systemd ----
DISTRO_FEATURES:append = " systemd usrmerge"
VIRTUAL-RUNTIME_init_manager = "systemd"
VIRTUAL-RUNTIME_initscripts = "systemd-compat-units"

# ---- Serial console (debug) ----
ENABLE_UART = "1"

# ---- U-Boot ----
# Disabled — U-Boot can cause HDMI issues on RPi5, boot kernel directly
RPI_USE_U_BOOT = "0"

# ---- Shared caches (keep outside build dir for reuse) ----
DL_DIR ?= "${TOPDIR}/../downloads"
SSTATE_DIR ?= "${TOPDIR}/../sstate-cache"

# ---- Host SDK architecture ----
SDKMACHINE ?= "x86_64"

# ---- Disable SPDX license metadata generation ----
INHERIT:remove = "create-spdx"
# ==== meta-wolfssl-linux-fips managed config ==== END
LOCALCONF

# --------------------------------------------------------------------------
# Ensure default network config exists (DHCP)
# --------------------------------------------------------------------------

NETCONF="${SCRIPT_DIR}/layers/meta-network-overrides/recipes-core/network-config/files/20-wired.network"

if [ ! -f "${NETCONF}" ]; then
    echo "[CONFIG] Writing default network config (DHCP)..."
    mkdir -p "$(dirname "${NETCONF}")"
    cat > "${NETCONF}" <<'NETEOF'
[Match]
Name=end0 eth0

[Network]
DHCP=yes
NETEOF
fi

echo "[CONFIG] Done. Run 'make build' to build the image."
