# Sample provenance

Pinned files in this directory (`wolfssl-5.9.1.cdx.json`, `wolfssl-5.9.1.spdx.json`)
were produced with the **autotools** path:

```sh
cd "$WOLFSSL_DIR" && ./configure && make sbom
```

They reflect a **configured library build** — the full `wolfssl:build:*`
properties are captured from `options.h`.

The binary artifact digest (the library `PackageChecksum` / component hash and
the `libwolfssl.44.dylib` file entry) is intentionally an **all-zeros
sentinel**, not a captured hash. A compiled binary is not bit-reproducible
across toolchains, platforms, and timestamps, so a pinned sample must not ship
a concrete, real-looking binary hash that a customer might copy verbatim.
Running `./scripts/refresh-samples.sh` against your own build replaces the
sentinel with the real digest of the artifact you actually shipped.

They are **not** the same as the **embedded** demo under
[`../wolfssl-component-embedded/`](../wolfssl-component-embedded/), which uses
`user_settings.h` and a trimmed `--srcs` list (source-merkle checksum).

Regenerate autotools samples and fix the product stub checksum:

```sh
./scripts/refresh-samples.sh
```

## A note on timestamps

The sample SBOMs carry different `metadata.timestamp` / `created` values because
they were generated at different times, not in a single run:

- `wolfssl-component/` (autotools): `2026-05-12`
- `wolfssl-component-embedded/` (embedded demo): `2026-05-18`
- `product-acme-connect-gateway.*` (product stub): `2026-05-18`

This is expected for hand-pinned samples and does not affect validation
(`scripts/validate.sh` checks cross-document checksums, not timestamps).
Regenerating via `refresh-samples.sh` will update them to the current time.
