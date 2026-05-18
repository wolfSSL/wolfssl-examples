# Supply-chain artefacts — today vs roadmap

Honest status for customer conversations. This is **not** a commitment schedule.

| Capability | Status | What you do today |
|--------------|--------|-------------------|
| **SBOM** (SPDX 2.3 + CycloneDX 1.6) | **Available** | `make sbom` or `scripts/gen-sbom` |
| **Config-accurate build properties** | **Available** | Read `wolfssl:build:*` in `.cdx.json` |
| **Embedded source-merkle checksum** | **Available** | `gen-sbom` with `--srcs` (no `libwolfssl.a` required) |
| **Commercial license in SBOM** | **Available** | `CRA_LICENSE_OVERRIDE=LicenseRef-wolfSSL-Commercial ./scripts/generate-wolfssl-sbom.sh` (or `make-commercial-sample.sh` to derive from pinned GPL samples) |
| **Reproducible SBOM timestamps** | **Available** | `SOURCE_DATE_EPOCH` |
| **OmniBOR / `make bomsh`** | **Available** | Linux **build host** only; optional for CRA |
| **`pkg:github` PURL** | **Available** | Auto-canonicalised by `generate-wolfssl-sbom.sh` post-process; resolves in OSV / GHSA / Snyk / Trivy without per-vendor mapping |
| **Cryptographic-asset draft** (CycloneDX 1.6) | **Draft sample** | Hand-rolled `wolfssl-<ver>.cbom-draft.cdx.json` alongside SBOM (4–6 starter entries); upstream automation: roadmap |
| **Formal CBOM** (`cryptographic-asset` profile, all primitives) | **Roadmap** | Use draft sample + `wolfssl:build:*` properties |
| **VEX templates / automation** | **Roadmap** | Your scanner + wolfSSL [advisories](https://www.wolfssl.com/docs/security-vulnerabilities/) |
| **CSAF 2.0 advisory feed** (`/.well-known/csaf/`) | **Roadmap** | Human-readable [advisories](https://www.wolfssl.com/docs/security-vulnerabilities/) today; CSAF 2.0 publication is on the roadmap (BSI's CRA reference architecture assumes CSAF) |
| **Signed SBOMs** (in-toto / cosign / Sigstore) | **Roadmap** | Unsigned today; signing is conspicuous-by-absence for a crypto vendor and is on the roadmap |
| **SBOM publication channel** | **Roadmap** | Per-release artefacts on GitHub Releases (proposed); `wolfssl.com/sbom/` (proposed); discovery via PURL is the long-term goal |
| **Product SBOM tool** | **Out of scope** | Your BOM platform or manual merge |

Upstream implementation detail: [wolfssl/doc/SBOM.md](https://github.com/wolfSSL/wolfssl/blob/master/doc/SBOM.md).

---

## Vulnerability-handling roadmap (Pillar 4)

The kit's vulnerability-handling pillar is the only **ongoing** CRA obligation.
Status of wolfSSL Inc.'s own filings is tracked here so customers can see what
they're actually inheriting when they reference us as a component supplier.

| Capability | Status | Notes |
|------------|--------|-------|
| `security.txt` (RFC 9116) | **Available** | [`/.well-known/security.txt`](https://www.wolfssl.com/.well-known/security.txt) |
| Coordinated Vulnerability Disclosure policy | **Available** | [`/.well-known/vulnerability-disclosure-policy.txt`](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt) |
| CNA status | **Available** | wolfSSL is a CVE Numbering Authority |
| Public SLA (24h ack / 72h triage) | **Pending leadership approval** | Will be added to CVD policy once approved |
| 24h ENISA reporting (Art. 14) runbook | **In progress** | Owner assignment pending; on-call rotation TBD |
| EU Authorised Representative (Art. 18) | **In progress** | wolfSSL Inc. is US-established; AR appointment underway |
| CSAF 2.0 advisory feed | **Roadmap** | See above |

See [`wolfssl-inc-auditor-packet/`](wolfssl-inc-auditor-packet/) for the manufacturer-side
filings wolfSSL Inc. ships under CRA.
