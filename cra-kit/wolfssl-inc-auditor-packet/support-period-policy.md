# Support-period policy — wolfSSL libraries

**Status:** ✅ Decided & published
**CRA reference:** Art. 13(2), Art. 13(8)

## Commitment

wolfSSL Inc. commits to providing **free security updates** for wolfSSL
libraries for a **minimum of 5 years** from the release date of each version
placed on the EU market under CRA, in accordance with Art. 13(2) and 13(8).

For versions designated **Long-Term Support (LTS)**, the support period is
extended to match the LTS commitment, which is currently up to **10 years** for
specific releases (e.g. those certified to FIPS 140-3 or covered by commercial
LTS contracts).

## Scope of "security update"

A security update under this policy is any release that:

- Addresses a vulnerability disclosed via wolfSSL's [Coordinated Vulnerability Disclosure policy](https://www.wolfssl.com/.well-known/vulnerability-disclosure-policy.txt) or assigned a CVE by wolfSSL as a CNA;
- Is published as a tagged GitHub release with accompanying SBOM (`*.cdx.json`, `*.spdx.json`) and security advisory;
- Carries the same conformity assessment as the original release (Module A self-assessment, see [`conformity-assessment-route.md`](conformity-assessment-route.md)).

Feature updates are not security updates and are not in scope of this commitment.

## Release line policy

| Release line | Support period | Notes |
|--------------|----------------|-------|
| Mainline releases | **5 years** from release date | Default per Art. 13(8) |
| LTS releases | **10 years** from release date | Designated explicitly at release time |
| FIPS 140-3-certified versions | Bound to FIPS certificate validity | May extend or shorten depending on NIST recertification |
| Commercial-license customers | Per commercial agreement | Often extends past CRA minimum; never less than CRA minimum |

## Where this is published

- This policy file (committed to [github.com/wolfSSL/wolfssl-examples](https://github.com/wolfSSL/wolfssl-examples)).
- Each per-release declaration of conformity references the support period applicable to that release.
- Customer-visible: [wolfSSL release notes](https://github.com/wolfSSL/wolfssl/releases) note the support window.

## What this means for customers

If you embed a wolfSSL release in your product:

- **Match or exceed** wolfSSL's support window in your own product's support-period commitment. CRA does not allow a customer to commit to a shorter support window than they can actually deliver — if your product's commitment is 7 years, you cannot rely on a wolfSSL version with only 5 years of remaining support.
- **Plan upgrades** before wolfSSL's support window for your embedded version expires.
- **Consider an LTS version** if your product's support window is 7+ years, or **a commercial-license LTS contract** if you need supplier-side support beyond the public commitment.

## References

- CRA Art. 13(2): support period at least 5 years, unless the product is expected to be in use for a shorter period (and longer where the expected lifetime is longer)
- CRA Art. 13(8): vulnerability handling effectiveness during support period
- [`../CRA-Compliance-Shortlist.md`](../CRA-Compliance-Shortlist.md) — pillar 4 + "Beyond this kit"
