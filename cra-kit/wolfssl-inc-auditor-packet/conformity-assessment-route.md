# Conformity assessment route — wolfSSL libraries

**Status:** ✅ Decided & published (route only; per-release execution begins 11 Dec 2027)
**CRA reference:** Art. 32, Annex VIII

## Decision

wolfSSL Inc. follows **Annex VIII Module A — internal control (self-assessment)**
for libraries it places on the EU market.

## Why Module A

Module A is the appropriate route when:

- The product is **default class** under Annex III/IV (see [`classification-statement.md`](classification-statement.md)).
- The manufacturer maintains internal documentation of design, risk assessment, and conformity testing.
- No Notified Body involvement is required.

All three apply to wolfSSL libraries.

## What Module A requires

Module A obligates wolfSSL Inc. to:

1. **Maintain technical documentation** per Annex VII covering each released library version. See [`technical-documentation-outline.md`](technical-documentation-outline.md).
2. **Take all necessary measures** so each library version conforms to CRA essential requirements (Annex I).
3. **Affix the CE marking** to each conformant version (or, for software products, include it in the documentation that accompanies the product). See [`ce-marking-statement.md`](ce-marking-statement.md).
4. **Draw up and sign a written declaration of conformity** (Art. 28). See [`declaration-of-conformity.template.md`](declaration-of-conformity.template.md).
5. **Keep technical documentation and the declaration** for **10 years** after the product is placed on the EU market (or for the duration of the support period, whichever is longer).

## Notified Body engagement — not used

Notified Body involvement is required when a product is classified as
**Annex III "important class II"** or **Annex IV "critical"**. wolfSSL libraries
are neither. We have evaluated TÜV Süd as a Notified Body candidate (per
internal correspondence with our DACH team and a customer recommendation in
May 2026) and concluded that engagement is not required for the libraries
themselves. Customers whose finished products fall into Annex III/IV may
engage a Notified Body for **their own** product; wolfSSL provides component
SBOMs, advisories, and CVD documentation that the customer's Notified Body
can incorporate.

## What this means for customers

If your finished product is default class, you follow Module A like we do.
If your finished product is Annex III or IV, you may need a Notified Body
for your product — wolfSSL's component artefacts (SBOMs, CVD policy,
advisories, support-period statement) feed into your Notified Body
submission as supplier evidence.

## References

- CRA Art. 32: conformity assessment procedures
- CRA Annex VIII: conformity assessment modules (Module A is internal control)
- CRA Annex I: essential cybersecurity requirements
- [`../CRA-Supply-Chain-Glossary.md`](../CRA-Supply-Chain-Glossary.md) — Module A, Conformity assessment, Notified Body
