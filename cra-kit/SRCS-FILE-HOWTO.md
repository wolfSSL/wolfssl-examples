# Generating a wolfSSL source list for `CRA_SBOM_SRCS_FILE`

The embedded SBOM path hashes every wolfSSL `.c` file you compile — not the
library binary.  That list comes from your build system.  This guide shows how
to extract it for the most common embedded build systems.

Once you have the file, pass it to the kit script:

```sh
CRA_SBOM_MODE=embedded \
CRA_SBOM_SRCS_FILE=/path/to/wolfssl-srcs.txt \
CRA_SBOM_SRCS_ONLY_FROM_FILE=true \
WOLFSSL_DIR=/path/to/wolfssl \
./scripts/generate-wolfssl-sbom.sh
```

`CRA_SBOM_SRCS_ONLY_FROM_FILE=true` suppresses the demo watermark and uses
only the paths in your file.  Omit it to merge your list with the kit's
built-in 9-file demo list (keeps the `wolfssl:sbom:demo=true` watermark).

Manual extraction is now optional for most build systems.  Set the right
environment variable for your build system and the kit script extracts the
source list automatically — no `CRA_SBOM_SRCS_FILE` needed.  See the
relevant section below for the variable to set and any tool requirements.

---

## Custom Makefile

### Option A — add a `print-wolfssl-srcs` target (recommended)

Add this to your `Makefile`.  Replace `$(WOLFSSL_SRCS)` with however your
project names the wolfSSL source variable:

```makefile
.PHONY: print-wolfssl-srcs
print-wolfssl-srcs:
	@printf '%s\n' $(WOLFSSL_SRCS)
```

Then extract:

```sh
make print-wolfssl-srcs > wolfssl-srcs.txt
```

This is immune to recursive makes, response files, and multi-rule compilation.

### Option B — `make -n` dry-run (when you cannot modify the Makefile)

```sh
make -n 2>/dev/null \
  | grep -oE '[^ ]+wolfssl[^ ]+\.c' \
  | sort -u \
  > wolfssl-srcs.txt
```

`make -n` prints compiler command lines without running them, so a missing
cross-compiler is not a problem.  The grep pattern matches any token that
contains `wolfssl` and ends in `.c`.

**Limitation**: fails if sources are passed via response files (`@srcs.rsp`)
or compiled through recursive `$(MAKE) -C` sub-invocations that do not echo
the final compile lines.  Use Option A in those cases.

### Automatic extraction

Set `CRA_SBOM_MAKEFILE_DIR` to the directory containing your project Makefile, then run the
kit script with no `CRA_SBOM_SRCS_FILE`:

```sh
CRA_SBOM_MODE=embedded \
CRA_SBOM_MAKEFILE_DIR=/path/to/your/project \
WOLFSSL_DIR=/path/to/wolfssl \
./scripts/generate-wolfssl-sbom.sh
```

The script tries the `print-wolfssl-srcs` target first; if that target does not exist, it
falls back to `make -n` dry-run.  Either way the extracted list is used automatically —
no manual `CRA_SBOM_SRCS_FILE` needed.

---

## CMake — `compile_commands.json`

Enable compile commands at configure time:

```sh
cmake -B build /path/to/your/project \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

Extract wolfSSL library sources:

```sh
WOLFSSL_DIR=/path/to/wolfssl
jq -r '.[].file' build/compile_commands.json \
  | grep "^${WOLFSSL_DIR}/" \
  | grep -E "/(wolfcrypt/src|src)/[^/]+\.c$" \
  | sort -u \
  > wolfssl-srcs.txt
```

The `grep -E` step restricts to `src/` and `wolfcrypt/src/` — without it,
`examples/` and `tests/` files are included, which inflates the SBOM with
files you did not ship.

**Requirements**: `jq` (`apt install jq` / `brew install jq`).

### Automatic extraction

Set `WOLFSSL_BUILD_DIR` to your cmake build directory.  The kit script detects
`compile_commands.json` automatically and extracts wolfssl sources without manual steps:

```sh
CRA_SBOM_MODE=embedded \
WOLFSSL_BUILD_DIR=/path/to/build \
WOLFSSL_DIR=/path/to/wolfssl \
./scripts/generate-wolfssl-sbom.sh
```

Requires `jq` on the host.

---

## Zephyr RTOS

Zephyr uses CMake internally.  Add `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` at
build time to get `compile_commands.json` in your build directory:

```sh
west build -b <board> -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DZEPHYR_EXTRA_MODULES=/path/to/wolfssl
```

`compile_commands.json` is written by CMake at configure time — it exists
even if the compilation itself fails (e.g., missing cross-compiler).

Extract wolfSSL library sources:

```sh
WOLFSSL_DIR=/path/to/wolfssl
jq -r '.[].file' build/compile_commands.json \
  | grep "^${WOLFSSL_DIR}/" \
  | grep -E "/(wolfcrypt/src|src)/[^/]+\.c$" \
  | sort -u \
  > wolfssl-srcs.txt
```

All paths in `compile_commands.json` are absolute.  The `WOLFSSL_DIR` filter
matches entries from the wolfssl module directly; no path translation needed.
A typical wolfssl Zephyr build produces around 89 library sources
(`wolfcrypt/src/` + `src/`).

**Requirements**: `jq` must be installed on the host running the extraction
(not the target board).

### Automatic extraction

Same as CMake — set `WOLFSSL_BUILD_DIR` to the `west build` output directory:

```sh
CRA_SBOM_MODE=embedded \
WOLFSSL_BUILD_DIR=/path/to/wolfssl-app/build \
WOLFSSL_DIR=/path/to/wolfssl \
./scripts/generate-wolfssl-sbom.sh
```

Requires `jq`.  The `compile_commands.json` must have been generated at cmake configure time
(pass `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to `west build`).

---

## ESP-IDF

ESP-IDF uses CMake and writes `compile_commands.json` to the `build/`
subdirectory automatically.  Build your project normally:

```sh
idf.py build
```

When wolfssl is added as a **managed component** (via `idf_component.yml`
declaring `wolfssl/wolfssl`), the ESP-IDF component manager downloads it into
`managed_components/wolfssl__wolfssl/` inside your project.  The directory name
is `wolfssl__wolfssl` (registry namespace and name joined with double
underscore).

Extract the wolfssl library sources:

```sh
PROJECT_DIR=/path/to/your/esp-idf-project
jq -r '.[].file' "${PROJECT_DIR}/build/compile_commands.json" \
  | grep "^${PROJECT_DIR}/managed_components/wolfssl__wolfssl/" \
  | grep -E "/(wolfcrypt/src|src)/[^/]+\.c$" \
  | sort -u \
  > wolfssl-srcs.txt
```

All paths in `compile_commands.json` are absolute.  The
`grep -E "/(wolfcrypt/src|src)/[^/]+\.c$"` step excludes build-generated
files (e.g., `build/project_elf_src_esp32.c`) that also appear under the
project directory.

If wolfssl is added as a **local component** (placed manually in
`components/wolfssl/` rather than managed), replace `managed_components/wolfssl__wolfssl`
with `components/wolfssl` in the filter.

### Automatic extraction

Set `WOLFSSL_BUILD_DIR` to your project's `build/` directory:

```sh
CRA_SBOM_MODE=embedded \
WOLFSSL_BUILD_DIR=/path/to/esp-idf-project/build \
WOLFSSL_DIR=/path/to/wolfssl \
./scripts/generate-wolfssl-sbom.sh
```

The script detects the ESP-IDF managed-component layout (`managed_components/wolfssl__wolfssl/`)
automatically when `WOLFSSL_DIR` sources are not found under `WOLFSSL_BUILD_DIR` directly.
Requires `jq`.

---

## Keil MDK / uVision (`.uvprojx`)

> **Note**: Keil MDK is Windows-only and requires a license.  This section
> was verified against real wolfSSL Keil project files from
> `wolfssl/IDE/MDK5-ARM/`.  The CMSIS Pack note below reflects actual
> wolfSSL project structure.

Keil projects integrate wolfssl in one of two ways — the extraction method
differs between them.

### Option A — wolfSSL CMSIS Pack (modern, recommended)

The official wolfSSL Keil projects (e.g., `wolfSSL-Lib.uvprojx`) use the
**CMSIS Pack RTE (Run-Time Environment)**.  In this mode the wolfssl sources
are **not listed in the `.uvprojx` file** — they are resolved at build time
from the installed wolfSSL CMSIS pack.  The project XML records which pack
components are selected, not which `.c` files they compile.

To find the source list, locate the installed pack descriptor:

```
# Windows
%LOCALAPPDATA%\Arm\Packs\wolfSSL\wolfSSL\<version>\wolfSSL.pdsc

# Linux / macOS (Keil Studio / CMSIS-Toolbox)
~/.arm/Packs/wolfSSL/wolfSSL/<version>/wolfSSL.pdsc
```

The `.pdsc` file is XML.  Extract the `.c` sources for your selected
component group (e.g., `wolfCrypt/CORE`):

```python
#!/usr/bin/env python3
"""Extract .c sources for a wolfSSL CMSIS Pack component from its .pdsc."""
import sys, xml.etree.ElementTree as ET

pdsc = ET.parse(sys.argv[1])
cgroup = sys.argv[2] if len(sys.argv) > 2 else ''   # e.g. "wolfCrypt"

for comp in pdsc.findall('.//component'):
    if cgroup and comp.get('Cgroup', '') != cgroup:
        continue
    for f in comp.findall('.//file[@category="source"]'):
        name = f.get('name', '')
        if name.lower().endswith('.c'):
            print(name.replace('\\', '/'))
```

Usage:

```sh
python3 extract-pdsc-srcs.py wolfSSL.pdsc wolfCrypt > wolfssl-srcs.txt
```

Paths in the `.pdsc` are relative to the pack root directory.  Prefix with
the pack install path to make them absolute before passing to `gen-sbom`.

### Option B — wolfssl sources listed directly in the project

Older or custom projects may list wolfssl `.c` files explicitly as
`<File><FilePath>` entries under `<Groups>`.  The Python script
from the CMSIS approach will produce no output for these — use this
instead:

```python
#!/usr/bin/env python3
"""Extract explicit .c FilePath entries from a Keil .uvprojx (non-pack)."""
import sys, xml.etree.ElementTree as ET

proj = ET.parse(sys.argv[1])
paths = set()
for file_elem in proj.findall('.//File'):
    fp   = file_elem.find('FilePath')
    ft   = file_elem.find('FileType')
    if fp is None or not fp.text:
        continue
    ftype = int(ft.text) if ft is not None and ft.text else 0
    if ftype == 1 or fp.text.lower().endswith('.c'):
        paths.add(fp.text.replace('\\', '/'))

for p in sorted(paths):
    print(p)
```

Usage:

```sh
python3 extract-keil-srcs.py MyProject.uvprojx > wolfssl-srcs.txt
```

Paths are relative to the `.uvprojx` file.  Resolve to absolute before
passing to `gen-sbom`.

**How to tell which option you need**: open the `.uvprojx` in a text editor
and search for `<RTE>`.  If present and `<component Cvendor="wolfSSL">` is
inside it, you are using the CMSIS Pack (Option A).  If wolfssl `.c` files
appear under `<Groups>` directly, use Option B.

### Automatic extraction

Set `CRA_SBOM_KEIL_PROJECT` to the path of your `.uvprojx` file:

```sh
CRA_SBOM_MODE=embedded \
CRA_SBOM_KEIL_PROJECT=/path/to/MyProject.uvprojx \
WOLFSSL_DIR=/path/to/wolfssl \
./scripts/generate-wolfssl-sbom.sh
```

The script parses the project file and chooses Option A (CMSIS Pack) or Option B
(explicit FilePath) automatically based on whether a `<component Cvendor="wolfSSL">` is
present in the RTE block.  For CMSIS Pack mode the installed `.pdsc` must be present at
`~/.arm/Packs/wolfSSL/wolfSSL/<version>/wolfSSL.pdsc`.  Requires `python3`.

---

## IAR Embedded Workbench (`.ewp`)

> **Note**: IAR EW is Windows-only and requires a license.  This section
> was verified against real wolfSSL IAR project files from
> `wolfssl/IDE/IAR-EWARM/`.

IAR stores source files as `<file><name>` elements with a `$PROJ_DIR$`
path prefix (IAR's built-in variable for the directory containing the `.ewp`
file) and Windows backslash separators.

**Important**: wolfssl sources live under `wolfcrypt/src/` and `src/` — neither
path segment contains the string `"wolfssl"`.  Do **not** filter by `"wolfssl"`
substring; instead filter by path depth or accept all `.c` files from the
project.

```python
#!/usr/bin/env python3
r"""
Extract .c source paths from an IAR EWARM .ewp project file.

Paths are emitted as absolute paths (resolves $PROJ_DIR$ automatically).
Pass --raw to keep the original $PROJ_DIR$ prefix instead.

Usage:
  python3 extract-iar-srcs.py MyProject.ewp [--raw] > wolfssl-srcs.txt
"""
import sys, os, argparse, xml.etree.ElementTree as ET


def is_excluded(file_elem):
    """True if the file is excluded from at least one build configuration."""
    return file_elem.find('excluded') is not None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('ewp')
    ap.add_argument('--raw', action='store_true',
                    help='Keep $PROJ_DIR$ prefix instead of resolving')
    args = ap.parse_args()

    proj_dir = os.path.dirname(os.path.abspath(args.ewp))
    proj = ET.parse(args.ewp)
    paths = set()

    for file_elem in proj.findall('.//file'):
        if is_excluded(file_elem):
            continue
        name = file_elem.find('name')
        if name is None or not name.text:
            continue
        raw = name.text
        if not raw.lower().endswith('.c'):
            continue
        if args.raw:
            paths.add(raw.replace('\\', '/'))
        else:
            resolved = raw.replace('$PROJ_DIR$', proj_dir)
            paths.add(os.path.normpath(resolved.replace('\\', '/')))

    for p in sorted(paths):
        print(p)


if __name__ == '__main__':
    main()
```

Usage:

```sh
# Absolute paths (ready for gen-sbom)
python3 extract-iar-srcs.py wolfSSL-Lib.ewp > wolfssl-srcs.txt

# Keep $PROJ_DIR$ prefix (for inspection)
python3 extract-iar-srcs.py wolfSSL-Lib.ewp --raw > wolfssl-srcs.txt
```

This produces 65 sources (56 under `wolfcrypt/src/`, 9 under `src/`) for the
standard `wolfSSL-Lib.ewp` project.

**Caveats**:
- The script skips files that appear in `<excluded>` blocks (per-configuration
  exclusions).  If you need sources for a specific configuration only, check
  `<excluded><configuration>` matches against your target config name.
- Application-specific `.c` files (test runners, benchmark harness) will also
  appear; remove them from `wolfssl-srcs.txt` manually if they are not part
  of your shipped wolfssl build.

### Automatic extraction

Set `CRA_SBOM_IAR_PROJECT` to the path of your `.ewp` file:

```sh
CRA_SBOM_MODE=embedded \
CRA_SBOM_IAR_PROJECT=/path/to/wolfSSL-Lib.ewp \
WOLFSSL_DIR=/path/to/wolfssl \
./scripts/generate-wolfssl-sbom.sh
```

The script resolves `$PROJ_DIR$` automatically.  Requires `python3`.

---

## Verifying the output

After generating `wolfssl-srcs.txt`, sanity-check it:

```sh
# Count should match your mental model of what you compile
wc -l wolfssl-srcs.txt

# All paths should exist on disk
while IFS= read -r f; do
  [ -f "$f" ] || echo "MISSING: $f"
done < wolfssl-srcs.txt

# No duplicates (gen-sbom deduplicates, but worth checking)
sort wolfssl-srcs.txt | uniq -d
```
