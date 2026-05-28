#!/bin/bash
#
# set-network.sh - Write systemd-networkd config for the RPi5 image
#
# Usage:
#   ./set-network.sh dhcp
#   ./set-network.sh static 192.168.1.100 [GATEWAY] [PREFIX] [DNS]
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NETCONF="${SCRIPT_DIR}/layers/meta-network-overrides/recipes-core/network-config/files/20-wired.network"

MODE="$1"
ADDR="$2"
GW="${3:-}"
PREFIX="${4:-24}"
DNS="${5:-8.8.8.8}"

if [ "${MODE}" = "dhcp" ]; then
    echo "[CONFIG] Setting network to DHCP..."
    cat > "${NETCONF}" <<'EOF'
[Match]
Name=end0 eth0

[Network]
DHCP=yes
EOF

elif [ "${MODE}" = "static" ] && [ -n "${ADDR}" ]; then
    # Auto-detect gateway if not provided: use .1 of the same subnet
    if [ -z "${GW}" ]; then
        GW="$(echo "${ADDR}" | sed 's/\.[0-9]*$/.1/')"
    fi

    echo "[CONFIG] Setting static IP: ${ADDR}/${PREFIX} gw ${GW} dns ${DNS}"
    cat > "${NETCONF}" <<EOF
[Match]
Name=end0 eth0

[Network]
Address=${ADDR}/${PREFIX}
Gateway=${GW}
DNS=${DNS}
EOF

else
    echo "Usage:"
    echo "  $0 dhcp"
    echo "  $0 static <IP> [GATEWAY] [PREFIX] [DNS]"
    echo ""
    echo "Examples:"
    echo "  $0 dhcp"
    echo "  $0 static 192.168.1.100"
    echo "  $0 static 192.168.1.100 192.168.1.1 24 8.8.8.8"
    exit 1
fi

echo ""
cat "${NETCONF}"
echo ""
echo "Rebuild the image with 'make build' to apply."
