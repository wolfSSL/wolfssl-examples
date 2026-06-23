#!/bin/sh
# Generate wolfTPM component SBOMs (autotools make sbom, cmake sbom, or direct gen-sbom).
#
# Mode selection:
#   CRA_SBOM_MODE=autotools|cmake
#     autotools (default when configure+Makefile exist): runs `make sbom`
#     cmake: auto-extracts sources from compile_commands.json and runs gen-sbom directly
#
# Required variables:
#   WOLFSSL_DIR=path/to/wolfssl         (source tree root; provides gen-sbom)
#   WOLFTPM_DIR=path/to/wolftpm         (source tree root)
#
# Mode-specific variables:
#   WOLFTPM_BUILD_DIR=path/to/build     (cmake mode: path to cmake build directory;
#                                         triggers compile_commands.json auto-extraction)
#
# Optional variables:
#   CRA_LICENSE_OVERRIDE=<SPDX-id>      (e.g. LicenseRef-wolfTPM-Commercial)
#   CRA_LICENSE_TEXT=<path>             (required when CRA_LICENSE_OVERRIDE is a
#                                        LicenseRef-* id: plain-text licence embedded
#                                        in the SBOM; gen-sbom / make sbom hard-fail
#                                        without it.)
set -eu

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
KIT_DIR=$(dirname "$SCRIPT_DIR")
# shellcheck disable=SC2015  # `|| true` is a deliberate set -e guard, not if-then-else
WOLFTPM_DIR=${WOLFTPM_DIR:-$(cd "$KIT_DIR/../../wolftpm" 2>/dev/null && pwd || true)}
WOLFSSL_DIR=${WOLFSSL_DIR:-$(cd "$KIT_DIR/../../wolfssl" 2>/dev/null && pwd || true)}
OUT_DIR=${CRA_SBOM_OUT_DIR:-"$KIT_DIR/auditor-packet/wolftpm-component"}

if [ -z "${WOLFTPM_DIR:-}" ] || [ ! -d "$WOLFTPM_DIR" ]; then
    echo "ERROR: wolfTPM source not found." >&2
    echo "  Set WOLFTPM_DIR to your wolftpm checkout." >&2
    exit 1
fi

if [ -z "${WOLFSSL_DIR:-}" ] || [ ! -d "$WOLFSSL_DIR" ]; then
    echo "ERROR: wolfSSL source not found (needed for gen-sbom)." >&2
    echo "  Set WOLFSSL_DIR to your wolfssl checkout." >&2
    exit 1
fi

GEN="$WOLFSSL_DIR/scripts/gen-sbom"
if [ ! -f "$GEN" ]; then
    echo "ERROR: $GEN not found (need wolfSSL with SBOM support)." >&2
    exit 1
fi

VERSION=$(sed -n \
    's/.*LIBWOLFTPM_VERSION_STRING[[:space:]]*"\([^"]*\)".*/\1/p' \
    "$WOLFTPM_DIR/wolftpm/version.h" 2>/dev/null || true)
if [ -z "$VERSION" ]; then
    echo "ERROR: could not extract version from $WOLFTPM_DIR/wolftpm/version.h" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
CDX_OUT="$OUT_DIR/wolftpm-${VERSION}.cdx.json"
SPDX_OUT="$OUT_DIR/wolftpm-${VERSION}.spdx.json"

echo "wolfTPM tree: $WOLFTPM_DIR"
echo "wolfSSL tree: $WOLFSSL_DIR"
echo "Outputs:      $CDX_OUT"
echo "              $SPDX_OUT"
if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
    echo "License override: $CRA_LICENSE_OVERRIDE"
fi

# A LicenseRef-* override requires the actual licence text to be embedded
# in the SBOM (SPDX 2.3 §10.1). Catch the omission early.
if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
    case "$CRA_LICENSE_OVERRIDE" in
        LicenseRef-*)
            if [ -z "${CRA_LICENSE_TEXT:-}" ]; then
                echo "ERROR: CRA_LICENSE_OVERRIDE=$CRA_LICENSE_OVERRIDE is a LicenseRef-* identifier," >&2
                echo "       but CRA_LICENSE_TEXT is not set. SPDX 2.3 requires the licence text to be" >&2
                echo "       embedded for any LicenseRef-* used in licenseConcluded/licenseDeclared." >&2
                echo "       Re-run with CRA_LICENSE_TEXT=/path/to/wolftpm-commercial-license.txt" >&2
                exit 1
            fi
            if [ ! -f "$CRA_LICENSE_TEXT" ]; then
                echo "ERROR: CRA_LICENSE_TEXT=$CRA_LICENSE_TEXT not found." >&2
                exit 1
            fi
            ;;
    esac
fi

# Canonicalize CRA_LICENSE_TEXT to an absolute path: the autotools path runs
# `make sbom` inside a `cd "$WOLFTPM_DIR"` subshell, where a relative path would
# otherwise resolve against the wolfTPM tree rather than the caller's CWD.
if [ -n "${CRA_LICENSE_TEXT:-}" ] && [ -f "$CRA_LICENSE_TEXT" ]; then
    CRA_LICENSE_TEXT=$(CDPATH='' cd -- "$(dirname -- "$CRA_LICENSE_TEXT")" && pwd)/$(basename -- "$CRA_LICENSE_TEXT")
    echo "License text:     $CRA_LICENSE_TEXT"
fi

# Accumulator for temp files; cleaned up on exit.
_auto_tempfiles=""
trap 'rm -f ${_auto_tempfiles:-}' EXIT

_auto_extract_srcs() {
    # Extract wolftpm sources from compile_commands.json (CMake build).
    if [ -n "${WOLFTPM_BUILD_DIR:-}" ] && [ -f "$WOLFTPM_BUILD_DIR/compile_commands.json" ]; then
        _ccdb="$WOLFTPM_BUILD_DIR/compile_commands.json"
        if ! command -v jq >/dev/null 2>&1; then
            echo "ERROR: jq is required to auto-extract sources from compile_commands.json." >&2
            echo "       Install jq, or set CRA_SBOM_SRCS_FILE manually." >&2
            exit 1
        fi
        _auto=$(mktemp "${TMPDIR:-/tmp}/wolftpm-auto-srcs.XXXXXX")
        _auto_tempfiles="${_auto_tempfiles:-} $_auto"
        jq -r '.[].file' "$_ccdb" \
            | grep "^${WOLFTPM_DIR}/" \
            | grep -E '/src/[^/]+\.c$' \
            | sort -u > "$_auto"
        if [ -s "$_auto" ]; then
            _n=$(wc -l < "$_auto" | tr -d ' ')
            echo "    Auto-extracted $_n wolftpm sources from compile_commands.json"
            CRA_SBOM_SRCS_FILE="$_auto"
            return 0
        fi
        echo "    WARNING: compile_commands.json found but yielded no wolftpm sources." >&2
    fi
}

_run_autotools() {
    echo "==> Autotools path: make sbom"
    _tree_ver=$(sed -n \
        's/.*LIBWOLFTPM_VERSION_STRING[[:space:]]*"\([^"]*\)".*/\1/p' \
        "$WOLFTPM_DIR/wolftpm/version.h" 2>/dev/null || true)
    if [ -n "$_tree_ver" ] && [ "$_tree_ver" != "$VERSION" ]; then
        echo "ERROR: wolfTPM tree is version $_tree_ver but detected version is $VERSION." >&2
        exit 1
    fi
    (cd "$WOLFTPM_DIR" && {
        if [ ! -f Makefile ]; then
            echo "       Running ./configure first..."
            ./configure
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
        cp -f "wolftpm-${VERSION}.cdx.json" "$CDX_OUT"
        cp -f "wolftpm-${VERSION}.spdx.json" "$SPDX_OUT"
        if [ -f "wolftpm-${VERSION}.spdx" ]; then
            cp -f "wolftpm-${VERSION}.spdx" "$OUT_DIR/"
        fi
    })
}

_run_cmake() {
    echo "==> cmake path: gen-sbom with compile_commands.json source extraction"
    if [ -z "${WOLFTPM_BUILD_DIR:-}" ]; then
        echo "ERROR: WOLFTPM_BUILD_DIR is not set." >&2
        echo "       Set it to your cmake out-of-source build directory." >&2
        echo "       Example: cmake -B build && WOLFTPM_BUILD_DIR=\$PWD/build $0" >&2
        exit 1
    fi
    if [ ! -d "$WOLFTPM_BUILD_DIR" ]; then
        echo "ERROR: WOLFTPM_BUILD_DIR=$WOLFTPM_BUILD_DIR is not a directory." >&2
        exit 1
    fi

    CRA_SBOM_SRCS_FILE=""
    _auto_extract_srcs

    if [ -z "${CRA_SBOM_SRCS_FILE:-}" ]; then
        echo "ERROR: could not extract wolftpm sources from compile_commands.json." >&2
        echo "       Reconfigure cmake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON, or" >&2
        echo "       switch to autotools mode (CRA_SBOM_MODE=autotools)." >&2
        exit 1
    fi

    PYTHON3=$(command -v python3 2>/dev/null || command -v python 2>/dev/null || true)
    if [ -z "$PYTHON3" ]; then
        echo "ERROR: python3 not found in PATH." >&2
        exit 1
    fi

    set -- \
        --name wolftpm \
        --version "$VERSION" \
        --supplier "wolfSSL Inc." \
        --license-file "$WOLFTPM_DIR/LICENSE" \
        --srcs-file "$CRA_SBOM_SRCS_FILE" \
        --cdx-out "$CDX_OUT" \
        --spdx-out "$SPDX_OUT"
    if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
        set -- "$@" --license-override "$CRA_LICENSE_OVERRIDE"
        if [ -n "${CRA_LICENSE_TEXT:-}" ]; then
            set -- "$@" --license-text "$CRA_LICENSE_TEXT"
        fi
    fi
    "$PYTHON3" "$GEN" "$@"
}

MODE=${CRA_SBOM_MODE:-}
case "$MODE" in
    autotools) _run_autotools ;;
    cmake) _run_cmake ;;
    "")
        if [ -n "${WOLFTPM_BUILD_DIR:-}" ] && [ -d "${WOLFTPM_BUILD_DIR}" ]; then
            MODE=cmake
            _run_cmake
        elif [ -f "$WOLFTPM_DIR/Makefile" ] && [ -f "$WOLFTPM_DIR/configure" ]; then
            MODE=autotools
            _run_autotools
        else
            echo "ERROR: could not detect build mode." >&2
            echo "       Set CRA_SBOM_MODE=autotools or cmake, and ensure the build" >&2
            echo "       directory exists (WOLFTPM_BUILD_DIR) or configure has been run" >&2
            echo "       in WOLFTPM_DIR." >&2
            exit 1
        fi
        ;;
    *)
        echo "ERROR: CRA_SBOM_MODE must be 'autotools' or 'cmake', not '$MODE'" >&2
        exit 1
        ;;
esac

# ---- Post-process: PURL canonicalization ----
# gen-sbom emits pkg:generic/wolftpm@X by default; rewrite to the canonical
# pkg:github/wolfSSL/wolfTPM@vX form expected in the auditor packet.
if ! CDX_OUT="$CDX_OUT" SPDX_OUT="$SPDX_OUT" \
   python3 <<'PY'
import json, os, pathlib

cdx = pathlib.Path(os.environ["CDX_OUT"])
spdx = pathlib.Path(os.environ["SPDX_OUT"])

GENERIC = "pkg:generic/wolftpm@"
GITHUB  = "pkg:github/wolfSSL/wolfTPM@v"

def canonicalize_purl(s):
    if isinstance(s, str) and s.startswith(GENERIC):
        return GITHUB + s[len(GENERIC):]
    return s

if cdx.exists():
    d = json.loads(cdx.read_text())
    comp = d.get("metadata", {}).get("component", {})
    comp["purl"] = canonicalize_purl(comp.get("purl", ""))
    cdx.write_text(json.dumps(d, indent=2) + "\n")
    print(f"Post-processed {cdx.name}")

if spdx.exists():
    d = json.loads(spdx.read_text())
    for pkg in d.get("packages", []):
        for ref in pkg.get("externalRefs", []):
            if ref.get("referenceType") == "purl":
                ref["referenceLocator"] = canonicalize_purl(ref.get("referenceLocator", ""))
    spdx.write_text(json.dumps(d, indent=2) + "\n")
    print(f"Post-processed {spdx.name}")
PY
then
    echo "ERROR: post-process failed (PURL canonicalization incomplete)." >&2
    echo "       The emitted SBOM may carry pkg:generic PURLs; not trusting it." >&2
    exit 1
fi

echo "Done."
