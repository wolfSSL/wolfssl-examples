# ISO 15118 over TLS 1.3

The TLS layer of Plug & Charge: a charging station (SECC) and a vehicle (EVCC)
authenticating each other with a realistic V2G certificate hierarchy, plus the
ECDSA operation that an ISO 15118 XML signature wraps.

```sh
$ ./generate_v2g_certs.sh
$ make
$ make check
```

`contract-install` needs `--enable-x963kdf` and reports `SKIP` without it:

```sh
$ cd wolfssl && ./configure --enable-x963kdf && make && sudo make install
```

| File | |
| --- | --- |
| `generate_v2g_certs.sh` | builds the V2G hierarchy: three branches, four tiers each |
| `secc-server.c` | the charging station |
| `evcc-client.c` | the vehicle |
| `v2g-signature.c` | ECDSA-P256-SHA256 over an EXI fixture |
| `contract-install.c` | contract certificate installation: the key wrapping. Needs `--enable-x963kdf` |

This targets **ISO 15118-20**: TLS 1.3 with mutual authentication, which is what
replaced -2's unilateral TLS plus application-layer signature. There is no -2
variant here, deliberately — its mandated suite is `TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256`,
static ECDH, which needs `WOLFSSL_STATIC_DH` and is the direction the ecosystem
is leaving.

## The certificate hierarchy

```
V2G Root CA
  +-- CPO Sub-CA 1 -- CPO Sub-CA 2 -- SECC leaf          the charger
  +-- MO  Sub-CA 1 -- MO  Sub-CA 2 -- Contract leaf      the vehicle
  +-- OEM Sub-CA 1 -- OEM Sub-CA 2 -- Provisioning leaf  the vehicle, as built
```

Four tiers root to leaf on every branch, all chaining to the same V2G Root CA,
so each end verifies the other against **one** trust anchor. All secp256r1 with
SHA-256, as the standard requires.

The OEM provisioning branch is the vehicle's factory identity. It is not used
for TLS — its key does ECDH during contract certificate installation, which
is why that leaf carries `keyAgreement` rather than `clientAuth`.

Details worth noting because they are the sort of thing that is wrong in
hand-rolled test PKI:

* **Three certificates go on the wire, not four.** The root is a locally
  configured anchor and is never transmitted. `secc-chain.pem` and
  `contract-chain.pem` are leaf-first with the two sub-CAs and no root.
* **Sub-CAs are path-length limited.** Sub-CA 1 is `pathlen:1`, Sub-CA 2 is
  `pathlen:0`, so a Sub-CA 2 cannot mint another CA and quietly deepen the
  hierarchy.
* **Validity shortens down the tiers** — 10 years for the root, 5 and 3 for the
  sub-CAs, 60 days for the SECC leaf, 2 years for the contract certificate.
* **Key usage is per role**: `serverAuth` on the SECC leaf, `clientAuth` on the
  contract leaf, `keyCertSign`+`cRLSign` on the CAs.
* **The contract CN carries the e-mobility account identifier**
  (`DE-MO-C0123456789-3`), and the SECC leaf carries a `subjectAltName` the
  vehicle checks with `wolfSSL_check_domain_name()` — verifying the chain is
  valid is not the same as verifying you are talking to the station you meant
  to.

`openssl verify` runs over all three chains at the end of the script, so a
mistake in the hierarchy fails there rather than inside a handshake.

## Bounding the chain depth

ISO 15118 caps the hierarchy, and wolfSSL's knob for that is
`MAX_CHAIN_DEPTH`. It is a **library** macro — it lives in `src/internal.c`, so
putting it in this directory's `CFLAGS` does nothing. Set it when building
wolfSSL:

```sh
$ cd wolfssl
$ ./configure CPPFLAGS="-DMAX_CHAIN_DEPTH=4"
$ make && sudo make install
```

The check is `totalCerts >= MAX_CHAIN_DEPTH` as the peer's Certificate message
is parsed, so the value is the largest number of certificates a peer may send.
At 4 it comfortably admits the 3 these chains transmit, while refusing an
unbounded chain. Tighten it to 3 if you want to reject anything deeper than
this hierarchy exactly. The TLS examples were verified against a library built
with `-DMAX_CHAIN_DEPTH=4`; the default is 9.

## Cipher suites

Both ends pin the list to the two suites -20 names:

```c
wolfSSL_CTX_set_cipher_list(ctx,
        "TLS13-AES128-GCM-SHA256:TLS13-CHACHA20-POLY1305-SHA256");
```

Without that, wolfSSL negotiates whatever TLS 1.3 suite both ends prefer —
`TLS_AES_256_GCM_SHA384` here — which works fine and would fail a conformance
run. Restricting on both ends is what makes the result reproducible.

## The refusal path, and a TLS 1.3 surprise

```sh
$ ./evcc-client 15118 --no-contract
```

The SECC sets `WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT`, so a vehicle with no
contract certificate gets no session. But watch where it finds out:

```
  TLS TLSv1.3, TLS_AES_128_GCM_SHA256
  SECC chain verified to the V2G root, name matched
  -> SessionSetupReq: DE-MO-C0123456789-3
  no reply: received alert fatal error

evcc-client: REFUSED by the SECC after the handshake
```

`wolfSSL_connect()` **succeeded**. Under TLS 1.3 the client completes its half
of the handshake before the server has validated the client certificate, so the
refusal arrives as an alert on the first read. A TLS 1.2 EVCC would have failed
inside `wolfSSL_connect()`. If you are moving one to 1.3, the error handling has
to move with it.

## v2g-signature

Messages needing proof of origin — authorisation requests, metering receipts,
sales tariffs — carry an XML signature over the EXI encoding of the signed
element, using ECDSA secp256r1 with SHA-256 and the contract key. This example
is that operation and nothing else: digest, sign with the contract private key,
verify against the public key taken out of the contract certificate, and confirm
that a single flipped bit in the input breaks it.

The input is a **fixture** — a byte string standing in for an
already-canonicalised EXI fragment. It is not real EXI, and the example says so
in its output.

**What is missing and why.** wolfSSL has no EXI codec and should not grow one;
XML canonicalisation is an XML problem, not a crypto one; and which elements are
signed, inside what `SignedInfo` structure, is 15118's schema. Producing those
badly would be worse than not producing them. Everything from the digest onward
is real.

## contract-install

Where the vehicle's charging key comes from, and the reason Plug & Charge works
at all. The vehicle leaves the factory with an OEM provisioning certificate and
its private key. To get a contract it asks, and the backend must deliver a
contract certificate **and the matching private key** over a path it does not
control. So the private key is wrapped to the provisioning certificate's key:

```
backend   ephemeral EC key pair                        -> DHpublickey
          ECDH(ephemeral private, provisioning public) -> shared secret
          X9.63 KDF with SHA-256                       -> session key
          AES-128-CBC(session key, random IV)          -> encrypted key

vehicle   ECDH(provisioning private, DHpublickey)      -> same secret
          same KDF                                     -> same session key
          AES-128-CBC decrypt                          -> contract key
```

The `ContractSignatureEncryptedPrivateKey` is 48 bytes: a 16 byte IV followed by
the 32 byte secp256r1 scalar as exactly two AES blocks, so no padding is
involved.

```
== the vehicle installs the contract ==
      shared secret and session key derived
  contract private key unwrapped                     ok
  unwrapped scalar matches the contract key          ok
  recovered key signs, contract certificate verifies ok

== only this vehicle can do it ==
  wrong key recovers different bytes                 ok
  those bytes are NOT the contract key               ok
```

The third check is the one that matters. Comparing the recovered scalar against
the original only works because this example holds both; a real vehicle does
not. So it also **signs with the recovered key and verifies against the contract
certificate**, which is what proves the key it installed is the key the
certificate attests. The negative case repeats the entire unwrap with a second
provisioning key and confirms the result is not the contract key.

### Two things that will bite you

**`wc_ecc_shared_secret()` needs an RNG on the private key.** wolfSSL is built
with `ECC_TIMING_RESISTANT` by default (`--enable-harden`), so the scalar
multiplication is blinded, and blinding needs entropy. Without
`wc_ecc_set_rng(key, rng)` first you get `MISSING_RNG_E` (-236), which reads
like a bad key and is not. A key straight from `wc_ecc_make_key_ex()` does not
carry one either.

**The KDF's SharedInfo is spec-defined and this example picks the eMAID.** That
is the one input here I could not confirm against the standard. Get it wrong and
you derive a session key no real backend agrees with — the arithmetic all
works and nothing tells you. Check it against the revision you are implementing.

## Not implemented

* **SECC Discovery Protocol (SDP)** — the examples take a port on the command
  line instead. A real EVCC learns it over UDP multicast.
* **The V2G message state machine** — once the session is up, these exchange a
  short marker so you can see data flowing, and stop.

## Confirm against your revision

The suite names, the hierarchy shape and the certificate attributes here reflect
ISO 15118-20 as I understand it, but the standard is paywalled and revision
specific. Check the requirements against the document you are certifying
against before building on this. The wolfSSL side — TLS 1.3, secp256r1,
`MAX_CHAIN_DEPTH`, the cipher list strings, the ECDH and KDF calls — is
verified. The KDF SharedInfo, called out under `contract-install`, is the one
input I could not confirm.

These are test credentials generated on demand. Never ship them.
