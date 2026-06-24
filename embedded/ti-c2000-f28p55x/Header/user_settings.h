/* user_settings.h
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* wolfSSL user settings for the TI C2000 C28x DSP (LAUNCHXL-F28P55X).
 *
 * IMPORTANT - this is a SCOPED build for a new architecture bring-up:
 *   - The C28x has 16-bit bytes (CHAR_BIT == 16).  wolfSSL's byte-packing
 *     code assumes 8-bit octets, so this build is deliberately limited to the
 *     algorithms whose correctness we validate with known-answer tests:
 *         SHA-256, SHA-384/512, SHA3, SHAKE128/256, and ML-DSA-87 verify only.
 *   - RSA / ECC / DH / TFM / SP-bignum are NOT built, which avoids the
 *     big-integer code paths most sensitive to the 16-bit-byte layout.
 *   - The device has no hardware TRNG and no SHA3 accelerator; everything here
 *     is portable C software crypto.  ML-DSA verify is deterministic and uses
 *     no entropy.  The RNG stub below is for benchmark/test wiring ONLY and is
 *     NOT cryptographically secure - do not ship it.
 */

#ifndef WOLFSSL_USER_SETTINGS_C2000_H
#define WOLFSSL_USER_SETTINGS_C2000_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Platform / C28x toolchain                                                 */
/* ------------------------------------------------------------------------- */
#undef  SINGLE_THREADED
#define SINGLE_THREADED

#undef  WOLFCRYPT_ONLY          /* crypto only, no TLS layer */
#define WOLFCRYPT_ONLY

#undef  NO_FILESYSTEM
#define NO_FILESYSTEM

#undef  NO_WRITEV
#define NO_WRITEV

#undef  NO_MAIN_DRIVER          /* our main() lives in wolf_main.c */
#define NO_MAIN_DRIVER

#undef  NO_DEV_RANDOM
#define NO_DEV_RANDOM

#undef  NO_SESSION_CACHE
#define NO_SESSION_CACHE

#undef  WOLFSSL_IGNORE_FILE_WARN
#define WOLFSSL_IGNORE_FILE_WARN

#undef  BENCH_EMBEDDED          /* reduced benchmark sizes for an MCU */
#define BENCH_EMBEDDED

/* C28x integer widths: char/short/int = 16-bit, long = 32-bit,
 * long long = 64-bit.  wolfCrypt needs a true 64-bit word for SHA3/SHA-512
 * and ML-DSA, which long long provides. */
#undef  SIZEOF_LONG
#define SIZEOF_LONG       4
#undef  SIZEOF_LONG_LONG
#define SIZEOF_LONG_LONG  8

/* Pull in the toolchain's <limits.h> so CHAR_BIT is the real value (16 on
 * C28x) instead of wolfSSL's 8-bit fallback, and so UINT_MAX reflects the
 * 16-bit int.  This is critical for correct byte math. */
#undef  HAVE_LIMITS_H
#define HAVE_LIMITS_H

/* C28x has 16-bit int.  WC_16BIT_CPU makes wolfSSL use the correct integer
 * widths (word16 = unsigned int = 16-bit, word32 = unsigned long = 32-bit);
 * without it wolfSSL assumes int is 32-bit and word32 comes out only 16 bits,
 * silently truncating all 32-bit crypto math.  C28x also has a native 64-bit
 * long long, so WORD64_AVAILABLE is preserved via the SIZEOF_LONG_LONG==8
 * guard added to types.h (for SHA-512/SHA-3/ML-DSA). */
#undef  WC_16BIT_CPU
#define WC_16BIT_CPU

/* The C28x is little-endian.  Leave BIG_ENDIAN_ORDER undefined so the default
 * little-endian path is used.  (Verify on hardware with the SHA KATs.) */

/* CHAR_BIT==16 portability levers (see README "16-bit byte" section):
 *   ML-DSA bit-packing has fast paths that cast a byte buffer to word32/word64
 *   (gated on WOLFSSL_MLDSA_ALIGNMENT <= 2/4/8).  On a 16-bit-byte machine a
 *   wordN spans N/2 cells, not N octets, so every such cast is wrong.  Force
 *   the fully portable byte-by-byte path by making the alignment exceed all
 *   the gates (max gate is <= 8).
 *   The Keccak state-aliasing hazard (xorbuf/memcpy over the word64 state) is
 *   handled by the gated octet-wise SHA3 path in sha3.c (WC_SHA3_BYTEWISE). */
#undef  WOLFSSL_GENERAL_ALIGNMENT
#define WOLFSSL_GENERAL_ALIGNMENT 2
#undef  WOLFSSL_MLDSA_ALIGNMENT
#define WOLFSSL_MLDSA_ALIGNMENT 16

/* Force portable C cores - no x86/ARM assembly. */
#undef  WOLFSSL_NO_ASM
#define WOLFSSL_NO_ASM
#undef  WC_SHA3_NO_ASM
#define WC_SHA3_NO_ASM

/* Compile misc.c as its own translation unit (extern, not inline).  The TI
 * CGT treats a plain `inline` definition as a C99 extern-inline and emits no
 * out-of-line copy, so the inline-misc path leaves xorbuf/ByteReverseWords64
 * unresolved at link.  NO_INLINE makes them ordinary extern functions; the
 * build compiles wolfcrypt/src/misc.c once. */
#undef  NO_INLINE
#define NO_INLINE

/* ------------------------------------------------------------------------- */
/* Hashing - the scoped subset                                               */
/* ------------------------------------------------------------------------- */
/* SHA-256/224 are on by default (NO_SHA256 not defined). */

#undef  WOLFSSL_SHA512
#define WOLFSSL_SHA512
#undef  WOLFSSL_SHA384
#define WOLFSSL_SHA384

#undef  WOLFSSL_SHA3
#define WOLFSSL_SHA3
#undef  WOLFSSL_SHAKE128
#define WOLFSSL_SHAKE128
#undef  WOLFSSL_SHAKE256
#define WOLFSSL_SHAKE256

/* ------------------------------------------------------------------------- */
/* ML-DSA-87 (Dilithium L5) - VERIFY ONLY                                    */
/* ------------------------------------------------------------------------- */
#undef  WOLFSSL_HAVE_MLDSA
#define WOLFSSL_HAVE_MLDSA

/* Build only ML-DSA-87 (level 5). */
#undef  WOLFSSL_NO_ML_DSA_44
#define WOLFSSL_NO_ML_DSA_44
#undef  WOLFSSL_NO_ML_DSA_65
#define WOLFSSL_NO_ML_DSA_65

/* Raw key/sig import - no ASN.1 (both modes). */
#undef  WOLFSSL_MLDSA_NO_ASN1
#define WOLFSSL_MLDSA_NO_ASN1

#ifdef WOLF_MLDSA_SIGN
/* Sign build also exercises the ML-DSA-44/65 parameter sets (smaller than 87)
 * via keygen+sign+verify round-trips. */
#undef  WOLFSSL_NO_ML_DSA_44
#undef  WOLFSSL_NO_ML_DSA_65
/* Full ML-DSA-87 (keygen + sign + verify).  Build with `make SIGN=1`.  ML-DSA-87
 * keygen/sign/verify need a large working set; enabling EVERY small-memory path
 * brings the peak heap to ~30 KW (measured on host), which fits the C28x when
 * the sign linker script gives .sysmem the RAMGS0-3 block.  NOTE: the dev RNG
 * stub below is NOT secure - real signing needs a true TRNG. */
#undef  WOLFSSL_MLDSA_MAKE_KEY_SMALL_MEM
#define WOLFSSL_MLDSA_MAKE_KEY_SMALL_MEM
#undef  WOLFSSL_MLDSA_SIGN_SMALL_MEM
#define WOLFSSL_MLDSA_SIGN_SMALL_MEM
#undef  WOLFSSL_MLDSA_VERIFY_SMALL_MEM
#define WOLFSSL_MLDSA_VERIFY_SMALL_MEM
#undef  WOLFSSL_MLDSA_SMALL_MEM_POLY64
#define WOLFSSL_MLDSA_SMALL_MEM_POLY64
/* ML-DSA-87 sign/keygen put large polynomial arrays on the stack by default,
 * which overflows the C28x's low-RAM (< 0x10000) stack.  WOLFSSL_SMALL_STACK
 * moves those to the heap (sized by the sign linker for ~31 KW peak). */
#undef  WOLFSSL_SMALL_STACK
#define WOLFSSL_SMALL_STACK
#else
/* Verify path only (auto-derives NO_MAKE_KEY + NO_SIGN + public-key-only),
 * with the small-memory streaming verifier and no heap (buffers pinned into
 * the key struct). */
#undef  WOLFSSL_MLDSA_VERIFY_ONLY
#define WOLFSSL_MLDSA_VERIFY_ONLY
#undef  WOLFSSL_MLDSA_VERIFY_SMALL_MEM
#define WOLFSSL_MLDSA_VERIFY_SMALL_MEM
#undef  WOLFSSL_MLDSA_VERIFY_NO_MALLOC
#define WOLFSSL_MLDSA_VERIFY_NO_MALLOC
/* Smallest verify RAM: stream the signature's z vector one polynomial at a
 * time instead of pinning the whole l-vector (saves ~6 KB on ML-DSA-87).
 * Measured on F28P55x: sizeof(wc_MlDsaKey) 20,048 -> 13,904 bytes. */
#undef  WOLFSSL_MLDSA_VERIFY_SMALLEST_MEM
#define WOLFSSL_MLDSA_VERIFY_SMALLEST_MEM
/* Optional on this part: also define WOLFSSL_MLDSA_ASSIGN_KEY to keep the
 * public key in flash (by reference) instead of copying it into the key
 * struct - that removes a further ~5 KB of RAM (the 2,592-octet public key is
 * a 5,184-byte byte[] on the 16-bit-cell C28x), bringing ML-DSA-87 verify to
 * ~10.7 KB total RAM.  Left off by default because the caller must keep the
 * public-key buffer valid for the key's lifetime. */
#endif

/* ------------------------------------------------------------------------- */
/* Disabled: big-integer / asymmetric (keeps TFM/ECC/integer out of build)   */
/* ------------------------------------------------------------------------- */
#ifdef WOLF_ECC
/* ------------------------------------------------------------------------- */
/* ECC P-256 (SECP256R1) via SP single-precision math (sp_c32.c), no malloc.  */
/* Enabled with EXTRA_CFLAGS=--define=WOLF_ECC (ECC=1 build).  Covers ECDSA    */
/* keygen/sign/verify and ECDH.  Uses raw mp_int (sign/verify _ex) APIs so no  */
/* ASN/DER is required for bring-up.                                           */
/* ------------------------------------------------------------------------- */
#undef  HAVE_ECC
#define HAVE_ECC
#undef  ECC_USER_CURVES        /* build only the curves we name (just P-256) */
#define ECC_USER_CURVES
#undef  HAVE_ECC256
#define HAVE_ECC256
#undef  HAVE_ECC_VERIFY
#define HAVE_ECC_VERIFY
#undef  HAVE_ECC_SIGN
#define HAVE_ECC_SIGN
#undef  HAVE_ECC_DHE           /* ECDH shared secret */
#define HAVE_ECC_DHE
#undef  ECC_TIMING_RESISTANT
#define ECC_TIMING_RESISTANT
/* SP single-precision P-256 backend (32-bit limbs in sp_c32.c). */
#undef  WOLFSSL_SP_MATH
#define WOLFSSL_SP_MATH
#undef  WOLFSSL_HAVE_SP_ECC
#define WOLFSSL_HAVE_SP_ECC
#undef  WOLFSSL_SP_NO_MALLOC
#define WOLFSSL_SP_NO_MALLOC
#undef  WOLFSSL_SP_SMALL
#define WOLFSSL_SP_SMALL
#undef  SP_WORD_SIZE
#define SP_WORD_SIZE 32
/* C28x int is 16-bit (WC_16BIT_CPU) but we use 32-bit SP digits + octet-wise
 * byte I/O, so opt past the conservative SP-vs-16-bit guard. */
#undef  WOLFSSL_SP_ALLOW_16BIT_CPU
#define WOLFSSL_SP_ALLOW_16BIT_CPU
/* NO_BIG_INT stays off: SP provides the mp_int interface. */
#else
/* No multi-precision integer math at all (no SP/TFM/integer.c).  ML-DSA verify
 * and the hashes use no mp_int, and this avoids the SP-math vs WC_16BIT_CPU
 * conflict. */
#undef  NO_BIG_INT
#define NO_BIG_INT
#endif /* WOLF_ECC */

#undef  NO_RSA
#define NO_RSA
#undef  NO_DH
#define NO_DH
#undef  NO_DSA
#define NO_DSA
#ifndef WOLF_ECC
/* HAVE_ECC intentionally not defined - no ECC. */
#endif
#ifdef WOLF_MLKEM
/* ML-KEM (FIPS 203) via the wolfCrypt C implementation.  Build only the
 * ML-KEM-768 parameter set, force the generic C path (no x86/ARM asm), and use
 * the small-memory / no-malloc options so it fits the bare-metal RAM budget.
 * Validated on the C28x by test.c's mlkem_test KAT. */
#undef  WOLFSSL_HAVE_MLKEM
#define WOLFSSL_HAVE_MLKEM
#undef  WOLFSSL_WC_MLKEM
#define WOLFSSL_WC_MLKEM
/* All three ML-KEM parameter sets (512/768/1024); each validated by a
 * self-consistent encap/decap round-trip on hardware. */
#undef  WOLFSSL_NO_ML_KEM_512
#undef  WOLFSSL_NO_ML_KEM_1024
#undef  WC_MLKEM_NO_ASM
#define WC_MLKEM_NO_ASM
#undef  WOLFSSL_MLKEM_NO_MALLOC
#define WOLFSSL_MLKEM_NO_MALLOC
#undef  WOLFSSL_MLKEM_MAKEKEY_SMALL_MEM
#define WOLFSSL_MLKEM_MAKEKEY_SMALL_MEM
#undef  WOLFSSL_MLKEM_ENCAPSULATE_SMALL_MEM
#define WOLFSSL_MLKEM_ENCAPSULATE_SMALL_MEM
#else
#undef  WOLFSSL_HAVE_MLKEM     /* no ML-KEM unless MLKEM=1 */
#endif
#undef  NO_ASN
#define NO_ASN                 /* ML-DSA + ECC raw import; no asn.c needed */
#undef  NO_CERTS
#define NO_CERTS
#undef  NO_PWDBASED
#define NO_PWDBASED
#undef  NO_PKCS7
#define NO_PKCS7
#undef  NO_PKCS12
#define NO_PKCS12
#undef  NO_SIG_WRAPPER
#define NO_SIG_WRAPPER

/* ------------------------------------------------------------------------- */
/* Disabled: unused symmetric / legacy (shrink the first build)              */
/* ------------------------------------------------------------------------- */
#ifdef WOLF_AES
/* AES software (table-driven) with CBC, CTR, CFB and GCM.  GCM_SMALL selects
 * the byte-wise GHASH (cleanest on a 16-bit-byte target; no word64 needed). */
#undef  NO_AES
#undef  HAVE_AES_CBC
#define HAVE_AES_CBC
#undef  HAVE_AES_DECRYPT
#define HAVE_AES_DECRYPT
#undef  WOLFSSL_AES_COUNTER
#define WOLFSSL_AES_COUNTER
#undef  WOLFSSL_AES_CFB
#define WOLFSSL_AES_CFB
#undef  HAVE_AESGCM
#define HAVE_AESGCM
#undef  GCM_SMALL
#define GCM_SMALL
#undef  WOLFSSL_AES_DIRECT
#define WOLFSSL_AES_DIRECT
#undef  WOLFSSL_AES_128
#define WOLFSSL_AES_128
#undef  WOLFSSL_AES_192
#define WOLFSSL_AES_192
#undef  WOLFSSL_AES_256
#define WOLFSSL_AES_256
#else
#undef  NO_AES                 /* AES enabled only with AES=1 */
#define NO_AES
#endif

/* Curve25519 (X25519) + Ed25519.  Enabled with EXTRA_CFLAGS=--define=WOLF_25519
 * (X25519=1 build).  No __uint128_t and no SP-25519 backend on C28x, so the
 * default fe[10] 32-bit-limb field arithmetic is used; Ed25519 reuses the
 * already-enabled SHA-512. */
#ifdef WOLF_25519
#undef  HAVE_CURVE25519
#define HAVE_CURVE25519
#undef  HAVE_CURVE25519_KEY_IMPORT
#define HAVE_CURVE25519_KEY_IMPORT
#undef  HAVE_CURVE25519_KEY_EXPORT
#define HAVE_CURVE25519_KEY_EXPORT
#undef  HAVE_ED25519
#define HAVE_ED25519
#undef  HAVE_ED25519_KEY_IMPORT
#define HAVE_ED25519_KEY_IMPORT
#undef  HAVE_ED25519_KEY_EXPORT
#define HAVE_ED25519_KEY_EXPORT
#endif

/* Curve448 (X448) + Ed448.  Enabled with EXTRA_CFLAGS=--define=WOLF_448
 * (X448=1 build).  The default fe_448 field backend needs __uint128_t for
 * sc448 (scalar mod-order arithmetic), which the C28x toolchain lacks, so the
 * CURVE448_SMALL / ED448_SMALL byte-array backends are selected instead.  They
 * are octet-correct on CHAR_BIT != 8 (carry-stores masked with WC_OCTET).
 * Ed448 reuses the already-enabled SHAKE256. */
#ifdef WOLF_448
#undef  HAVE_CURVE448
#define HAVE_CURVE448
#undef  CURVE448_SMALL
#define CURVE448_SMALL
#undef  HAVE_CURVE448_KEY_IMPORT
#define HAVE_CURVE448_KEY_IMPORT
#undef  HAVE_CURVE448_KEY_EXPORT
#define HAVE_CURVE448_KEY_EXPORT
#undef  HAVE_ED448
#define HAVE_ED448
#undef  ED448_SMALL
#define ED448_SMALL
#undef  HAVE_ED448_KEY_IMPORT
#define HAVE_ED448_KEY_IMPORT
#undef  HAVE_ED448_KEY_EXPORT
#define HAVE_ED448_KEY_EXPORT
#endif

#undef  NO_DES3
#define NO_DES3
#undef  NO_RC4
#define NO_RC4
#undef  NO_MD4
#define NO_MD4
#undef  NO_MD5
#define NO_MD5
#undef  NO_SHA                 /* SHA-1 not in scope */
#define NO_SHA
/* HMAC + HKDF (RFC 2104 / RFC 5869) on top of the SHA-2/SHA-3 hashes.
 * Enabled with EXTRA_CFLAGS=--define=WOLF_HKDF (HKDF=1 build). */
#ifdef WOLF_HKDF
#undef  NO_HMAC
#undef  HAVE_HKDF
#define HAVE_HKDF
#else
#undef  NO_HMAC                /* relax if the harness needs it */
#define NO_HMAC
#endif

/* ChaCha20-Poly1305 AEAD (RFC 8439).  Enabled with --define=WOLF_CHACHA. */
#ifdef WOLF_CHACHA
#undef  HAVE_CHACHA
#define HAVE_CHACHA
#undef  HAVE_POLY1305
#define HAVE_POLY1305
#endif

/* AES-CCM / CMAC / GMAC on top of the AES core (--define=WOLF_AESEXTRA). */
#ifdef WOLF_AESEXTRA
#undef  HAVE_AESCCM
#define HAVE_AESCCM
#undef  WOLFSSL_CMAC
#define WOLFSSL_CMAC
#undef  HAVE_AESGCM            /* GMAC is GCM with no plaintext */
#define HAVE_AESGCM
#undef  GCM_SMALL
#define GCM_SMALL
/* Extra AES modes (all in aes.c; SIV/EAX build on CMAC). */
#undef  WOLFSSL_AES_XTS
#define WOLFSSL_AES_XTS
#undef  WOLFSSL_AES_OFB
#define WOLFSSL_AES_OFB
#undef  WOLFSSL_AES_SIV
#define WOLFSSL_AES_SIV
#undef  WOLFSSL_AES_EAX
#define WOLFSSL_AES_EAX
#endif

/* RSA verify via the SP math backend (--define=WOLF_RSA). */
#ifdef WOLF_RSA
#undef  NO_RSA
#undef  NO_BIG_INT             /* SP provides the mp_int interface (was off) */
#undef  SP_WORD_SIZE
#define SP_WORD_SIZE 32        /* 32-bit SP digits on the 16-bit-int C28x */
#undef  WOLFSSL_SP_MATH
#define WOLFSSL_SP_MATH
#undef  WOLFSSL_SP_MATH_ALL
#undef  WOLFSSL_HAVE_SP_RSA
#define WOLFSSL_HAVE_SP_RSA
#undef  WOLFSSL_SP_NO_2048     /* RSA-2048 verify only (KAT is 2048-bit) */
#undef  WOLFSSL_SP_NO_3072
#define WOLFSSL_SP_NO_3072
#undef  WOLFSSL_SP_4096
#undef  WOLFSSL_SP_ALLOW_16BIT_CPU
#define WOLFSSL_SP_ALLOW_16BIT_CPU
#undef  WOLFSSL_SP_NO_MALLOC
#define WOLFSSL_SP_NO_MALLOC
#undef  WC_RSA_BLINDING        /* verify only; no blinding RNG needed */
#undef  WOLFSSL_RSA_VERIFY_ONLY
#define WOLFSSL_RSA_VERIFY_ONLY
#undef  WOLFSSL_RSA_PUBLIC_ONLY
#define WOLFSSL_RSA_PUBLIC_ONLY
#else
#undef  NO_RSA
#define NO_RSA
#endif

/* DH (FFDHE-2048) via the SP math backend (--define=WOLF_DH).  Shares the SP
 * config with WOLF_RSA; uses the same sp_int.c/sp_c32.c 2048-bit modexp. */
#ifdef WOLF_DH
#undef  NO_DH
#undef  NO_BIG_INT
#undef  SP_WORD_SIZE
#define SP_WORD_SIZE 32
#undef  WOLFSSL_SP_MATH
#define WOLFSSL_SP_MATH
#undef  WOLFSSL_HAVE_SP_DH
#define WOLFSSL_HAVE_SP_DH
#undef  WOLFSSL_SP_NO_3072      /* FFDHE-2048 only */
#define WOLFSSL_SP_NO_3072
#undef  WOLFSSL_SP_4096
#undef  WOLFSSL_SP_ALLOW_16BIT_CPU
#define WOLFSSL_SP_ALLOW_16BIT_CPU
#undef  WOLFSSL_SP_NO_MALLOC
#define WOLFSSL_SP_NO_MALLOC
#undef  HAVE_FFDHE_2048
#define HAVE_FFDHE_2048
#endif

/* SHA-1 (legacy) - octet-correct on CHAR_BIT!=8 via the same WIDE_BYTE I/O as
 * SHA-2 (--define=WOLF_SHA1). */
#ifdef WOLF_SHA1
#undef  NO_SHA
#endif

/* RSA-2048 sign (private key, SP CRT path) - --define=WOLF_RSASIGN.  Mutually
 * exclusive with the verify-only WOLF_RSA build.  Raw key import keeps it
 * ASN-free.  Deterministic PKCS#1 v1.5 sign (no blinding -> no RNG). */
#ifdef WOLF_RSASIGN
#undef  NO_RSA
#undef  NO_BIG_INT
#undef  SP_WORD_SIZE
#define SP_WORD_SIZE 32
#undef  WOLFSSL_SP_MATH
#define WOLFSSL_SP_MATH
#undef  WOLFSSL_HAVE_SP_RSA
#define WOLFSSL_HAVE_SP_RSA
#undef  WOLFSSL_SP_NO_3072
#define WOLFSSL_SP_NO_3072
#undef  WOLFSSL_SP_4096
#undef  WOLFSSL_SP_ALLOW_16BIT_CPU
#define WOLFSSL_SP_ALLOW_16BIT_CPU
#undef  WOLFSSL_SP_NO_MALLOC
#define WOLFSSL_SP_NO_MALLOC
#undef  WC_RSA_BLINDING
#endif

/* ------------------------------------------------------------------------- */
/* Memory                                                                    */
/* ------------------------------------------------------------------------- */
/* Use the TI runtime heap (malloc/free) for test/benchmark temporaries.
 * ML-DSA verify itself allocates nothing (WOLFSSL_MLDSA_VERIFY_NO_MALLOC).
 * NO_WOLFSSL_MEMORY routes XMALLOC straight to malloc/free with no wrapper.
 *
 * WOLF_MEM_PROFILE (MEMPROF=1) instead leaves NO_WOLFSSL_MEMORY undefined so
 * settings.h enables USE_WOLFSSL_MEMORY; wolf_main.c then installs a heap
 * high-water tracker via wolfSSL_SetAllocators().  It also turns on the stack
 * high-water paint.  Measurement build only. */
#ifdef WOLF_MEM_PROFILE
    #undef  WOLF_STACK_PROFILE
    #define WOLF_STACK_PROFILE
#else
    #undef  NO_WOLFSSL_MEMORY
    #define NO_WOLFSSL_MEMORY
#endif

/* ------------------------------------------------------------------------- */
/* Time - benchmark elapsed time comes from a CPU timer (wolf_main.c)        */
/* ------------------------------------------------------------------------- */
#undef  NO_ASN_TIME
#define NO_ASN_TIME

#undef  WOLFSSL_USER_CURRTIME   /* benchmark.c uses our current_time() */
#define WOLFSSL_USER_CURRTIME

/* XTIME stub - hashing and ML-DSA verify never use wall-clock time.  Define
 * XTIME directly (not TIME_OVERRIDES) so wolfSSL leaves ours in place and
 * defaults XGMTIME on its own (nothing in this scoped build calls XGMTIME). */
extern long my_time(long* t);
#undef  XTIME
#define XTIME(tl)  my_time((tl))

/* ------------------------------------------------------------------------- */
/* RNG - real SHA-256 Hash-DRBG seeded by a DEV-ONLY test seed                */
/* ------------------------------------------------------------------------- */
/* The F28P550SJ has no hardware TRNG, so there is no real entropy source.
 * WOLFSSL_GENSEED_FORTEST makes random.c supply a built-in wc_GenerateSeed
 * (an incrementing test value) that feeds the standard SHA-256 Hash-DRBG.
 * This exercises the real DRBG code path (what a production build with a TRNG
 * would use) and lets random_test pass - but the seed is NOT random, so this
 * is DEV/TEST ONLY and MUST NOT be shipped.  Replace wc_GenerateSeed with a
 * real TRNG before any production use. */
#undef  WOLFSSL_GENSEED_FORTEST
#define WOLFSSL_GENSEED_FORTEST

/* Run every self-test to completion and report each, so macro_test (a 16-bit
 * safe-math self-test that currently fails on C28x) does not abort the suite
 * before the SHA/SHAKE/ML-DSA KATs run. */
#undef  TEST_ALWAYS_RUN_TO_END
#define TEST_ALWAYS_RUN_TO_END

#ifdef __cplusplus
}
#endif

#endif /* WOLFSSL_USER_SETTINGS_C2000_H */
