# wolfSSL CRA Kit

Example project and scripts for teams that ship products containing wolfSSL and
need **EU Cyber Resilience Act (CRA)**-style **software transparency** artifacts.

**This kit does not make your product “CRA compliant.”** It shows how to obtain
and nest **wolfSSL component evidence** inside **your** product SBOM and auditor
packet.

**Not legal advice.** Map obligations to your product class and role with counsel.

**wolfSSL's own CRA posture.** wolfSSL Inc. is itself a **manufacturer** under
the CRA for libraries it places on the EU market. We publish our own
[`security.txt`](https://www.wolfssl.com/.well-known/security.txt) and
[CVD policy](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt),
and the manufacturer-side filings we ship under CRA — Annex III/IV
classification, conformity assessment route, declaration of conformity
template, EU Authorised Representative status, support-period policy,
and vulnerability-handling process — are in
[`wolfssl-inc-auditor-packet/`](wolfssl-inc-auditor-packet/). Use them as
reference templates for **your** product.

| Document | Use |
|----------|-----|
| [`CRA-Compliance-Shortlist.md`](CRA-Compliance-Shortlist.md) | Four pillars towards CRA (components, secure boot, data in transfer, vulnerability handling) |
| [`CRA-Cheat-Sheet.md`](CRA-Cheat-Sheet.md) | **Who provides what** — you vs wolfSSL (print/PDF) |
| [`CRA-Supply-Chain-Glossary.md`](CRA-Supply-Chain-Glossary.md) | Full terminology (**self-contained in this kit**) |
| [`SKILL.md`](SKILL.md) | **AI playbook** — agent checklist, scripts, Cursor install |
| [`ROADMAP.md`](ROADMAP.md) | SBOM / CBOM / VEX / bomsh / CSAF — today vs roadmap |
| [`auditor-packet/`](auditor-packet/) | **Customer-side worked example** — fictional Acme Connect Gateway + wolfSSL SBOM samples |
| [`wolfssl-inc-auditor-packet/`](wolfssl-inc-auditor-packet/) | **Manufacturer-side filings** — what wolfSSL Inc. itself ships under CRA |

**Self-contained:** all customer-facing docs live in this directory. You only need a
separate **wolfSSL source tree** (with SBOM support) to **regenerate** component SBOMs.

---

## Prerequisites

- **wolfSSL** source with SBOM support (see [wolfSSL SBOM feature (upstream)](#wolfssl-sbom-feature-upstream) below).
  Typical layout:

  ```
  wolf/
  ├── wolfssl/              ← WOLFSSL_DIR (default: ../../wolfssl from here)
  └── wolfssl-examples/
      └── cra-kit/          ← you are here
  ```

- **Python 3** for `scripts/gen-sbom` (embedded path) and `scripts/validate.sh`.
- **`pcpp`** (optional for embedded): install on the **same** interpreter as `python3`:
  `python3 -m pip install pcpp`. If `pip install pcpp` used conda but your shell runs
  `/usr/local/bin/python3`, use `CRA_PYTHON=python` or rely on the script's automatic
  **compiler `-dM -E` fallback** (no pcpp required).
- **Cross-compile note for embedded** (`-dM -E` fallback only): the script defaults to
  host `cc`. For target-accurate macros set `CC=arm-none-eabi-gcc` (or your toolchain)
  before running so the SBOM reflects target `__ARM_ARCH`, `__SIZEOF_LONG__`, etc.
  rather than your laptop's. Skip this if you have `pcpp` installed.
- **Optional schema validators** (used by `validate.sh` if installed):
  - [`cyclonedx-cli`](https://github.com/CycloneDX/cyclonedx-cli/releases) for CycloneDX 1.6 schema validation
  - [`pyspdxtools`](https://pypi.org/project/spdx-tools/) (`pip install spdx-tools`) for SPDX 2.3 schema validation

---

## All the “BOMs” (today vs roadmap)

| Name | What it lists | Who owns it | wolfSSL today | Roadmap |
|------|----------------|-------------|---------------|---------|
| **Product SBOM** | Entire shipped product | **You** | — | — |
| **Component SBOM** | wolfSSL only | **wolfSSL** (you integrate) | **Yes** — SPDX 2.3 + CycloneDX 1.6 | Ongoing |
| **VEX** | Does CVE X apply to our build? | **You** | [Advisories](https://www.wolfssl.com/docs/security-vulnerabilities/) (VEX inputs) | Templates / automation |
| **CBOM** | Crypto algorithms / modules | **You**; we **signal** | **Partial** — `wolfssl:build:*` in CycloneDX | Formal `cryptographic-asset` |
| **OmniBOR / bomsh** | How the library binary was built | **wolfSSL** (optional) | **Yes** — Linux **host** only | Same |

Details: [`ROADMAP.md`](ROADMAP.md).

**Plain summary:** SBOM = what’s inside. Crypto build properties = what crypto you
compiled in (CBOM direction). bomsh = how the library was built (optional). Product
SBOM = your job.

---

## Which path are you?

| Profile | Build | Generate wolfSSL SBOM |
|---------|-------|------------------------|
| **A. Linux / server / Yocto / package** | `./configure && make` | `make sbom` in wolfSSL tree |
| **B. Embedded / RTOS / IDE** | `user_settings.h` + your Makefile / Keil / Zephyr / ESP-IDF | `./scripts/generate-embedded-sbom.sh` (kit demo) or upstream `gen-sbom` |
| **C. Commercial license** | Either | `CRA_LICENSE_OVERRIDE=LicenseRef-wolfSSL-Commercial CRA_LICENSE_TEXT=/path/to/commercial-license.txt ./scripts/generate-wolfssl-sbom.sh` |

> **Commercial (`LicenseRef-*`) overrides require `CRA_LICENSE_TEXT`** pointing at
> the plain-text licence you received from wolfSSL. SPDX 2.3 §10.1 requires the
> licence text to be embedded for any `LicenseRef-*`; both `gen-sbom` and
> `make sbom` hard-fail without it. A stock SPDX id (e.g. `Apache-2.0`) needs no
> text. If you don't have the text file handy, use
> [`scripts/make-commercial-sample.sh`](scripts/make-commercial-sample.sh) to
> derive a commercial sample from the pinned GPL samples instead.

**Every manufacturer still:**

1. Maintains a **product SBOM** (all components).
2. **References or copies** wolfSSL’s `.spdx.json` / `.cdx.json` into it.
3. **Regenerates** wolfSSL SBOM when `user_settings.h` or your source list changes.
4. Owns **vulnerability handling** (process + owner).
5. Uses **bomsh** only if an auditor or contract requires build proof — on a **Linux** host.

---

## Quick start

### 1. Validate the bundled sample (no wolfSSL build required)

```sh
cd wolfssl-examples/cra-kit
./scripts/validate.sh
```

### 2. Regenerate component SBOMs (requires wolfSSL with `make sbom`)

```sh
export WOLFSSL_DIR=../../wolfssl
./scripts/refresh-samples.sh    # make sbom + auto-fix product SPDX checksum
```

Or without updating the product stub checksum:

```sh
./scripts/generate-wolfssl-sbom.sh          # default: autotools if Makefile exists
CRA_SBOM_MODE=embedded ./scripts/generate-wolfssl-sbom.sh   # rarely used for packet/
./scripts/generate-embedded-sbom.sh         # writes wolfssl-component-embedded/

CRA_LICENSE_OVERRIDE=LicenseRef-wolfSSL-Commercial \
    CRA_LICENSE_TEXT=/path/to/wolfssl-commercial-license.txt \
    ./scripts/generate-wolfssl-sbom.sh      # commercial-license sample (text required)
./scripts/make-commercial-sample.sh         # derive from pinned GPL samples (no rebuild)
```

**Pinned samples** in `auditor-packet/wolfssl-component/` are from **`make sbom`**
(autotools), with a sibling `*.commercial.{cdx,spdx}.json` showing the override pattern.
Embedded regen produces a **different** SBOM (watermarked `wolfssl:sbom:demo=true`) —
see [`auditor-packet/wolfssl-component/SAMPLE-PROVENANCE.md`](auditor-packet/wolfssl-component/SAMPLE-PROVENANCE.md).

### 3. Study the sample product packet

Open [`auditor-packet/00-INDEX.md`](auditor-packet/00-INDEX.md) — fictional **Acme
Connect Gateway** shows CycloneDX `bom` external reference and SPDX
`externalDocumentRefs` pointing at wolfSSL’s files.

### 4. Integrate into your real product SBOM

Copy the pattern from `product-acme-connect-gateway.*` in [`auditor-packet/`](auditor-packet/) — both
SPDX `externalDocumentRefs` and CycloneDX `bom` external references are shown
end-to-end. For the upstream technical reference on `make sbom` flags, output
formats, and `SBOM_LICENSE_OVERRIDE` for commercial licensees, see
[`wolfssl/doc/SBOM.md`](https://github.com/wolfSSL/wolfssl/blob/master/doc/SBOM.md).

---

## `make bomsh` — Linux host only (simple explanation)

`make bomsh` is **optional** for most CRA transparency needs. Use it when someone
asks: *“Prove this `libwolfssl.so` was built from these exact sources.”*

**Why only Linux?** Bomsh runs **bomtrace3** — a patched **strace** that watches
every compiler call during a **full rebuild**. That program is built and tested on
**Linux build machines** (normal `ptrace`, no kernel patches).

| Your situation | What to do |
|----------------|------------|
| Build on **Linux** | `make bomsh` after `make sbom` in wolfSSL |
| Build on **macOS / Windows** | Run bomsh in **Linux CI**, **WSL2**, or a **container** |
| Ship firmware to **MCU / RTOS** | **Target OS does not matter** — tracing runs on the **build host** |
| **Embedded**, no Linux in house | Use **`gen-sbom`** for SBOM on any OS; skip bomsh unless required |

The sample packet does **not** ship `omnibor/` (large). See
[`auditor-packet/wolfssl-component/README-bomsh.md`](auditor-packet/wolfssl-component/README-bomsh.md).

Full detail: [wolfssl/doc/SBOM.md §3](https://github.com/wolfSSL/wolfssl/blob/master/doc/SBOM.md).

---

## wolfSSL SBOM feature (upstream)

SBOM and optional bomsh provenance are developed in the main **wolfSSL** repository:

| Item | Location |
|------|----------|
| Generator | `wolfssl/scripts/gen-sbom` |
| Autotools | `make sbom`, `make bomsh` |
| CI | `wolfssl/.github/workflows/sbom.yml` |
| Reference (flags, formats, OmniBOR) | [doc/SBOM.md](https://github.com/wolfSSL/wolfssl/blob/master/doc/SBOM.md) |
| Customer-facing CRA narrative, glossary, auditor packet, AI playbook | this kit (you are here) |

Use a wolfSSL tree where the `make sbom` (and optionally `make bomsh`) targets are
available before running the scripts here. Once these targets land on `master`, any
recent wolfSSL checkout works; until then, use the integration branch / PR.

Pinned sample version: see [`VERSION`](VERSION) (default **5.9.1**).

---

## Embedded demo settings

[`user_settings.h`](user_settings.h) in this directory is included when
`WOLFSSL_USER_SETTINGS` is defined for `./scripts/generate-embedded-sbom.sh`.
Production SBOMs must use **your** project's `user_settings.h` and **your** full
`--srcs` list (every wolfSSL `.c` you compile).

See **[SRCS-FILE-HOWTO.md](SRCS-FILE-HOWTO.md)** for instructions on extracting
your wolfSSL source list from common embedded build systems (Makefile, CMake,
Zephyr, ESP-IDF, Keil, IAR) and passing it via `CRA_SBOM_SRCS_FILE`.

---

## Presentation

15-minute co-sponsor slide track: [`presentations/SLIDE-OUTLINE.md`](presentations/SLIDE-OUTLINE.md).

Handouts: [`CRA-Cheat-Sheet.md`](CRA-Cheat-Sheet.md) + [`CRA-Supply-Chain-Glossary.md`](CRA-Supply-Chain-Glossary.md);
point AI users at [`SKILL.md`](SKILL.md) (copy to `.cursor/skills/wolfssl-cra-kit/`).

---

## Agent skill

[`SKILL.md`](SKILL.md) is a customer deliverable (not internal-only) — see
[`presentations/SLIDE-OUTLINE.md`](presentations/SLIDE-OUTLINE.md). Copy to
`.cursor/skills/wolfssl-cra-kit/` for Cursor.

---

## FAQ

**Do we need our own SBOM?**  
Yes — for the whole product you place on the EU market.

**Is wolfSSL’s SBOM enough alone?**  
No — nest or reference it in your product SBOM (see `auditor-packet/`).

**SPDX or CycloneDX?**  
wolfSSL ships both; use what your tools expect.

**Do we need bomsh for CRA?**  
Usually no. SBOM alone covers most transparency asks.

**What about CBOM?**  
Many RFQs ask for crypto inventory. Today: `wolfssl:build:*` properties in
CycloneDX from your real config. Formal CycloneDX CBOM: **roadmap** — see
[`ROADMAP.md`](ROADMAP.md).

**FIPS builds?**  
The SBOM generator does not change validated module code; your FIPS boundary
documentation remains separate.

**What does this kit NOT cover?**  
Software transparency only. **Structural** CRA obligations are out of scope:
appointing an EU Authorised Representative (Art. 18), product classification
(Annex III/IV), conformity assessment + CE marking, full technical
documentation per Annex VII, the support-period commitment, and importer /
distributor obligations. See [`CRA-Compliance-Shortlist.md`](CRA-Compliance-Shortlist.md)
"Beyond this kit" for the list. Engage CRA counsel or consultant — these are
legal/structural decisions, not artefacts.

**Are we outside the EU? (US / Asia / etc.)**  
Then you almost certainly need an **EU Authorised Representative** (Art. 18)
appointed in writing **before** placing your product on the EU market. Either
contract a third-party AR service or use an existing EU subsidiary. This is a
long-lead item — start now, do not wait for September 2026.

---

## Further reading

### OpenSSF guidance

- [CRA Brief Guide for OSS Developers](https://best.openssf.org/CRA-Brief-Guide-for-OSS-Developers.html)
  — When the CRA applies to open source projects and what obligations fall on
  manufacturers integrating OSS components into commercial products.
- [SBOM in Compliance](https://sbom-catalog.openssf.org/sbom-compliance.html)
  — OpenSSF SBOM Everywhere SIG survey of the global regulatory landscape:
  CRA, NTIA minimum elements, US EO 14028, Germany TR-03183, others.
- [Getting Started with SBOMs](https://sbom-catalog.openssf.org/getting-started)
  — OpenSSF guidance on SBOM generation approaches (build-integrated vs.
  separate tooling), phase selection, publication. wolfSSL's `make sbom`
  follows the build-integrated approach.
- [OpenSSF CRA Policy Hub](https://openssf.org/category/policy/cra/)
  — Ongoing OpenSSF coverage of CRA developments and community responses.
- [SBOM Everywhere Wiki](https://sbom-catalog.openssf.org/) — tooling
  catalog, working group resources, naming conventions, cross-format
  guidance for SPDX and CycloneDX.

### Standards

- SPDX 2.3 specification: <https://spdx.github.io/spdx-spec/v2.3/>
- CycloneDX 1.6 specification: <https://cyclonedx.org/specification/overview/>
- NTIA minimum elements for an SBOM:
  <https://www.ntia.gov/report/2021/minimum-elements-software-bill-materials-sbom>
- RFC 9116 (`security.txt`): <https://www.rfc-editor.org/rfc/rfc9116>

---

## Support

Questions about this kit: **support@wolfssl.com**

Security reports: see [`security.txt`](https://www.wolfssl.com/.well-known/security.txt)
and our [Coordinated Vulnerability Disclosure policy](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt).
Do **not** send vulnerability details to `support@` — use the security contact
listed in `security.txt`.
