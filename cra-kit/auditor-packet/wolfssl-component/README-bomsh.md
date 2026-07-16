# Optional: OmniBOR / bomsh bundle

`make bomsh` is **not** included in this sample packet. Most CRA transparency
workflows need the SBOM files only.

When an auditor or contract requires **build provenance**:

1. On a **Linux** build host (or Linux CI / WSL2 / container), in your wolfSSL tree:
   ```sh
   ./configure && make sbom && make bomsh
   ```
2. Add to your release bundle:
   - `omnibor/` directory (Merkle DAG of build inputs/outputs)
   - `omnibor.wolfssl-<version>.spdx.json` (file-level provenance)

**Sample shape:** see [`omnibor.wolfssl-5.9.1.spdx.json.sample`](omnibor.wolfssl-5.9.1.spdx.json.sample) — a
truncated illustrative document (3 source files instead of every wolfSSL `.c`,
placeholder gitoids instead of real ones) so customers know what shape `make bomsh`
produces before they run it.

**Why Linux only?** `bomsh` uses `bomtrace3`, a patched `strace` that records
compiler invocations during a full rebuild. That tooling is built and supported
on Linux hosts. The **target** of your firmware (MCU, RTOS, etc.) does not need
to run Linux — only the machine **tracing the build** does.

Details: [wolfssl/doc/SBOM.md §3](https://github.com/wolfSSL/wolfssl/blob/master/doc/SBOM.md)
