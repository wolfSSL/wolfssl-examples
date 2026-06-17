#!/bin/sh
# Force embedded gen-sbom (user_settings.h + --srcs) into wolfssl-component-embedded/.
set -eu

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
KIT_DIR=$(dirname "$SCRIPT_DIR")
export CRA_SBOM_MODE=embedded
export CRA_SBOM_OUT_DIR="$KIT_DIR/auditor-packet/wolfssl-component-embedded"
exec "$SCRIPT_DIR/generate-wolfssl-sbom.sh"
