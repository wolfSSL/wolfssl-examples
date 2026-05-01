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

Test mode is the default and is selected by the Makefile (not the
header). To build for the real SRAM PUF on hardware, override the
`PUF_TEST` variable:

```bash
make PUF_TEST=0
```

This drops the `-DWOLFSSL_PUF_TEST` define and includes `puf_sram_region`
(placed in the `.puf_sram` NOLOAD section) so `wc_PufReadSram()` reads
the real power-on SRAM contents.

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
- **Production RNG** - This example wires wolfCrypt's RNG through
  `CUSTOM_RAND_GENERATE_BLOCK` (in `user_settings.h`) to
  `custom_rand_gen_block()` in `stm32.c`, which uses the STM32H5 RNG
  peripheral over HSI48. When porting to another MCU, replace the
  implementation behind `custom_rand_gen_block()` (or remap
  `CUSTOM_RAND_GENERATE_BLOCK` to your platform's hardware RNG hook).

## Reproducing on the m33mu Emulator

The m33mu Cortex-M33 emulator can simulate cold-boot SRAM and seeded
noise so the BCH reconstruction path can be exercised without rebooting
real hardware:

```bash
# Deterministic SRAM, boot 0 - enrolls and reconstructs cleanly
m33mu --puf-seed 0xDEADBEEF --puf-cold-boot 0 Build/puf_example.elf

# Same seed/boot with 2 bit flips per 127-bit codeword - within BCH(t=10)
m33mu --puf-seed 0xDEADBEEF --puf-cold-boot 0 --puf-noise 2 \
      Build/puf_example.elf
```

Identity must match between the enrollment and reconstruction prints as
long as noise stays within the BCH correction budget (10 flips per
127-bit codeword; safe margin 2-4).
