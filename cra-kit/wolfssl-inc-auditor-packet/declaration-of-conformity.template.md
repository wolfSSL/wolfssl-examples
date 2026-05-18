# Declaration of conformity — template

**Status:** 🟡 Template ready; per-release signed declarations begin 11 Dec 2027
**CRA reference:** Art. 28, Annex V (declaration of conformity contents)

This template will be customised and signed for each conformant wolfSSL release
placed on the EU market from 11 Dec 2027 onwards. Customers may adapt this
template for their own products.

---

## EU Declaration of Conformity

**1. Product identification**

- Name: [PRODUCT NAME, e.g. wolfSSL]
- Version: [VERSION, e.g. 5.9.1]
- Type: [TYPE, e.g. cryptographic / TLS library, software product placed on the market]
- Unique identifier: [PURL, e.g. `pkg:github/wolfSSL/wolfssl@v5.9.1`]

**2. Manufacturer**

- Name: wolfSSL Inc.
- Postal address: [WOLFSSL INC. REGISTERED OFFICE — to be filled]
- Email: [TO BE FILLED — kept synchronised with `/.well-known/security.txt` once wolfSSL Inc.'s security alias is provisioned]
- Website: https://www.wolfssl.com/

**3. EU Authorised Representative** (Art. 18, required for non-EU manufacturers)

- Name: [TO BE FILLED — see `eu-authorised-representative.md`]
- Postal address: [TO BE FILLED]
- Mandate effective date: [TO BE FILLED]

**4. Object of the declaration**

This declaration of conformity is issued under the sole responsibility of the
manufacturer and applies to the object of the declaration described in
section 1.

**5. Conformity statement**

The object of the declaration described above is in conformity with the
relevant Union harmonisation legislation:

- **Regulation (EU) 2024/2847 (Cyber Resilience Act)** — essential requirements set out in Annex I.

**6. References to relevant standards or specifications**

- [HARMONISED STANDARDS USED, once published — likely candidates: EN 18031 series, ETSI EN 303 645 for IoT-relevant deployments]
- Or, where harmonised standards are not yet available: a description of the technical specifications applied (see Annex VII technical documentation).

**7. Conformity assessment procedure**

Annex VIII **Module A — internal control** (see [`conformity-assessment-route.md`](conformity-assessment-route.md)).
No Notified Body involvement required for default-class products.

**8. Additional information**

- Software bill of materials: see corresponding `wolfssl-<version>.cdx.json` and `.spdx.json` (released alongside the binary).
- Vulnerability handling process: [`vulnerability-handling-process.md`](vulnerability-handling-process.md) and [https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt).
- Support period: [`support-period-policy.md`](support-period-policy.md).

**9. Signature**

- Place: [LOCATION OF ISSUE]
- Date: [DATE OF ISSUE]
- Name and function: [SIGNATORY NAME, e.g. Larry Stefonic, CEO, wolfSSL Inc.]
- Signature: ___________________

---

## Notes for customers adapting this template

1. Fields in `[BRACKETS]` must be filled before signature.
2. The declaration must be drawn up in **at least one of the official languages** of the Member State where the product is placed on the market. English is generally accepted but verify with your EU Authorised Representative.
3. The signed declaration is part of the **technical documentation** (Annex VII) and must be retained for **10 years**.
4. The declaration accompanies the product. For software products, this typically means including it in the release tarball, in a `LEGAL/` directory, or alongside the SBOMs.
