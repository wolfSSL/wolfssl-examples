/* 28p55x_wolf_flash_lnk.cmd
 *
 * Linker command file for the wolfCrypt test/benchmark on the
 * TI LAUNCHXL-F28P55X (TMS320F28P550SJ), running from flash.  EABI only
 * (build driverlib and this project with --abi=eabi).
 *
 * Differs from the stock C2000Ware generic flash linker in three ways that
 * the ML-DSA-87 verify path requires:
 *   - A large STACK in low RAM.  The C28x stack pointer is 16-bit, so the
 *     stack must live below 0x10000.  RAMLS0-7 (0x8000..0xBFFF, 16 KW) is the
 *     largest contiguous low block; the verify key struct (~7 KW pinned by
 *     WOLFSSL_MLDSA_VERIFY_NO_MALLOC) plus Keccak frames live here.
 *   - A real heap (.sysmem) in low RAMGS0-1 for the test/benchmark malloc
 *     temporaries (ML-DSA verify itself allocates nothing).
 *   - .bss/.data placed in the high RAMGS2-3 + RAMLS8-9 blocks so they do not
 *     compete with the stack for low RAM.
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.  GPLv3 - see project headers.
 */

-stack 0x4000   /* 16 KW C28x stack (full RAMLS0-7); wolfcrypt_test's deep call
                 * chain + SHAKE/large-hash locals (~1.6 KW) need the headroom */
-heap  0x2000   /* 8 KW heap (RAMGS1); RAMGS0 now hosts RAM-resident hot code.
                 * ML-DSA verify uses no heap; test/benchmark temporaries fit. */

MEMORY
{
   BEGIN            : origin = 0x080000, length = 0x000002

   BOOT_RSVD        : origin = 0x000002, length = 0x000126  /* M0, used by boot ROM stack */
   RAMM0            : origin = 0x000128, length = 0x0002D8
   RAMM1            : origin = 0x000400, length = 0x000400

   /* Low RAM (< 0x10000): usable for the 16-bit-SP stack and the heap. */
   RAMLS_STACK      : origin = 0x008000, length = 0x004000  /* RAMLS0-7, 16 KW */
   /* RAMGS0 hosts RAM-resident hot code (.TI.ramfunc), RAMGS1 the heap.  At
    * 150 MHz the flash has wait states; running the Keccak permutation and the
    * ML-DSA NTTs from RAM removes the instruction-fetch stalls. */
   RAMGS_RAMCODE    : origin = 0x00C000, length = 0x002000  /* RAMGS0, 8 KW */
   RAMGS_HEAP       : origin = 0x00E000, length = 0x002000  /* RAMGS1, 8 KW */

   /* High RAM (>= 0x10000): data only (no stack). */
   RAMGS_HI         : origin = 0x010000, length = 0x004000  /* RAMGS2-3, 16 KW */
   RAMLS_HI         : origin = 0x014000, length = 0x004000  /* RAMLS8-9, 16 KW */

   /* Flash banks (ML-DSA verify code + const test vectors are large). */
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
   /* Hot crypto relocated to RAM (loaded from flash, copied by Device_init's
    * Ramfuncs memcpy).  Listed before .text so these subsections are claimed
    * here, not by the generic .text below.  --gen_func_subsections=on makes
    * each function its own .text:<name> input section. */
   .TI.ramfunc      : { sha3.obj(.text:BlockSha3)
                        wc_mldsa.obj(.text:mldsa_ntt_c)
                        wc_mldsa.obj(.text:mldsa_invntt_c)
                        wc_mldsa.obj(.text:mldsa_ntt_small_c) }
                        LOAD = FLASH_BANK0,
                        RUN  = RAMGS_RAMCODE,
                        LOAD_START(RamfuncsLoadStart),
                        LOAD_SIZE(RamfuncsLoadSize),
                        LOAD_END(RamfuncsLoadEnd),
                        RUN_START(RamfuncsRunStart),
                        RUN_SIZE(RamfuncsRunSize),
                        RUN_END(RamfuncsRunEnd),
                        ALIGN(8)
   .text            : >> FLASH_BANK0 | FLASH_BANK1 | FLASH_BANK2 | FLASH_BANK3 | FLASH_BANK4, ALIGN(8)
   .cinit           : > FLASH_BANK0 | FLASH_BANK1 | FLASH_BANK2 | FLASH_BANK3 | FLASH_BANK4, ALIGN(8)
   .switch          : > FLASH_BANK0 | FLASH_BANK1 | FLASH_BANK2 | FLASH_BANK3 | FLASH_BANK4, ALIGN(8)
   .reset           : > RESET, TYPE = DSECT     /* unused */

   .stack           : > RAMLS_STACK

   .bss             : >> RAMGS_HI | RAMLS_HI
   .bss:output      : > RAMGS_HI
   .init_array      : > FLASH_BANK0 | FLASH_BANK1 | FLASH_BANK2 | FLASH_BANK3 | FLASH_BANK4, ALIGN(8)
   .const           : >> FLASH_BANK0 | FLASH_BANK1 | FLASH_BANK2 | FLASH_BANK3 | FLASH_BANK4, ALIGN(8)
   .data            : >> RAMGS_HI | RAMLS_HI
   .sysmem          : > RAMGS_HEAP
}
