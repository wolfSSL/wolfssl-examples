#!/bin/bash
# Wrap an image with the extended-boot header and program it to QSPI
# 0x60100000 on KIT_PSE84_AI through the on-board KitProg3.
#
# Usage: tools/flash.sh Build/wolfcrypt_test.bin
#
# Environment: MTB_PROGTOOLS (default /opt/Tools/ModusToolboxProgtools-1.6),
# KITPROG_SERIAL to pick the probe when several are attached.

set -e

IMG=${1:?usage: $0 image.bin}
MTB_PROGTOOLS=${MTB_PROGTOOLS:-/opt/Tools/ModusToolboxProgtools-1.6}
OCD=$MTB_PROGTOOLS/openocd
SCRIPTS=$MTB_PROGTOOLS/mtb-programmer/scripts
HERE=$(cd "$(dirname "$0")" && pwd)

WRAPPED=$(mktemp --suffix=.bin)
trap 'rm -f "$WRAPPED"' EXIT
python3 "$HERE/mcuboot_wrap.py" "$IMG" "$WRAPPED"

SERIAL_ARGS=()
if [ -n "$KITPROG_SERIAL" ]; then
    SERIAL_ARGS=(-c "adapter serial $KITPROG_SERIAL")
fi

# "reset run" would leave the part in KitProg3 acquire mode.
"$OCD/bin/openocd" -s "$OCD/scripts" -s "$SCRIPTS" \
    -c "set SCRIPTS_DIR $SCRIPTS" \
    -f interface/kitprog3.cfg "${SERIAL_ARGS[@]}" -c "transport select swd" \
    -f CAT1D/kit_PSE84_AI_external.cfg \
    -c "init; reset init" \
    -c "flash write_image erase $WRAPPED 0x60100000 bin" \
    -c "verify_image $WRAPPED 0x60100000 bin" \
    -c "reset halt; resume; shutdown"
