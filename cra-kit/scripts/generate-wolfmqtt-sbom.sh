#!/bin/sh
# Generate wolfMQTT component SBOM (autotools make sbom, or embedded gen-sbom).
#
# Mode selection:
#   CRA_SBOM_MODE=autotools|embedded   (default: autotools)
#     autotools: runs `make sbom` inside the wolfMQTT tree
#     embedded:  runs gen-sbom directly over the MQTT protocol sources, hashing
#                them with an OmniBOR gitoid Merkle hash. Use this when wolfMQTT
#                is compiled into firmware (ESP-IDF, Arduino, STM32, bare-metal)
#                and there is no .so/.a to hash.
#
# Required variables:
#   WOLFSSL_DIR=path/to/wolfssl     (source tree root; must contain scripts/gen-sbom)
#   WOLFMQTT_DIR=path/to/wolfMQTT  (source tree root)
#
# Embedded-mode variables (CRA_SBOM_MODE=embedded):
#   CRA_SBOM_SRCS_FILE=path/to/srcs.txt (explicit .c list, one path per line;
#                                         used verbatim — highest priority)
#   CRA_SBOM_KEIL_PROJECT=path/to/x.uvprojx (parse Keil project for .c sources)
#   CRA_SBOM_IAR_PROJECT=path/to/x.ewp   (parse IAR project for .c sources)
#   CRA_SBOM_MAKEFILE_DIR=path/to/dir    (run `make -n` to extract .c sources)
#   CRA_SBOM_BUILD_DIR=path/to/build    (CMake/ESP-IDF build dir; sources are
#                                         read from its compile_commands.json)
#   CRA_SBOM_NO_HASH=true               (emit SBOM without an artifact hash,
#                                         skipping the source list — for NDA
#                                         customers who cannot share source lists;
#                                         WARNING: not suitable for production compliance)
#
# Optional variables:
#   CRA_LICENSE_OVERRIDE=<SPDX-id>  (e.g. LicenseRef-wolfSSL-Commercial)
#   CRA_LICENSE_TEXT=<path>          (required when CRA_LICENSE_OVERRIDE is LicenseRef-*)
#   CRA_SBOM_OUT_DIR=<path>          (default: <kit>/auditor-packet/wolfmqtt-component)
set -eu
# Enable pipefail when the shell supports it (bash/ksh/some dash builds).
# Plain POSIX sh may not; tolerate its absence so the script still runs.
# shellcheck disable=SC3040
if (set -o pipefail) 2>/dev/null; then set -o pipefail; fi

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
KIT_DIR=$(dirname "$SCRIPT_DIR")

# Shared source-extraction helper (Keil/IAR/Makefile/compile_commands.json).
# shellcheck disable=SC1091  # sourced helper, resolved at runtime
. "$SCRIPT_DIR/_cra-sbom-extract.sh"

# shellcheck disable=SC2015
# shellcheck disable=SC2015  # fallback to unset on cd failure is intentional
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

_run_autotools() {
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
}

_run_embedded() {
    echo "==> Embedded path: gen-sbom over MQTT protocol sources"

    # wolfMQTT's MQTT protocol implementation lives entirely in src/mqtt_*.c.
    # Unlike wolfTPM there are no platform-specific HAL files to exclude and no
    # host-only sources, so the mqtt_*.c glob is the correct default source set.
    #
    # wolfcrypt/wolfSSL crypto sources and wolfSSL TLS sources are deliberately
    # NOT hashed here: they are a separate component with their own SBOM,
    # produced by generate-wolfssl-sbom.sh. Mixing them in would double-count
    # the crypto component and misattribute its provenance to wolfMQTT.
    if [ ! -d "$WOLFMQTT_DIR/src" ]; then
        echo "ERROR: $WOLFMQTT_DIR/src not found; cannot locate MQTT sources." >&2
        exit 1
    fi

    GEN="$WOLFSSL_DIR/scripts/gen-sbom"
    if [ ! -f "$GEN" ]; then
        echo "ERROR: $GEN not found (need wolfSSL with SBOM support)." >&2
        exit 1
    fi

    PY=$(command -v python3 2>/dev/null || command -v python 2>/dev/null || true)
    if [ -z "$PY" ]; then
        echo "ERROR: python3 (or python) not found; required to run gen-sbom." >&2
        exit 1
    fi

    # wolfMQTT ships its own licence file; detect the SPDX id from it (not from
    # the wolfSSL tree, which carries a different LICENSING file).
    LICENSE_FILE=""
    for _lf in "$WOLFMQTT_DIR/LICENSE" "$WOLFMQTT_DIR/COPYING" "$WOLFMQTT_DIR/LICENSING"; do
        if [ -f "$_lf" ]; then
            LICENSE_FILE="$_lf"
            break
        fi
    done
    if [ -z "$LICENSE_FILE" ]; then
        echo "ERROR: no LICENSE/COPYING/LICENSING file found in $WOLFMQTT_DIR." >&2
        exit 1
    fi

    SRCS_LIST=$(mktemp "${TMPDIR:-/tmp}/wolfmqtt-srcs.XXXXXX") || {
        echo "ERROR: mktemp failed for the source-list temp file." >&2
        exit 1
    }
    # gen-sbom requires exactly one of --options-h / --user-settings to source
    # its build properties. Those describe wolfSSL's crypto/TLS configuration,
    # which belongs to the wolfssl component's SBOM, not wolfMQTT's. Feed an
    # empty options file so gen-sbom records no (and thus no misattributed)
    # build defines for the wolfMQTT component.
    EMPTY_OPTS=$(mktemp "${TMPDIR:-/tmp}/wolfmqtt-opts.XXXXXX") || {
        echo "ERROR: mktemp failed for the empty options temp file." >&2
        exit 1
    }
    # _cra_auto_tempfiles collects any temp files the shared extractor creates;
    # initialise it so the EXIT trap is safe under `set -u` even when the
    # extractor adds nothing (e.g. the default-glob path).
    _cra_auto_tempfiles=""
    trap 'rm -f "$SRCS_LIST" "$EMPTY_OPTS" $_cra_auto_tempfiles' EXIT

    # CRA_SBOM_NO_HASH emits a placeholder checksum and skips the source list
    # entirely (for NDA customers who cannot share source lists).
    if [ "${CRA_SBOM_NO_HASH:-}" = "true" ] || [ "${CRA_SBOM_NO_HASH:-}" = "1" ]; then
        echo "    NOTE: CRA_SBOM_NO_HASH=true: emitting SBOM without artifact hash."
        echo "          WARNING: not suitable for production CRA compliance." >&2
        _count=0
        set -- --no-artifact-hash --cdx-out "$CDX_OUT" --spdx-out "$SPDX_OUT"
    else
        # Resolve the source list via the shared extractor (CRA_SBOM_SRCS_FILE,
        # Keil/IAR projects, Makefile dry-run, or compile_commands.json). It
        # returns 2 when no extraction method is selected, in which case we
        # fall back to the default mqtt_*.c glob.
        _cra_rc=0
        _cra_extract_srcs "$WOLFMQTT_DIR" "wolfmqtt" "$SRCS_LIST" || _cra_rc=$?

        if [ "$_cra_rc" -eq 2 ]; then
            # No extraction method active: use default glob (all src/mqtt_*.c
            # sorted).  MQTT has no HAL split, so the full source set is the
            # right default for bare-metal builds without an extractable list.
            echo "    Source list: default glob $WOLFMQTT_DIR/src/mqtt_*.c"
            for _c in "$WOLFMQTT_DIR"/src/mqtt_*.c; do
                [ -f "$_c" ] && echo "$_c"
            done | sort > "$SRCS_LIST"
        elif [ "$_cra_rc" -ne 0 ]; then
            exit 1
        fi

        if [ ! -s "$SRCS_LIST" ]; then
            echo "ERROR: no MQTT source files found to hash." >&2
            exit 1
        fi

        # Validate every resolved path exists before handing the file to gen-sbom.
        _count=0
        while IFS= read -r _src; do
            [ -n "$_src" ] || continue
            if [ ! -f "$_src" ]; then
                echo "ERROR: listed source not found: $_src" >&2
                exit 1
            fi
            _count=$((_count + 1))
        done < "$SRCS_LIST"

        set -- --srcs-file "$SRCS_LIST" --cdx-out "$CDX_OUT" --spdx-out "$SPDX_OUT"
    fi
    if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
        set -- "$@" --license-override "$CRA_LICENSE_OVERRIDE"
        if [ -n "${CRA_LICENSE_TEXT:-}" ]; then
            set -- "$@" --license-text "$CRA_LICENSE_TEXT"
        fi
    fi

    "$PY" "$GEN" \
        --name wolfmqtt --version "$VERSION" \
        --license-file "$LICENSE_FILE" \
        --options-h "$EMPTY_OPTS" \
        "$@" || {
            echo "ERROR: gen-sbom failed." >&2
            exit 1
        }

    if [ "${CRA_SBOM_NO_HASH:-}" = "true" ] || [ "${CRA_SBOM_NO_HASH:-}" = "1" ]; then
        echo "NOTE: artifact hash omitted (CRA_SBOM_NO_HASH)"
    else
        echo "NOTE: hashed ${_count} source file(s)"
    fi
}

case "${CRA_SBOM_MODE:-autotools}" in
    autotools) _run_autotools ;;
    embedded)  _run_embedded ;;
    *)
        echo "ERROR: unknown CRA_SBOM_MODE='${CRA_SBOM_MODE:-}' (expected 'autotools' or 'embedded')" >&2
        exit 1
        ;;
esac

# Verify the generator actually produced non-empty SBOM files.
for _out in "$CDX_OUT" "$SPDX_OUT"; do
    if [ ! -s "$_out" ]; then
        echo "ERROR: expected SBOM output missing or empty: $_out" >&2
        exit 1
    fi
done

echo "Done."
