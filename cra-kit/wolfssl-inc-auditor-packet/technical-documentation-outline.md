# Technical documentation outline — Annex VII

**Status:** 🟠 In progress — outline complete; per-release populated documents on roadmap
**CRA reference:** Annex VII (technical documentation contents)

CRA Annex VII enumerates the contents of the technical documentation file that
manufacturers must maintain (and retain for **10 years** after market placement)
for each conformant product. This file is not made public; it is held by the
manufacturer (and the EU AR) and produced to authorities on request.

## Outline of wolfSSL Inc.'s per-release technical documentation file

For each wolfSSL library version placed on the EU market under CRA, the
following sections are populated:

### 1. General description

- Product name, version, intended purpose
- Variants and configurations (e.g. FIPS-validated build, embedded build, commercial-license build)
- Identification of integrated components (the wolfSSL SBOM itself)

### 2. Design and manufacturing

- Architectural description (TLS state machine, cryptographic API surfaces, build system)
- Source-tree organisation (where to find what)
- Build instructions and reproducibility settings (`SOURCE_DATE_EPOCH`, `make sbom`, `make bomsh`)
- Reference to the SBOM: `wolfssl-<version>.cdx.json`, `.spdx.json`

### 3. Cybersecurity risk assessment

- Threat model: what wolfSSL is designed to protect, what it is not
- Attack surface analysis (network-facing TLS handshake, parser surfaces, key management)
- Risk-mitigation choices (timing-resistance flags, side-channel hardening, deprecated algorithm exclusions)
- Reference to relevant external assessments (FIPS 140-3 Cryptographic Module Validation Program reports, third-party penetration tests where commissioned)

### 4. List of harmonised standards applied

- [TO BE FILLED once CRA harmonised standards are published]
- Where standards are not available: technical specifications applied (e.g. RFC 5246, RFC 8446 for TLS; FIPS 140-3 for the FIPS-validated build)

### 5. Conformity assessment route

- Annex VIII Module A (self-assessment) — see [`conformity-assessment-route.md`](conformity-assessment-route.md)

### 6. Vulnerability handling

- CVD policy (link to `/.well-known/vulnerability-disclosure-policy.txt`)
- Process narrative (see [`vulnerability-handling-process.md`](vulnerability-handling-process.md))
- Per-release: any open advisories at time of release, with their CVE IDs

### 7. Support-period commitment

- See [`support-period-policy.md`](support-period-policy.md)
- Per-release: explicit support window dates

### 8. Declaration of conformity

- Signed declaration per Art. 28 — see [`declaration-of-conformity.template.md`](declaration-of-conformity.template.md)

### 9. Software bill of materials

- `wolfssl-<version>.cdx.json` (CycloneDX 1.6)
- `wolfssl-<version>.spdx.json` (SPDX 2.3)
- Optional: `omnibor.wolfssl-<version>.spdx.json` (build provenance via `make bomsh`)
- Optional: `wolfssl-<version>.cbom-draft.cdx.json` (cryptographic-asset draft)

### 10. CE marking

- See [`ce-marking-statement.md`](ce-marking-statement.md)

## Retention

- **10 years** from the date the product is placed on the EU market, or for the duration of the support period (whichever is longer).
- Held by wolfSSL Inc. **and** the EU Authorised Representative ([`eu-authorised-representative.md`](eu-authorised-representative.md)).

## What this means for customers

You maintain a parallel Annex VII file for **your** finished product. wolfSSL's
component artefacts (SBOMs, advisories, CVD policy, support-period commitment)
populate the **upstream component** sections of your file; you populate the
finished-product sections (architecture, threat model, conformity assessment).
Our file is not yours; yours integrates ours.

## References

- CRA Annex VII (technical documentation)
- CRA Art. 31 (technical documentation retention)
- [`../CRA-Compliance-Shortlist.md`](../CRA-Compliance-Shortlist.md) — Annex VII row in "Beyond this kit"
