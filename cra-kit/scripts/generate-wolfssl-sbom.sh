#!/bin/sh
# Generate wolfSSL component SBOMs (autotools make sbom or embedded gen-sbom).
#   CRA_SBOM_MODE=autotools|embedded   (default: autotools if configure+Makefile exist)
#   WOLFSSL_DIR=path/to/wolfssl
#   CRA_PYTHON=python3                 (optional: interpreter with pcpp for embedded path)
#   CRA_LICENSE_OVERRIDE=<SPDX-id>     (optional: e.g. LicenseRef-wolfSSL-Commercial)
#   CRA_LICENSE_TEXT=<path>            (required when CRA_LICENSE_OVERRIDE is a
#                                       LicenseRef-* id: the plain-text licence
#                                       embedded in the SBOM. gen-sbom / make sbom
#                                       hard-fail without it.)
set -eu

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
KIT_DIR=$(dirname "$SCRIPT_DIR")
# shellcheck disable=SC2015  # `|| true` is a deliberate set -e guard, not if-then-else
WOLFSSL_DIR=${WOLFSSL_DIR:-$(cd "$KIT_DIR/../../wolfssl" 2>/dev/null && pwd || true)}
OUT_DIR=${CRA_SBOM_OUT_DIR:-"$KIT_DIR/auditor-packet/wolfssl-component"}
VERSION_FILE="$KIT_DIR/VERSION"

if [ -z "${WOLFSSL_DIR:-}" ] || [ ! -d "$WOLFSSL_DIR" ]; then
    echo "ERROR: wolfSSL source not found." >&2
    echo "  Set WOLFSSL_DIR to your wolfssl checkout (sibling of wolfssl-examples)." >&2
    exit 1
fi

# shellcheck disable=SC1090
. "$VERSION_FILE" 2>/dev/null || true
VERSION=${WOLFSSL_VERSION:-5.9.1}

mkdir -p "$OUT_DIR"
CDX_OUT="$OUT_DIR/wolfssl-${VERSION}.cdx.json"
SPDX_OUT="$OUT_DIR/wolfssl-${VERSION}.spdx.json"

echo "wolfSSL tree: $WOLFSSL_DIR"
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
                echo "       Re-run with CRA_LICENSE_TEXT=/path/to/wolfssl-commercial-license.txt," >&2
                echo "       or use scripts/make-commercial-sample.sh to derive from the pinned GPL samples." >&2
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
# `make sbom` inside a `cd "$WOLFSSL_DIR"` subshell, where a relative path would
# otherwise resolve against the wolfSSL tree rather than the caller's CWD.
if [ -n "${CRA_LICENSE_TEXT:-}" ] && [ -f "$CRA_LICENSE_TEXT" ]; then
    CRA_LICENSE_TEXT=$(CDPATH='' cd -- "$(dirname -- "$CRA_LICENSE_TEXT")" && pwd)/$(basename -- "$CRA_LICENSE_TEXT")
    echo "License text:     $CRA_LICENSE_TEXT"
fi

# Pick a Python that can `import pcpp` (pip may target a different python3 than /usr/local/bin).
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

_embedded_srcs() {
    # Demo list only — production SBOMs must mirror every wolfSSL .c on your link line.
    # Outputs from this list are watermarked wolfssl:sbom:demo=true.
    for f in \
        "$WOLFSSL_DIR/wolfcrypt/src/aes.c" \
        "$WOLFSSL_DIR/wolfcrypt/src/sha.c" \
        "$WOLFSSL_DIR/wolfcrypt/src/sha256.c" \
        "$WOLFSSL_DIR/wolfcrypt/src/random.c" \
        "$WOLFSSL_DIR/wolfcrypt/src/ecc.c" \
        "$WOLFSSL_DIR/wolfcrypt/src/wc_port.c" \
        "$WOLFSSL_DIR/src/tls.c" \
        "$WOLFSSL_DIR/src/tls13.c" \
        "$WOLFSSL_DIR/src/keys.c"
    do
        if [ -f "$f" ]; then
            echo "$f"
        fi
    done
}

_run_embedded() {
    echo "==> Embedded path: gen-sbom with CRA Kit user_settings.h"
    echo "    NOTE: --srcs uses the kit's built-in 9-file DEMO list. Production SBOMs"
    echo "          must pass every wolfSSL .c file you compile. Output is watermarked"
    echo "          wolfssl:sbom:demo=true so this can never silently ship."
    if [ ! -f "$KIT_DIR/user_settings.h" ]; then
        echo "ERROR: $KIT_DIR/user_settings.h missing (demo settings for WOLFSSL_USER_SETTINGS)." >&2
        exit 1
    fi
    GEN="$WOLFSSL_DIR/scripts/gen-sbom"
    if [ ! -f "$GEN" ]; then
        echo "ERROR: $GEN not found (need wolfSSL with SBOM support)." >&2
        exit 1
    fi

    SETTINGS_H="$WOLFSSL_DIR/wolfssl/wolfcrypt/settings.h"
    if [ ! -f "$SETTINGS_H" ]; then
        echo "ERROR: $SETTINGS_H not found." >&2
        exit 1
    fi

    # Build the positional list of source files newline-safely so paths that
    # contain spaces survive (POSIX sh has no arrays; unquoted command
    # substitution would word-split and corrupt such paths).
    set --
    while IFS= read -r _src; do
        [ -n "$_src" ] || continue
        set -- "$@" "$_src"
    done <<EOF
$(_embedded_srcs)
EOF

    # Optional commercial license override (LicenseRef-wolfSSL-Commercial etc).
    # A LicenseRef-* override must be accompanied by --license-text (validated
    # up front above); a stock SPDX id needs no text.
    set -- "$@" --cdx-out "$CDX_OUT" --spdx-out "$SPDX_OUT"
    if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
        set -- "$@" --license-override "$CRA_LICENSE_OVERRIDE"
        if [ -n "${CRA_LICENSE_TEXT:-}" ]; then
            set -- "$@" --license-text "$CRA_LICENSE_TEXT"
        fi
    fi

    if _py=$(_python_with_pcpp); then
        echo "       Using $_py (pcpp) for --user-settings"
        "$_py" "$GEN" \
            --name wolfssl --version "$VERSION" \
            --license-file "$WOLFSSL_DIR/LICENSING" \
            --user-settings "$SETTINGS_H" \
            --user-settings-include "$WOLFSSL_DIR" \
            --user-settings-include "$KIT_DIR" \
            --user-settings-define WOLFSSL_USER_SETTINGS \
            --srcs "$@"
        return 0
    fi

    echo "NOTE: pcpp not found for python3/python; using compiler -dM -E -> --options-h"
    echo "      Install pcpp on the same interpreter: python3 -m pip install pcpp"
    echo "      (conda users: pip install pcpp often targets conda python, not /usr/local/bin/python3)"
    echo "      Cross builds: set CC=arm-none-eabi-gcc (or your target compiler) so the"
    echo "      fallback reflects target macros, not the host's."

    # Use mktemp so the temp filename is unpredictable: a fixed PID-based name in
    # a shared/CI directory could be pre-created or raced by another job.
    DEFINES_H=$(mktemp "${TMPDIR:-/tmp}/wolfssl-defines.XXXXXX") || {
        echo "ERROR: mktemp failed for the defines temp file." >&2
        exit 1
    }
    # Clean up the temp defines file on every exit path, including a failing
    # generator run (it previously leaked the file under `set -e` if the
    # final gen-sbom invocation failed before the manual `rm -f`).
    trap 'rm -f "$DEFINES_H"' EXIT
    CC=${CC:-cc}
    if ! "$CC" -dM -E \
        -I"$WOLFSSL_DIR" \
        -I"$KIT_DIR" \
        -DWOLFSSL_USER_SETTINGS \
        -include "$SETTINGS_H" \
        -x c /dev/null >"$DEFINES_H" 2>/dev/null; then
        echo "ERROR: $CC -dM -E failed; install pcpp or set CC to your cross-compiler." >&2
        exit 1
    fi

    PYTHON=python3
    command -v python3 >/dev/null 2>&1 || PYTHON=python
    "$PYTHON" "$GEN" \
        --name wolfssl --version "$VERSION" \
        --license-file "$WOLFSSL_DIR/LICENSING" \
        --options-h "$DEFINES_H" \
        --srcs "$@"
}

_run_autotools() {
    echo "==> Autotools path: make sbom"
    # `make sbom` names its output after the wolfSSL TREE's version
    # (PACKAGE_VERSION), not the kit's pinned VERSION. If they differ, the
    # `cp` below would otherwise fail with a cryptic "No such file or
    # directory" under `set -eu`. Detect the mismatch early and explain it.
    _tree_ver=$(sed -n \
        's/.*LIBWOLFSSL_VERSION_STRING[[:space:]]*"\([^"]*\)".*/\1/p' \
        "$WOLFSSL_DIR/wolfssl/version.h" 2>/dev/null || true)
    if [ -n "$_tree_ver" ] && [ "$_tree_ver" != "$VERSION" ]; then
        echo "ERROR: wolfSSL tree is version $_tree_ver but the kit is pinned to $VERSION." >&2
        echo "       'make sbom' emits wolfssl-${_tree_ver}.* while the pinned auditor" >&2
        echo "       packet references wolfssl-${VERSION}.*. Check out a wolfSSL $VERSION" >&2
        echo "       tree, or update cra-kit/VERSION (and the pinned sample references)." >&2
        exit 1
    fi
  (cd "$WOLFSSL_DIR" && {
      if [ ! -f Makefile ]; then
          echo "       Running ./configure first..."
          ./configure
      fi
      if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
          if [ -n "${CRA_LICENSE_TEXT:-}" ]; then
              make sbom SBOM_LICENSE_OVERRIDE="$CRA_LICENSE_OVERRIDE" \
                        SBOM_LICENSE_TEXT="$CRA_LICENSE_TEXT"
          else
              make sbom SBOM_LICENSE_OVERRIDE="$CRA_LICENSE_OVERRIDE"
          fi
      else
          make sbom
      fi
      cp -f "wolfssl-${VERSION}.cdx.json" "$CDX_OUT"
      cp -f "wolfssl-${VERSION}.spdx.json" "$SPDX_OUT"
      if [ -f "wolfssl-${VERSION}.spdx" ]; then
          cp -f "wolfssl-${VERSION}.spdx" "$OUT_DIR/"
      fi
  })
}

MODE=${CRA_SBOM_MODE:-}
case "$MODE" in
    embedded) _run_embedded ;;
    autotools) _run_autotools ;;
    "")
        if [ -f "$WOLFSSL_DIR/Makefile" ] && [ -f "$WOLFSSL_DIR/configure" ]; then
            MODE=autotools
            _run_autotools
        else
            MODE=embedded
            _run_embedded
        fi
        ;;
    *)
        echo "ERROR: CRA_SBOM_MODE must be 'autotools' or 'embedded', not '$MODE'" >&2
        exit 1
        ;;
esac

# ---- Post-process: demo watermarks (+ defensive PURL canonicalization) ----
# Current gen-sbom already emits pkg:github/wolfSSL/wolfssl@vX natively, so the
# PURL rewrite below is a defensive no-op kept only for older wolfSSL trees that
# emitted pkg:generic/wolfssl@X. The substantive step here is the demo
# watermark: embedded outputs from the kit's 9-file demo --srcs list get a
# wolfssl:sbom:demo property so a downstream auditor cannot mistake them for
# production-complete SBOMs.
if ! CDX_OUT="$CDX_OUT" SPDX_OUT="$SPDX_OUT" CRA_SBOM_MODE_FINAL="$MODE" \
python3 <<'PY'
import json, os, pathlib

cdx = pathlib.Path(os.environ["CDX_OUT"])
spdx = pathlib.Path(os.environ["SPDX_OUT"])
demo = os.environ.get("CRA_SBOM_MODE_FINAL") == "embedded"

GENERIC = "pkg:generic/wolfssl@"
GITHUB = "pkg:github/wolfSSL/wolfssl@v"

def canonicalize_purl(s):
    if isinstance(s, str) and s.startswith(GENERIC):
        return GITHUB + s[len(GENERIC):]
    return s

if cdx.exists():
    d = json.loads(cdx.read_text())
    comp = d.get("metadata", {}).get("component", {})
    comp["purl"] = canonicalize_purl(comp.get("purl", ""))
    if demo:
        props = comp.setdefault("properties", [])
        if not any(p.get("name") == "wolfssl:sbom:demo" for p in props):
            props.append({
                "name": "wolfssl:sbom:demo",
                "value": "true (built-in --srcs list, not production-complete)"
            })
    cdx.write_text(json.dumps(d, indent=2) + "\n")
    print(f"Post-processed {cdx.name}" + (": demo watermark added" if demo else ": no changes needed"))

if spdx.exists():
    d = json.loads(spdx.read_text())
    for pkg in d.get("packages", []):
        for ref in pkg.get("externalRefs", []):
            if ref.get("referenceType") == "purl":
                ref["referenceLocator"] = canonicalize_purl(ref.get("referenceLocator", ""))
        if demo:
            existing = pkg.get("comment", "")
            marker = "DEMO ARTIFACT (built-in --srcs list, not production-complete)."
            if marker not in existing:
                pkg["comment"] = (marker + " " + existing).strip()
    spdx.write_text(json.dumps(d, indent=2) + "\n")
    print(f"Post-processed {spdx.name}" + (": demo watermark added" if demo else ": no changes needed"))
PY
then
    echo "ERROR: post-process failed (PURL canonicalization/watermarking incomplete)." >&2
    echo "       The emitted SBOM may carry pkg:generic PURLs or lack demo watermarks; not trusting it." >&2
    exit 1
fi

echo "Done."
