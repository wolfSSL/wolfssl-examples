# Auditor packet index (fictional Acme Connect Gateway)

Example of what a **manufacturer** might bundle alongside wolfSSL component
artefacts. **Not legal advice** — adapt to your product and counsel.

| File | Role |
|------|------|
| `product-acme-connect-gateway.cdx.json` | **Your** product SBOM (CycloneDX) — references wolfSSL |
| `product-acme-connect-gateway.spdx.json` | **Your** product SBOM (SPDX) — `externalDocumentRefs` to wolfSSL |
| `wolfssl-component/wolfssl-5.9.1.cdx.json` | wolfSSL component SBOM — **autotools / make sbom** sample (GPL) |
| `wolfssl-component/wolfssl-5.9.1.spdx.json` | wolfSSL component SBOM (SPDX, GPL) |
| `wolfssl-component/wolfssl-5.9.1.commercial.cdx.json` | wolfSSL component SBOM with commercial license override |
| `wolfssl-component/wolfssl-5.9.1.commercial.spdx.json` | wolfSSL component SBOM (SPDX) with commercial license override |
| `wolfssl-component/wolfssl-5.9.1.cbom-draft.cdx.json` | Hand-rolled cryptographic-asset draft (CycloneDX 1.6 CBOM profile) |
| `wolfssl-component/SAMPLE-PROVENANCE.md` | How the pinned autotools samples were produced |
| `wolfssl-component/omnibor.wolfssl-5.9.1.spdx.json.sample` | Truncated OmniBOR / bomsh provenance sample |
| `wolfssl-component-embedded/` | Optional embedded `gen-sbom` output (generated locally; gitignored) |
| `wolfssl-component/README-bomsh.md` | Optional OmniBOR — not included by default |

Also provide: your vulnerability process, release notes, and the upstream
wolfSSL disclosure context — [`security.txt`](https://www.wolfssl.com/.well-known/security.txt),
[CVD policy](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt),
and [advisories](https://www.wolfssl.com/docs/security-vulnerabilities/).

**This packet shows the software-transparency artefacts only.** A complete
CRA conformity packet for a real product also includes:

- Declaration of conformity (Art. 28)
- Technical documentation per Annex VII (risk assessment, design info, support-period commitment, vulnerability handling process)
- Proof of conformity assessment (self-declared per Art. 32 Module A, or Notified Body certificate per product class)
- Identity of the EU Authorised Representative (Art. 18) if the manufacturer is established outside the EU
- CE marking declaration

See [`../CRA-Compliance-Shortlist.md`](../CRA-Compliance-Shortlist.md)
"Beyond this kit" for the structural obligations not covered by SBOMs.

**Regenerate autotools samples + product checksum:** `./scripts/refresh-samples.sh`
