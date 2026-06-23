#!/bin/sh
# Generate wolfSentry component SBOMs via gen-sbom.
#
# Required variables:
#   WOLFSENTRY_DIR=path/to/wolfsentry   (source tree root)
#
# gen-sbom location (one required):
#   WOLFSSL_DIR=path/to/wolfssl         (gen-sbom taken from scripts/gen-sbom)
#   CRA_GEN_SBOM=path/to/gen-sbom      (direct path; overrides WOLFSSL_DIR)
#
# Optional variables:
#   CRA_SBOM_OUT_DIR=<path>             (default: $KIT_DIR/auditor-packet/wolfsentry-component)
#   CC=<compiler>                       (default: cc; for -dM -E options dump)
set -eu

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
KIT_DIR=$(dirname "$SCRIPT_DIR")

# shellcheck disable=SC2015
WOLFSENTRY_DIR=${WOLFSENTRY_DIR:-$(cd "$KIT_DIR/../../wolfsentry" 2>/dev/null && pwd || true)}
OUT_DIR=${CRA_SBOM_OUT_DIR:-"$KIT_DIR/auditor-packet/wolfsentry-component"}

if [ -z "${WOLFSENTRY_DIR:-}" ] || [ ! -d "$WOLFSENTRY_DIR" ]; then
    echo "ERROR: wolfSentry source not found." >&2
    echo "  Set WOLFSENTRY_DIR to your wolfsentry checkout (sibling of wolfssl-examples)." >&2
    exit 1
fi

# Resolve gen-sbom: CRA_GEN_SBOM takes precedence, then WOLFSSL_DIR.
if [ -n "${CRA_GEN_SBOM:-}" ]; then
    GEN_SBOM="$CRA_GEN_SBOM"
elif [ -n "${WOLFSSL_DIR:-}" ]; then
    GEN_SBOM="$WOLFSSL_DIR/scripts/gen-sbom"
else
    echo "ERROR: gen-sbom location not specified." >&2
    echo "  Set WOLFSSL_DIR (path to wolfssl repo) or CRA_GEN_SBOM (direct path to gen-sbom)." >&2
    exit 1
fi

if [ ! -f "$GEN_SBOM" ]; then
    echo "ERROR: gen-sbom not found: $GEN_SBOM" >&2
    exit 1
fi

# Extract version from wolfsentry/wolfsentry.h macros.
HEADER="$WOLFSENTRY_DIR/wolfsentry/wolfsentry.h"
if [ ! -f "$HEADER" ]; then
    echo "ERROR: version header not found: $HEADER" >&2
    exit 1
fi
VERSION=$(awk '
    /WOLFSENTRY_VERSION_MAJOR/ { maj=$3 }
    /WOLFSENTRY_VERSION_MINOR/ { min=$3 }
    /WOLFSENTRY_VERSION_TINY/  { tiny=$3 }
    END { print maj"."min"."tiny }
' "$HEADER")
if [ -z "$VERSION" ] || [ "$VERSION" = ".." ]; then
    echo "ERROR: could not extract version from $HEADER" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
CDX_OUT="$OUT_DIR/wolfsentry-${VERSION}.cdx.json"
SPDX_OUT="$OUT_DIR/wolfsentry-${VERSION}.spdx.json"

echo "wolfSentry tree: $WOLFSENTRY_DIR"
echo "Version:         $VERSION"
echo "gen-sbom:        $GEN_SBOM"
echo "Outputs:         $CDX_OUT"
echo "                 $SPDX_OUT"

# Enumerate all .c sources from $WOLFSENTRY_DIR/src/ (the directory the Makefile
# compiles from; conditional sources like json/ and lwip/ are subdirs of src/).
SRCS=$(find "$WOLFSENTRY_DIR/src" -name "*.c" | sort)
if [ -z "$SRCS" ]; then
    echo "ERROR: no .c files found under $WOLFSENTRY_DIR/src/" >&2
    exit 1
fi
_n=$(echo "$SRCS" | wc -l | tr -d ' ')
echo "Sources:         $_n .c files from $WOLFSENTRY_DIR/src/"

# Dump compiler defines for --options-h (no user_settings.h; wolfsentry is
# configured via Makefile flags, not a settings header).
CC=${CC:-cc}
_defines_h=$(mktemp "${TMPDIR:-/tmp}/wolfsentry-defines.XXXXXX")
trap 'rm -f "$_defines_h"' EXIT
if ! "$CC" -dM -E -I"$WOLFSENTRY_DIR" -x c /dev/null >"$_defines_h" 2>/dev/null; then
    echo "ERROR: $CC -dM -E failed; set CC to an available compiler." >&2
    exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
    echo "ERROR: python3 not found in PATH." >&2
    exit 1
fi

# Build --srcs argument list from the source enumeration.
# shellcheck disable=SC2086
set -- $SRCS

python3 "$GEN_SBOM" \
    --name wolfsentry \
    --version "$VERSION" \
    --supplier "wolfSSL Inc." \
    --license-file "$WOLFSENTRY_DIR/LICENSING" \
    --options-h "$_defines_h" \
    --srcs "$@" \
    --cdx-out "$CDX_OUT" \
    --spdx-out "$SPDX_OUT"

# Post-process: rewrite pkg:generic/wolfsentry@X -> pkg:github/wolfSSL/wolfsentry@vX
# gen-sbom emits pkg:generic/{name}@{version} for non-wolfssl names; the canonical
# PURL for wolfsentry is the GitHub package form.
if ! CDX_OUT="$CDX_OUT" SPDX_OUT="$SPDX_OUT" VERSION="$VERSION" \
python3 <<'PY'
import json, os, pathlib

cdx  = pathlib.Path(os.environ["CDX_OUT"])
spdx = pathlib.Path(os.environ["SPDX_OUT"])
ver  = os.environ["VERSION"]

GENERIC = "pkg:generic/wolfsentry@"
GITHUB  = "pkg:github/wolfSSL/wolfsentry@v"

def fix(s):
    if isinstance(s, str) and s.startswith(GENERIC):
        return GITHUB + s[len(GENERIC):]
    return s

if cdx.exists():
    d = json.loads(cdx.read_text())
    comp = d.get("metadata", {}).get("component", {})
    comp["purl"] = fix(comp.get("purl", ""))
    cdx.write_text(json.dumps(d, indent=2) + "\n")
    print(f"Post-processed {cdx.name}: PURL -> {comp['purl']}")

if spdx.exists():
    d = json.loads(spdx.read_text())
    for pkg in d.get("packages", []):
        for ref in pkg.get("externalRefs", []):
            if ref.get("referenceType") == "purl":
                ref["referenceLocator"] = fix(ref.get("referenceLocator", ""))
    spdx.write_text(json.dumps(d, indent=2) + "\n")
    print(f"Post-processed {spdx.name}: PURL canonicalized")
PY
then
    echo "ERROR: PURL post-processing failed; SBOM may carry pkg:generic PURLs." >&2
    exit 1
fi

echo "Done."
