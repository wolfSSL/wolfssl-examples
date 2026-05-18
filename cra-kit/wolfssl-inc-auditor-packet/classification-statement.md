# Classification statement — wolfSSL libraries (Annex III / IV)

**Status:** ✅ Decided & published
**CRA reference:** Annex III, Annex IV; Art. 6 (classes of products with digital elements)

## Decision

wolfSSL Inc. classifies the following products as **default class** ("Class I")
for CRA purposes:

| Product | Classification | Rationale |
|---------|----------------|-----------|
| **wolfSSL** (TLS library) | **Default class** (not Annex III, not Annex IV) | A general-purpose TLS / cryptographic library is not a finished product type listed in Annex III or Annex IV. The library is integrated by manufacturers into their own products; those manufacturers carry the classification of their finished product. |
| **wolfCrypt** (cryptographic library) | **Default class** | Same reasoning. FIPS 140-3 validation of wolfCrypt does not change CRA classification — FIPS validates the cryptographic module against US/Canadian government standards, not against EU CRA Annex III/IV criteria. |
| **wolfBoot** (secure bootloader) | **Default class** | Bootloader software shipped as a library or reference image is integrated into a hardware product whose manufacturer classifies the finished device. |
| **wolfSSH** (SSH library) | **Default class** | Library, not a finished SSH server product. |
| **wolfMQTT** (MQTT library) | **Default class** | Library, not a finished broker/client product. |

## Reasoning

Annex III and Annex IV list **finished product categories** (password managers,
network management systems, browsers, hardware security modules, smart meters
of certain types, etc.). wolfSSL Inc. does not ship any such finished product
on the EU market. Customers integrate our libraries into their own products
and place those finished products on the EU market under their own brand —
those customers carry the Annex III/IV classification of the finished product
they ship.

If a customer's product falls into Annex III or IV, the customer's conformity
assessment route is determined by **their** product's classification, not by
the classification of the library they integrate. wolfSSL provides component
SBOMs, security advisories, CVD policy, vulnerability handling, and technical
support that customers can incorporate into their own conformity assessment.

## Counter-example

Were wolfSSL Inc. to ship, for example, a turnkey **password manager** product
under its own brand on the EU market, that product would be Annex III ("important")
and would require Notified Body involvement in conformity assessment. We do not
ship such a product.

## What this means for customers

If you ship a product on the EU market that contains wolfSSL, classify your
**finished product** under Annex III/IV — not the wolfSSL library inside it.
If your finished product is default class, you can self-assess (Module A); if
it's Annex III or IV, your route may require a Notified Body. wolfSSL's
classification doesn't determine yours.

## References

- [`../CRA-Compliance-Shortlist.md`](../CRA-Compliance-Shortlist.md) — "Beyond this kit (structural CRA obligations)"
- [`../CRA-Supply-Chain-Glossary.md`](../CRA-Supply-Chain-Glossary.md) — Annex III, Annex IV, Notified Body definitions
- CRA text Annex III: list of important products
- CRA text Annex IV: list of critical products
