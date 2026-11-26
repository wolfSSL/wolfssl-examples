# wolfCrypt test and benchmark for the NXP LPC55S6x

A single application for the LPCXpresso55S69 that runs `wolfcrypt_test()` and
then `benchmark_test()` over the debug UART, configured for exactly the
algorithms the wolfSSL NXP LPC55S6x port accelerates in hardware.

## What is accelerated

`WOLFSSL_NXP_LPC55S6X` enables three blocks:

| Block     | Covers                                              |
| --------- | --------------------------------------------------- |
| RNG       | TRNG, used to seed the Hash-DRBG                    |
| HashCrypt | AES-128/192/256 ECB, CBC, CTR; SHA-1, SHA-256       |
| CASPER    | RSA public key operations (ECC: see below)           |

`user_settings.h` enables those and nothing else, so both the test and the
benchmark report only what this part actually accelerates. Everything the
LPC55S6x has no engine for -- Curve25519, ChaCha20-Poly1305, AES-GCM/CCM,
SHA-512, SHA-3, MD5, 3DES -- is left out.

RSA is the one split case. CASPER handles only the *public* half; the hook for
it in `wc_RsaFunctionSync()` (`rsa.c`) sits ahead of the SP path, so SP math
with Cortex-M assembly picks up the private key operations.

### ECC is absent because the port cannot reach CASPER's ECC

CASPER does accelerate ECC, and `casper_port.c` implements it -- `casper_ecc_mulmod()`
and `casper_ecc_mul2add()`, with P-256, P-384 and P-521 branches, driving the
SDK's `CASPER_ECC_SECP256R1_Mul()` and friends. Nothing calls them. `ecc.c` has
no reference to CASPER at all, and `WOLFSSL_NXP_CASPER_ECC_MULMOD` and
`WOLFSSL_NXP_CASPER_ECC_MUL2ADD` appear nowhere outside `casper_port.c` itself.

The history explains it: commit `3878271e3` added those functions and, in the
same change, commented out `WOLFSSL_NXP_CASPER_ECC_MUL2ADD` and
`WOLFSSL_SP_MULMOD` in `settings.h`. `WOLFSSL_SP_MULMOD` -- the SP-math hook
the accelerated path was meant to arrive through -- does not exist anywhere in
wolfSSL. `WOLFSSL_NXP_LPC55S6X` consequently defines only
`WOLFSSL_NXP_CASPER_RSA_PUB_EXPTMOD`.

So `HAVE_ECC` here would give software ECC, not accelerated ECC, which is not
what this example is for. Enable it if you want a software baseline; the
hardware numbers will not change until the ecc.c side is wired up.

### CFB and OFB are deliberately absent

HashCrypt also accelerates AES-CFB and AES-OFB, and `hashcrypt_port.c`
implements both, but only for whole 16-byte blocks. `wolfcrypt_test()` drives
them with sub-block lengths on purpose -- 8-byte calls in `aesofb_test()`, 4
and 27 bytes in `aescfb_test()` -- so enabling them would report failures that
say nothing about the hardware. Define `WOLFSSL_AES_CFB` and `WOLFSSL_AES_OFB`
in `user_settings.h` if you want them in the benchmark and can accept that.

The same caveat list also notes that AES-CTR fails when the counter wraps from
all-`FF` to zero. The test suite's counter-wrap vectors are already compiled
out for this configuration.

### WOLFSSL_CRYPT_HW_MUTEX is pinned off

`wc_port.h` turns `WOLFSSL_CRYPT_HW_MUTEX` on whenever `WOLFSSL_NXP_CASPER` is
defined, and `hashcrypt_port.c` has an `#error` against exactly that -- so the
two halves of `WOLFSSL_NXP_LPC55S6X` will not compile together at the default
setting. `user_settings.h` pins it to 0. This build is `SINGLE_THREADED`, where
the mutex compiles away to a stub, so nothing is lost; a threaded build on this
part would need the port fixed first.

## Building

The port is built against the NXP MCUXpresso SDK, which the Makefile locates
through `MCUXSDK`. Point it at an `mcuxsdk-core` checkout that west has
populated, so `devices/`, `examples/` and `components/` sit alongside
`drivers/`:

```sh
west init -m https://github.com/nxp-mcuxpresso/mcuxsdk-manifests mcuxsdk-ws
cd mcuxsdk-ws && west update
```

wolfSSL is expected as a sibling of `wolfssl-examples`; override `WOLFSSL` if
it is elsewhere. Then, in this directory:

```sh
make MCUXSDK=/path/to/mcuxsdk-ws/mcuxsdk
```

That produces `build/wolfcrypt-lpc55s6x.elf` and `.bin`. A recent
`arm-none-eabi-gcc` is needed; set `CROSS_COMPILE` if yours is prefixed
differently.

To measure what the hardware is worth, build the identical algorithm set in
software:

```sh
make clean && make MCUXSDK=/path/to/mcuxsdk-ws/mcuxsdk HW_CRYPTO=0
```

The TRNG stays on in both, since it is the board's only entropy source -- the
two builds differ only in the AES, SHA and RSA implementations.

## Running

Flash the `.bin` to the LPCXpresso55S69 and open the board's debug UART at
**115200 8N1**. It comes up on FLEXCOMM0 (`PIO0_29`/`PIO0_30`), which is the
LPC-Link2 virtual COM port.

The application boots at 150 MHz, initialises the TRNG, runs the test suite,
then the benchmark, and halts.

## Notes on the code

`main.c` does three things worth pointing out:

- It calls `RNG_Init(RNG)` before `wolfCrypt_Init()`. wolfSSL's
  `wc_GenerateSeed()` calls `RNG_GetRandomData()` directly and never
  initialises the peripheral, so the ring oscillator has to be powered and the
  entropy accumulator warmed up by the application.
- `current_time()` backs the benchmark's timebase with SysTick. It reads the
  reload counter alongside the millisecond count, because at 150 MHz HashCrypt
  routinely finishes an operation inside a single 1 ms tick.
- `wolfCrypt_Init()` is what brings up CASPER and HashCrypt, via
  `wc_casper_init()` and `wc_hashcrypt_init()`. There is no separate hardware
  init to call.
- `wolf_printf()` is what `XPRINTF` points at, so that `printf` inside
  `test.c` and `benchmark.c` routes through it. wolfCrypt ends its lines with
  a bare `\n` and neither SDK debug console translates that, which staircases
  the output on a serial terminal. Rather than buffer whole lines, it formats
  with the SDK's `StrFormatPrintf()` and inserts the CR in the character
  callback. That also means the lite console's own formatter goes unreferenced
  and the linker drops it, so the image is slightly smaller than it was with
  `XPRINTF` mapped straight to `PRINTF`.

Board support -- `board.c`, `clock_config.c`, `pin_mux.c` -- is compiled
straight out of the SDK tree rather than copied here, so there is nothing to
re-sync when the SDK moves.

## Status

Both configurations build clean and the hardware paths are the ones that link:
`wc_Sha256Update`, `wc_ShaUpdate`, `wc_AesCbcEncrypt` and `wc_AesCtrEncrypt`
all resolve to `hashcrypt_port.o`, and `casper_rsa_public_exptmod` to
`casper_port.o`.

The newline handling was checked by compiling `fsl_str.c` and the callback for
the host and running wolfCrypt-shaped format strings through them: every `\n`
came out as `\r\n`, with float, width and hex conversions intact.

The images have not been run on hardware.
