#ifndef USER_H
#define USER_H
#include <stddef.h>
#include <utility_wolf.h>

extern int uefi_snprintf_wolfssl(char* buffer, size_t n, const char* format, ...);
#define XSNPRINTF uefi_snprintf_wolfssl
extern int uefi_printf_wolfssl(const char*, ...);
#define XPRINTF uefi_printf_wolfssl
extern void* uefi_memcpy_wolfssl(void* dest, const void* src, size_t len);
#define XMEMCPY uefi_memcpy_wolfssl
extern void* uefi_memset_wolfssl(void* str, int c, size_t n);
#define XMEMSET uefi_memset_wolfssl
extern int uefi_strncmp_wolfssl(const char* s1, const char* s2, size_t n);
#define XSTRNCMP uefi_strncmp_wolfssl
#define XFFLUSH uefi_wolfssl_fflush

#define XMALLOC XMALLOC
#define XFREE XFREE
#define XREALLOC XREALLOC
#define SINGLE_THREADED
#define NO_FILESYSTEM
#define STRING_USER
#define NO_DSA
#define NO_MD4

#define WOLFSSL_AES_CFB
#define HAVE_PKCS7
#define WC_RNG_SEED_CB
#undef HAVE_INTEL_RDRAND
#undef HAVE_INTEL_RDSEED
#undef USE_INTEL_SPEEDUP
#undef WOLFSSL_ASM

#define WOLFSSL_SHA3
#define WC_SHA3_NO_ASM
#define WC_MLKEM_NO_ASM
#define WOLFSSL_HAVE_MLKEM
#define HAVE_PBKDF2

/* ChaCha20 / Poly1305 */
#define HAVE_CHACHA
#define HAVE_POLY1305
#define HAVE_ONE_TIME_AUTH

/* Curve25519 / Ed25519 */
#define HAVE_CURVE25519
#define HAVE_ED25519

/* Curve448 / Ed448 */
#define HAVE_CURVE448
#define HAVE_ED448

/* Post-quantum: Dilithium (ML-DSA) — native wolfSSL implementation */
#define HAVE_DILITHIUM
#define WOLFSSL_WC_DILITHIUM
/* All levels enabled by default (use WOLFSSL_NO_ML_DSA_44/65/87 to disable) */

/* Falcon requires liboqs; omit unless liboqs is available */
/* #define HAVE_FALCON */

/* SHA-224 */
#define WOLFSSL_SHA224

#ifdef WOLFSSL_HAVE_MLKEM
    #define WOLFSSL_MLKEM_KYBER
    #define WOLFSSL_WC_MLKEM
    #define WC_MLKEM_NO_ASM
    #define WC_SHA3_NO_ASM
    #ifndef WOLFSSL_SHAKE128
        #define WOLFSSL_SHAKE128
    #endif
    #ifndef WOLFSSL_SHAKE256
        #define WOLFSSL_SHAKE256
    #endif
#else
    #define WOLFSSL_NO_SHAKE128
    #define WOLFSSL_NO_SHAKE256
#endif


#define XPRINTF uefi_printf_wolfssl
#define XFPRINTF uefi_fprintf_wolfssl
#define XSNPRINTF uefi_snprintf_wolfssl
#define XVSNPRINTF uefi_vsnprintf_wolfssl
#define XSTRNCMP uefi_strncmp_wolfssl
#define XSTRCMP uefi_strcmp_wolfssl
#define XSTRLEN uefi_strlen_wolfssl
#define XSTRNCPY uefi_strncpy_wolfssl
#define XSTRCASECMP uefi_strcasecmp_wolfssl
#define XSTRNCASECMP uefi_strncasecmp_wolfssl
#define XSTRNSTR uefi_strnstr_wolfssl
#define XSTRNCAT uefi_strncat_wolfssl
#define XMEMCMP uefi_memcmp_wolfssl
#define XMEMMOVE uefi_memmove_wolfssl


/* Only Matter for the test */
#define CERT_PREFIX "\\" /* Assume everything needs is at the root of device */
#define CERT_PATH_SEP "\\"

/* Debugging UEFI (for example code only not wolfSSL proper) */
#if 1
    #undef UEFI_VERBOSE_DEBUG
    #define UEFI_VERBOSE_DEBUG
#endif

/* AES-NI hardware acceleration (hw build only) */
#ifdef UEFI_HW_ACCEL
    #define WOLFSSL_AESNI
#endif


/* ------------------------------------------------------------------------- */
/* Platform */
/* ------------------------------------------------------------------------- */
#undef  WOLFSSL_GENERAL_ALIGNMENT
#define WOLFSSL_GENERAL_ALIGNMENT   4

#if 1
    #undef  SINGLE_THREADED
    #define SINGLE_THREADED
#else
    #define HAVE_THREAD_LS
#endif

#ifdef SINGLE_THREADED
    #undef  NO_THREAD_LS
    #define NO_THREAD_LS
#endif

#undef  WOLFSSL_USER_IO
#define WOLFSSL_USER_IO

#undef  WOLFSSL_SMALL_STACK
//#define WOLFSSL_SMALL_STACK



/* ------------------------------------------------------------------------- */
/* Math Configuration */
/* ------------------------------------------------------------------------- */

#undef  SIZEOF_LONG_LONG
#define SIZEOF_LONG_LONG 8


#undef  USE_FAST_MATH
#if 1
    #define USE_FAST_MATH
    #define FP_MAX_BITS 16384

    #undef  TFM_TIMING_RESISTANT
    #define TFM_TIMING_RESISTANT

#else
    #define WOLFSSL_SP_MATH_ALL
    #define WOLFSSL_SP_INT_NEGATIVE
    #define WOLFSSL_SP_SMALL      /* use smaller version of code */
    /* Maximum math bits (largest supported key bits) */
    #undef SP_INT_BITS
    #define SP_INT_BITS 8192
#endif



/* ------------------------------------------------------------------------- */
/* Crypto */
/* ------------------------------------------------------------------------- */
/* RSA */
#undef NO_RSA
#if 1

    /* half as much memory but twice as slow */
    #undef  RSA_LOW_MEM
    #define RSA_LOW_MEM

    /* Enables blinding mode, to prevent timing attacks */
    #if 1
        #undef  WC_RSA_BLINDING
        #define WC_RSA_BLINDING
    #else
        #undef  WC_NO_HARDEN
        #define WC_NO_HARDEN
    #endif

    /* RSA PSS Support */
    #if 1
        #undef WC_RSA_PSS
        #define WC_RSA_PSS

        #undef WOLFSSL_PSS_LONG_SALT
        #define WOLFSSL_PSS_LONG_SALT

        #undef WOLFSSL_PSS_SALT_LEN_DISCOVER
        #define WOLFSSL_PSS_SALT_LEN_DISCOVER
    #endif

    #if 1
        #define WC_RSA_NO_PADDING
    #endif

    #define WOLFSSL_KEY_GEN
    #define WOLFSSL_RSA_KEY_CHECK

#else
    #define NO_RSA
#endif


/* ECC */
#undef HAVE_ECC
#if 1
    #define HAVE_ECC

    /* Manually define enabled curves */
    #undef  ECC_USER_CURVES
    #define ECC_USER_CURVES

    #ifdef ECC_USER_CURVES
        /* Manual Curve Selection */
        #define HAVE_ECC192
        #define HAVE_ECC224
        #undef NO_ECC256
        #define HAVE_ECC256
        #define HAVE_ECC384
        #define HAVE_ECC521
    #endif

    /* Fixed point cache (speeds repeated operations against same private key) */
    #undef  FP_ECC
    //#define FP_ECC
    #ifdef FP_ECC
        /* Bits / Entries */
        #undef  FP_ENTRIES
        #define FP_ENTRIES  2
        #undef  FP_LUT
        #define FP_LUT      4
    #endif

    /* Optional ECC calculation method */
    /* Note: doubles heap usage, but slightly faster */
    #undef  ECC_SHAMIR
    //#define ECC_SHAMIR

    /* Reduces heap usage, but slower */
    #undef  ECC_TIMING_RESISTANT
    #define ECC_TIMING_RESISTANT

    /* Use alternate ECC size for ECC math */
    #ifdef USE_FAST_MATH
        #undef  ALT_ECC_SIZE
        #define ALT_ECC_SIZE

        /* Speedups specific to curve */
        #ifndef NO_ECC256
            #undef  TFM_ECC256
            #define TFM_ECC256
        #endif
    #endif
#endif

/* DH */
#undef  NO_DH
#if 1
    #define HAVE_DH
    /* Use table for DH instead of -lm (math) lib dependency */
    #if 1
        #define HAVE_DH_DEFAULT_PARAMS
        #define WOLFSSL_DH_CONST
        #define HAVE_FFDHE_2048
        #define HAVE_FFDHE_3072
        #define HAVE_FFDHE_4096
        #define HAVE_FFDHE_6144
        #define HAVE_FFDHE_8192
    #endif
#else
    #define NO_DH
#endif


/* AES */
#undef NO_AES
#if 1
    #undef  HAVE_AES_CBC
    #define HAVE_AES_CBC

    #undef  HAVE_AESGCM
    #define HAVE_AESGCM

    /* GCM Method (slowest to fastest): GCM_SMALL, GCM_WORD32, GCM_TABLE or
     *                                  GCM_TABLE_4BIT */
    #define GCM_TABLE_4BIT

    #undef  WOLFSSL_AES_DIRECT
    #define WOLFSSL_AES_DIRECT

    #undef  HAVE_AES_ECB
    #define HAVE_AES_ECB

    #undef  WOLFSSL_AES_COUNTER
    #define WOLFSSL_AES_COUNTER

    #undef  HAVE_AESCCM
    #define HAVE_AESCCM

    #undef WOLFSSL_AES_OFB
    #define WOLFSSL_AES_OFB

#else
    #define NO_AES
#endif

#undef NO_DES3
#define NO_DES3

/* ------------------------------------------------------------------------- */
/* Hashing */
/* ------------------------------------------------------------------------- */
/* Sha */
#undef NO_SHA
#if 1
    /* 1k smaller, but 25% slower */
    // #define USE_SLOW_SHA
#else
    #define NO_SHA
#endif

/* Sha256 */
#undef NO_SHA256
#if 1
    /* Sha224 */
    #if 1
        #define WOLFSSL_SHA224
    #endif
#else
    #define NO_SHA256
#endif

/* Sha512 */
#undef WOLFSSL_SHA512
#if 1
    #define WOLFSSL_SHA512

    /* Sha384 */
    #undef  WOLFSSL_SHA384
    #if 1
        #define WOLFSSL_SHA384
    #endif
#endif

/* MD5 */
#undef  NO_MD5
#define NO_MD5

/* HKDF / PRF */
#undef HAVE_HKDF
#if 1
    #define HAVE_HKDF
    #define WOLFSSL_HAVE_PRF
#endif

/* CMAC */
#undef WOLFSSL_CMAC
#if 1
    #define WOLFSSL_CMAC
#endif


/* ------------------------------------------------------------------------- */
/* RNG */
/* ------------------------------------------------------------------------- */

/* Choose RNG method */
#if 1
    /* Use built-in P-RNG (SHA256 based) with HW RNG */
    /* P-RNG + HW RNG (P-RNG is ~8K) */
    #undef  HAVE_HASHDRBG
    #define HAVE_HASHDRBG
#else
    #undef  WC_NO_HASHDRBG
    #define WC_NO_HASHDRBG
#endif

    /* Bypass P-RNG and use only HW RNG */
#if 1
    #define CUSTOM_RAND_TYPE      unsigned int
    extern int uefi_random_gen(char* output, unsigned int sz);
    #undef  CUSTOM_RAND_GENERATE_SEED
    #define CUSTOM_RAND_GENERATE_SEED  uefi_random_gen
#endif

/* ------------------------------------------------------------------------- */
/* Debugging */
/* ------------------------------------------------------------------------- */

#undef DEBUG_WOLFSSL
#undef NO_ERROR_STRINGS
#if 0
    #define DEBUG_WOLFSSL
#else
    #if 0
        #define NO_ERROR_STRINGS
    #endif
#endif

/* ------------------------------------------------------------------------- */
/* Enable features */
/* ------------------------------------------------------------------------- */
#define XMALLOC_USER
#define WOLFSSL_IGNORE_FILE_WARN

/* ------------------------------------------------------------------------- */
/* Disable features */
/* ------------------------------------------------------------------------- */

#define WOLFCRYPT_ONLY
#define NO_ASN_TIME
#define NO_OLD_TLS
#define NO_INLINE
#define NO_SIG_WRAPPER
#define NO_CRYPT_BENCHMARK
#define NO_MAIN_DRIVER

#endif /* USER_H */
