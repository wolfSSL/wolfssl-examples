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

---

## Zephyr RTOS

> **Note**: This section was written from knowledge of Zephyr's build system.
> It has not been run against a live Zephyr + wolfssl-zephyr workspace.
> Verify on your own board before using in production.  Corrections welcome.

Zephyr uses CMake and always writes `compile_commands.json` to the build dir:

```sh
west build -b <board> -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

(Some Zephyr versions enable `compile_commands.json` by default; check
`build/compile_commands.json` after a plain `west build`.)

Extract wolfSSL sources:

```sh
WOLFSSL_DIR=/path/to/wolfssl
jq -r '.[].file' build/compile_commands.json \
  | grep "^${WOLFSSL_DIR}/" \
  | grep -E "/(wolfcrypt/src|src)/[^/]+\.c$" \
  | sort -u \
  > wolfssl-srcs.txt
```

The filter pattern is the same as the CMake section above.  Zephyr may also
compile wolfssl via its module system (`modules/crypto/wolfssl/`); adjust the
`grep "^${WOLFSSL_DIR}/"` prefix to match your workspace layout.

---

## ESP-IDF

> **Note**: This section was written from knowledge of ESP-IDF's build system.
> It has not been run against a live ESP-IDF + wolfssl component workspace.
> Verify on your own target before using in production.  Corrections welcome.

ESP-IDF also uses CMake.  Build your project normally:

```sh
idf.py build
```

ESP-IDF writes `compile_commands.json` to the `build/` subdirectory:

```sh
WOLFSSL_DIR=/path/to/wolfssl
jq -r '.[].file' build/compile_commands.json \
  | grep "^${WOLFSSL_DIR}/" \
  | grep -E "/(wolfcrypt/src|src)/[^/]+\.c$" \
  | sort -u \
  > wolfssl-srcs.txt
```

If wolfssl is installed as a managed component, it may appear under
`build/esp-idf/wolfssl/` or `components/wolfssl/`; adjust the path filter
accordingly.

---

## Keil MDK / uVision (`.uvprojx`)

> **Note**: This section was written from knowledge of the `.uvprojx` XML
> schema.  It has not been tested with a live Keil installation (Windows-only,
> licensed tool).  Verify path separators and group filtering on your project
> before using in production.  Corrections welcome.

Keil stores source files in XML.  Extract wolfSSL sources with Python:

```python
#!/usr/bin/env python3
"""Extract wolfssl .c paths from a Keil .uvprojx file."""
import sys
import xml.etree.ElementTree as ET

proj = ET.parse(sys.argv[1])
paths = set()
for fp in proj.findall('.//File/FilePath'):
    path = fp.text or ''
    if path.lower().endswith('.c') and 'wolfssl' in path.lower():
        # Keil stores paths with backslashes; normalise.
        paths.add(path.replace('\\', '/'))

for p in sorted(paths):
    print(p)
```

Usage:

```sh
python3 extract-keil-srcs.py MyProject.uvprojx > wolfssl-srcs.txt
```

**Caveats**:
- Paths are relative to the `.uvprojx` file; prefix `WOLFSSL_DIR` if
  `gen-sbom` needs absolute paths.
- Groups named "Exclude" or marked `<FileType>` ≠ 1 (C source) should
  be filtered; the snippet above may need extending for complex projects.

---

## IAR Embedded Workbench (`.ewp`)

> **Note**: This section was written from knowledge of the `.ewp` XML schema.
> It has not been tested with a live IAR installation (Windows-only, licensed
> tool).  Verify on your own project before using in production.  Corrections
> welcome.

IAR uses a similar XML format:

```python
#!/usr/bin/env python3
"""Extract wolfssl .c paths from an IAR .ewp file."""
import sys
import xml.etree.ElementTree as ET

proj = ET.parse(sys.argv[1])
paths = set()
for name in proj.findall('.//file/name'):
    path = name.text or ''
    if path.lower().endswith('.c') and 'wolfssl' in path.lower():
        paths.add(path.replace('\\', '/'))

for p in sorted(paths):
    print(p)
```

Usage:

```sh
python3 extract-iar-srcs.py MyProject.ewp > wolfssl-srcs.txt
```

**Caveats**: IAR paths are typically `$PROJ_DIR$\wolfssl\...` — strip the
`$PROJ_DIR$` prefix and replace with the absolute path before passing to
`gen-sbom`.

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
