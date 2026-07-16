# CE marking — wolfSSL libraries

**Status:** 🟡 Will affix from first CRA-applicable release after 11 Dec 2027
**CRA reference:** Art. 30 (rules and conditions for affixing the CE marking)

## Decision

wolfSSL Inc. will affix the CE marking to wolfSSL libraries placed on the EU
market from **11 Dec 2027** (full CRA applicability date) onwards, having
completed the Annex VIII Module A self-assessment per
[`conformity-assessment-route.md`](conformity-assessment-route.md).

## How CE marking is affixed for software products

CRA Art. 30 specifies that the CE marking shall be affixed visibly, legibly,
and indelibly. For software products that lack a physical surface, the
established practice (per the Blue Guide on the implementation of EU product
rules) is to affix the marking:

1. **In the documentation** that accompanies the product (release notes, README, or a dedicated `LEGAL/` directory in the release tarball).
2. **On the website** where the product is downloaded or distributed (`wolfssl.com` product page).
3. **In a machine-readable form**, where applicable (e.g. as a property in the SBOM).

wolfSSL will use all three locations.

## What CE marking represents

The CE marking is the manufacturer's declaration that:

- The product conforms to all applicable Union harmonisation legislation (here, the CRA and any other EU acts that apply, e.g. RED if shipped as part of radio equipment).
- The conformity assessment procedure has been completed (Module A self-assessment).
- A declaration of conformity (Art. 28) has been drawn up and signed.
- Technical documentation (Annex VII) is held and available to authorities on request.

It is **not** a quality mark, a certification, or a guarantee. It is a
manufacturer's self-declaration of regulatory conformity.

## Where the CE mark will appear in wolfSSL releases

- `LEGAL/CE-marking.txt` — text statement plus the CE logo (PDF) in the release tarball
- `wolfssl-<version>.cdx.json` — `metadata.properties[].name = "wolfssl:ce-marking"`, value "applied" with date
- Release notes — visible CE statement section
- wolfssl.com release page — CE marking image alongside download link

## What this means for customers

If you ship a finished product into the EU containing wolfSSL, you affix CE
marking to **your finished product**, not to the wolfSSL component. Your CE
marking is backed by **your** declaration of conformity, **your** technical
documentation, and **your** conformity assessment. wolfSSL's component-level
CE marking does not transfer to your product.

If your finished product is also subject to other CE-required directives
(e.g. the Radio Equipment Directive, Machinery Regulation), the CE marking
covers all applicable acts collectively — list each in your declaration of
conformity.

## References

- CRA Art. 30 (CE marking)
- CRA Art. 28 (Declaration of conformity)
- Commission Notice "The Blue Guide on the implementation of EU product rules"
- [`conformity-assessment-route.md`](conformity-assessment-route.md)
- [`declaration-of-conformity.template.md`](declaration-of-conformity.template.md)
