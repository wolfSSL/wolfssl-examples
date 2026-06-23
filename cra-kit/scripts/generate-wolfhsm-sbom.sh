#!/bin/sh
# Generate wolfHSM component SBOM (embedded gen-sbom path).
#
# wolfHSM is a Makefile-only library with no autotools configure step.
# This script always uses the embedded gen-sbom path: it enumerates
# wolfHSM sources directly from the source tree and derives compile-time
# defines via CC -dM -E (or pcpp when available).
#
# Required variables:
#   WOLFSSL_DIR=path/to/wolfssl   (source tree root; must contain scripts/gen-sbom)
#   WOLFHSM_DIR=path/to/wolfHSM  (source tree root)
#
# Optional variables:
#   CC=<compiler>                 (default: cc; set for cross builds)
#   CRA_PYTHON=python3            (interpreter with pcpp)
#   CRA_LICENSE_OVERRIDE=<SPDX>  (e.g. LicenseRef-wolfSSL-Commercial)
#   CRA_LICENSE_TEXT=<path>       (required when CRA_LICENSE_OVERRIDE is LicenseRef-*)
set -eu

# Accumulator for temp files; cleaned up on exit.
_auto_tempfiles=""
trap 'rm -f ${_auto_tempfiles:-}' EXIT

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
KIT_DIR=$(dirname "$SCRIPT_DIR")

# Locate WOLFSSL_DIR (default: sibling of wolfssl-examples).
# shellcheck disable=SC2015
WOLFSSL_DIR=${WOLFSSL_DIR:-$(cd "$KIT_DIR/../../wolfssl" 2>/dev/null && pwd || true)}
# WOLFHSM_DIR has no sensible default; must be explicit.
WOLFHSM_DIR=${WOLFHSM_DIR:-}

if [ -z "${WOLFSSL_DIR:-}" ] || [ ! -d "$WOLFSSL_DIR" ]; then
    echo "ERROR: wolfSSL source not found." >&2
    echo "  Set WOLFSSL_DIR to your wolfssl checkout." >&2
    exit 1
fi

if [ -z "${WOLFHSM_DIR:-}" ] || [ ! -d "$WOLFHSM_DIR" ]; then
    echo "ERROR: WOLFHSM_DIR is not set or not a directory." >&2
    echo "  Set WOLFHSM_DIR to your wolfHSM source tree." >&2
    exit 1
fi

GEN="$WOLFSSL_DIR/scripts/gen-sbom"
if [ ! -f "$GEN" ]; then
    echo "ERROR: $GEN not found (need wolfSSL with SBOM support)." >&2
    exit 1
fi

# Parse version from ChangeLog.md: first line matching "## wolfHSM Release vX.Y.Z"
VERSION=$(sed -n 's/^# wolfHSM Release v\([0-9][0-9.]*\).*/\1/p' \
    "$WOLFHSM_DIR/ChangeLog.md" 2>/dev/null | head -1)
if [ -z "$VERSION" ]; then
    echo "ERROR: could not parse version from $WOLFHSM_DIR/ChangeLog.md." >&2
    exit 1
fi

OUT_DIR=${CRA_SBOM_OUT_DIR:-"$KIT_DIR/auditor-packet/wolfhsm-component"}
mkdir -p "$OUT_DIR"
CDX_OUT="$OUT_DIR/wolfhsm-${VERSION}.cdx.json"
SPDX_OUT="$OUT_DIR/wolfhsm-${VERSION}.spdx.json"

echo "wolfHSM tree: $WOLFHSM_DIR"
echo "wolfSSL tree: $WOLFSSL_DIR"
echo "Version:      $VERSION"
echo "Outputs:      $CDX_OUT"
echo "              $SPDX_OUT"
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

# Canonicalize CRA_LICENSE_TEXT to absolute path (subshells resolve relative paths
# against their own CWD; gen-sbom may be invoked from a different directory).
if [ -n "${CRA_LICENSE_TEXT:-}" ] && [ -f "$CRA_LICENSE_TEXT" ]; then
    CRA_LICENSE_TEXT=$(CDPATH='' cd -- "$(dirname -- "$CRA_LICENSE_TEXT")" && pwd)/$(basename -- "$CRA_LICENSE_TEXT")
    echo "License text:     $CRA_LICENSE_TEXT"
fi

# Pick a Python that can `import pcpp`.
_python_with_pcpp() {
    for py in ${CRA_PYTHON:-} python3 python; do
        [ -n "$py" ] || continue
        if command -v "$py" >/dev/null 2>&1 && \
           "$py" -c "import pcpp" 2>/dev/null; then
            echo "$py"
            return 0
        fi
    done
    return 1
}

# Enumerate wolfHSM sources: all *.c directly under src/ (no recursion).
_wolfhsm_srcs() {
    find "$WOLFHSM_DIR/src" -maxdepth 1 -name "*.c" | sort
}

echo "==> Embedded path: gen-sbom with CC -dM -E (no user_settings.h)"

# Write collected source paths to a temp file for --srcs-file.
_srcs_file=$(mktemp "${TMPDIR:-/tmp}/wolfhsm-srcs.XXXXXX")
_auto_tempfiles="${_auto_tempfiles:-} $_srcs_file"
_wolfhsm_srcs > "$_srcs_file"
_n=$(wc -l < "$_srcs_file" | tr -d ' ')
echo "    Sources: $_n .c files from $WOLFHSM_DIR/src/"
if [ ! -s "$_srcs_file" ]; then
    echo "ERROR: no .c files found under $WOLFHSM_DIR/src/." >&2
    exit 1
fi

# Build license-override args.
_license_args=""
if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
    _license_args="--license-override $CRA_LICENSE_OVERRIDE"
    if [ -n "${CRA_LICENSE_TEXT:-}" ]; then
        _license_args="$_license_args --license-text $CRA_LICENSE_TEXT"
    fi
fi

if _py=$(_python_with_pcpp); then
    echo "    Using $_py (pcpp) for --user-settings"
    # wolfHSM has no user_settings.h equivalent; pass settings.h from wolfssl
    # so gen-sbom has a preprocessable configuration source.  The include path
    # covers wolfHSM headers and the wolfssl tree.
    SETTINGS_H="$WOLFSSL_DIR/wolfssl/wolfcrypt/settings.h"
    if [ ! -f "$SETTINGS_H" ]; then
        echo "ERROR: $SETTINGS_H not found." >&2
        exit 1
    fi
    # shellcheck disable=SC2086
    "$_py" "$GEN" \
        --name wolfhsm \
        --version "$VERSION" \
        --supplier "wolfSSL Inc." \
        --license-file "$WOLFHSM_DIR/LICENSING" \
        --user-settings "$SETTINGS_H" \
        --user-settings-include "$WOLFHSM_DIR" \
        --user-settings-include "$WOLFSSL_DIR" \
        --srcs-file "$_srcs_file" \
        --cdx-out "$CDX_OUT" \
        --spdx-out "$SPDX_OUT" \
        ${_license_args}
else
    echo "NOTE: pcpp not found; using CC -dM -E -> --options-h"
    echo "      Install pcpp: python3 -m pip install pcpp"
    echo "      For cross builds: set CC=<target-compiler>"

    CC=${CC:-cc}
    _defines=$(mktemp "${TMPDIR:-/tmp}/wolfhsm-defines.XXXXXX")
    _auto_tempfiles="${_auto_tempfiles:-} $_defines"
    if ! "$CC" -dM -E \
        -I"$WOLFHSM_DIR" \
        -I"$WOLFSSL_DIR" \
        -x c /dev/null >"$_defines" 2>/dev/null; then
        echo "ERROR: $CC -dM -E failed; install pcpp or set CC to your cross-compiler." >&2
        exit 1
    fi

    _python=python3
    command -v python3 >/dev/null 2>&1 || _python=python
    # shellcheck disable=SC2086
    "$_python" "$GEN" \
        --name wolfhsm \
        --version "$VERSION" \
        --supplier "wolfSSL Inc." \
        --license-file "$WOLFHSM_DIR/LICENSING" \
        --options-h "$_defines" \
        --srcs-file "$_srcs_file" \
        --cdx-out "$CDX_OUT" \
        --spdx-out "$SPDX_OUT" \
        ${_license_args}
fi

echo "Done."
