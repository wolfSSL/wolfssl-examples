# CRA co-sponsor slide track (~15 min)

Companion kit: [`../CRA-Cheat-Sheet.md`](../CRA-Cheat-Sheet.md) ·
[`../CRA-Supply-Chain-Glossary.md`](../CRA-Supply-Chain-Glossary.md) ·
[`../SKILL.md`](../SKILL.md) · [`../auditor-packet/`](../auditor-packet/)

---

## Slide: Shortlist towards CRA compliance

Use **[`CRA-Compliance-Shortlist.md`](../CRA-Compliance-Shortlist.md)** — two columns per pillar:
**your job** vs **wolfSSL helps**.

| Pillar | On slide (customer) | wolfSSL |
|--------|---------------------|---------|
| **Know your software components** | Survey all integrated components: who maintains them? how do you track vulns/releases? | SBOMs for our products; continuous vulnerability management and updates |
| **Implement secure boot** | Most influential action today: trusted firmware + update path aligned with complaint/timing rules | **wolfBoot** |
| **Remote data processing / data in transfer** | CRA covers data between device and network — use current crypto and secure protocols | **TLS**, **SSH**, **MQTTS**, … |
| **Vulnerability handling & reporting** | Published CVD policy + `security.txt`; 24h reporting via the **SRP** (to coordinator CSIRT + ENISA, Art. 14/16); on-call coverage — process, not a deliverable | wolfSSL [`security.txt`](https://www.wolfssl.com/.well-known/security.txt) + [CVD policy](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt) as reference templates; advisories; CNA |

**Bridge to this session:** pillar 1 is where the **CRA Kit** lands (SBOM, auditor packet, scripts).

---

## Slide: Promise — what you leave with

**You will leave with:**

1. **Who provides what** — what **you** provide vs what **wolfSSL** provides  
   → [`CRA-Cheat-Sheet.md`](../CRA-Cheat-Sheet.md) (print/PDF)  
   → full terms: [`CRA-Supply-Chain-Glossary.md`](../CRA-Supply-Chain-Glossary.md)

2. **A worked example** — wolfSSL CRA Kit  
   → [`wolfssl-examples/cra-kit/auditor-packet/`](../auditor-packet/)

3. **Helper scripts + AI playbook** — product SBOM, nest wolfSSL, optional **bomsh** on **Linux CI** only  
   → **[`SKILL.md`](../SKILL.md)** for AI-assisted execution (Cursor / agents)

---

## Talking points

| Instead of… | Say… |
|-------------|------|
| Learn every acronym | “Cheat sheet for roles; glossary in the same kit.” |
| wolfSSL is CRA compliant | “Component SBOMs from us; **product** SBOM and vuln process from you.” |
| We ship CBOM | “Build properties today; formal CBOM profile on the roadmap.” |
| You need bomsh | “Usually no — Linux CI only if a contract asks.” |
| AI is extra | “**SKILL.md** is the playbook—copy it into Cursor and run the scripts with your tree.” |

---

## Demo path (optional live)

```bash
cd wolfssl-examples/cra-kit
./scripts/validate.sh
```

Show `auditor-packet/product-acme-connect-gateway.cdx.json` → wolfSSL component reference.

Optional: show copying `SKILL.md` into `.cursor/skills/wolfssl-cra-kit/`.

---

## Kit documents (handout stack)

| Layer | File |
|-------|------|
| Who provides what (1 page) | `CRA-Cheat-Sheet.md` |
| Glossary (reference) | `CRA-Supply-Chain-Glossary.md` |
| AI playbook | `SKILL.md` |
| Full guide | `README.md` |
