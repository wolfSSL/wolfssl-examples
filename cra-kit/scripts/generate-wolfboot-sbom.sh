#!/bin/sh
# Generate wolfBoot component SBOMs via gen-sbom.
#
# wolfBoot is build-configuration-specific: its compiled source list depends
# on TARGET, SIGN, HASH, and EXT_FLASH.  This script runs `make -n` against
# the wolfBoot tree to extract the exact set of .c files for the requested
# configuration, then calls gen-sbom directly.
#
# wolfcrypt sources are compiled directly into the wolfBoot image (there is
# no separate wolfssl shared library).  They appear in OBJS alongside core
# wolfBoot sources and are therefore included as wolfBoot's own component
# sources, not as a separate dependency.
#
# Required variables:
#   WOLFBOOT_DIR=path/to/wolfBoot     (source tree root)
#   WOLFBOOT_TARGET=<target>          (e.g. stm32h7, x86_64_efi)
#   WOLFBOOT_SIGN=<scheme>            (e.g. ECC256, RSA2048, ED25519)
#
# Optional variables:
#   WOLFBOOT_HASH=<hash>              (default: SHA256)
#   WOLFBOOT_EXT_FLASH=<0|1>         (default: 0)
#   CRA_PYTHON=python3               (Python interpreter with gen-sbom deps)
#   CRA_LICENSE_OVERRIDE=<SPDX-id>  (e.g. LicenseRef-wolfBoot-Commercial)
#   CRA_LICENSE_TEXT=<path>          (required when CRA_LICENSE_OVERRIDE is a
#                                     LicenseRef-* id: plain-text license
#                                     embedded in the SBOM)
#   CRA_SBOM_OUT_DIR=<path>          (override output directory)
#   CRA_SBOM_SRCS_FILE=path          explicit .c file list (overrides make -n)
#   CRA_SBOM_KEIL_PROJECT=path       auto-extract from Keil .uvprojx (overrides make -n)
#   CRA_SBOM_IAR_PROJECT=path        auto-extract from IAR .ewp (overrides make -n)
#   CRA_SBOM_NO_HASH — not yet supported; blocked on SBOM-cgz (gen-sbom --no-artifact-hash)
set -eu

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
KIT_DIR=$(dirname "$SCRIPT_DIR")

# Shared source-extraction helper. It appends temp files it creates to
# _cra_auto_tempfiles, so the EXIT trap below cleans both lists.
_auto_tempfiles=""
_cra_auto_tempfiles=""
trap 'rm -f ${_auto_tempfiles:-} ${_cra_auto_tempfiles:-}' EXIT

# shellcheck source=_cra-sbom-extract.sh disable=SC1091
. "$SCRIPT_DIR/_cra-sbom-extract.sh"

# Default wolfBoot directory: sibling of the wolfssl-examples checkout.
# shellcheck disable=SC2015
WOLFBOOT_DIR=${WOLFBOOT_DIR:-$(cd "$KIT_DIR/../../wolfBoot" 2>/dev/null && pwd || true)}

if [ -z "${WOLFBOOT_DIR:-}" ] || [ ! -d "$WOLFBOOT_DIR" ]; then
    echo "ERROR: wolfBoot source not found." >&2
    echo "  Set WOLFBOOT_DIR to your wolfBoot checkout." >&2
    exit 1
fi

if [ -z "${WOLFBOOT_TARGET:-}" ]; then
    echo "ERROR: WOLFBOOT_TARGET is not set." >&2
    echo "  Example: WOLFBOOT_TARGET=stm32h7 $0" >&2
    exit 1
fi

if [ -z "${WOLFBOOT_SIGN:-}" ]; then
    echo "ERROR: WOLFBOOT_SIGN is not set." >&2
    echo "  Example: WOLFBOOT_SIGN=ECC256 $0" >&2
    exit 1
fi

WOLFBOOT_HASH=${WOLFBOOT_HASH:-SHA256}
WOLFBOOT_EXT_FLASH=${WOLFBOOT_EXT_FLASH:-0}

OUT_DIR=${CRA_SBOM_OUT_DIR:-"$KIT_DIR/auditor-packet/wolfboot-component"}

# gen-sbom lives inside the wolfssl submodule under wolfBoot.
GEN_SBOM="$WOLFBOOT_DIR/lib/wolfssl/scripts/gen-sbom"
if [ ! -f "$GEN_SBOM" ]; then
    echo "ERROR: gen-sbom not found at $GEN_SBOM" >&2
    echo "  Ensure the wolfssl submodule is initialized:" >&2
    echo "    git -C \"$WOLFBOOT_DIR\" submodule update --init lib/wolfssl" >&2
    exit 1
fi

# Extract version from wolfBoot's version header.
VERSION=$(sed -n \
    's/.*LIBWOLFBOOT_VERSION_STRING[[:space:]]*"\([^"]*\)".*/\1/p' \
    "$WOLFBOOT_DIR/include/wolfboot/version.h")
if [ -z "$VERSION" ]; then
    echo "ERROR: could not detect wolfBoot version from $WOLFBOOT_DIR/include/wolfboot/version.h" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
CDX_OUT="$OUT_DIR/wolfboot-${VERSION}.cdx.json"
SPDX_OUT="$OUT_DIR/wolfboot-${VERSION}.spdx.json"

echo "wolfBoot tree:  $WOLFBOOT_DIR"
echo "Configuration:  TARGET=$WOLFBOOT_TARGET SIGN=$WOLFBOOT_SIGN HASH=$WOLFBOOT_HASH EXT_FLASH=$WOLFBOOT_EXT_FLASH"
echo "Version:        $VERSION"
echo "Outputs:        $CDX_OUT"
echo "                $SPDX_OUT"
if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
    echo "License override: $CRA_LICENSE_OVERRIDE"
fi

# A LicenseRef-* override requires the license text to be embedded in the SBOM
# (SPDX 2.3 §10.1).  gen-sbom hard-fails without it; catch the omission here.
if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
    case "$CRA_LICENSE_OVERRIDE" in
        LicenseRef-*)
            if [ -z "${CRA_LICENSE_TEXT:-}" ]; then
                echo "ERROR: CRA_LICENSE_OVERRIDE=$CRA_LICENSE_OVERRIDE is a LicenseRef-* identifier," >&2
                echo "       but CRA_LICENSE_TEXT is not set. SPDX 2.3 requires the license text to be" >&2
                echo "       embedded for any LicenseRef-* used in licenseConcluded/licenseDeclared." >&2
                echo "       Re-run with CRA_LICENSE_TEXT=/path/to/wolfboot-license.txt" >&2
                exit 1
            fi
            if [ ! -f "$CRA_LICENSE_TEXT" ]; then
                echo "ERROR: CRA_LICENSE_TEXT=$CRA_LICENSE_TEXT not found." >&2
                exit 1
            fi
            ;;
    esac
fi

# Canonicalize CRA_LICENSE_TEXT to an absolute path.
if [ -n "${CRA_LICENSE_TEXT:-}" ] && [ -f "$CRA_LICENSE_TEXT" ]; then
    CRA_LICENSE_TEXT=$(CDPATH='' cd -- "$(dirname -- "$CRA_LICENSE_TEXT")" && pwd)/$(basename -- "$CRA_LICENSE_TEXT")
fi

# Extract the configuration-specific source list.
#
# Priority order:
#   1. CRA_SBOM_SRCS_FILE    explicit .c list
#   2. CRA_SBOM_KEIL_PROJECT Keil .uvprojx
#   3. CRA_SBOM_IAR_PROJECT  IAR .ewp
#   4. make -n TARGET/SIGN   wolfBoot's product-specific default (below)
#
# The shared helper handles 1-3. wolfBoot's Makefile path is product-specific
# (driven by TARGET/SIGN/HASH/EXT_FLASH, not the generic CRA_SBOM_MAKEFILE_DIR /
# compile_commands.json handlers), so we blank those two env vars before calling
# the helper and run our own make -n below when no IDE project is set.
_srcs_tmp=$(mktemp "${TMPDIR:-/tmp}/wolfboot-sbom-srcs.XXXXXX")
_auto_tempfiles="${_auto_tempfiles:-} $_srcs_tmp"

_saved_makefile_dir="${CRA_SBOM_MAKEFILE_DIR:-}"
_saved_build_dir="${CRA_SBOM_BUILD_DIR:-}"
CRA_SBOM_MAKEFILE_DIR=""
CRA_SBOM_BUILD_DIR=""

_cra_rc=0
_cra_extract_srcs "$WOLFBOOT_DIR" "wolfboot" "$_srcs_tmp" || _cra_rc=$?

CRA_SBOM_MAKEFILE_DIR="$_saved_makefile_dir"
CRA_SBOM_BUILD_DIR="$_saved_build_dir"

if [ "$_cra_rc" -eq 0 ]; then
    _n=$(wc -l < "$_srcs_tmp" | tr -d ' ')
    echo "    Extracted $_n source files (from IDE project / source list)"
elif [ "$_cra_rc" -eq 2 ]; then
    # No IDE project set: use wolfBoot's product-specific make -n extraction.
    #
    # `make -n TARGET=... SIGN=...` prints every command make would run without
    # executing them.  The compiler invocations include every .c file on the
    # wolfBoot link line, including both core wolfBoot sources and wolfcrypt
    # files compiled inline.  grep -oE pulls out every .c argument; sort -u
    # deduplicates.
    echo "Extracting source list via make -n TARGET=$WOLFBOOT_TARGET SIGN=$WOLFBOOT_SIGN ..."
    make --no-print-directory \
        -C "$WOLFBOOT_DIR" \
        -n \
        TARGET="$WOLFBOOT_TARGET" \
        SIGN="$WOLFBOOT_SIGN" \
        HASH="$WOLFBOOT_HASH" \
        EXT_FLASH="$WOLFBOOT_EXT_FLASH" \
        2>/dev/null \
        | grep -oE '[^ ]+\.c' \
        | grep -v '\.h' \
        | sort -u > "$_srcs_tmp" || true

    if [ ! -s "$_srcs_tmp" ]; then
        echo "ERROR: make -n yielded no .c source files for TARGET=$WOLFBOOT_TARGET SIGN=$WOLFBOOT_SIGN." >&2
        echo "       Check that TARGET and SIGN are valid for this wolfBoot tree." >&2
        exit 1
    fi

    _n=$(wc -l < "$_srcs_tmp" | tr -d ' ')
    echo "    Extracted $_n source files"
else
    exit 1
fi

# Resolve paths to absolute: make -n emits relative paths; gen-sbom needs to
# open the files to compute gitoid hashes.
_srcs_abs_tmp=$(mktemp "${TMPDIR:-/tmp}/wolfboot-sbom-srcs-abs.XXXXXX")
_auto_tempfiles="${_auto_tempfiles:-} $_srcs_abs_tmp"

while IFS= read -r _src; do
    [ -n "$_src" ] || continue
    # Paths from make -n are relative to wolfBoot root.
    if [ "${_src#/}" = "$_src" ]; then
        _abs="$WOLFBOOT_DIR/$_src"
    else
        _abs="$_src"
    fi
    # Skip paths that do not exist on disk (generated files, stubs, etc.).
    if [ -f "$_abs" ]; then
        echo "$_abs"
    fi
done < "$_srcs_tmp" > "$_srcs_abs_tmp"

if [ ! -s "$_srcs_abs_tmp" ]; then
    echo "ERROR: no source files from make -n exist on disk." >&2
    echo "       Verify WOLFBOOT_DIR=$WOLFBOOT_DIR and submodules are initialized." >&2
    exit 1
fi

_n_abs=$(wc -l < "$_srcs_abs_tmp" | tr -d ' ')
echo "    Resolved $_n_abs paths ($(( _n - _n_abs )) non-existent skipped)"

# Preprocess build settings for gen-sbom --options-h.
#
# wolfBoot has no options.h (it is not an autotools project).  We use
# cc -dM -E on the host with wolfBoot's include dirs to produce a flat
# #define file that gen-sbom can parse for algorithm enablement.
_defines_tmp=$(mktemp "${TMPDIR:-/tmp}/wolfboot-sbom-defines.XXXXXX")
_auto_tempfiles="${_auto_tempfiles:-} $_defines_tmp"

CC=${CC:-cc}
echo "    Preprocessing build settings via $CC -dM -E ..."
if ! "$CC" -dM -E \
    -I"$WOLFBOOT_DIR/include" \
    -I"$WOLFBOOT_DIR/lib/wolfssl" \
    -I"$WOLFBOOT_DIR/lib/wolfssl/wolfcrypt/src" \
    -DWOLFSSL_USER_SETTINGS \
    -x c /dev/null > "$_defines_tmp" 2>/dev/null; then
    echo "ERROR: $CC -dM -E failed; install a host C compiler or set CC." >&2
    exit 1
fi

# Build gen-sbom argument list.
_PYTHON=${CRA_PYTHON:-python3}
command -v "$_PYTHON" >/dev/null 2>&1 || \
    { echo "ERROR: $_PYTHON not found. Set CRA_PYTHON to your Python interpreter." >&2; exit 1; }

# Read absolute source paths into positional parameters.
# ponytail: gen-sbom lacks --srcs-file; pass list as positional args
# ceiling: ARG_MAX; upgrade path: SBOM-cgz adds --srcs-file to gen-sbom
set --
while IFS= read -r _src; do
    [ -n "$_src" ] || continue
    set -- "$@" "$_src"
done < "$_srcs_abs_tmp"

_license_override=${CRA_LICENSE_OVERRIDE:-GPL-3.0-only}
set -- "$@" \
    --cdx-out "$CDX_OUT" \
    --spdx-out "$SPDX_OUT" \
    --license-override "$_license_override"

if [ -n "${CRA_LICENSE_TEXT:-}" ]; then
    set -- "$@" --license-text "$CRA_LICENSE_TEXT"
fi

echo "==> Running gen-sbom ..."
"$_PYTHON" "$GEN_SBOM" \
    --name wolfboot \
    --version "$VERSION" \
    --supplier "wolfSSL Inc." \
    --license-file "$WOLFBOOT_DIR/LICENSE" \
    --options-h "$_defines_tmp" \
    --srcs "$@"

echo "SBOM written:"
echo "  $CDX_OUT"
echo "  $SPDX_OUT"
