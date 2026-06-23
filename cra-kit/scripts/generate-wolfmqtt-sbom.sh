#!/bin/sh
# Generate wolfMQTT component SBOM (autotools make sbom path).
#
# wolfMQTT uses autotools. This script runs `make sbom` inside the wolfMQTT
# source tree and copies the resulting SBOM files to the auditor packet.
#
# Required variables:
#   WOLFSSL_DIR=path/to/wolfssl     (source tree root; must contain scripts/gen-sbom)
#   WOLFMQTT_DIR=path/to/wolfMQTT  (source tree root)
#
# Optional variables:
#   CRA_LICENSE_OVERRIDE=<SPDX-id>  (e.g. LicenseRef-wolfSSL-Commercial)
#   CRA_LICENSE_TEXT=<path>          (required when CRA_LICENSE_OVERRIDE is LicenseRef-*)
#   CRA_SBOM_OUT_DIR=<path>          (default: <kit>/auditor-packet/wolfmqtt-component)
set -eu

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
KIT_DIR=$(dirname "$SCRIPT_DIR")

# shellcheck disable=SC2015
WOLFSSL_DIR=${WOLFSSL_DIR:-$(cd "$KIT_DIR/../../wolfssl" 2>/dev/null && pwd || true)}
WOLFMQTT_DIR=${WOLFMQTT_DIR:-}

if [ -z "${WOLFSSL_DIR:-}" ] || [ ! -d "$WOLFSSL_DIR" ]; then
    echo "ERROR: wolfSSL source not found." >&2
    echo "  Set WOLFSSL_DIR to your wolfssl checkout (contains scripts/gen-sbom)." >&2
    exit 1
fi

if [ -z "${WOLFMQTT_DIR:-}" ] || [ ! -d "$WOLFMQTT_DIR" ]; then
    echo "ERROR: WOLFMQTT_DIR is not set or not a directory." >&2
    echo "  Set WOLFMQTT_DIR to your wolfMQTT source tree." >&2
    exit 1
fi

GEN="$WOLFSSL_DIR/scripts/gen-sbom"
if [ ! -f "$GEN" ]; then
    echo "ERROR: $GEN not found (need wolfSSL with SBOM support)." >&2
    exit 1
fi

# Parse version from wolfmqtt/version.h.
VERSION=$(sed -n \
    's/.*LIBWOLFMQTT_VERSION_STRING[[:space:]]*"\([^"]*\)".*/\1/p' \
    "$WOLFMQTT_DIR/wolfmqtt/version.h" 2>/dev/null || true)
if [ -z "$VERSION" ]; then
    echo "ERROR: could not parse version from $WOLFMQTT_DIR/wolfmqtt/version.h." >&2
    exit 1
fi

OUT_DIR=${CRA_SBOM_OUT_DIR:-"$KIT_DIR/auditor-packet/wolfmqtt-component"}
mkdir -p "$OUT_DIR"
CDX_OUT="$OUT_DIR/wolfmqtt-${VERSION}.cdx.json"
SPDX_OUT="$OUT_DIR/wolfmqtt-${VERSION}.spdx.json"

echo "wolfMQTT tree: $WOLFMQTT_DIR"
echo "wolfSSL tree:  $WOLFSSL_DIR"
echo "Version:       $VERSION"
echo "Outputs:       $CDX_OUT"
echo "               $SPDX_OUT"
if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
    echo "License override: $CRA_LICENSE_OVERRIDE"
fi

# A LicenseRef-* override requires the licence text to be embedded (SPDX 2.3 §10.1).
if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
    case "$CRA_LICENSE_OVERRIDE" in
        LicenseRef-*)
            if [ -z "${CRA_LICENSE_TEXT:-}" ]; then
                echo "ERROR: CRA_LICENSE_OVERRIDE=$CRA_LICENSE_OVERRIDE is a LicenseRef-* identifier," >&2
                echo "       but CRA_LICENSE_TEXT is not set. SPDX 2.3 requires the licence text" >&2
                echo "       to be embedded. Re-run with CRA_LICENSE_TEXT=/path/to/license.txt." >&2
                exit 1
            fi
            if [ ! -f "$CRA_LICENSE_TEXT" ]; then
                echo "ERROR: CRA_LICENSE_TEXT=$CRA_LICENSE_TEXT not found." >&2
                exit 1
            fi
            ;;
    esac
fi

# Canonicalize CRA_LICENSE_TEXT to an absolute path: make sbom runs inside a
# subshell `cd "$WOLFMQTT_DIR"`, where a relative path would resolve against
# the wolfMQTT tree rather than the caller's CWD.
if [ -n "${CRA_LICENSE_TEXT:-}" ] && [ -f "$CRA_LICENSE_TEXT" ]; then
    CRA_LICENSE_TEXT=$(CDPATH='' cd -- "$(dirname -- "$CRA_LICENSE_TEXT")" && pwd)/$(basename -- "$CRA_LICENSE_TEXT")
    echo "License text:  $CRA_LICENSE_TEXT"
fi

echo "==> Autotools path: make sbom"

# Detect whether the wolfMQTT tree is already configured; run ./configure first
# if no Makefile is present.
(cd "$WOLFMQTT_DIR" && {
    if [ ! -f Makefile ]; then
        echo "       Running ./configure first (WOLFSSL_DIR=$WOLFSSL_DIR)..."
        ./configure --with-wolfssl="$WOLFSSL_DIR"
    fi
    if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
        if [ -n "${CRA_LICENSE_TEXT:-}" ]; then
            make sbom WOLFSSL_DIR="$WOLFSSL_DIR" \
                      SBOM_LICENSE_OVERRIDE="$CRA_LICENSE_OVERRIDE" \
                      SBOM_LICENSE_TEXT="$CRA_LICENSE_TEXT"
        else
            make sbom WOLFSSL_DIR="$WOLFSSL_DIR" \
                      SBOM_LICENSE_OVERRIDE="$CRA_LICENSE_OVERRIDE"
        fi
    else
        make sbom WOLFSSL_DIR="$WOLFSSL_DIR"
    fi
    cp -f "wolfmqtt-${VERSION}.cdx.json" "$CDX_OUT"
    cp -f "wolfmqtt-${VERSION}.spdx.json" "$SPDX_OUT"
    if [ -f "wolfmqtt-${VERSION}.spdx" ]; then
        cp -f "wolfmqtt-${VERSION}.spdx" "$OUT_DIR/"
    fi
})

echo "Done."
