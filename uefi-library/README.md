# wolfCrypt UEFI Library

## wolfCrypt as a UEFI Boot-Service Driver

A UEFI boot-services driver that exposes wolfCrypt cryptographic operations to
other UEFI applications and drivers via an EFI protocol interface. 

Applications running in efi after loading the wolfcrypt driver can use the
`wolfcrypt_protocol` to call wolfCrypt functions directly from UEFI.

See `test_app.c` for example usage.



### Directory Structure

```
uefi-library/
├── Makefile
├── user_settings.h        wolfSSL platform/feature configuration
├── wolfcrypt_api.h        EFI protocol GUID + WOLFCRYPT_PROTOCOL struct
├── utility_wolf.h         UEFI utility function declarations
├── NvVars                 OVMF NVRAM state file (boot order, etc.)
├── startup.nsh            Dual-build fallback NSH script
├── startup-single.nsh     Single-build NSH script
├── src/
│   ├── driver.c           EFI driver entry point + EFIAPI wrappers
│   ├── test_app.c         Smoke-test UEFI application
│   └── utility_wolf.c     UEFI platform utility implementations
└── README.md
```

---

## Prerequisites (Linux)

| Package | Purpose |
|---------|---------|
| `gnu-efi` | EFI CRT0, linker scripts, headers (`/usr/include/efi`) |
| `gcc-multilib` | 32-bit cross-build support (`-m32`) |
| `qemu-system-x86` | x86_64 and i386 UEFI emulation |
| `ovmf` | UEFI firmware for QEMU (`/usr/share/ovmf/OVMF.fd`) |
| wolfssl source | wolfCrypt source tree (see `WOLFSSL_PATH` below) |

Install on Debian/Ubuntu:
```bash
sudo apt install gnu-efi gcc-multilib qemu-system-x86 ovmf
```

---

## Build Targets

| Target | Architecture | AES-NI | Description |
|--------|-------------|--------|-------------|
| `lib` | x86_64 | yes | Driver with AES-NI acceleration |
| `lib-nohw` | x86_64 | no | Pure software driver |
| `lib32` | i386 | no | 32-bit driver (software only) |
| `lib32-nohw` | i386 | no | Alias for lib32 |
| `run` | x86_64 | yes | Build + QEMU (KVM, host CPU) |
| `run-nohw` | x86_64 | no | Build + QEMU (qemu64 CPU, no AES-NI) |
| `run32` | i386 | no | Build + QEMU i386 |
| `run32-nohw` | i386 | no | Alias for run32 |
| `run-fallback-nohw` | x86_64 | both | Build hw+nohw, run QEMU without AES-NI |
| `clean` | — | — | Remove all build artifacts |

---

## WOLFSSL_PATH Variable

The Makefile defaults to:
```makefile
WOLFSSL_PATH ?= $(abspath $(CURDIR)/../../wolfssl)
```
This resolves to the `wolfssl` directory that is a sibling of `wolfssl-examples`.
Override on the command line:
```bash
make run-nohw WOLFSSL_PATH=/path/to/wolfssl
```

The wolfssl source does not need to be configured or compiled separately — the
Makefile compiles only the required `.c` files from the wolfssl tree directly.

---

## hw vs nohw Builds

**`lib` (hw)** compiles with `-maes -mpclmul -DUEFI_HW_ACCEL`, enabling AES-NI
intrinsics. At runtime the driver checks CPUID for AES-NI support in `efi_main`.
If AES-NI is absent the driver prints a message and returns `EFI_UNSUPPORTED`,
staying unloaded. This enables the CPUID-based fallback mechanism.

**`lib-nohw`** compiles without hardware intrinsics, using portable C
implementations. It loads on any x86_64 CPU regardless of AES-NI.

---

## CPUID-Based Fallback Mechanism

The `run-fallback-nohw` target demonstrates automatic driver selection:

1. Both `libwolfcrypt.efi` (hw) and `libwolfcrypt-nohw.efi` (software-only) are
   built and copied to the `efifs/` FAT image.
2. `startup.nsh` is installed in `efifs/`:
   ```
   fs0:
   load libwolfcrypt.efi
   if %lasterror% == 0 then
       goto loaded
   endif
   echo Falling back to software-only driver...
   load libwolfcrypt-nohw.efi
   :loaded
   test.efi
   ```
3. QEMU is launched with `-cpu qemu64` (no AES-NI).
4. The hw driver detects the missing capability and returns `EFI_UNSUPPORTED`.
5. The shell sees a non-zero `%lasterror%` and loads the nohw driver instead.
6. `test.efi` runs against whichever driver is resident.

This mirrors real-world deployment where a single firmware image ships both
variants, choosing at boot time based on hardware capability.

---

## Example Invocations

```bash
# Safest — software only, no KVM or AES-NI required
make run-nohw

# Hardware accelerated (requires KVM host with AES-NI), runs in KVM/qemu
make run

# Demonstrate fallback on qemu64: CPU does not support AES-NI
# hw driver `load` fails, exits with EFI_UNSUPPORTED, nohw driver loaded
# as fallback via startup.nsh
make run-fallback-nohw

# 32-bit build, software only, run in qemu32
make run32-nohw

# Build only, no QEMU (full HW accelerated build)
make lib-nohw
```

---

## Protocol Interface

The driver installs a `WOLFCRYPT_PROTOCOL` on its image handle. Consumer
applications locate it via:
```c
#include "wolfcrypt_api.h"

static EFI_GUID gWolfCryptProtocolGuid = WOLFCRYPT_PROTOCOL_GUID;
WOLFCRYPT_PROTOCOL *wc = NULL;
Status = BS->LocateProtocol(&gWolfCryptProtocolGuid, NULL, (VOID **)&wc);
```

The protocol struct provides function pointers for:
- **Symmetric**: AES (ECB/CBC/CFB/CTR/OFB/GCM/CCM), ChaCha20, Poly1305,
  ChaCha20-Poly1305 AEAD, CMAC
- **Hash**: SHA-1, SHA-224, SHA-256, SHA-384, SHA-512, SHA-3 (256/384/512),
  SHAKE-128, SHAKE-256
- **MAC**: HMAC
- **Asymmetric**: RSA, ECC, DH, Curve25519, Ed25519
- **Post-quantum**: ML-KEM (Kyber), Dilithium (ML-DSA), Falcon, SPHINCS+, XMSS, LMS
- **KDF**: PBKDF2, PKCS12-PBKDF, HKDF
- **RNG**: hardware-seeded DRBG via UEFI `EFI_RNG_PROTOCOL`
- **Misc**: logging, error strings, version

See `wolfcrypt_api.h` for the complete `WOLFCRYPT_PROTOCOL` definition.


