# Sample provenance

Pinned files in this directory (`wolfssl-5.9.1.cdx.json`, `wolfssl-5.9.1.spdx.json`)
were produced with the **autotools** path:

```sh
cd "$WOLFSSL_DIR" && ./configure && make sbom
```

They reflect a **configured library build** (SHA-256 of `libwolfssl` and full
`wolfssl:build:*` properties from `options.h`).

They are **not** the same as the **embedded** demo under
[`../wolfssl-component-embedded/`](../wolfssl-component-embedded/), which uses
`user_settings.h` and a trimmed `--srcs` list (source-merkle checksum).

Regenerate autotools samples and fix the product stub checksum:

```sh
./scripts/refresh-samples.sh
```
