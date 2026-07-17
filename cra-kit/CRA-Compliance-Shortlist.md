# Shortlist towards CRA compliance

**Not legal advice.** The EU Cyber Resilience Act applies to **your product** as a whole.
wolfSSL helps on **specific pillars** below; you remain the **manufacturer** for market obligations.

This page is the **product-level shortlist** (what to do). For **software transparency** work
(SBOM, nesting, sample auditor folder), use the **[CRA Kit](README.md)** cheat sheet and
[`CRA-Cheat-Sheet.md`](CRA-Cheat-Sheet.md).

---

## 1. Know your software components

| **Your job (manufacturer)** | **wolfSSL can help** |
|----------------------------|----------------------|
| Run a **survey** of every component in your embedded system or product: What is it? Who maintains it? Is it actively developed? How do you learn about vulnerabilities, fixes, and releases? | **Component SBOMs** (SPDX + CycloneDX) for wolfSSL libraries you ship — `make sbom` / `gen-sbom` |
| Build and maintain a **product SBOM** for the whole thing you place on the EU market | **Continuous vulnerability management**: [security advisories](https://www.wolfssl.com/docs/security-vulnerabilities/), coordinated disclosure, updates — see wolfSSL [`security.txt`](https://www.wolfssl.com/.well-known/security.txt) and [CVD policy](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt) |
| Own vulnerability **process**, owners, and fix timelines for **your** release | Nest or reference our component SBOM in yours — worked example: [`auditor-packet/`](auditor-packet/) |

**CRA Kit focus:** pillar 1 — who provides what cheat sheet, glossary, scripts, [`SKILL.md`](SKILL.md).

---

## 2. Implement secure boot

| **Your job (manufacturer)** | **wolfSSL can help** |
|----------------------------|----------------------|
| Treat secure boot as one of the **most influential actions** you can take now: firmware that boots **trusted**, with a defined path to **update** when needed | **[wolfBoot](https://www.wolfssl.com/products/wolfboot/)** — secure bootloader for embedded systems |
| Align update mechanics with your **complaint / incident** procedures and required **timelines** under CRA | Integration with wolfSSL/wolfCrypt; see wolfBoot docs and support |

Secure boot is **product architecture**, not something an SBOM file alone satisfies.

---

## 3. Bring remote data processing and data-in-transfer up to compliance

CRA is **not only about software inventory** — it also concerns **data** moving between the device and the network.

| **Your job (manufacturer)** | **wolfSSL can help** |
|----------------------------|----------------------|
| Map **remote processing** and **connectivity** in your product (cloud, OTA, admin interfaces, telemetry) | Implementations of **state-of-the-art** secure protocols, for example: |
| Use **current cryptography** and **secure protocols** for data in transfer; document what is enabled in **your** build | **TLS** (wolfSSL), **SSH** (wolfSSH), **MQTTS** (wolfMQTT), and related stacks |
| Reflect enabled algorithms in **your** product documentation / SBOM / crypto inventory | Build properties in CycloneDX today (`wolfssl:build:*`); formal CBOM profile: **roadmap** |

---

## 4. Handle vulnerabilities and report on time

CRA imposes **continuous** vulnerability handling obligations on manufacturers
(Art. 13) and a hard **24-hour** reporting clock for actively exploited
vulnerabilities (Art. 14). This is the only CRA pillar that requires **ongoing
operational capacity**, not a one-time deliverable.

| **Your job (manufacturer)** | **wolfSSL can help** |
|----------------------------|----------------------|
| Publish a **Coordinated Vulnerability Disclosure (CVD) policy** and a working security contact (`security.txt` per RFC 9116) so researchers can reach you | Reference templates: wolfSSL's [`security.txt`](https://www.wolfssl.com/.well-known/security.txt) and [CVD policy](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt) |
| Operate a **vulnerability handling process** with named owners and stated response targets | wolfSSL [security advisories](https://www.wolfssl.com/docs/security-vulnerabilities/) for libraries you ship; wolfSSL is a CVE Numbering Authority |
| File via the **Single Reporting Platform** (to your coordinator CSIRT + ENISA) **within 24 hours** when a vulnerability in your product is **actively exploited** (Art. 14/16); follow up at 72 hours and a final report at 14 days | wolfSSL handles reporting for **wolfSSL libraries placed on the EU market by wolfSSL Inc.**; coordinate with us on shared advisories |
| Maintain **on-call coverage** including weekends and holidays so the 24-hour clock can be met at any time | — |

This pillar is **not satisfied by SBOM artefacts alone** — it requires
documented process, named owners, and on-call capacity. The 24-hour ENISA clock
starts from your **awareness** of active exploitation, not from public disclosure.

---

## Beyond this kit (structural CRA obligations)

The four pillars above cover **software transparency**. A full CRA conformity
assessment also requires structural obligations that **this kit does not
cover** — flag these to your CRA consultant or counsel **before** assuming
SBOMs alone make you ready:

| Obligation | Article | What it means |
|------------|---------|---------------|
| **EU Authorised Representative** | Art. 18 | Manufacturers established **outside** the EU must appoint a written-mandated representative **inside** the EU before placing a product on the EU market. Either contract a third-party AR service or use an existing EU subsidiary. |
| **Product classification** | Annex III / IV | Determines whether conformity assessment is self-declared (default class) or requires a **Notified Body** (important / critical class). Notified-body queues are already long — if you may need one, get in queue early. |
| **Conformity assessment + CE mark** | Art. 32, 30 | Module A (self-assessment) or external review per classification; CE marking before placing the product on the EU market. |
| **Technical documentation** | Annex VII | Risk assessment, secure-design rationale, vulnerability handling process, support-period commitment — more than the SBOM. |
| **Free security updates** | Art. 13(8) | Minimum 5-year support period for security updates by default (longer if the product's expected lifetime is longer). |
| **Importer / distributor obligations** | Art. 19, 20 | If your product enters the EU via an importer or moves through distributors, additional obligations attach to those parties. |

These are **legal and structural decisions**, not artefacts you can generate
from source code. wolfSSL ships SBOMs, security-policy templates, and the
narrative in this kit; **you** appoint your EU AR, classify your product, run
your conformity assessment, and produce your declaration of conformity. If
you do not yet have a CRA consultant, engaging one for the
classification + AR questions specifically is usually the highest-leverage
early step.

**See how wolfSSL Inc. itself answers each of these.**
[`wolfssl-inc-auditor-packet/`](wolfssl-inc-auditor-packet/) holds the
manufacturer-side filings wolfSSL Inc. ships under CRA: Annex III/IV
classification statement, conformity assessment route, declaration of
conformity template, EU Authorised Representative status, support-period
policy, vulnerability-handling process, technical documentation outline,
and CE marking statement. Where decisions are made, they're stated; where
they're in flight (EU AR appointment, public SLA), the gap is named.
Adapt as a template for your own product.

---

## How this maps to the CRA Kit

| Shortlist pillar | Kit deliverable |
|------------------|-----------------|
| Know your components | Cheat sheet (who provides what), glossary, `auditor-packet/`, generate/validate scripts |
| Secure boot | Out of scope for SBOM files — evaluate **wolfBoot** separately |
| Data in transfer | Configure and document **your** protocol stack; wolfSSL ships crypto libraries, not your full product compliance |
| Vulnerability handling & reporting | Outside scope of SBOM artefacts — see Art. 13/14 obligations above; wolfSSL's own [CVD policy](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt) and [`security.txt`](https://www.wolfssl.com/.well-known/security.txt) are usable as reference templates |
| Structural CRA obligations (EU AR, Annex III/IV, CE, technical docs, support period) | **Out of scope** for this kit — see "Beyond this kit" section above; engage CRA counsel or consultant |

**You will leave with (presentation Promise):**

1. **Who provides what** — [`CRA-Cheat-Sheet.md`](CRA-Cheat-Sheet.md)  
2. **Worked example** — [`auditor-packet/`](auditor-packet/)  
3. **Helper scripts + AI playbook** — product SBOM, nest wolfSSL, optional bomsh on **Linux CI** + [`SKILL.md`](SKILL.md)

---

## Related wolfSSL products (beyond this kit)

| Area | Product / doc |
|------|----------------|
| TLS / wolfCrypt | [wolfssl.com](https://www.wolfssl.com/) · upstream SBOM reference: [doc/SBOM.md](https://github.com/wolfSSL/wolfssl/blob/master/doc/SBOM.md) |
| Secure boot | [wolfBoot](https://www.wolfssl.com/products/wolfboot/) |
| SSH | wolfSSH |
| MQTT | wolfMQTT |

**Questions about this kit:** support@wolfssl.com · **Security reports:** see [`security.txt`](https://www.wolfssl.com/.well-known/security.txt)
