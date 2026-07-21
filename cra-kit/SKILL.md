---
name: wolfssl-cra-kit
description: >-
  wolfSSL CRA Kit playbook: who-provides-what cheat sheet, full glossary,
  auditor-packet sample, generate/validate/refresh scripts for product SBOM +
  nested wolfSSL SBOM, bomsh Linux-only, vulnerability handling (CVD policy +
  security.txt), and pointers to structural CRA obligations (EU Authorised
  Representative Art. 18, Annex III/IV product classification, conformity
  assessment, CE mark) that this kit does NOT cover. Use with Cursor, Claude,
  or any agent for EU CRA software transparency (make sbom, SPDX, CycloneDX).
---

# wolfSSL CRA Kit — AI playbook

Use this file with **Cursor**, **Claude Code**, **Copilot**, or any coding agent
to drive the kit's scripts and narrative without re-explaining CRA terms.

**Not legal advice.** Never claim “CRA compliant.” **Product SBOM** is always yours;
wolfSSL ships **component** evidence only.

wolfSSL Inc. is itself a manufacturer under CRA for libraries it places on the
EU market — see our [`security.txt`](https://www.wolfssl.com/.well-known/security.txt),
[CVD policy](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt),
and the [`wolfssl-inc-auditor-packet/`](wolfssl-inc-auditor-packet/) (manufacturer-side
filings: classification, conformity assessment, declaration of conformity template,
EU AR status, support-period, vulnerability-handling process) as reference templates
for the customer's own CRA artefacts.

---

## What you leave with (matches the presentation)

| Deliverable | File / folder |
|-------------|----------------|
| **CRA shortlist** (4 pillars: components, secure boot, data in transfer, vulnerability handling) | [CRA-Compliance-Shortlist.md](CRA-Compliance-Shortlist.md) |
| **Who provides what** (you vs wolfSSL) | [CRA-Cheat-Sheet.md](CRA-Cheat-Sheet.md) |
| **Full glossary** (SBOM, CBOM, bomsh, …) | [CRA-Supply-Chain-Glossary.md](CRA-Supply-Chain-Glossary.md) |
| **Worked example (customer-side)** | [auditor-packet/](auditor-packet/) — fictional Acme Connect Gateway |
| **Manufacturer-side filings (wolfSSL Inc.)** | [wolfssl-inc-auditor-packet/](wolfssl-inc-auditor-packet/) — classification, DoC template, EU AR status, etc. |
| **Scripts + agent checklist** | This SKILL — below |

---

## Install (Cursor)

```bash
mkdir -p .cursor/skills/wolfssl-cra-kit
cp wolfssl-examples/cra-kit/SKILL.md .cursor/skills/wolfssl-cra-kit/SKILL.md
```

Point the agent at `wolfssl-examples/cra-kit/` (clone or monorepo path).
Set `WOLFSSL_DIR` to your wolfSSL source tree when regenerating SBOMs.

**Other tools:** paste this file into the system prompt, or `@`-mention the kit README.

---

## Agent checklist

**Before starting**, confirm with the customer (do not assume):

- Where is the customer **established** (US / EU / other)? If outside the EU, flag the **EU Authorised Representative** requirement (Art. 18) — long-lead item, start now.
- What is the **product classification** under Annex III/IV? Self-declared (default class) or Notified Body required (important / critical)? Flag if unknown — Notified Body queues are long.
- Is the customer's CRA work **on track for 11 Sep 2026** (Art. 14 reporting wave) and **11 Dec 2027** (full applicability)? If structural items are open, SBOM work alone won't make them ready.

Then run the SBOM execution checklist:

1. **Component SBOM**
   - `cd wolfssl-examples/cra-kit`
   - `WOLFSSL_DIR=/path/to/wolfssl ./scripts/generate-wolfssl-sbom.sh`
   - Or in wolfSSL: `make sbom` (needs `pip install spdx-tools`)

2. **Product SBOM**
   - Open `auditor-packet/product-acme-connect-gateway.{cdx,spdx}.json`
   - Mirror how wolfSSL is referenced/nested for **your** product name and version
   - Embedded builds: `CRA_SBOM_MODE=embedded ./scripts/generate-embedded-sbom.sh` + your `user_settings.h`

3. **Validate without rebuilding wolfSSL**
   - `./scripts/validate.sh`

4. **Refresh pinned samples** (maintainers / after wolfSSL release)
   - `WOLFSSL_DIR=... ./scripts/refresh-samples.sh`

5. **Optional — bomsh**
   - Only if contract/auditor requires; **Linux build host** only (`make bomsh` in wolfSSL tree)
   - Not in the sample auditor packet by default

---

## Scope rules (tell the agent)

- **Product SBOM** = customer owns entire shipped product.
- **Component SBOM** = wolfSSL only; nest via SPDX `externalDocumentRefs` or CycloneDX `bom` ref.
- **CBOM** = partial today (`wolfssl:build:*`); do not claim full CycloneDX CBOM profile.
- **VEX** = customer + scanner; wolfSSL provides advisories, not VEX files.
- **bomsh** = optional provenance; not required for most CRA transparency asks.
- **Vulnerability handling (Art. 13/14/16)** = customer publishes their own CVD policy + `security.txt`, runs on-call, files 24h reports via the **Single Reporting Platform** (to their coordinator CSIRT + ENISA) for their product; wolfSSL provides reference templates and handles reporting only for libraries placed on the EU market by wolfSSL Inc.
- **Structural CRA (out of scope for this kit)** = EU Authorised Representative (Art. 18 — required if customer is outside the EU), Annex III/IV classification (determines self-cert vs Notified Body), conformity assessment + CE mark (Art. 32, 30), technical documentation (Annex VII), support-period commitment (Art. 13(8), 5+ years default). When a customer asks "are we ready?", surface these — SBOMs alone are not enough. Recommend engaging CRA counsel or consultant.

---

## Scripts

| Script | Purpose |
|--------|---------|
| `scripts/validate.sh` | JSON + SPDX checksum on sample packet |
| `scripts/refresh-samples.sh` | `make sbom` + patch product SPDX checksum |
| `scripts/generate-wolfssl-sbom.sh` | `CRA_SBOM_MODE=autotools\|embedded` |
| `scripts/generate-embedded-sbom.sh` | → `auditor-packet/wolfssl-component-embedded/` |

Embedded demo: `user_settings.h` + `WOLFSSL_USER_SETTINGS`.

---

## Sample paths

- Product: `auditor-packet/product-acme-connect-gateway.{spdx,cdx}.json`
- Component: `auditor-packet/wolfssl-component/wolfssl-5.9.1.*`
- Embedded (optional): `auditor-packet/wolfssl-component-embedded/`

---

## Example prompts

- “Walk me through nesting wolfSSL’s CycloneDX SBOM into our product SBOM using `auditor-packet/` as a template.”
- “Run `validate.sh` and fix any checksum mismatch after I regenerated the component SBOM.”
- “Generate an embedded SBOM with our `user_settings.h` and list which algorithms appear in `wolfssl:build:*`.”
- “Do we need bomsh for CRA? When would we run it on Linux CI only?”
- “We're a US company shipping into the EU — what CRA structural items do we need beyond the SBOM?”
- “What's the difference between Annex III and Annex IV classification, and how does it affect our conformity assessment?”

---

## Upstream docs (wolfSSL repo)

- [doc/SBOM.md](https://github.com/wolfSSL/wolfssl/blob/master/doc/SBOM.md) — SBOM/Bomsh feature reference (flags, formats, commercial license override, OmniBOR)
