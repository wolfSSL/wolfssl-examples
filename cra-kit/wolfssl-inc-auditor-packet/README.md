# wolfSSL Inc. — manufacturer-side CRA filings

This directory shows what wolfSSL Inc. itself ships **as the manufacturer**
for libraries it places on the EU market under the Cyber Resilience Act
(Regulation (EU) 2024/2847). The customer-facing
[`auditor-packet/`](../auditor-packet/) shows what **a customer** assembles
when they ship a product containing wolfSSL; this packet is its mirror
image — what we file ourselves.

**Why this exists.** Earlier versions of the kit told customers to declare
themselves manufacturers, appoint EU Authorised Representatives, classify
their products under Annex III/IV, and run vulnerability-reporting rotations —
without showing what wolfSSL had done on any of those fronts. The kit's
audience reasonably read that as *"do as we say, not as we do."* This
directory closes that gap. Where a decision is made, it is stated.
Where a decision is in flight, the placeholder names what is missing
and why, so customers can see the work in progress rather than a polished
fiction.

**Status conventions used below:**

- ✅ **Decided & published** — wolfSSL Inc. has made and published this decision.
- 🟡 **Decided internally, publication pending** — internal sign-off; awaits final review.
- 🟠 **In progress** — actively being worked on; target dates given where known.
- ⏳ **Pending leadership decision** — the call has not yet been made.

**Not legal advice.** These artefacts are templates and statements of position;
they are not, and do not replace, the actual signed legal documents wolfSSL Inc.
files with EU regulators or its EU Authorised Representative.

---

## Contents

See [`00-INDEX.md`](00-INDEX.md) for the file list and CRA article mapping.

## Use as a template

Customers shipping their own products into the EU can copy the structure here,
fill in their own product details, and adapt the placeholders. Where wolfSSL
Inc.'s position is firm (e.g. Class I self-certification per Art. 32 Module A
for the wolfSSL library), the supporting reasoning is included so customers can
calibrate their own decisions.

## Customer-facing analogue

If you are looking for the customer-side worked example (a fictional product,
*Acme Connect Gateway*, that includes wolfSSL), see
[`../auditor-packet/`](../auditor-packet/).
