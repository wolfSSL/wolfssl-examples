# wolfCrypt on Infineon PSOC Edge E8

Bare-metal wolfCrypt test and benchmark for the PSOC Edge E84 Cortex-M33 on the KIT_PSE84_AI board. The example runs in the Secure state as the first CM33 image Infineon's extended boot launches, in place from QSPI. It uses the on-chip crypto engine (MXCRYPTO) through wolfSSL's `WOLFSSL_PSOC6_CRYPTO` port for SHA-1/2/3 and AES (ECB, CBC, CFB, GCM), and seeds the Hash-DRBG from the engine's TRNG.

## Requirements

- `arm-none-eabi-gcc` with newlib
- wolfSSL, with PSOC Edge support in the `WOLFSSL_PSOC6_CRYPTO` port
- Infineon device support and CMSIS, at the releases this example was tested with:

```sh
git clone --depth 1 -b release-v1.8.0 https://github.com/Infineon/mtb-dsl-pse8xxgp
git clone --depth 1 -b release-v1.5.0 https://github.com/Infineon/mtb-template-pse8xxgp
git clone --depth 1 -b release-v1.8.0 https://github.com/Infineon/core-lib
git clone --depth 1 -b v6.4.0 https://github.com/ARM-software/CMSIS_6
```

- ModusToolbox Programming Tools, for OpenOCD and the KitProg3 QSPI flash loader

## Building

The Makefile expects wolfSSL and the four repositories above next to `wolfssl-examples`. Override `WOLFSSL_ROOT`, `PSE84_DSL`, `PSE84_TEMPLATE`, `CORE_LIB` and `CMSIS` to point elsewhere.

```sh
make                 # Build/wolfcrypt_test.bin and Build/wolfcrypt_bench.bin
make PSOC_CRYPTO=0   # software only, in Build-sw/
make PSOC_CRYPTO_ECC=1  # ECDSA verify on the engine too, in Build-ecc/
```

Only the PDL crypto core drivers are compiled, so no ModusToolbox project or BSP is needed. `PSE84_PART` selects the part header (default `pse846gps2dbzc4a`).

## Running

```sh
tools/flash.sh Build/wolfcrypt_test.bin
tools/flash.sh Build/wolfcrypt_bench.bin
```

`tools/flash.sh` wraps the image in the MCUboot-format header that extended boot expects (`tools/mcuboot_wrap.py`) and programs it to QSPI `0x60100000` through the kit's KitProg3. Set `MTB_PROGTOOLS` if the tools are not in `/opt/Tools/ModusToolboxProgtools-1.6`, and `KITPROG_SERIAL` if several probes are attached. Output appears on the KitProg3 COM port at 115200 baud.

This replaces the factory CM33 image, so read QSPI back first to keep the stock demo.

## Notes

- The clocks are left as extended boot sets them: the CM33 at 200 MHz and the console's peripheral clock at 100 MHz. Benchmark timing uses the DWT cycle counter.
- Code and constants run from QSPI through the Secure code-bus alias. The port translates such addresses for the crypto engine, which reads memory over the system bus.
- ECC runs in software with wolfSSL's SP math assembly, which verifies faster than the engine; `PSOC_CRYPTO_ECC=1` moves ECDSA verify to the engine. `WOLFSSL_SP_SMALL` is set because the full-size SP tables are slow to read from XIP flash.
