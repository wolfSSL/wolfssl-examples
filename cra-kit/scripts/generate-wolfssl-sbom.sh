#!/bin/sh
# Generate wolfSSL component SBOMs (autotools make sbom, cmake sbom, or embedded gen-sbom).
#
# Mode selection:
#   CRA_SBOM_MODE=autotools|cmake|embedded
#     autotools (default when configure+Makefile exist): runs `make sbom`
#     cmake: runs `cmake --build $WOLFSSL_BUILD_DIR --target sbom`
#     embedded: runs gen-sbom directly with source files and user_settings.h
#
# Required variables:
#   WOLFSSL_DIR=path/to/wolfssl         (source tree root)
#
# Mode-specific variables:
#   WOLFSSL_BUILD_DIR=path/to/build     (cmake mode: path to cmake build directory;
#                                         embedded: also triggers compile_commands.json
#                                         auto-extraction when present)
#   CRA_SBOM_SRCS_FILE=path/to/srcs.txt (embedded: file listing .c paths, one per line;
#                                         combined with the built-in demo list unless
#                                         CRA_SBOM_SRCS_ONLY_FROM_FILE=true)
#   CRA_SBOM_SRCS_ONLY_FROM_FILE=true   (embedded: skip the built-in demo list and
#                                         use only paths from CRA_SBOM_SRCS_FILE)
#   CRA_SBOM_NO_HASH=true               (embedded: emit SBOM without a real artifact
#                                         hash; use when no source list is available)
#   CRA_SBOM_MAKEFILE_DIR=<path>        (embedded: auto-extract srcs via make -n)
#   CRA_SBOM_KEIL_PROJECT=<path>        (embedded: auto-extract srcs from .uvprojx)
#   CRA_SBOM_IAR_PROJECT=<path>         (embedded: auto-extract srcs from .ewp)
#
# Optional variables:
#   CRA_PYTHON=python3                  (interpreter with pcpp; for embedded path)
#   CRA_LICENSE_OVERRIDE=<SPDX-id>      (e.g. LicenseRef-wolfSSL-Commercial)
#   CRA_LICENSE_TEXT=<path>             (required when CRA_LICENSE_OVERRIDE is a
#                                        LicenseRef-* id: plain-text licence embedded
#                                        in the SBOM; gen-sbom / make sbom hard-fail
#                                        without it.)
set -eu

# Accumulator for temp files created by _auto_extract_srcs; cleaned up on exit.
_auto_tempfiles=""
trap 'rm -f ${_auto_tempfiles:-}' EXIT

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

_auto_extract_srcs() {
    # Method 1: compile_commands.json (CMake / Zephyr / ESP-IDF)
    if [ -n "${WOLFSSL_BUILD_DIR:-}" ] && [ -f "$WOLFSSL_BUILD_DIR/compile_commands.json" ]; then
        _ccdb="$WOLFSSL_BUILD_DIR/compile_commands.json"
        if ! command -v jq >/dev/null 2>&1; then
            echo "ERROR: jq is required to auto-extract sources from compile_commands.json." >&2
            echo "       Install jq, or set CRA_SBOM_SRCS_FILE manually. See SRCS-FILE-HOWTO.md." >&2
            exit 1
        fi
        _auto=$(mktemp "${TMPDIR:-/tmp}/wolfssl-auto-srcs.XXXXXX")
        _auto_tempfiles="${_auto_tempfiles:-} $_auto"
        jq -r '.[].file' "$_ccdb" \
            | grep "^${WOLFSSL_DIR}/" \
            | grep -E '/(wolfcrypt/src|src)/[^/]+\.c$' \
            | sort -u > "$_auto"
        if [ ! -s "$_auto" ]; then
            _esp_proj=$(dirname "$WOLFSSL_BUILD_DIR")
            jq -r '.[].file' "$_ccdb" \
                | grep "^${_esp_proj}/managed_components/wolfssl__wolfssl/" \
                | grep -E '/(wolfcrypt/src|src)/[^/]+\.c$' \
                | sort -u > "$_auto"
        fi
        if [ -s "$_auto" ]; then
            _n=$(wc -l < "$_auto" | tr -d ' ')
            echo "    Auto-extracted $_n wolfssl sources from compile_commands.json"
            CRA_SBOM_SRCS_FILE="$_auto"
            CRA_SBOM_SRCS_ONLY_FROM_FILE=true
            return 0
        fi
        echo "    WARNING: compile_commands.json found but yielded no wolfssl sources; trying next method." >&2
    fi

    # Method 2: Makefile
    if [ -n "${CRA_SBOM_MAKEFILE_DIR:-}" ]; then
        if [ ! -d "$CRA_SBOM_MAKEFILE_DIR" ]; then
            echo "ERROR: CRA_SBOM_MAKEFILE_DIR=$CRA_SBOM_MAKEFILE_DIR is not a directory." >&2
            exit 1
        fi
        _auto=$(mktemp "${TMPDIR:-/tmp}/wolfssl-auto-srcs.XXXXXX")
        _auto_tempfiles="${_auto_tempfiles:-} $_auto"
        if make --no-print-directory -C "$CRA_SBOM_MAKEFILE_DIR" -n print-wolfssl-srcs >/dev/null 2>&1; then
            make --no-print-directory -C "$CRA_SBOM_MAKEFILE_DIR" print-wolfssl-srcs 2>/dev/null \
                | sort -u > "$_auto"
        fi
        if [ ! -s "$_auto" ]; then
            make -C "$CRA_SBOM_MAKEFILE_DIR" -n 2>/dev/null \
                | grep -oE '[^ ]+wolfssl[^ ]+\.c' \
                | sort -u > "$_auto" || true
        fi
        if [ -s "$_auto" ]; then
            _n=$(wc -l < "$_auto" | tr -d ' ')
            echo "    Auto-extracted $_n wolfssl sources via Makefile (CRA_SBOM_MAKEFILE_DIR=$CRA_SBOM_MAKEFILE_DIR)"
            CRA_SBOM_SRCS_FILE="$_auto"
            CRA_SBOM_SRCS_ONLY_FROM_FILE=true
            return 0
        fi
        echo "ERROR: CRA_SBOM_MAKEFILE_DIR is set but make yielded no wolfssl sources." >&2
        echo "       Add a 'print-wolfssl-srcs' target or ensure 'make -n' references wolfssl .c files." >&2
        exit 1
    fi

    # Method 3: Keil .uvprojx
    if [ -n "${CRA_SBOM_KEIL_PROJECT:-}" ]; then
        if [ ! -f "$CRA_SBOM_KEIL_PROJECT" ]; then
            echo "ERROR: CRA_SBOM_KEIL_PROJECT=$CRA_SBOM_KEIL_PROJECT not found." >&2
            exit 1
        fi
        if ! command -v python3 >/dev/null 2>&1; then
            echo "ERROR: python3 is required to parse a Keil .uvprojx file." >&2
            exit 1
        fi
        _auto=$(mktemp "${TMPDIR:-/tmp}/wolfssl-auto-srcs.XXXXXX")
        _auto_tempfiles="${_auto_tempfiles:-} $_auto"
        python3 - "$CRA_SBOM_KEIL_PROJECT" "$WOLFSSL_DIR" > "$_auto" <<'PYEOF'
import sys, os, glob, xml.etree.ElementTree as ET

proj_file = sys.argv[1]
wolfssl_dir = sys.argv[2] if len(sys.argv) > 2 else ''
proj_dir = os.path.dirname(os.path.abspath(proj_file))
proj = ET.parse(proj_file)
paths = set()

rte = proj.find('.//RTE')
if rte is not None and rte.find('.//component[@Cvendor="wolfSSL"]') is not None:
    # CMSIS Pack RTE: sources come from the installed pack .pdsc
    pdsc_candidates = sorted(glob.glob(
        os.path.expanduser('~/.arm/Packs/wolfSSL/wolfSSL/*/wolfSSL.pdsc')
    ))
    if os.name == 'nt':
        appdata = os.environ.get('LOCALAPPDATA', '')
        pdsc_candidates += sorted(glob.glob(
            os.path.join(appdata, 'Arm', 'Packs', 'wolfSSL', 'wolfSSL', '*', 'wolfSSL.pdsc')
        ))
    if pdsc_candidates:
        pdsc_file = pdsc_candidates[-1]
        pack_dir = os.path.dirname(pdsc_file)
        pdsc = ET.parse(pdsc_file)
        for f in pdsc.findall('.//file[@category="source"]'):
            name = f.get('name', '')
            if name.lower().endswith('.c'):
                paths.add(os.path.normpath(os.path.join(pack_dir, name.replace('\\', '/'))))
    elif wolfssl_dir and os.path.isdir(wolfssl_dir):
        # Pack not installed locally; enumerate sources directly from WOLFSSL_DIR.
        # The CMSIS Pack contains the full wolfssl library (wolfcrypt/src/ + src/).
        for subdir in ('wolfcrypt/src', 'src'):
            d = os.path.join(wolfssl_dir, subdir)
            if os.path.isdir(d):
                for name in os.listdir(d):
                    if name.endswith('.c'):
                        paths.add(os.path.join(d, name))
else:
    for file_elem in proj.findall('.//File'):
        fp = file_elem.find('FilePath')
        ft = file_elem.find('FileType')
        if fp is None or not fp.text:
            continue
        ftype = int(ft.text) if ft is not None and ft.text else 0
        if ftype == 1 or fp.text.lower().endswith('.c'):
            abs_path = os.path.normpath(
                os.path.join(proj_dir, fp.text.replace('\\', '/'))
            )
            paths.add(abs_path)

for p in sorted(paths):
    print(p)
PYEOF
        if [ -s "$_auto" ]; then
            _n=$(wc -l < "$_auto" | tr -d ' ')
            echo "    Auto-extracted $_n wolfssl sources from Keil project"
            CRA_SBOM_SRCS_FILE="$_auto"
            CRA_SBOM_SRCS_ONLY_FROM_FILE=true
            return 0
        fi
        echo "ERROR: CRA_SBOM_KEIL_PROJECT is set but no sources were extracted from $CRA_SBOM_KEIL_PROJECT." >&2
        exit 1
    fi

    # Method 4: IAR .ewp
    if [ -n "${CRA_SBOM_IAR_PROJECT:-}" ]; then
        if [ ! -f "$CRA_SBOM_IAR_PROJECT" ]; then
            echo "ERROR: CRA_SBOM_IAR_PROJECT=$CRA_SBOM_IAR_PROJECT not found." >&2
            exit 1
        fi
        if ! command -v python3 >/dev/null 2>&1; then
            echo "ERROR: python3 is required to parse an IAR .ewp file." >&2
            exit 1
        fi
        _auto=$(mktemp "${TMPDIR:-/tmp}/wolfssl-auto-srcs.XXXXXX")
        _auto_tempfiles="${_auto_tempfiles:-} $_auto"
        python3 - "$CRA_SBOM_IAR_PROJECT" > "$_auto" <<'PYEOF'
import sys, os, xml.etree.ElementTree as ET

def is_excluded(file_elem):
    return file_elem.find('excluded') is not None

proj_file = sys.argv[1]
proj_dir = os.path.dirname(os.path.abspath(proj_file))
proj = ET.parse(proj_file)
paths = set()

for file_elem in proj.findall('.//file'):
    if is_excluded(file_elem):
        continue
    name_elem = file_elem.find('name')
    if name_elem is None or not name_elem.text:
        continue
    raw = name_elem.text
    if not raw.lower().endswith('.c'):
        continue
    resolved = raw.replace('$PROJ_DIR$', proj_dir)
    paths.add(os.path.normpath(resolved.replace('\\', '/')))

for p in sorted(paths):
    print(p)
PYEOF
        if [ -s "$_auto" ]; then
            _n=$(wc -l < "$_auto" | tr -d ' ')
            echo "    Auto-extracted $_n wolfssl sources from IAR project"
            CRA_SBOM_SRCS_FILE="$_auto"
            CRA_SBOM_SRCS_ONLY_FROM_FILE=true
            return 0
        fi
        echo "ERROR: CRA_SBOM_IAR_PROJECT is set but no sources were extracted from $CRA_SBOM_IAR_PROJECT." >&2
        exit 1
    fi
}

_run_embedded() {
    echo "==> Embedded path: gen-sbom with CRA Kit user_settings.h"
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

    # --no-artifact-hash: skip all source-file logic and emit a placeholder hash.
    # Use when no compiled library AND no source file list is accessible.
    if [ "${CRA_SBOM_NO_HASH:-}" = "true" ] || [ "${CRA_SBOM_NO_HASH:-}" = "1" ]; then
        if [ -n "${CRA_SBOM_SRCS_FILE:-}" ] || [ -n "${CRA_SBOM_SRCS_ONLY_FROM_FILE:-}" ]; then
            echo "ERROR: CRA_SBOM_NO_HASH cannot be combined with CRA_SBOM_SRCS_FILE." >&2
            exit 1
        fi
        echo "    NOTE: CRA_SBOM_NO_HASH=true: emitting SBOM with placeholder hash."
        echo "          Contact wolfssl@wolfssl.com to discuss integrity verification"
        echo "          options before using this in production."
        set -- --no-artifact-hash --cdx-out "$CDX_OUT" --spdx-out "$SPDX_OUT"
        if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
            set -- "$@" --license-override "$CRA_LICENSE_OVERRIDE"
            if [ -n "${CRA_LICENSE_TEXT:-}" ]; then
                set -- "$@" --license-text "$CRA_LICENSE_TEXT"
            fi
        fi
        _py=$(command -v python3 2>/dev/null || command -v python)
        [ -n "$_py" ] || { echo "ERROR: python3 not found." >&2; exit 1; }
        "$_py" "$GEN" \
            --name wolfssl --version "$VERSION" \
            --license-file "$WOLFSSL_DIR/LICENSING" \
            --user-settings "$SETTINGS_H" \
            --user-settings-include "$WOLFSSL_DIR" \
            --user-settings-include "$KIT_DIR" \
            --user-settings-define WOLFSSL_USER_SETTINGS \
            "$@"
        return 0
    fi

    # Auto-extract if caller didn't supply CRA_SBOM_SRCS_FILE
    [ -z "${CRA_SBOM_SRCS_FILE:-}" ] && [ "${CRA_SBOM_NO_HASH:-}" != "true" ] && \
        _auto_extract_srcs

    # Build the source file list.
    #
    # Priority:
    #   CRA_SBOM_SRCS_ONLY_FROM_FILE=true  — use only the caller-supplied file
    #   CRA_SBOM_SRCS_FILE (without ONLY)  — merge file with built-in demo list
    #   (neither)                           — use built-in demo list
    #
    # The built-in 9-file demo list is for kit demonstration only.  Production
    # SBOMs MUST list every wolfSSL .c file on your link line.  The post-
    # processing step below watermarks demo outputs with wolfssl:sbom:demo=true.
    set --
    if [ "${CRA_SBOM_SRCS_ONLY_FROM_FILE:-}" = "true" ]; then
        if [ -z "${CRA_SBOM_SRCS_FILE:-}" ]; then
            echo "ERROR: CRA_SBOM_SRCS_ONLY_FROM_FILE=true requires CRA_SBOM_SRCS_FILE." >&2
            exit 1
        fi
        echo "    Using source list from $CRA_SBOM_SRCS_FILE (CRA_SBOM_SRCS_ONLY_FROM_FILE=true)"
    else
        echo "    NOTE: --srcs uses the kit's built-in 9-file DEMO list. Production SBOMs"
        echo "          must list every wolfSSL .c file you compile. Set CRA_SBOM_SRCS_FILE"
        echo "          to your link-time source list to replace the demo list."
        echo "          Output is watermarked wolfssl:sbom:demo=true."
        while IFS= read -r _src; do
            [ -n "$_src" ] || continue
            set -- "$@" "$_src"
        done <<EOF
$(_embedded_srcs)
EOF
    fi

    # Optional caller-supplied source list file (combined with or replacing the demo list).
    _srcs_file_args=""
    if [ -n "${CRA_SBOM_SRCS_FILE:-}" ]; then
        if [ ! -f "$CRA_SBOM_SRCS_FILE" ]; then
            echo "ERROR: CRA_SBOM_SRCS_FILE=$CRA_SBOM_SRCS_FILE not found." >&2
            exit 1
        fi
        _srcs_file_args="--srcs-file $CRA_SBOM_SRCS_FILE"
    fi

    # Optional commercial license override (LicenseRef-wolfSSL-Commercial etc).
    # A LicenseRef-* override must be accompanied by --license-text (validated
    # up front above); a stock SPDX id needs no text.
    # Append the --srcs positional args last; argparse stops --srcs consumption
    # at the next -- option, so --cdx-out / --spdx-out end the list cleanly.
    # Capture whether positional srcs exist before output flags are appended.
    _srcs_flag=""
    [ $# -gt 0 ] && _srcs_flag="--srcs"
    set -- "$@" --cdx-out "$CDX_OUT" --spdx-out "$SPDX_OUT"
    if [ -n "${CRA_LICENSE_OVERRIDE:-}" ]; then
        set -- "$@" --license-override "$CRA_LICENSE_OVERRIDE"
        if [ -n "${CRA_LICENSE_TEXT:-}" ]; then
            set -- "$@" --license-text "$CRA_LICENSE_TEXT"
        fi
    fi

    if _py=$(_python_with_pcpp); then
        echo "       Using $_py (pcpp) for --user-settings"
        # shellcheck disable=SC2086
        "$_py" "$GEN" \
            --name wolfssl --version "$VERSION" \
            --license-file "$WOLFSSL_DIR/LICENSING" \
            --user-settings "$SETTINGS_H" \
            --user-settings-include "$WOLFSSL_DIR" \
            --user-settings-include "$KIT_DIR" \
            --user-settings-define WOLFSSL_USER_SETTINGS \
            ${_srcs_file_args} \
            ${_srcs_flag} "$@"
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
    _auto_tempfiles="${_auto_tempfiles:-} $DEFINES_H"
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
    # shellcheck disable=SC2086
    "$PYTHON" "$GEN" \
        --name wolfssl --version "$VERSION" \
        --license-file "$WOLFSSL_DIR/LICENSING" \
        --options-h "$DEFINES_H" \
        ${_srcs_file_args} \
        ${_srcs_flag} "$@"
}

_run_cmake() {
    echo "==> cmake path: cmake --build --target sbom"
    if [ -z "${WOLFSSL_BUILD_DIR:-}" ]; then
        echo "ERROR: WOLFSSL_BUILD_DIR is not set." >&2
        echo "       Set it to your cmake out-of-source build directory." >&2
        echo "       Example: cmake -B build && WOLFSSL_BUILD_DIR=\$PWD/build $0" >&2
        exit 1
    fi
    if [ ! -d "$WOLFSSL_BUILD_DIR" ]; then
        echo "ERROR: WOLFSSL_BUILD_DIR=$WOLFSSL_BUILD_DIR is not a directory." >&2
        exit 1
    fi
    if ! command -v cmake >/dev/null 2>&1; then
        echo "ERROR: cmake not found in PATH." >&2
        exit 1
    fi

    # Detect version from cmake cache so we can find the output files.
    # cmake -L/-LA both omit :STATIC (internal) entries; grep the cache file directly.
    _cmake_ver=$(grep -m1 '^CMAKE_PROJECT_VERSION:STATIC=' \
        "$WOLFSSL_BUILD_DIR/CMakeCache.txt" 2>/dev/null | cut -d= -f2)
    if [ -z "$_cmake_ver" ]; then
        echo "WARNING: could not detect PROJECT_VERSION from cmake cache; using kit VERSION=$VERSION" >&2
        _cmake_ver="$VERSION"
    fi
    if [ "$_cmake_ver" != "$VERSION" ]; then
        echo "ERROR: cmake build has wolfSSL $_cmake_ver but the kit is pinned to $VERSION." >&2
        echo "       Update cra-kit/VERSION or reconfigure cmake against wolfSSL $VERSION." >&2
        exit 1
    fi

    cmake --build "$WOLFSSL_BUILD_DIR" --target sbom

    _cdx_src="$WOLFSSL_BUILD_DIR/wolfssl-${VERSION}.cdx.json"
    _spdx_src="$WOLFSSL_BUILD_DIR/wolfssl-${VERSION}.spdx.json"
    _tv_src="$WOLFSSL_BUILD_DIR/wolfssl-${VERSION}.spdx"
    for _f in "$_cdx_src" "$_spdx_src"; do
        if [ ! -f "$_f" ]; then
            echo "ERROR: expected cmake sbom output not found: $_f" >&2
            echo "       The sbom target may have failed; check cmake build output above." >&2
            exit 1
        fi
    done

    cp -f "$_cdx_src"  "$CDX_OUT"
    cp -f "$_spdx_src" "$SPDX_OUT"
    if [ -f "$_tv_src" ]; then
        cp -f "$_tv_src" "$OUT_DIR/"
    fi
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
    cmake) _run_cmake ;;
    "")
        if [ -n "${WOLFSSL_BUILD_DIR:-}" ] && [ -d "${WOLFSSL_BUILD_DIR}" ]; then
            MODE=cmake
            _run_cmake
        elif [ -f "$WOLFSSL_DIR/Makefile" ] && [ -f "$WOLFSSL_DIR/configure" ]; then
            MODE=autotools
            _run_autotools
        else
            MODE=embedded
            _run_embedded
        fi
        ;;
    *)
        echo "ERROR: CRA_SBOM_MODE must be 'autotools', 'cmake', or 'embedded', not '$MODE'" >&2
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
   CRA_SBOM_SRCS_ONLY_FROM_FILE="${CRA_SBOM_SRCS_ONLY_FROM_FILE:-}" \
python3 <<'PY'
import json, os, pathlib

cdx = pathlib.Path(os.environ["CDX_OUT"])
spdx = pathlib.Path(os.environ["SPDX_OUT"])
demo = os.environ.get("CRA_SBOM_MODE_FINAL") == "embedded" and \
       os.environ.get("CRA_SBOM_SRCS_ONLY_FROM_FILE") != "true"

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
