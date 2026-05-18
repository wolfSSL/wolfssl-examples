# Embedded component SBOM (optional sample)

This directory's `wolfssl-*.{cdx,spdx}.json` outputs are **gitignored** — generate
them locally with the embedded path. Only this README is committed.

```sh
export WOLFSSL_DIR=../../wolfssl   # wolfSSL tree with scripts/gen-sbom
python3 -m pip install pcpp        # same python3 as in your PATH (see README)
./scripts/generate-embedded-sbom.sh
```

If pcpp is not on your `python3`, the script falls back to `cc -dM -E` and `--options-h`
(no extra install). For cross builds, set `CC=arm-none-eabi-gcc` (or your target
compiler) so the fallback reflects target macros, not the host's.

Uses [`../../user_settings.h`](../../user_settings.h) via `WOLFSSL_USER_SETTINGS` and a
**demo** `--srcs` list (see `scripts/generate-wolfssl-sbom.sh`). Production firmware
must pass **your** `user_settings.h` and **every** wolfSSL `.c` file you compile.
Embedded outputs are watermarked `wolfssl:sbom:demo=true` so an auditor can tell at
a glance that they came from the kit's demo `--srcs` list and not a real build.

Outputs differ from [`../wolfssl-component/`](../wolfssl-component/) (autotools /
`make sbom`). Compare `wolfssl:sbom:hash-kind` in the CycloneDX files.
