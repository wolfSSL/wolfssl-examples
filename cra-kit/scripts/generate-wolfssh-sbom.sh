#!/bin/sh
# Generate wolfSSH component SBOM (autotools make sbom, or embedded gen-sbom).
#
# Mode selection:
#   CRA_SBOM_MODE=autotools|embedded   (default: autotools)
#     autotools: builds libwolfssh.so and runs `make sbom`
#     embedded:  hashes wolfSSH source files directly (no .so is produced when
#                wolfSSH is compiled into firmware), via wolfSSL's gen-sbom.
#
# Required variables:
#   WOLFSSL_DIR=path/to/wolfssl         (source tree root; provides gen-sbom)
#   WOLFSSH_DIR=path/to/wolfssh         (source tree root)
#
# Embedded-mode variables:
#   CRA_SBOM_BUILD_DIR=path/to/build    (embedded: dir containing
#                                        compile_commands.json for CMake / ESP-IDF
#                                        / Zephyr builds, used to extract the exact
#                                        wolfSSH .c files on the link line)
#   CRA_SBOM_SRCS_FILE=path/to/srcs.txt (embedded: explicit list of wolfSSH .c
#                                        paths, one per line; takes priority over
#                                        every other source-resolution method)
#   CRA_SBOM_KEIL_PROJECT=path          (embedded: auto-extract from Keil .uvprojx)
#   CRA_SBOM_IAR_PROJECT=path           (embedded: auto-extract from IAR .ewp)
#   CRA_SBOM_MAKEFILE_DIR=path          (embedded: auto-extract via make -n dry-run)
#   CRA_SBOM_NO_HASH=true               (embedded: emit SBOM without an artifact
#                                        hash, skipping the source list — for NDA
#                                        customers who cannot share source lists;
#                                        WARNING: not suitable for production compliance)
#
# Optional variables:
#   CRA_SBOM_OUT_DIR=<path>             (output directory; default auditor-packet)
#   CRA_LICENSE_OVERRIDE=<SPDX-id>      (e.g. LicenseRef-wolfSSH-Commercial)
#   CRA_LICENSE_TEXT=<path>             (required when CRA_LICENSE_OVERRIDE is a
#                                        LicenseRef-* id: plain-text licence embedded
#                                        in the SBOM; make sbom / gen-sbom hard-fail
#                                        without it.)
# POSIX sh (script is #!/bin/sh and run via `sh`); dash has no `set -o pipefail`,
# so we use `set -eu` like the rest of the kit. Pipelines that must not mask a
# failed first stage are checked explicitly instead.
set -eu

# Accumulator for temp files (embedded source extraction); cleaned up on exit.
# Why: mktemp temp files must not leak if any later command fails under set -e;
# a single EXIT trap removes them on every exit path including errors.
_auto_tempfiles=""
# _cra_auto_tempfiles is populated by _cra-sbom-extract.sh's helpers; clean both.
trap 'rm -f ${_auto_tempfiles:-} ${_cra_auto_tempfiles:-}' EXIT

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
KIT_DIR=$(dirname "$SCRIPT_DIR")

# shared extraction methods (Keil, IAR, Makefile, compile_commands.json)
. "$SCRIPT_DIR/_cra-sbom-extract.sh"
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

# Pick a Python that can `import pcpp` (pip may target a different python3 than
# the one first on PATH). pcpp lets gen-sbom walk settings.h without a compiler.
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

# Resolve the wolfSSH source list for embedded mode into the file named by $1.
#
# Source set rule: ONLY wolfSSH protocol sources (${WOLFSSH_DIR}/src/*.c) are
# product-owned and belong in this component. The wolfcrypt / wolfSSL sources
# that wolfSSH links against are NOT included here — they are a separate
# component covered by generate-wolfssl-sbom.sh and referenced as a dependency.
# Mixing them in would double-count the crypto library across two SBOMs.
#
# Priority order (first match wins), all but the last handled by
# _cra_extract_srcs from _cra-sbom-extract.sh:
#   1. CRA_SBOM_SRCS_FILE   — explicit user list beats anything inferred, because
#                             the user knows their exact link line.
#   2. CRA_SBOM_KEIL_PROJECT — parse Keil .uvprojx, filter to WOLFSSH_DIR.
#   3. CRA_SBOM_IAR_PROJECT  — parse IAR .ewp, filter to WOLFSSH_DIR.
#   4. CRA_SBOM_MAKEFILE_DIR — `make -n` dry-run, filter to WOLFSSH_DIR.
#   5. compile_commands.json at CRA_SBOM_BUILD_DIR — per-build extraction for
#                             CMake / ESP-IDF / Zephyr.
#   6. Default: all ${WOLFSSH_DIR}/src/*.c sorted — the "all sources" fallback
#                             for toolchains where we cannot infer the exact
#                             subset compiled. Listing all sources is the safe
#                             over-approximation: it never omits a file that
#                             shipped.
_resolve_wolfssh_srcs() {
    _out="$1"

    # Try every env-var-driven extraction method (SRCS_FILE, Keil, IAR,
    # Makefile, compile_commands.json). rc=2 means none was set: fall back to
    # the wolfSSH default glob below.
    _cra_rc=0
    _cra_extract_srcs "$WOLFSSH_DIR" "wolfssh" "$_out" || _cra_rc=$?
    if [ "$_cra_rc" -eq 0 ]; then
        return 0
    elif [ "$_cra_rc" -ne 2 ]; then
        exit 1  # _cra_extract_srcs already printed the error
    fi

    # No extraction env var set: every wolfSSH src/*.c. A POSIX glob expands in
    # sorted order; guard the no-match case where the pattern stays literal.
    : > "$_out"
    for _c in "$WOLFSSH_DIR"/src/*.c; do
        [ -f "$_c" ] || continue
        printf '%s\n' "$_c" >> "$_out"
    done
    if [ ! -s "$_out" ]; then
        echo "ERROR: no wolfSSH sources found in $WOLFSSH_DIR/src/*.c." >&2
        exit 1
    fi
    _n=$(wc -l < "$_out" | tr -d ' ')
    echo "    Using all $_n wolfSSH sources from $WOLFSSH_DIR/src/*.c (default)"
    return 0
}

_run_embedded() {
    echo "==> Embedded path: gen-sbom hashing wolfSSH source files"

    GEN="$WOLFSSL_DIR/scripts/gen-sbom"
    if [ ! -f "$GEN" ]; then
        echo "ERROR: $GEN not found (need a wolfSSL tree with SBOM support)." >&2
        exit 1
    fi

    # gen-sbom's embedded entry point walks wolfSSL's settings.h to resolve the
    # build config. wolfSSH headers pull in wolfSSL headers, so we reuse the same
    # settings.h + kit user_settings.h the wolfssl embedded path uses.
    SETTINGS_H="$WOLFSSL_DIR/wolfssl/wolfcrypt/settings.h"
    if [ ! -f "$SETTINGS_H" ]; then
        echo "ERROR: $SETTINGS_H not found." >&2
        exit 1
    fi
    if [ ! -f "$KIT_DIR/user_settings.h" ]; then
        echo "ERROR: $KIT_DIR/user_settings.h missing (demo WOLFSSL_USER_SETTINGS)." >&2
        exit 1
    fi

    if ! command -v python3 >/dev/null 2>&1; then
        echo "ERROR: python3 not found in PATH (required by gen-sbom)." >&2
        exit 1
    fi

    # CRA_SBOM_NO_HASH emits a placeholder checksum and skips the source list
    # entirely (for NDA customers who cannot share source lists).
    if [ "${CRA_SBOM_NO_HASH:-}" = "true" ] || [ "${CRA_SBOM_NO_HASH:-}" = "1" ]; then
        echo "       NOTE: CRA_SBOM_NO_HASH=true: emitting SBOM without artifact hash."
        echo "             WARNING: not suitable for production CRA compliance." >&2
        set -- --no-artifact-hash --cdx-out "$CDX_OUT" --spdx-out "$SPDX_OUT"
    else
        # Resolve the source list into a temp file (cleaned up by the EXIT trap).
        _srcs=$(mktemp "${TMPDIR:-/tmp}/wolfssh-srcs.XXXXXX") || {
            echo "ERROR: mktemp failed for the source-list temp file." >&2
            exit 1
        }
        _auto_tempfiles="${_auto_tempfiles:-} $_srcs"
        _resolve_wolfssh_srcs "$_srcs"

        # Validate every resolved path exists before handing the file to gen-sbom.
        while IFS= read -r _src; do
            [ -n "$_src" ] || continue
            if [ ! -f "$_src" ]; then
                echo "ERROR: source file does not exist: $_src" >&2
                exit 1
            fi
        done < "$_srcs"
        if [ ! -s "$_srcs" ]; then
            echo "ERROR: resolved source list is empty." >&2
            exit 1
        fi
        set -- --srcs-file "$_srcs" --cdx-out "$CDX_OUT" --spdx-out "$SPDX_OUT"
    fi
    if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
        set -- "$@" --license-override "$CRA_LICENSE_OVERRIDE"
        if [ -n "${CRA_LICENSE_TEXT:-}" ]; then
            set -- "$@" --license-text "$CRA_LICENSE_TEXT"
        fi
    fi

    if _py=$(_python_with_pcpp); then
        echo "       Using $_py (pcpp) for --user-settings"
    else
        echo "ERROR: no python3/python with pcpp installed (required for embedded mode)." >&2
        echo "       Install it on the same interpreter: python3 -m pip install pcpp" >&2
        exit 1
    fi

    "$_py" "$GEN" \
        --name wolfssh --version "$VERSION" \
        --license-file "$WOLFSSH_DIR/LICENSING" \
        --user-settings "$SETTINGS_H" \
        --user-settings-include "$WOLFSSL_DIR" \
        --user-settings-include "$KIT_DIR" \
        --user-settings-define WOLFSSL_USER_SETTINGS \
        "$@"

    # Verify gen-sbom actually produced both outputs and they are non-empty.
    for _f in "$CDX_OUT" "$SPDX_OUT"; do
        if [ ! -s "$_f" ]; then
            echo "ERROR: expected output missing or empty: $_f" >&2
            exit 1
        fi
    done
}

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

MODE=${CRA_SBOM_MODE:-autotools}
case "$MODE" in
    embedded) _run_embedded ;;
    autotools) _run_autotools ;;
    *)
        echo "ERROR: CRA_SBOM_MODE must be 'autotools' or 'embedded', not '$MODE'" >&2
        exit 1
        ;;
esac

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

echo "Done. SBOM outputs:"
echo "  $CDX_OUT"
echo "  $SPDX_OUT"
