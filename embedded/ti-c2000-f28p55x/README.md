# wolfCrypt on TI C2000 C28x (LAUNCHXL-F28P55X)

Bare-metal wolfCrypt test/benchmark for the TI LAUNCHXL-F28P55X board (TMS320F28P550SJ, C2000 C28x DSP, 150 MHz, 133 KB RAM, 1088 KB flash). The C28x is a word-addressed DSP where `CHAR_BIT == 16`; this example exercises the wolfSSL `CHAR_BIT != 8` support and is validated on hardware with known-answer tests (KATs).

## What this build includes

The default build runs a KAT suite plus `wolfcrypt_test` and (optionally) `benchmark`. Optional algorithms are enabled with `make` toggles (see Build options). Everything below is validated on hardware:

- SHA-224/256, SHA-384/512, SHA-512/224, SHA-512/256
- SHA3-224/256/384/512, SHAKE128, SHAKE256 (split-64 Keccak permutation, ~53% faster than the generic C path on C28x)
- ML-DSA-87 (Dilithium level 5) verify, and the full keygen+sign+verify round-trip (`SIGN=1`)
- ML-KEM-768 (FIPS 203) keygen/encap/decap round-trip (`MLKEM=1`)
- AES-128/192/256 CBC/CTR/CFB/GCM (`AES=1`); AES-CMAC, AES-CCM, AES-GMAC (`AESEXTRA=1`)
- HMAC-SHA256 + HKDF (`HKDF=1`)
- ChaCha20-Poly1305 AEAD + Poly1305 (`CHACHA=1`)
- X25519 + Ed25519 (`X25519=1`)
- ECDSA + ECDH over SECP256R1 via SP math (`ECC=1`)
- RSA-2048 PKCS#1 v1.5 verify via SP math (`RSA=1`)

## The 16-bit-byte (CHAR_BIT == 16) story

On the C28x a `char`/`unsigned char` (wolfSSL's `byte`) holds 16 bits, the minimum addressable unit is 16 bits, and `(byte)x` masks to 16 bits, not 8. wolfCrypt octet handling that assumed an 8-bit byte was made `CHAR_BIT`-agnostic behind the `WOLFSSL_WIDE_BYTE` gate (auto-enabled for `CHAR_BIT != 8` and known 16-bit-char TI toolchains), with zero change on 8-bit targets. The fixes fall into a few recurring classes:

- Byte/word aliasing: serializing a `word32`/`word64` by casting it to `byte*` reads/writes cells, not octets. Replaced with explicit shift-based octet I/O (shared helpers in `wolfcrypt/src/misc.c`: `WordsFromBytesBE32`, `BytesFromWordsBE32`, `BytesFromWordsLE32`, the 64-bit variants, and octet-correct `readUnalignedWord32/64`).
- `(byte)x` not truncating to an octet: masked with `WC_OCTET(x)` (`(byte)((x) & 0xFF)`), a no-op on 8-bit.
- Integer-promotion bugs: `1U << n` is 16-bit on C28x (use `1UL`); `sizeof(t) * 8` for a bit width is wrong when `CHAR_BIT != 8` (use `CHAR_BIT * sizeof(t)`).
- `sizeof` counting cells, not octets: e.g. `CHACHA_CHUNK_BYTES` must be `16 * 4`, not `16 * sizeof(word32)` (which is 32 on C28x and halves the ChaCha block).

These changes live in the wolfSSL tree (see `IDE/C2000/README.md` there for the full list); this example is the on-hardware harness that validates them.

## Toolchain

- **C2000Ware** (driverlib + device support). Assumed at `~/ti/C2000Ware_26_01_00_00`; override with `C2000WARE=`.
- **TI C2000 codegen** (`cl2000`, the classic CGT). Ships with Code Composer Studio or standalone. Point `CGT_ROOT` at the dir containing `bin/cl2000`.
- For flashing: **DSLite** (bundled with CCS) or **UniFlash**, driving the onboard XDS110. A ready `ccxml/F28P550SJ.ccxml` is included.

## Build

Headless, from this directory:

```
make CGT_ROOT=/path/to/ti-cgt-c2000_xx.y.z
```

Output: `out/wolfcrypt_f28p55x.out` (load image) and `out/wolfcrypt_f28p55x.map`. Point `WOLFROOT` at a wolfSSL checkout with the `CHAR_BIT != 8` support if it is not the default `../../../wolfssl-alt`.

### Build options

Each is `make <NAME>=1` (default 0 unless noted), additive on top of the default SHA + ML-DSA-verify build:

| Option | Adds |
|---|---|
| `ECC=1` | ECDSA/ECDH over SECP256R1 (SP math) |
| `MLKEM=1` | ML-KEM-768 (FIPS 203) |
| `AES=1` | AES-CBC/CTR/CFB/GCM (table-driven, `GCM_SMALL`) |
| `AESEXTRA=1` | AES-CMAC, AES-CCM, AES-GMAC (implies the AES core) |
| `X25519=1` | Curve25519 (X25519) + Ed25519 |
| `HKDF=1` | HMAC + HKDF (RFC 2104 / RFC 5869) |
| `CHACHA=1` | ChaCha20-Poly1305 AEAD (RFC 8439) |
| `RSA=1` | RSA-2048 verify (SP math, 2048-only, verify/public-only) |
| `SIGN=1` | Full ML-DSA-87 keygen+sign+verify demo (dedicated linker script, 32 KW heap, no test/bench harness) |
| `BENCH=1` | Run only `benchmark` instead of `wolfcrypt_test` (they need separate images on this RAM-limited part) |

Other knobs: `CGT_ROOT` (required), `C2000WARE`, `WOLFROOT`, and `CIO=1` (route `printf` to the CCS/JTAG console via CIO instead of the default SCI/UART).

CCS GUI alternative: create an empty F28P550SJ project, add the `WC_SRCS` subset from the `Makefile`, `Source/wolf_main.c`, the device-support `device.c` + `f28p55x_codestartbranch.asm`, link `driverlib.lib`, set the predefines (`_LAUNCHXL_F28P55X`, `_FLASH`, `WOLFSSL_USER_SETTINGS`) and include paths, and use `28p55x_wolf_flash_lnk.cmd`.

## Console output

`printf` goes to the **SCI/UART by default**: `stdout` is redirected to **SCIA on GPIO28/29**, the XDS110 virtual COM, at **115200 8N1**, so output appears on a serial terminal (or `uart-monitor`) with no debugger attached. Build with **`CIO=1`** to route output to the CCS console via CIO over JTAG instead (useful under an active debug session, and required by the DSS capture scripts). Every stdout byte is also mirrored to the `g_log[]` RAM buffer, readable over JTAG even when the UART is not.

## Flash and run

Boot-mode switch S3 default (GPIO24 + GPIO32 both high) boots from flash. With CCS DSLite:

```
DSLite flash -c ccxml/F28P550SJ.ccxml -e -f -v out/wolfcrypt_f28p55x.out
```

For clean **UART** output the JTAG probe must be RELEASED while the target runs: an active debug session garbles the XDS110 backchannel, and DSLite leaves the CPU halted after flashing. So flash, then reset+run and DETACH the debugger (leave it running) -- the board free-runs from flash and loops `main()`. Then read the serial port (e.g. `uart-monitor tail /dev/ttyACM<N>` at 115200 8N1). The `ti-c2000-c28x` skill automates this: `run_uart.sh [CHACHA=1 ...]` builds (SCI default), flashes, resets+runs+detaches (`run_detach.js`), and prints the clean UART. For JTAG-console capture instead, build `CIO=1` and use `run_capture.js` (holds JTAG, reads CIO).

## Benchmark results

Measured at 150 MHz (SHA-256 Hash-DRBG via `WOLFSSL_GENSEED_FORTEST`; build with `BENCH=1`). Representative throughput:

| Primitive | Throughput |
|---|---|
| SHA-256 | ~284 KiB/s |
| SHA-384 / SHA-512 (+512-224/256) | ~166 KiB/s |
| SHA3-224 / 256 / 384 / 512 | ~279 / 264 / 206 / 146 KiB/s |
| SHAKE128 / SHAKE256 | ~319 / 264 KiB/s |
| RNG (Hash-DRBG) | ~122 KiB/s |

ML-DSA-87 (asymmetric, @150 MHz): verify ~225 ms/op; keygen and signing also run (`SIGN=1`). ML-DSA-87 verify memory: ~10.7 KB total (key/workspace struct + ~2 KB stack, zero heap) with `WOLFSSL_MLDSA_VERIFY_SMALLEST_MEM` + `WOLFSSL_MLDSA_ASSIGN_KEY`.

## Memory notes

- Clock: 150 MHz from the 20 MHz crystal Y2 (`device.h` `DEVICE_SETCLOCK_CFG`), flash `RWAIT=3`.
- The C28x stack pointer is 16-bit, so the stack must live below address `0x10000`. `28p55x_wolf_flash_lnk.cmd` puts a 16 KW stack in RAMLS0-7, RAM-resident hot code (`BlockSha3` + the ML-DSA NTTs) in RAMGS0, an 8 KW heap in RAMGS1, and `.bss`/`.data` in the high RAMGS2-3 / RAMLS8-9 blocks. Flash output sections span all five banks.
- Big SP/`*_NO_MALLOC` structs (ecc_key, RsaKey, ChaChaPoly_Aead) belong in `.bss`/static, not on the stack: the SP point/modexp call tree plus a stack-allocated key can overflow the 16 KW stack.
- `wc_RsaSSL_Verify` in the `RSA_VERIFY_ONLY` / `SP_NO_MALLOC` config runs the modexp in place in the caller's buffer, so the output buffer must be at least the key size (256 B for RSA-2048).

## RNG caveat

The F28P55x has **no hardware TRNG**. The example uses `WOLFSSL_GENSEED_FORTEST` (random.c's built-in incrementing test seed feeding the real SHA-256 Hash-DRBG): exercises the real DRBG path but is **development-only, not cryptographically secure**. For production, wire a real entropy source into `wc_GenerateSeed()`.
