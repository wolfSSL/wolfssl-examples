# wolfCrypt SRAM PUF Example

Bare-metal example demonstrating SRAM PUF (Physically Unclonable Function)
on Cortex-M targets (tested on NUCLEO-H563ZI).

## Overview

SRAM PUF exploits the random power-on state of SRAM memory cells to derive
device-unique cryptographic keys. Each chip has a unique SRAM "fingerprint"
caused by manufacturing variations.

This example demonstrates:

1. **Enrollment** - Read raw SRAM, generate helper data using BCH(127,64,t=10)
   error-correcting codes
2. **Reconstruction** - Re-read (noisy) SRAM, use helper data to recover the
   same stable bits despite bit flips (corrects up to 10 per 127-bit codeword)
3. **Key derivation** - Use HKDF-SHA256 to derive a 256-bit cryptographic key
4. **Device identity** - SHA-256 hash of stable bits serves as a unique device ID

## Building

### Requirements

- `arm-none-eabi-gcc` toolchain

### Test Mode (default)

Uses synthetic SRAM data -- runs on any target, no real hardware needed:

```bash
make
```

### Real Hardware Mode

For actual SRAM PUF on hardware, edit `user_settings.h` and comment out
`WOLFSSL_PUF_TEST`, then build:

```bash
make
```

### Output

Build output is placed in `./Build/`:
- `puf_example.elf` - Loadable ELF binary
- `puf_example.hex` - Intel HEX for flash programmers

## Flashing

### OpenOCD (NUCLEO-H563ZI)

```bash
openocd -f interface/stlink.cfg -f target/stm32h5x.cfg \
    -c "program Build/puf_example.elf verify reset exit"
```

When multiple ST-Links are connected, specify the serial number:

```bash
openocd -f interface/stlink.cfg \
    -c "adapter serial <YOUR_SERIAL>" \
    -f target/stm32h5x.cfg \
    -c "program Build/puf_example.elf verify reset exit"
```

## UART Output

Connect to the board's UART (typically 115200 baud) to see output:

```
--- wolfCrypt SRAM PUF Example ---

PUF initialized.
Mode: TEST (synthetic SRAM data)

Enrollment complete.
Identity (enrollment): 3ad99904f92897bad1a21bc9cbc3ab8f2dc4bc40dfe6e161c741f98ef8dd7e01
Derived key (enrollment): aa8573f70a3253ca567500bdcd610face6a140e5fc68047e02d3f13958dcc480

--- Simulating power cycle (noisy SRAM) ---

Reconstruction complete (BCH corrected noisy bits).
Identity (reconstructed): 3ad99904f92897bad1a21bc9cbc3ab8f2dc4bc40dfe6e161c741f98ef8dd7e01
PASS: Identity matches after reconstruction.
Derived key (reconstructed): aa8573f70a3253ca567500bdcd610face6a140e5fc68047e02d3f13958dcc480
PASS: Derived key matches after reconstruction.

--- PUF example complete ---
```

## Customizing for Your MCU

### Linker Script

Edit `linker.ld` to match your MCU's memory map:

```ld
MEMORY
{
    FLASH   (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K  /* your flash size */
    RAM     (rwx) : ORIGIN = 0x20000000, LENGTH = 636K   /* total - PUF_RAM */
    PUF_RAM (rw)  : ORIGIN = 0x2009F000, LENGTH = 4K     /* end of SRAM */
}
```

The `PUF_RAM` region must be at the end of SRAM and marked `NOLOAD` so the
startup code does not zero it.

### Architecture

Edit the `ARCHFLAGS` in `Makefile`:

```makefile
ARCHFLAGS = -mcpu=cortex-m33 -mthumb
```

## API Usage

```c
wc_PufCtx ctx;
uint8_t helperData[WC_PUF_HELPER_BYTES];
uint8_t key[WC_PUF_KEY_SZ];

/* First boot: Enroll */
wc_PufInit(&ctx);
wc_PufReadSram(&ctx, sram_addr, sram_size);
wc_PufEnroll(&ctx);
memcpy(helperData, ctx.helperData, WC_PUF_HELPER_BYTES);
/* Store helperData to flash/NVM (it is NOT secret) */

/* Subsequent boots: Reconstruct */
wc_PufInit(&ctx);
wc_PufReadSram(&ctx, sram_addr, sram_size);
wc_PufReconstruct(&ctx, helperData, WC_PUF_HELPER_BYTES);
wc_PufDeriveKey(&ctx, info, infoSz, key, sizeof(key));

/* Always zeroize when done */
wc_PufZeroize(&ctx);
```

## Security Notes

- **Helper data is public** - It does not reveal the key. Safe to store
  unencrypted in flash or transmit over the network.
- **SRAM must not be accessed before PUF read** - Any read or write to the
  PUF SRAM region before `wc_PufReadSram()` will corrupt the power-on entropy.
- **Production RNG** - Replace the dummy `my_rng_seed_gen()` with your
  MCU's hardware RNG (e.g., STM32 RNG peripheral).
