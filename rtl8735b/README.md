# wolfCrypt on RealTek AmebaPro2 (RTL8735B) -- Hardware Unique Key (HUK)

This example demonstrates wolfCrypt AES-GCM, AES-ECB, AES-CBC, AES-CTR, HMAC-SHA256 and ECDSA P-256 bound to the RealTek RTL8735B silicon **Hardware Unique Key (HUK)** through the wolfCrypt crypto-callback (CryptoCb) framework. A 256-bit "seed" is run through the AmebaPro2 HAL secure HKDF key-ladder against the HUK to derive a per-purpose working key inside a secure key-storage slot; that working key never enters software. The application just sets the seed as the key and uses the normal wolfCrypt APIs -- the RealTek HUK crypto-callback device does the rest.

It is built inside the RealTek AmebaPro2 FreeRTOS SDK (which provides the startup, HAL, and the `elf2bin`/`uartfwburn` image+flash tooling).

## What it shows

- Registering the HUK crypto-callback device: `wc_Rtl8735b_HukRegister(WC_HUK_DEVID)`.
- AES-GCM (full payload), AES-ECB, AES-CBC and AES-CTR run under the HUK-derived key (init an `Aes` with `devId = WC_HUK_DEVID`, then use the usual `wc_AesGcmSetKey`/`wc_AesGcmEncrypt`, `wc_AesSetKey`/`wc_AesEcb*`/`wc_AesCbc*`, `wc_AesSetKeyDirect`/`wc_AesCtrEncrypt`).
- The key is device-bound and deterministic: the same seed yields the same key (so GMAC verifies and AES round-trips), and a different seed yields a different key (GCM decrypt returns `AES_GCM_AUTH_E`).
- The port tolerates unaligned caller buffers (it bounces them to satisfy the HAL's 32-byte alignment); the example includes an explicitly-unaligned GCM check.
- HMAC-SHA256 under the HUK (`wc_HmacInit` with `devId = WC_HUK_DEVID`, then `wc_HmacSetKey`/`wc_HmacUpdate`/`wc_HmacFinal`): the MAC is deterministic for a given seed and differs for a wrong seed.
- HUK-bound ECDSA P-256 sign: a key is generated in software, its private scalar is wrapped under the HUK (ECB-encrypted with the HUK device), and signing then goes through the HUK device (unwrap + sign); the signature verifies against the software public key.
- HW ECDSA P-256 verify offload: the same signature is then verified through the HW engine (the public point is imported into a `WC_HUK_DEVID` key so `wc_ecc_verify_hash` dispatches to `hal_ecdsa`), and a tampered digest is shown to be rejected. No HUK context is needed to verify -- any P-256 public key.

## Prerequisites

- RealTek AmebaPro2 FreeRTOS SDK: https://github.com/Ameba-AIoT/ameba-rtos-pro2
- RealTek ASDK 10.3.0 toolchain (GCC 10.3.0): https://github.com/Ameba-AIoT/ameba-toolchain (tag `V10.3.0-ameba-rtos-pro2`). The stock system `arm-none-eabi-gcc` does not build the SDK (newlib/lwip header clashes); use the ASDK toolchain.
- wolfSSL with the RealTek HUK port (`wolfcrypt/src/port/realtek/rtl8735b.c` and `wolfssl/wolfcrypt/port/realtek/rtl8735b.h`). Enable with `WOLFSSL_RTL8735B_HUK` and `WOLF_CRYPTO_CB`.

## Files

- `main.c` -- the example application (a FreeRTOS task that registers the HUK device and runs the AES-GCM/ECB/CBC/CTR checks plus unaligned-buffer GCM, in-place/multi-call CBC, non-12-byte-IV rejection, HMAC-SHA256, and HUK-bound ECDSA sign + HW verify checks).
- `user_settings.h` -- a lean wolfCrypt configuration for this example.
- `wolfcrypt_huk.cmake` -- wiring that adds wolfCrypt + the RealTek HUK port to the SDK app build.
- `test/` -- a separate `wolfcrypt_test` + `wolfcrypt_benchmark` example for this target with three backend modes (pure C, Thumb-2 / SP Cortex-M, and RealTek hardware offload); see `test/README.md`.

## Build

1. Copy this directory into the SDK as an example component:
   `cp -r rtl8735b <SDK>/component/example/wolfcrypt_huk`
2. Install the example `main()` as the project app:
   `cp <SDK>/component/example/wolfcrypt_huk/main.c <SDK>/project/realtek_amebapro2_v0_example/src/main.c`
3. Configure and build with the ASDK toolchain on PATH, pointing `WOLFSSL_ROOT` at your wolfSSL tree:
   ```
   export PATH=<asdk>/asdk-10.3.0/linux/newlib/bin:$PATH
   cd <SDK>/project/realtek_amebapro2_v0_example/GCC-RELEASE
   mkdir -p build && cd build
   cmake .. -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake \
       -DBUILD_FPGA=OFF -DBUILD_PXP=OFF \
       -DEXAMPLE=wolfcrypt_huk -DWOLFSSL_ROOT=<path-to-wolfssl>
   make flash -j8
   ```
   The output image is `build/flash_ntz.bin`.

## Flash and run

Put the board in download mode (J27 jumper + reset), then flash over UART with the SDK `uartfwburn` tool (from the SDK `tools/Pro2_PG_tool*` directory, which also holds `boot_recover.bin`, `flash_loader_nor.bin`, `flash_control_info.bin`):
```
./uartfwburn.linux -p <COM_PORT> -f <build>/flash_ntz.bin -b 3000000 -U
```
Remove the J27 jumper and reset to boot. The console (UART, 115200) prints:
```
=== wolfCrypt AmebaPro2 (RTL8735B) HUK example ===
[PASS] wolfCrypt_Init
[PASS] wc_Rtl8735b_HukRegister
== AES-GCM (full payload) under HUK-derived key ==
[PASS] AesInit(devId=WC_HUK_DEVID)
[PASS] AesGcmSetKey(seed,32)
[PASS] AesGcmEncrypt
[PASS] deterministic tag
[PASS] AesGcmDecrypt verifies
[PASS] plaintext round-trips
[PASS] wrong seed -> AES_GCM_AUTH_E
== AES-ECB under HUK-derived key ==  ... round-trip PASS
== AES-CBC under HUK-derived key ==  ... round-trip PASS
== AES-CTR under HUK-derived key ==  ... round-trip PASS
== AES-GCM with UNALIGNED buffers (port bounces) ==  ... round-trip PASS
== AES-CBC in-place + multi-call under HUK-derived key ==  ... round-trip PASS
== AES-GCM non-12-byte IV must hard-fail ==  ... 16-byte IV rejected PASS
== HMAC-SHA256 under HUK-derived key ==  ... deterministic / wrong-seed PASS
== ECDSA P-256 under HUK-derived key ==
[PASS] HUK ECDSA sign
[PASS] verify with software public key
[PASS] HW ECDSA verify (good sig)
[PASS] HW ECDSA verify (tampered -> reject)
=== done ===
```

## Notes

- The AmebaPro2 HAL crypto engine DMAs its key/IV/AAD/tag buffers on 32-byte boundaries; the port stages and bounces buffers as needed, so callers need not align (the example aligns its buffers anyway, and adds an explicitly-unaligned GCM case to exercise the bounce path).
- The seed is HKDF input, not the AES key itself -- it diversifies the HUK into a working key. Keep the seed stable for a given purpose to get a stable derived key.
- HUK-bound ECDSA P-256 signing: this example sets `hk.useHwEngine = 1` to drive the HW ECDSA engine (validated on the RTL8735B); set it to 0 for the software-after-unwrap path (the port default). `hk.otpPrkSel` can instead select an OTP-resident key (scalar never in software; needs an OTP key provisioned).
- Port internals (the HKDF key-ladder, slot configuration macros such as `WC_HUK_DEVID`, `WC_RTL8735B_*`) are documented in `wolfssl/wolfcrypt/port/realtek/rtl8735b.h` and `wolfcrypt/src/port/realtek/README.md` in the wolfSSL tree.
