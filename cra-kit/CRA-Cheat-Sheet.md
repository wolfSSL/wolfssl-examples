# wolfSSL CRA Supply Chain Cheat Sheet

**Who provides what** — **you** vs **wolfSSL**  
Print this page; use **[CRA-Supply-Chain-Glossary.md](CRA-Supply-Chain-Glossary.md)** for full definitions (SBOM, SPDX, CycloneDX, CBOM, VEX, bomsh, PURL, …).

**Not legal advice.** You are the **manufacturer** for your product on the EU market.
wolfSSL provides **component evidence** for the **wolfSSL library only**.
wolfSSL Inc. is itself a manufacturer under CRA for libraries it places on the EU market —
see our [`security.txt`](https://www.wolfssl.com/.well-known/security.txt),
[CVD policy](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt),
and our manufacturer-side filings in
[`wolfssl-inc-auditor-packet/`](wolfssl-inc-auditor-packet/) for reference.

Requires a wolfSSL tree with SBOM support (`make sbom` / `scripts/gen-sbom`).
`make sbom` also needs `pyspdxtools` (`pip install spdx-tools`).

**CRA Kit:** `wolfssl-examples/cra-kit/` · **AI playbook:** [SKILL.md](SKILL.md)  
**Product-level CRA shortlist (4 pillars):** [CRA-Compliance-Shortlist.md](CRA-Compliance-Shortlist.md)

---

## CRA compliance shortlist (four pillars)

| Pillar | You | wolfSSL |
|--------|-----|---------|
| **1. Know your components** | Product SBOM + vuln process for whole product | Component SBOMs, advisories, updates — **this kit** |
| **2. Secure boot** | Trusted firmware + update path | **wolfBoot** |
| **3. Data in transfer** | Secure protocols for remote/cloud traffic | **TLS**, **SSH**, **MQTTS**, … |
| **4. Vulnerability handling & reporting** | Published CVD policy + `security.txt`; 24h reporting via the **SRP** to your coordinator CSIRT + ENISA (Art. 14/16); on-call coverage | Reference templates: wolfSSL [`security.txt`](https://www.wolfssl.com/.well-known/security.txt) + [CVD policy](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt); advisories; CNA |

Detail: [CRA-Compliance-Shortlist.md](CRA-Compliance-Shortlist.md)

---

## Who provides what (you vs wolfSSL)

| | **You (product manufacturer)** | **wolfSSL (library supplier)** |
|---|-------------------------------|--------------------------------|
| **Inventory** | **Product SBOM** — OS, apps, all third-party code | **Component SBOM** — wolfSSL only (SPDX + CycloneDX) |
| **How you connect** | Nest or reference our files in your product SBOM | Ship `wolfssl-*.spdx.json` and `wolfssl-*.cdx.json` |
| **Vulnerabilities** | Your process + owner for the shipped product | [Advisories](https://www.wolfssl.com/docs/security-vulnerabilities/) + [CVD policy](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt) + [`security.txt`](https://www.wolfssl.com/.well-known/security.txt) |
| **Optional build proof** | Only if your contract/auditor asks | `make bomsh` / OmniBOR (**Linux build host** only) |

**Worked example:** [`auditor-packet/`](auditor-packet/) — fictional *Acme Connect Gateway* + wolfSSL SBOMs nested.

---

## What auditors ask

| Question | Term | wolfSSL today |
|----------|------|---------------|
| What software is in the product? | **SBOM** | `make sbom` or `gen-sbom` → SPDX + CycloneDX |
| What crypto is enabled in *your* build? | **CBOM** (path) | `wolfssl:build:*` in CycloneDX — not full `cryptographic-asset` yet |
| How was the library binary built? | **Provenance** | `make bomsh` (**Linux** host, optional) |

*See glossary for SPDX vs CycloneDX, VEX, PURL, OmniBOR.*

---

## BOMs at a glance

| Name | Owner | wolfSSL today |
|------|-------|---------------|
| **Product SBOM** | **You** | — |
| **Component SBOM** | **wolfSSL** (you nest) | **Yes** |
| **CBOM** | **You** document; we signal config | **Partial** (build properties) |
| **VEX** | **You** (+ scanner) | Advisories only |
| **bomsh** | **wolfSSL** (optional) | **Yes**, Linux host only |

Details: [CRA-Supply-Chain-Glossary.md](CRA-Supply-Chain-Glossary.md)

---

## Four decisions

| Question | Answer |
|----------|--------|
| Need **our own** SBOM? | **Yes** |
| wolfSSL SBOM **enough alone**? | **No** — nest or reference in yours |
| Need **bomsh** for CRA? | **Usually no** |
| **SPDX** or **CycloneDX**? | **Both** — use what your tools consume |

---

## Beyond this kit (don't skip)

This kit covers **software transparency** only. Before placing your product on
the EU market you also need:

| Obligation | Article | Action |
|------------|---------|--------|
| **EU Authorised Representative** | Art. 18 | Required if you're established outside the EU |
| **Product class** (Annex III/IV) | — | Determines self-cert vs **Notified Body** — long queues |
| **Conformity assessment + CE mark** | Art. 32, 30 | Module A or external review |
| **Technical documentation** | Annex VII | Risk assessment, support-period commitment |
| **Free security updates** | Art. 13(8) | 5+ year support period default |

Engage CRA counsel/consultant — these are legal/structural decisions, not
artefacts. See [`CRA-Compliance-Shortlist.md`](CRA-Compliance-Shortlist.md)
"Beyond this kit" for detail.

---

## What to read next

| Resource | File |
|----------|------|
| Full glossary | [CRA-Supply-Chain-Glossary.md](CRA-Supply-Chain-Glossary.md) |
| Integration guide | [README.md](README.md) |
| Sample auditor folder | [auditor-packet/](auditor-packet/) |
| AI + scripts playbook | [SKILL.md](SKILL.md) |
| Upstream SBOM reference (flags, formats, OmniBOR) | [wolfssl/doc/SBOM.md](https://github.com/wolfSSL/wolfssl/blob/master/doc/SBOM.md) |

**Questions about this kit:** support@wolfssl.com · **Security reports:** see [`security.txt`](https://www.wolfssl.com/.well-known/security.txt)
