/* 28p55x_wolf_sign_lnk.cmd
 *
 * Linker command file for the FULL ML-DSA-87 (keygen + sign + verify) build of
 * the wolfCrypt example on the TI LAUNCHXL-F28P55X (TMS320F28P550SJ), running
 * from flash.  EABI only.  Selected by `make SIGN=1`.
 *
 * ML-DSA-87 keygen/sign/verify need a much larger heap than the verify-only
 * build.  Even with every small-memory path enabled the peak is ~30 KW in a
 * single allocation, so this script dedicates the whole contiguous RAMGS0-3
 * block (32 KW) to the heap (.sysmem).  The stack stays in low RAMLS0-7 (the
 * 16-bit SP needs RAM < 0x10000) and .bss/.data live in RAMLS8-9.  The full
 * wolfcrypt test/benchmark are NOT built in this mode (they would not fit
 * alongside the 32 KW heap), so .bss is small.
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.  GPLv3 - see project headers.
 */

-stack 0x3000   /* 12 KW C28x stack (< 0x10000); big arrays go to heap via
                 * WOLFSSL_SMALL_STACK */
-heap  0x7E00   /* ~31.5 KW heap - ML-DSA-87 keygen/sign peak ~30.5 KW */

MEMORY
{
   BEGIN            : origin = 0x080000, length = 0x000002

   BOOT_RSVD        : origin = 0x000002, length = 0x000126  /* M0, used by boot ROM stack */
   RAMM0            : origin = 0x000128, length = 0x0002D8
   RAMM1            : origin = 0x000400, length = 0x000400

   /* Low RAM (< 0x10000): the 16-bit-SP stack (12 KW), plus 4 KW of RAM-
    * resident hot code carved from the top of RAMLS0-7.  The stack only uses
    * 0x3000, so 0xB000..0xBFFF is free for the Keccak permutation. */
   RAMLS_STACK      : origin = 0x008000, length = 0x003000  /* RAMLS0-5, 12 KW */
   RAMLS_RAMCODE    : origin = 0x00B000, length = 0x001000  /* RAMLS6-7, 4 KW */

   /* RAMGS0-3 is one contiguous 32 KW block (0xC000..0x13FFF): the heap. */
   RAMGS_BIG        : origin = 0x00C000, length = 0x008000  /* RAMGS0-3, 32 KW */

   /* High RAM: .bss/.data (no stack here). */
   RAMLS_HI         : origin = 0x014000, length = 0x004000  /* RAMLS8-9, 16 KW */

   /* Flash banks (ML-DSA sign code is large). */
   FLASH_BANK0      : origin = 0x080002, length = 0x01FFFE
   FLASH_BANK1      : origin = 0x0A0000, length = 0x020000
   FLASH_BANK2      : origin = 0x0C0000, length = 0x020000
   FLASH_BANK3      : origin = 0x0E0000, length = 0x020000
   FLASH_BANK4      : origin = 0x100000, length = 0x008000

   RESET            : origin = 0x3FFFC0, length = 0x000002
}

SECTIONS
{
   codestart        : > BEGIN
   .text            : >> FLASH_BANK0 | FLASH_BANK1 | FLASH_BANK2 | FLASH_BANK3, ALIGN(8)
   .cinit           : > FLASH_BANK0, ALIGN(8)
   .switch          : > FLASH_BANK0, ALIGN(8)
   .reset           : > RESET, TYPE = DSECT     /* unused */

   .stack           : > RAMLS_STACK

   .bss             : > RAMLS_HI
   .bss:output      : > RAMLS_HI
   .init_array      : > FLASH_BANK0, ALIGN(8)
   .const           : >> FLASH_BANK0 | FLASH_BANK1, ALIGN(8)
   .data            : > RAMLS_HI
   .sysmem          : > RAMGS_BIG

   .TI.ramfunc      : { sha3.obj(.text:BlockSha3) }
                         LOAD = FLASH_BANK0,
                         RUN  = RAMLS_RAMCODE,
                         LOAD_START(RamfuncsLoadStart),
                         LOAD_SIZE(RamfuncsLoadSize),
                         LOAD_END(RamfuncsLoadEnd),
                         RUN_START(RamfuncsRunStart),
                         RUN_SIZE(RamfuncsRunSize),
                         RUN_END(RamfuncsRunEnd),
                         ALIGN(8)
}
