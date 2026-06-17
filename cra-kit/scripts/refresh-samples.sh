#!/bin/sh
# Regenerate pinned autotools samples and sync the product SBOM hashes
# (SPDX externalDocumentRef checksum + CycloneDX bom externalReference hash).
set -eu

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
KIT_DIR=$(dirname "$SCRIPT_DIR")

export CRA_SBOM_MODE=autotools
export CRA_SBOM_OUT_DIR="$KIT_DIR/auditor-packet/wolfssl-component"
"$SCRIPT_DIR/generate-wolfssl-sbom.sh"

# shellcheck disable=SC1090,SC1091
. "$KIT_DIR/VERSION"
COMPONENT_SPDX="$KIT_DIR/auditor-packet/wolfssl-component/wolfssl-${WOLFSSL_VERSION}.spdx.json"
COMPONENT_CDX="$KIT_DIR/auditor-packet/wolfssl-component/wolfssl-${WOLFSSL_VERSION}.cdx.json"
PRODUCT_SPDX="$KIT_DIR/auditor-packet/product-acme-connect-gateway.spdx.json"
PRODUCT_CDX="$KIT_DIR/auditor-packet/product-acme-connect-gateway.cdx.json"

COMPONENT_SPDX="$COMPONENT_SPDX" COMPONENT_CDX="$COMPONENT_CDX" \
PRODUCT_SPDX="$PRODUCT_SPDX" PRODUCT_CDX="$PRODUCT_CDX" \
python3 <<'PY'
import hashlib, json, os, pathlib

component_spdx = pathlib.Path(os.environ["COMPONENT_SPDX"])
component_cdx = pathlib.Path(os.environ["COMPONENT_CDX"])
product_spdx = pathlib.Path(os.environ["PRODUCT_SPDX"])
product_cdx = pathlib.Path(os.environ["PRODUCT_CDX"])

# --- SPDX side: pin externalDocumentRef checksum ---------------------------
spdx_digest = hashlib.sha256(component_spdx.read_bytes()).hexdigest()
doc = json.loads(product_spdx.read_text())
refs = doc.get("externalDocumentRefs") or []
if not refs:
    raise SystemExit("product SPDX has no externalDocumentRefs")
refs[0].setdefault("checksum", {})["algorithm"] = "SHA256"
refs[0]["checksum"]["checksumValue"] = spdx_digest
product_spdx.write_text(json.dumps(doc, indent=2) + "\n")
print(f"Updated {product_spdx.name} externalDocumentRef checksum -> {spdx_digest}")

# --- CycloneDX side: pin component externalReference hash ------------------
cdx_digest = hashlib.sha256(component_cdx.read_bytes()).hexdigest()
prod = json.loads(product_cdx.read_text())
patched = False
for comp in prod.get("components", []):
    if comp.get("name") == "wolfssl":
        for ref in comp.get("externalReferences", []):
            if ref.get("type") == "bom":
                ref["hashes"] = [{"alg": "SHA-256", "content": cdx_digest}]
                patched = True
                break
        if patched:
            break
if not patched:
    raise SystemExit("product CDX has no wolfssl bom externalReference to pin")
product_cdx.write_text(json.dumps(prod, indent=2) + "\n")
print(f"Updated {product_cdx.name} CycloneDX bom hash -> {cdx_digest}")
PY

"$SCRIPT_DIR/validate.sh"
