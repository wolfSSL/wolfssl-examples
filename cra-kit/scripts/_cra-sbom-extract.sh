#!/bin/sh
# _cra-sbom-extract.sh — shared source-extraction helper for CRA Kit SBOM scripts.
#
# Source this file; do not execute it directly. It provides one function,
# _cra_extract_srcs, that product SBOM scripts (generate-wolfssl-sbom.sh,
# generate-wolfssh-sbom.sh, ...) call to auto-detect the list of .c files that
# went into an embedded build.
#
# _cra_extract_srcs PRODUCT_DIR PRODUCT_NAME OUT_FILE
#
#   Tries each extraction method in priority order. On success, writes the
#   sorted, de-duplicated list of .c paths to OUT_FILE and returns 0. If a
#   method is selected (its env var is set) but yields no sources, prints an
#   error to stderr and returns 1. If no extraction env var is set at all,
#   returns 2 so the caller can fall back to its default source glob.
#
#   Methods, in priority order:
#     1. CRA_SBOM_SRCS_FILE    — copy verbatim to OUT_FILE (safety net; callers
#                                normally handle this themselves before calling)
#     2. CRA_SBOM_KEIL_PROJECT — parse .uvprojx, filter to PRODUCT_DIR
#     3. CRA_SBOM_IAR_PROJECT  — parse .ewp, filter to PRODUCT_DIR
#     4. CRA_SBOM_MAKEFILE_DIR — `make -n`, grep for PRODUCT_DIR/...*.c
#     5. CRA_SBOM_BUILD_DIR    — jq filter compile_commands.json to PRODUCT_DIR
#     none set                 — return 2
#
#   PRODUCT_DIR:  absolute path to the product source tree; used to filter paths.
#   PRODUCT_NAME: short name for log messages and the wolfssl CMSIS-Pack special
#                 case (e.g. "wolfssl", "wolfssh", "wolftpm").
#   OUT_FILE:     path the caller has already created (mktemp) for the result.
#
# The function appends any temp files it creates to _cra_auto_tempfiles so the
# caller's EXIT trap can clean them up. CRA_SBOM_NO_HASH is honoured by callers,
# not here: a caller that sees CRA_SBOM_NO_HASH=true should skip hashing (and
# thus skip this function) entirely.

_cra_extract_srcs() {
    _cra_product_dir="$1"
    _cra_product_name="$2"
    _cra_out_file="$3"

    if [ -z "$_cra_product_dir" ] || [ -z "$_cra_product_name" ] || [ -z "$_cra_out_file" ]; then
        echo "ERROR: _cra_extract_srcs requires PRODUCT_DIR PRODUCT_NAME OUT_FILE." >&2
        return 1
    fi

    # Method 1: explicit source list file. Callers usually handle this before
    # calling us, but honour it here too so the function is safe to call blindly.
    if [ -n "${CRA_SBOM_SRCS_FILE:-}" ]; then
        if [ ! -f "$CRA_SBOM_SRCS_FILE" ]; then
            echo "ERROR: CRA_SBOM_SRCS_FILE=$CRA_SBOM_SRCS_FILE not found." >&2
            return 1
        fi
        sort -u "$CRA_SBOM_SRCS_FILE" > "$_cra_out_file" || {
            echo "ERROR: failed to read CRA_SBOM_SRCS_FILE=$CRA_SBOM_SRCS_FILE." >&2
            return 1
        }
        if [ ! -s "$_cra_out_file" ]; then
            echo "ERROR: CRA_SBOM_SRCS_FILE=$CRA_SBOM_SRCS_FILE is empty." >&2
            return 1
        fi
        _cra_n=$(wc -l < "$_cra_out_file" | tr -d ' ')
        echo "    Using $_cra_n sources from CRA_SBOM_SRCS_FILE"
        return 0
    fi

    # Method 2: Keil .uvprojx
    if [ -n "${CRA_SBOM_KEIL_PROJECT:-}" ]; then
        if [ ! -f "$CRA_SBOM_KEIL_PROJECT" ]; then
            echo "ERROR: CRA_SBOM_KEIL_PROJECT=$CRA_SBOM_KEIL_PROJECT not found." >&2
            return 1
        fi
        if ! command -v python3 >/dev/null 2>&1; then
            echo "ERROR: python3 is required to parse a Keil .uvprojx file." >&2
            return 1
        fi
        # The CMSIS Pack RTE lookup only applies when PRODUCT_NAME=wolfssl:
        # wolfSSL ships a CMSIS Pack whose sources live in the installed pack,
        # not in the project. Other products have no CMSIS pack, so the parser
        # skips that path and enumerates <File>/<FilePath> entries directly.
        python3 - "$CRA_SBOM_KEIL_PROJECT" "$_cra_product_dir" "$_cra_product_name" \
            > "$_cra_out_file" <<'PYEOF' || {
import sys, os, glob, xml.etree.ElementTree as ET

proj_file = sys.argv[1]
product_dir = sys.argv[2] if len(sys.argv) > 2 else ''
product_name = sys.argv[3] if len(sys.argv) > 3 else ''
proj_dir = os.path.dirname(os.path.abspath(proj_file))
proj = ET.parse(proj_file)
paths = set()

rte = proj.find('.//RTE')
# CMSIS Pack RTE special case: wolfSSL only.
if (product_name == 'wolfssl' and rte is not None
        and rte.find('.//component[@Cvendor="wolfSSL"]') is not None):
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
    elif product_dir and os.path.isdir(product_dir):
        # Pack not installed locally; enumerate sources from PRODUCT_DIR.
        for subdir in ('wolfcrypt/src', 'src'):
            d = os.path.join(product_dir, subdir)
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
            echo "ERROR: failed to parse Keil project $CRA_SBOM_KEIL_PROJECT." >&2
            return 1
        }
        # wolfssl's CMSIS-Pack branch legitimately emits paths under the installed
        # pack dir (~/.arm/Packs/...), which lie OUTSIDE PRODUCT_DIR; filtering
        # would wrongly drop them. The wolfssl parser already constrains its
        # output to wolfssl sources, so skip the PRODUCT_DIR filter for wolfssl.
        if [ "$_cra_product_name" != "wolfssl" ]; then
            _cra_filter_to_product "$_cra_out_file" "$_cra_product_dir"
        fi
        if [ ! -s "$_cra_out_file" ]; then
            echo "ERROR: CRA_SBOM_KEIL_PROJECT is set but no $_cra_product_name sources were extracted from $CRA_SBOM_KEIL_PROJECT." >&2
            return 1
        fi
        _cra_n=$(wc -l < "$_cra_out_file" | tr -d ' ')
        echo "    Auto-extracted $_cra_n $_cra_product_name sources from Keil project"
        return 0
    fi

    # Method 3: IAR .ewp
    if [ -n "${CRA_SBOM_IAR_PROJECT:-}" ]; then
        if [ ! -f "$CRA_SBOM_IAR_PROJECT" ]; then
            echo "ERROR: CRA_SBOM_IAR_PROJECT=$CRA_SBOM_IAR_PROJECT not found." >&2
            return 1
        fi
        if ! command -v python3 >/dev/null 2>&1; then
            echo "ERROR: python3 is required to parse an IAR .ewp file." >&2
            return 1
        fi
        python3 - "$CRA_SBOM_IAR_PROJECT" > "$_cra_out_file" <<'PYEOF' || {
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
            echo "ERROR: failed to parse IAR project $CRA_SBOM_IAR_PROJECT." >&2
            return 1
        }
        # Preserve wolfssl's original behaviour (all project .c, unfiltered); other
        # products filter to PRODUCT_DIR to drop demo/BSP sources from the project.
        if [ "$_cra_product_name" != "wolfssl" ]; then
            _cra_filter_to_product "$_cra_out_file" "$_cra_product_dir"
        fi
        if [ ! -s "$_cra_out_file" ]; then
            echo "ERROR: CRA_SBOM_IAR_PROJECT is set but no $_cra_product_name sources were extracted from $CRA_SBOM_IAR_PROJECT." >&2
            return 1
        fi
        _cra_n=$(wc -l < "$_cra_out_file" | tr -d ' ')
        echo "    Auto-extracted $_cra_n $_cra_product_name sources from IAR project"
        return 0
    fi

    # Method 4: Makefile dry-run
    if [ -n "${CRA_SBOM_MAKEFILE_DIR:-}" ]; then
        if [ ! -d "$CRA_SBOM_MAKEFILE_DIR" ]; then
            echo "ERROR: CRA_SBOM_MAKEFILE_DIR=$CRA_SBOM_MAKEFILE_DIR is not a directory." >&2
            return 1
        fi
        if ! command -v make >/dev/null 2>&1; then
            echo "ERROR: make is required to auto-extract sources from CRA_SBOM_MAKEFILE_DIR." >&2
            return 1
        fi
        # `make -n` (dry run) emits the compile commands; pull out any .c path
        # that lives under PRODUCT_DIR. grep -F on the dir keeps the pattern
        # literal (PRODUCT_DIR may contain regex metacharacters).
        make -C "$CRA_SBOM_MAKEFILE_DIR" -n 2>/dev/null \
            | grep -oE '[^ ]+\.c' \
            | grep -F "$_cra_product_dir/" \
            | sort -u > "$_cra_out_file" || true
        if [ ! -s "$_cra_out_file" ]; then
            echo "ERROR: CRA_SBOM_MAKEFILE_DIR is set but make yielded no $_cra_product_name sources." >&2
            echo "       Ensure 'make -n' in $CRA_SBOM_MAKEFILE_DIR references .c files under $_cra_product_dir." >&2
            return 1
        fi
        _cra_n=$(wc -l < "$_cra_out_file" | tr -d ' ')
        echo "    Auto-extracted $_cra_n $_cra_product_name sources via Makefile (CRA_SBOM_MAKEFILE_DIR=$CRA_SBOM_MAKEFILE_DIR)"
        return 0
    fi

    # Method 5: compile_commands.json (CMake / Zephyr / ESP-IDF)
    if [ -n "${CRA_SBOM_BUILD_DIR:-}" ] && [ -f "$CRA_SBOM_BUILD_DIR/compile_commands.json" ]; then
        if ! command -v jq >/dev/null 2>&1; then
            echo "ERROR: jq is required to auto-extract sources from compile_commands.json." >&2
            echo "       Install jq, or set CRA_SBOM_SRCS_FILE manually. See SRCS-FILE-HOWTO.md." >&2
            return 1
        fi
        jq -r '.[].file' "$CRA_SBOM_BUILD_DIR/compile_commands.json" \
            | grep -F "$_cra_product_dir/" \
            | grep -E '/(wolfcrypt/src|src)/[^/]+\.c$' \
            | sort -u > "$_cra_out_file" || true
        if [ ! -s "$_cra_out_file" ]; then
            echo "ERROR: compile_commands.json in $CRA_SBOM_BUILD_DIR yielded no $_cra_product_name sources under $_cra_product_dir." >&2
            return 1
        fi
        _cra_n=$(wc -l < "$_cra_out_file" | tr -d ' ')
        echo "    Auto-extracted $_cra_n $_cra_product_name sources from compile_commands.json"
        return 0
    fi

    # No extraction method selected — caller should use its default glob.
    return 2
}

# _cra_filter_to_product OUT_FILE PRODUCT_DIR
# In-place filter: keep only lines that are .c paths under PRODUCT_DIR.
# grep -F keeps PRODUCT_DIR literal (it may contain regex metacharacters).
_cra_filter_to_product() {
    _cra_f="$1"
    _cra_dir="$2"
    _cra_tmp=$(mktemp "${TMPDIR:-/tmp}/cra-filter.XXXXXX") || {
        echo "ERROR: mktemp failed while filtering sources." >&2
        return 1
    }
    _cra_auto_tempfiles="${_cra_auto_tempfiles:-} $_cra_tmp"
    grep -F "$_cra_dir/" "$_cra_f" | grep -E '\.c$' | sort -u > "$_cra_tmp" || true
    mv -f "$_cra_tmp" "$_cra_f"
}
