/* user_settings.h -- wolfCrypt config for the AmebaPro2 (RTL8735B)
 * wolfcrypt_test + wolfcrypt_benchmark app, built inside the RealTek FreeRTOS
 * SDK.
 *
 * Three build modes select the crypto backend, chosen with -DRTL_BENCH_MODE=N
 * at configure time (see wolfcrypt_test.cmake), so the same sources produce all
 * three of the benchmarks documented in the port README:
 *
 *   RTL_BENCH_MODE = 1  pure C        portable big-integer + C symmetric/hash
 *                                     (no assembly): the reference baseline.
 *   RTL_BENCH_MODE = 2  Thumb-2 / SP  Cortex-M assembly: sp_cortexm.c for the
 *                       Cortex-M      public key math + thumb2-*-asm symmetric
 *                                     and hash. (default)
 *   RTL_BENCH_MODE = 3  RealTek HW    the HUK crypto-callback device over the
 *                                     silicon engine (hal_crypto / hal_ecdsa):
 *                                     AES, HMAC-SHA256 and ECDSA P-256
 *                                     sign/verify run in hardware. Set globally
 *                                     via WC_USE_DEVID; main.c registers the
 *                                     device. Mode 3 runs the benchmark only --
 *                                     the HUK device derives its key from the
 *                                     "key" bytes (HKDF seed), so the standard
 *                                     wolfcrypt_test known-answer vectors do not
 *                                     apply to it. */

#ifndef AMEBAPRO2_TEST_USER_SETTINGS_H
#define AMEBAPRO2_TEST_USER_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RTL_BENCH_MODE
    #define RTL_BENCH_MODE 2
#endif

/* ---- platform / RTOS ---- */
#define WOLFSSL_GENERAL_ALIGNMENT 4
#define SIZEOF_LONG_LONG 8
#define SINGLE_THREADED
#define NO_FILESYSTEM
#define NO_WRITEV
#define NO_MAIN_DRIVER             /* we call wolfcrypt_test/benchmark_test */
#define WOLFCRYPT_ONLY             /* no TLS layer */
#define WOLFSSL_SMALL_STACK
#define WOLFSSL_IGNORE_FILE_WARN

/* ---- single-precision math backend ----
 * Modes 1 and 3 use only the generic sp_int.c big-integer math (portable C, no
 * per-size fast paths). Mode 2 adds the Cortex-M assembly specific paths
 * (sp_cortexm.c: sp_RsaPublic_2048, sp_ecc_sign_256, ...) for RSA-2048/3072,
 * ECC P-256 and DH-2048; WOLFSSL_HAVE_SP_* select those and so only apply in
 * mode 2 (they need an SP-specific provider, which sp_int.c is not). */
#define WOLFSSL_SP_MATH_ALL
#define SP_WORD_SIZE 32
#if RTL_BENCH_MODE == 2
    #define WOLFSSL_HAVE_SP_RSA
    #define WOLFSSL_HAVE_SP_ECC
    #define WOLFSSL_HAVE_SP_DH
    /* Thumb-2/DSP UMAAL single-precision; sizes without an asm path fall back
     * to the generic sp_int.c. */
    #define WOLFSSL_SP_ARM_CORTEX_M_ASM
#endif

/* ---- Thumb-2 assembly for AES / SHA / ChaCha / Poly1305 (mode 2 only) ----
 * Cortex-M33 is ARMv8-M but runs the Thumb-2 (ARMv7-style, no NEON, no ARMv8-A
 * crypto extension) asm. WOLFSSL_ARMASM is a global switch, so every symmetric
 * module it covers gets its thumb2-*-asm.S in the source list. */
#if RTL_BENCH_MODE == 2
    #define WOLFSSL_ARMASM
    #define WOLFSSL_ARMASM_THUMB2
    #define WOLFSSL_ARMASM_NO_HW_CRYPTO
    #define WOLFSSL_ARMASM_NO_NEON
    #define WOLFSSL_ARM_ARCH 7
#endif

/* ---- mode 3: RealTek HUK crypto-callback device over the silicon engine ---- */
#if RTL_BENCH_MODE == 3
    #define WOLFSSL_RTL8735B_HUK
    #define WOLF_CRYPTO_CB
    /* The HUK device id (the port header honors a pre-defined WC_HUK_DEVID).
     * Route every benchmark op through it by default; ops the device does not
     * accelerate fall back to software automatically. Defined numerically here
     * to avoid pulling the port header into settings.h include order. */
    #define WC_HUK_DEVID 810
    #define WC_USE_DEVID  WC_HUK_DEVID
#endif

/* ---- RNG / DRBG (seeded from the SDK TRNG, see main.c) ---- */
#define HAVE_HASHDRBG
#define CUSTOM_RAND_GENERATE_SEED  amebapro2_rand_seed
#ifndef __ASSEMBLER__
    #include <stddef.h>
    int amebapro2_rand_seed(unsigned char* output, unsigned int sz);
#endif

/* ---- AES (all modes) ---- */
#define HAVE_AES_CBC
#define HAVE_AESGCM
#define GCM_TABLE_4BIT
#define HAVE_AESCCM
#define HAVE_AES_ECB
#define WOLFSSL_AES_DIRECT
#define WOLFSSL_AES_COUNTER
#define WOLFSSL_AES_128
#define WOLFSSL_AES_192
#define WOLFSSL_AES_256
#define WOLFSSL_CMAC

/* ---- ChaCha20 / Poly1305 ---- */
#define HAVE_CHACHA
#define HAVE_POLY1305

/* ---- hashing ---- */
#define WOLFSSL_SHA224
#define WOLFSSL_SHA384
#define WOLFSSL_SHA512
#define WOLFSSL_SHA3
#define HAVE_HKDF

/* ---- public key: ECC (P-256/P-384), Curve/Ed25519, RSA, DH ---- */
#define HAVE_ECC
#define ECC_USER_CURVES
#define HAVE_ECC256
#define HAVE_ECC384
#define ECC_SHAMIR
#define HAVE_ECC_SIGN
#define HAVE_ECC_VERIFY
#define HAVE_ECC_DHE
#define WOLFSSL_VALIDATE_ECC_IMPORT

#define WC_RSA_BLINDING
#define WOLFSSL_KEY_GEN            /* RSA/ECC key generation tests */

#define HAVE_DH
#define HAVE_FFDHE_2048
#define WOLFSSL_DH_CONST           /* avoid pow/log in DH param checks */

/* ---- ASN + in-memory test certs/keys (no filesystem) ---- */
#define USE_CERT_BUFFERS_2048
#define USE_CERT_BUFFERS_256

/* ---- benchmark: embedded sizes, no %f (ROM printf has no float) ---- */
#define BENCH_EMBEDDED
#define WOLFSSL_NO_FLOAT_FMT
#define WOLFSSL_USER_CURRTIME      /* current_time() provided in main.c */

/* ---- fixed wall-clock for X.509 date validation (EVB has no RTC set) ---- */
#ifndef __ASSEMBLER__
    #include <time.h>
    time_t amebapro2_time(time_t* t);
    #define XTIME(tl) amebapro2_time((tl))
#endif

/* ---- trims ---- */
#define NO_DES3
#define NO_RC4
#define NO_MD4
#define NO_MD5
#define NO_DSA
#define NO_PWDBASED
#define NO_PKCS12
#define NO_OLD_TLS
#define WC_NO_HARDEN

#ifdef __cplusplus
}
#endif

#endif /* AMEBAPRO2_TEST_USER_SETTINGS_H */
