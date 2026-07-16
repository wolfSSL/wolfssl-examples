#!/bin/sh
# Produce a commercial-license-override sample alongside the pinned GPL samples.
#
# This script is illustrative: it derives wolfssl-<ver>.commercial.{cdx,spdx}.json
# from the GPL pinned files by swapping the license fields and adding a
# wolfssl:license:override property. Auditors see the same build configuration,
# the same hashes of the source list, and a different license declaration —
# exactly the diff a paying wolfSSL customer's SBOM should show.
#
# In production, regenerate via:
#   CRA_LICENSE_OVERRIDE=LicenseRef-wolfSSL-Commercial \
#       ./scripts/generate-wolfssl-sbom.sh
set -eu

SCRIPT_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
KIT_DIR=$(dirname "$SCRIPT_DIR")

# shellcheck disable=SC1090,SC1091
. "$KIT_DIR/VERSION"
COMP_DIR="$KIT_DIR/auditor-packet/wolfssl-component"
GPL_CDX="$COMP_DIR/wolfssl-${WOLFSSL_VERSION}.cdx.json"
GPL_SPDX="$COMP_DIR/wolfssl-${WOLFSSL_VERSION}.spdx.json"
COMMERCIAL_CDX="$COMP_DIR/wolfssl-${WOLFSSL_VERSION}.commercial.cdx.json"
COMMERCIAL_SPDX="$COMP_DIR/wolfssl-${WOLFSSL_VERSION}.commercial.spdx.json"
LICENSE_ID=${CRA_LICENSE_OVERRIDE:-LicenseRef-wolfSSL-Commercial}

[ -f "$GPL_CDX" ]  || { echo "ERROR: $GPL_CDX not found (run refresh-samples first)" >&2; exit 1; }
[ -f "$GPL_SPDX" ] || { echo "ERROR: $GPL_SPDX not found (run refresh-samples first)" >&2; exit 1; }

GPL_CDX="$GPL_CDX" GPL_SPDX="$GPL_SPDX" \
COMMERCIAL_CDX="$COMMERCIAL_CDX" COMMERCIAL_SPDX="$COMMERCIAL_SPDX" \
LICENSE_ID="$LICENSE_ID" \
python3 <<'PY'
import json, os, pathlib, uuid

gpl_cdx = pathlib.Path(os.environ["GPL_CDX"])
gpl_spdx = pathlib.Path(os.environ["GPL_SPDX"])
out_cdx = pathlib.Path(os.environ["COMMERCIAL_CDX"])
out_spdx = pathlib.Path(os.environ["COMMERCIAL_SPDX"])
license_id = os.environ["LICENSE_ID"]

# --- CycloneDX side ----
d = json.loads(gpl_cdx.read_text())
d["serialNumber"] = "urn:uuid:" + str(uuid.uuid4())
comp = d.get("metadata", {}).get("component", {})
comp["licenses"] = [{"license": {"name": "wolfSSL Commercial License (" + license_id + ")"}}]
props = comp.setdefault("properties", [])
if not any(p.get("name") == "wolfssl:license:override" for p in props):
    props.append({"name": "wolfssl:license:override", "value": license_id})
out_cdx.write_text(json.dumps(d, indent=2) + "\n")
print(f"Wrote {out_cdx.name} (license override: {license_id})")

# --- SPDX side ----
d = json.loads(gpl_spdx.read_text())
d["documentNamespace"] = "urn:uuid:" + str(uuid.uuid4())
d["hasExtractedLicensingInfos"] = [
    {
        "licenseId": license_id,
        "extractedText": (
            "wolfSSL commercial license. See https://www.wolfssl.com/license/ for terms. "
            "Replaces the GPL-3.0-only declaration of the open-source distribution."
        ),
        "name": "wolfSSL Commercial License",
        "seeAlsos": ["https://www.wolfssl.com/license/"],
    }
]
# Only the wolfSSL package is relicensed. Dependency packages (e.g. zlib/Zlib,
# liboqs/MIT) keep their own upstream licenses; overwriting them would falsely
# claim the wolfSSL commercial license covers third-party code. This mirrors the
# CycloneDX side above, which only touches metadata.component.
for pkg in d.get("packages", []):
    if pkg.get("SPDXID") != "SPDXRef-Package-wolfssl" and pkg.get("name") != "wolfssl":
        continue
    pkg["licenseConcluded"] = license_id
    pkg["licenseDeclared"] = license_id
    existing = pkg.get("comment", "")
    marker = f"License override applied: {license_id}."
    if marker not in existing:
        pkg["comment"] = (marker + " " + existing).strip()
out_spdx.write_text(json.dumps(d, indent=2) + "\n")
print(f"Wrote {out_spdx.name} (license override: {license_id})")
PY
