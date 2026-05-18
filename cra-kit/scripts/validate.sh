#!/bin/sh
# Sanity checks on the example auditor packet.
#
# Mandatory:    JSON parse, SPDX externalDocumentRef checksum, CycloneDX bom hash (if pinned).
# Best-effort:  CycloneDX 1.6 schema (cyclonedx-cli) and SPDX 2.3 schema (pyspdxtools)
#               validation, when those tools are installed locally.
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
KIT_DIR=$(dirname "$SCRIPT_DIR")
AP="$KIT_DIR/auditor-packet"
PRODUCT_CDX="$AP/product-acme-connect-gateway.cdx.json"
PRODUCT_SPDX="$AP/product-acme-connect-gateway.spdx.json"

fail() { echo "FAIL: $*" >&2; exit 1; }
ok() { echo "OK: $*"; }

command -v python3 >/dev/null 2>&1 || fail "python3 required"

# shellcheck disable=SC1090
. "$KIT_DIR/VERSION" 2>/dev/null || WOLFSSL_VERSION=5.9.1
WOLF_CDX="$AP/wolfssl-component/wolfssl-${WOLFSSL_VERSION}.cdx.json"
WOLF_SPDX="$AP/wolfssl-component/wolfssl-${WOLFSSL_VERSION}.spdx.json"

for f in "$PRODUCT_CDX" "$PRODUCT_SPDX" "$WOLF_CDX" "$WOLF_SPDX"; do
    [ -f "$f" ] || fail "missing $f"
    python3 -c "import json; json.load(open('$f'))" || fail "invalid JSON: $f"
    ok "$(basename "$f") parses"
done

# CycloneDX 1.6 serialNumber must match urn:uuid:<v4-uuid>; auditors with strict
# validators (cyclonedx-cli) reject anything else. Catch this even when the tool
# isn't installed.
PRODUCT_CDX="$PRODUCT_CDX" WOLF_CDX="$WOLF_CDX" python3 <<'PY'
import json, os, re, sys
UUID = re.compile(r"^urn:uuid:[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$", re.I)
errors = []
for env in ("PRODUCT_CDX", "WOLF_CDX"):
    path = os.environ[env]
    sn = json.load(open(path)).get("serialNumber", "")
    if not UUID.match(sn):
        errors.append(f"{os.path.basename(path)}: serialNumber {sn!r} is not urn:uuid:<v4>")
if errors:
    sys.exit("CycloneDX serialNumber violation(s):\n  " + "\n  ".join(errors))
print("OK: CycloneDX serialNumbers are valid urn:uuid:<v4>")
PY

PRODUCT_SPDX="$PRODUCT_SPDX" WOLF_SPDX="$WOLF_SPDX" python3 <<'PY'
import hashlib, json, os, sys

product = json.load(open(os.environ["PRODUCT_SPDX"]))
wolf = open(os.environ["WOLF_SPDX"], "rb").read()
digest = hashlib.sha256(wolf).hexdigest()
refs = product.get("externalDocumentRefs") or []
if not refs:
    sys.exit("product SPDX has no externalDocumentRefs")
chk = refs[0].get("checksum", {}).get("checksumValue", "")
if chk.lower() != digest.lower():
    sys.exit(
        f"SPDX checksum mismatch:\n  embedded={chk}\n  actual  ={digest}\n"
        "Run scripts/refresh-samples.sh after regenerating wolfSSL SBOM."
    )
print("OK: product SPDX checksum matches wolfssl-component SBOM")
PY

PRODUCT_CDX="$PRODUCT_CDX" WOLF_CDX="$WOLF_CDX" python3 <<'PY'
import hashlib, json, os, sys

prod = json.load(open(os.environ["PRODUCT_CDX"]))
wolf_bytes = open(os.environ["WOLF_CDX"], "rb").read()
digest = hashlib.sha256(wolf_bytes).hexdigest()
comps = prod.get("components") or []
wolf = next((c for c in comps if c.get("name") == "wolfssl"), None)
if not wolf:
    sys.exit("product CDX has no wolfssl component")
if not wolf.get("supplier", {}).get("name"):
    sys.exit("product CDX wolfssl component has no supplier (NTIA min-elements gap)")
refs = wolf.get("externalReferences") or []
bom = next((r for r in refs if r.get("type") == "bom"), None)
if not bom:
    sys.exit("wolfssl component has no bom externalReference")
hashes = bom.get("hashes") or []
if not hashes:
    sys.exit("wolfssl component bom externalReference has no hashes (run refresh-samples.sh)")
got = hashes[0].get("content", "").lower()
if got == "to_be_pinned_by_refresh_samples":
    sys.exit("wolfssl component bom hash is the unpinned placeholder; run refresh-samples.sh")
if got != digest.lower():
    sys.exit(
        f"CycloneDX bom hash mismatch:\n  embedded={got}\n  actual  ={digest}\n"
        "Run scripts/refresh-samples.sh after regenerating wolfSSL SBOM."
    )
print("OK: product CycloneDX bom hash matches wolfssl-component CDX")
print("OK: product CycloneDX wolfssl component has supplier")
PY

# ---- Optional: cyclonedx-cli schema validation ----------------------------
CDX_TOOL=
if command -v cyclonedx-cli >/dev/null 2>&1; then
    CDX_TOOL=cyclonedx-cli
elif command -v cyclonedx >/dev/null 2>&1; then
    CDX_TOOL=cyclonedx
fi
if [ -n "$CDX_TOOL" ]; then
    for cdx in "$PRODUCT_CDX" "$WOLF_CDX"; do
        if "$CDX_TOOL" validate \
            --input-file "$cdx" \
            --input-format json \
            --input-version v1_6 \
            --fail-on-errors >/dev/null 2>&1; then
            ok "$(basename "$cdx") passes CycloneDX 1.6 schema validation ($CDX_TOOL)"
        else
            fail "$(basename "$cdx") fails CycloneDX 1.6 schema validation ($CDX_TOOL)"
        fi
    done
else
    echo "NOTE: cyclonedx-cli not installed; skipping CycloneDX 1.6 schema validation."
    echo "      Install: https://github.com/CycloneDX/cyclonedx-cli/releases"
fi

# ---- Optional: pyspdxtools schema validation ------------------------------
if command -v pyspdxtools >/dev/null 2>&1; then
    for spdx in "$PRODUCT_SPDX" "$WOLF_SPDX"; do
        if pyspdxtools -i "$spdx" >/dev/null 2>&1; then
            ok "$(basename "$spdx") passes SPDX 2.3 schema validation (pyspdxtools)"
        else
            fail "$(basename "$spdx") fails SPDX 2.3 schema validation (pyspdxtools)"
        fi
    done
else
    echo "NOTE: pyspdxtools not installed; skipping SPDX 2.3 schema validation."
    echo "      Install: pip install spdx-tools"
fi

ok "auditor packet validation passed"
