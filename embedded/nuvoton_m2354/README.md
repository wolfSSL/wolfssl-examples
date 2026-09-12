# wolfCrypt on the NuMaker-M2354

Builds `wolfcrypt_test` followed by the wolfCrypt benchmark for the
[NuMaker-M2354](https://www.nuvoton.com/board/numaker-m2354/) (NuMicro
M2354KJFAE, Cortex-M23, 1 MB flash, 256 KB SRAM), with the CRPT accelerator,
the TRNG and the Key Store driven through the crypto callback port in
`wolfcrypt/src/port/nuvoton` in the wolfSSL repository, whose README
covers what each engine does and what it declines.

## What you need

- `arm-none-eabi-gcc` (any recent release; developed against 13.2)
- A wolfSSL checkout carrying the Nuvoton port
- The Nuvoton BSP:

```sh
git clone --depth 1 https://github.com/OpenNuvoton/M2354BSP
```

Nothing from the BSP is copied into this tree; the build reads it where you
cloned it.

## Build

```sh
./build.sh --bsp /path/to/M2354BSP --wolfssl /path/to/wolfssl
```

wolfSSL defaults to `../../../wolfssl` and can also come from `WOLFSSL_ROOT`,
so the `--wolfssl` argument is unnecessary if you keep the two checkouts
side by side.

That produces `build/wolfcrypt-m2354.elf` and `build/wolfcrypt-m2354.bin` for
the secure world, which is also where a plain non-TrustZone application runs.

For the TrustZone split:

```sh
./build.sh --bsp /path/to/M2354BSP --tz
```

This compiles both halves - the port as the non-secure image sees it, and
`secure/nuvoton_nsc.c` with `-mcmse` - but does not link. A working TrustZone
application also needs a partition layout and a secure bootstrap, and those
belong to your project rather than to wolfSSL. `--tz` does show that the port
and the veneers agree on every signature, which is the part that is easy to get
wrong.

## Flash and run

`run_test.sh` does the whole cycle - find the probe, build, take the shared-bench lock, hand the console to the flasher, program, give the console back and capture:

```sh
./run_test.sh --bsp /path/to/M2354BSP --seconds 180
```

It takes a lock at `/tmp/claude-bench-locks/numaker-m2354.lock` so two people
cannot drive one board at once, and releases it even if the run fails.

It flashes with `pyocd`, which ships a **builtin** target for this exact part (`m2354kjfae`), so no CMSIS pack download is needed. That requires the Nu-Link2-Me to be presenting as a CMSIS-DAP probe; see the firmware note below.

Doing it by hand instead:

The board carries a detachable Nu-Link2-Me. Either of these works:

- **NuLink command line** (Nuvoton's tool, Windows and Linux):
  `NuLink -w APROM build/wolfcrypt-m2354.bin`
- **OpenOCD**, using Nuvoton's fork
  ([OpenNuvoton/OpenOCD-Nuvoton](https://github.com/OpenNuvoton/OpenOCD-Nuvoton))
  with `interface/nulink.cfg`. Nuvoton also publishes a `NuLink2_DAPLink.bin`
  adapter firmware that makes the probe a CMSIS-DAP device, after which
  `pyocd flash` works.

Console output goes to UART0 at **115200 8N1**, which the Nu-Link2-Me presents
as a virtual COM port.

## Expected output

```
wolfCrypt on NuMaker-M2354 (96 MHz)
TrustZone: secure world, direct BSP calls
Nuvoton crypto callback device registered as devId 820

--- wolfcrypt_test ---
...
--- benchmark ---
...
```

## Files

| File | What it is |
|---|---|
| `user_settings.h` | wolfCrypt configuration; a starting point for your own |
| `app.c` | Clocks, UART console, the CRPT interrupt handler, and `main()` |
| `secure/nuvoton_nsc.c` | Non-secure callable veneers for a TrustZone build |
| `build.sh` | The build described above |
| `run_test.sh` | Flash and capture on a real board, taking the bench lock |

`app.c` is worth reading for two things a project of your own also needs: the
`CRPT_IRQHandler` that routes the crypto interrupt to `ECC_DriverISR()`,
without which every ECC and RSA call times out, and the `current_time()` and
`LowResTimer()` the benchmark asks for.

## Status

`wolfcrypt_test` runs to completion on a NuMaker-M2354 with no failures, the
Key Store round trip in `keystore_test.c` passes, and the benchmark runs.

`WOLFSSL_KEY_GEN` is left off in `user_settings.h`, behind an `#if 0` that
explains why: it enables RSA key generation and a `wc_DhGenerateParams()` call
for a fresh 2048-bit parameter set, both software primality searches the
accelerator has no part in, and the DH one runs for hours here. A capture that
stops after the RSA test with no further output is usually that, not a fault. The port README in wolfSSL carries the throughput
table.

**`build.sh --tz` is a compile check**, not a runnable image: it builds both
halves so a change cannot break either, but it does not link them, because a
working TrustZone application also needs a partition layout and a secure
bootstrap that belong to your project rather than to this example.

The veneers in `secure/nuvoton_nsc.c` have been run on hardware separately, in
a two-image build against a part whose NSCBA is `0x00080000`. Every algorithm
in the port reached the accelerator from the non-secure world through them,
including ECC, which depends on the CRPT interrupt being taken in the secure
world. The wolfSSL port README records that result, and one behaviour in a
combined `wolfcrypt_test()` run that is not yet understood.

# Support

For questions please email support@wolfssl.com
