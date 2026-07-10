# wolfCrypt test + benchmark on RealTek AmebaPro2 (RTL8735B)

Runs the standard `wolfcrypt_test` self-test and `wolfcrypt_benchmark` on the RTL8735B "KM4" Arm Cortex-M33 inside the RealTek AmebaPro2 FreeRTOS SDK. It validates the wolfCrypt build and toolchain on this target and reports per-algorithm throughput in three backend modes, selected with `-DRTL_BENCH_MODE=N` at configure time:

| Mode | `RTL_BENCH_MODE` | Backend |
|------|------------------|---------|
| Pure C | `1` | Generic `sp_int.c` big-integer math + C symmetric/hash. The portable reference baseline (no assembly). |
| Thumb-2 / SP Cortex-M | `2` (default) | `sp_cortexm.c` for public-key math (Thumb-2/DSP UMAAL) + `thumb2-*-asm.S` for AES/SHA/ChaCha/Poly1305. |
| RealTek HW | `3` | The HUK crypto-callback device over the silicon engine (`hal_crypto` / `hal_ecdsa`): AES, HMAC-SHA256 and ECDSA P-256 sign/verify run in hardware (via `WC_USE_DEVID`). Benchmark only -- see note below. |

Modes 1 and 2 run the self-test then the benchmark. Mode 3 runs the benchmark only: the HUK device derives its working key from the "key" bytes (an HKDF seed), so the standard known-answer test vectors do not apply to it. Mode 3 routes every benchmark op through the HUK device id; ops the device does not accelerate (RSA, DH, ChaCha, hashing, key generation, ECDH) fall back to software automatically, so they show their software (mode-1) rates while the accelerated ops (AES, HMAC-SHA256, ECDSA P-256 sign/verify) show the hardware rates.

## Files

- `main.c` -- a FreeRTOS task that initializes wolfCrypt and runs the test/benchmark (registering the HUK device in mode 3). Provides the RNG seed (`rtw_get_random_bytes`), the benchmark time source (`hal_read_systime_us`), and a fixed wall clock for X.509 date checks.
- `user_settings.h` -- the wolfCrypt configuration; the `RTL_BENCH_MODE` switch selects the backend defines.
- `wolfcrypt_test.cmake` -- SDK app wiring; the `RTL_BENCH_MODE` switch selects the backend sources.

## Build and run

1. Copy this directory into the SDK as an example component:
   `cp -r rtl8735b/test <SDK>/component/example/wolfcrypt_test`
2. Install the example `main()` as the project app:
   `cp <SDK>/component/example/wolfcrypt_test/main.c <SDK>/project/realtek_amebapro2_v0_example/src/main.c`
3. Configure and build with the ASDK toolchain on PATH, pointing `WOLFSSL_ROOT` at your wolfSSL tree and choosing a mode:
   ```
   export PATH=<asdk>/asdk-10.3.0/linux/newlib/bin:$PATH
   cd <SDK>/project/realtek_amebapro2_v0_example/GCC-RELEASE
   mkdir -p build && cd build
   cmake .. -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake \
       -DBUILD_FPGA=OFF -DBUILD_PXP=OFF \
       -DEXAMPLE=wolfcrypt_test -DRTL_BENCH_MODE=2 \
       -DWOLFSSL_ROOT=<path-to-wolfssl>
   make flash -j8
   ```
   The output image is `build/flash_ntz.bin`. Flash it the same way as the HUK example (J27 download jumper + `uartfwburn`), then reset to boot; the console (UART, 115200) prints the test and benchmark results.

The measured numbers for all three modes are tabulated in the wolfSSL tree at `wolfcrypt/src/port/realtek/README.md`.
