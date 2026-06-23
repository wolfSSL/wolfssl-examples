#!/bin/sh
# Generate wolfSSH component SBOM (autotools make sbom).
#
# Required variables:
#   WOLFSSL_DIR=path/to/wolfssl         (source tree root; provides gen-sbom)
#   WOLFSSH_DIR=path/to/wolfssh         (source tree root)
#
# Optional variables:
#   CRA_LICENSE_OVERRIDE=<SPDX-id>      (e.g. LicenseRef-wolfSSH-Commercial)
#   CRA_LICENSE_TEXT=<path>             (required when CRA_LICENSE_OVERRIDE is a
#                                        LicenseRef-* id: plain-text licence embedded
#                                        in the SBOM; make sbom hard-fails without it.)
set -eu

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
KIT_DIR=$(dirname "$SCRIPT_DIR")
# shellcheck disable=SC2015  # `|| true` is a deliberate set -e guard, not if-then-else
WOLFSSL_DIR=${WOLFSSL_DIR:-$(cd "$KIT_DIR/../../wolfssl" 2>/dev/null && pwd || true)}
WOLFSSH_DIR=${WOLFSSH_DIR:-$(cd "$KIT_DIR/../../wolfSSH" 2>/dev/null && pwd || true)}
OUT_DIR=${CRA_SBOM_OUT_DIR:-"$KIT_DIR/auditor-packet/wolfssh-component"}

if [ -z "${WOLFSSL_DIR:-}" ] || [ ! -d "$WOLFSSL_DIR" ]; then
    echo "ERROR: wolfSSL source not found." >&2
    echo "  Set WOLFSSL_DIR to your wolfssl checkout (sibling of wolfssl-examples)." >&2
    exit 1
fi

if [ -z "${WOLFSSH_DIR:-}" ] || [ ! -d "$WOLFSSH_DIR" ]; then
    echo "ERROR: wolfSSH source not found." >&2
    echo "  Set WOLFSSH_DIR to your wolfSSH checkout (sibling of wolfssl-examples)." >&2
    exit 1
fi

VERSION=$(sed -n \
    's/.*LIBWOLFSSH_VERSION_STRING[[:space:]]*"\([^"]*\)".*/\1/p' \
    "$WOLFSSH_DIR/wolfssh/version.h" 2>/dev/null || true)
if [ -z "$VERSION" ]; then
    echo "ERROR: could not extract LIBWOLFSSH_VERSION_STRING from $WOLFSSH_DIR/wolfssh/version.h" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
CDX_OUT="$OUT_DIR/wolfssh-${VERSION}.cdx.json"
SPDX_OUT="$OUT_DIR/wolfssh-${VERSION}.spdx.json"

echo "wolfSSL tree: $WOLFSSL_DIR"
echo "wolfSSH tree: $WOLFSSH_DIR"
echo "Outputs:      $CDX_OUT"
echo "              $SPDX_OUT"
if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
    echo "License override: $CRA_LICENSE_OVERRIDE"
fi

# A LicenseRef-* override (e.g. the commercial license) requires the actual
# licence text to be embedded in the SBOM (SPDX 2.3 §10.1). Both gen-sbom and
# `make sbom` hard-fail without it, so catch the omission here with an
# actionable message instead of letting the run die deep in the generator.
if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
    case "$CRA_LICENSE_OVERRIDE" in
        LicenseRef-*)
            if [ -z "${CRA_LICENSE_TEXT:-}" ]; then
                echo "ERROR: CRA_LICENSE_OVERRIDE=$CRA_LICENSE_OVERRIDE is a LicenseRef-* identifier," >&2
                echo "       but CRA_LICENSE_TEXT is not set. SPDX 2.3 requires the licence text to be" >&2
                echo "       embedded for any LicenseRef-* used in licenseConcluded/licenseDeclared." >&2
                echo "       Re-run with CRA_LICENSE_TEXT=/path/to/wolfssh-commercial-license.txt." >&2
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
# `cd "$WOLFSSH_DIR"` subshell, where a relative path would resolve against the
# wolfSSH tree rather than the caller's CWD.
if [ -n "${CRA_LICENSE_TEXT:-}" ] && [ -f "$CRA_LICENSE_TEXT" ]; then
    CRA_LICENSE_TEXT=$(CDPATH='' cd -- "$(dirname -- "$CRA_LICENSE_TEXT")" && pwd)/$(basename -- "$CRA_LICENSE_TEXT")
    echo "License text:     $CRA_LICENSE_TEXT"
fi

_run_autotools() {
    echo "==> Autotools path: make sbom"
    # `make sbom` names its output after the wolfSSH TREE's version
    # (PACKAGE_VERSION). Detect mismatches early so the cp below doesn't fail
    # with a cryptic "No such file or directory" under `set -eu`.
    _tree_ver=$(sed -n \
        's/.*LIBWOLFSSH_VERSION_STRING[[:space:]]*"\([^"]*\)".*/\1/p' \
        "$WOLFSSH_DIR/wolfssh/version.h" 2>/dev/null || true)
    if [ -n "$_tree_ver" ] && [ "$_tree_ver" != "$VERSION" ]; then
        echo "ERROR: wolfSSH tree is version $_tree_ver but expected $VERSION." >&2
        exit 1
    fi
  (cd "$WOLFSSH_DIR" && {
      if [ ! -f Makefile ]; then
          echo "       Running ./configure first..."
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
      cp -f "wolfssh-${VERSION}.cdx.json" "$CDX_OUT"
      cp -f "wolfssh-${VERSION}.spdx.json" "$SPDX_OUT"
      if [ -f "wolfssh-${VERSION}.spdx" ]; then
          cp -f "wolfssh-${VERSION}.spdx" "$OUT_DIR/"
      fi
  })
}

_run_autotools

# ---- Post-process: defensive PURL canonicalization ----
# Current gen-sbom already emits pkg:github/wolfSSL/wolfSSH@vX natively for
# known component names; the rewrite below is a defensive no-op kept only for
# older generator versions that may emit pkg:generic/wolfssh@X.
if ! CDX_OUT="$CDX_OUT" SPDX_OUT="$SPDX_OUT" \
   python3 <<'PY'
import json, os, pathlib

cdx = pathlib.Path(os.environ["CDX_OUT"])
spdx = pathlib.Path(os.environ["SPDX_OUT"])

GENERIC = "pkg:generic/wolfssh@"
GITHUB = "pkg:github/wolfSSL/wolfSSH@v"

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
