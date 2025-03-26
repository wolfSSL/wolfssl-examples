#ifndef USER_H
#define USER_H

/* Need for size_t type */
#include <stddef.h>

#define WOLFCRYPT_ONLY
//#define HAVE_ENTROPY_MEMUSE
//#define ENTROPY_MEMUSE_FORCE_FAILURE
#define USE_FAST_MATH
#define WOLFSSL_SHA3
#define HAVE_HASHDRBG
#define SINGLE_THREADED
#define XMALLOC_USER
#define NO_ASN_TIME
//#define NO_FILESYSTEM
//#define NO_INLINE
#define WOLFSSL_USE_ALIGN
#define NO_SIG_WRAPPER
#define NO_PWDBASED
#define WOLFSSL_DH_CONST /* No pow or log function avaliable */
#define NO_ERROR_STRINGS
#define WOLFSSL_TLS13
#if 1
    #define WOLFSSL_AESNI
#endif
/* only in example code not wolfssl proper */
#define WOLFSSL_NEED_DYNAMIC_TYPE_FIX_UEFI
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

#if 1
    #define printf uefi_printf_wolfssl
    #define fprintf uefi_fprintf_wolfssl
    #define strerr uefi_strerr
    #define vsnprintf uefi_vsnprintf_wolfssl
    #define vprintf uefi_vprintf_wolfssl
    #define snprintf uefi_snprintf_wolfssl
    #define malloc uefi_malloc_wolfssl
    #define free uefi_free_wolfssl
    #define realloc uefi_realloc_wolfssl
    #define memcpy uefi_memcpy_wolfssl
    #define memset uefi_memset_wolfssl
    #define strncmp uefi_strncmp_wolfssl
    #define fflush uefi_wolfssl_fflush
#endif

#if 0
    #define NO_CRYPT_BENCHMARK
#else
    extern double current_time(int reset);
    /* Allows custom "custom_time()" function to be used for benchmark */
    #define WOLFSSL_USER_CURRTIME
    #define WOLFSSL_GMTIME
    #define USER_TICKS
    extern unsigned long uefi_time_wolfssl(unsigned long* timer);
    #define XTIME uefi_time_wolfssl
    #define WOLFSSL_NO_FLOAT_FMT
    //#define CONFIG_LOG_MODE_IMMEDIATE
    //#define NO_STDIO_FILESYSTEM
    //#define WOLFSSL_NEED_DYNAMIC_PATH_FIX_UEFI
    #define UEFI_BENCHMARK
#endif

#define NO_MAIN_DRIVER
#define WOLFSSL_IGNORE_FILE_WARN
extern void fipsEntry(void);
#define NO_ATTRIBUTE_CONSTRUCTOR

/* Only Matter for the test */
#define CERT_PREFIX "\\" /* Assume everything needs is at the root of device */
#define CERT_PATH_SEP "\\"

//#define WOLFSSL_WOLFSSH
#define FP_MAX_BITS 16384

#if 1 /* If Static buffers are required */
    #undef  USE_CERT_BUFFERS_2048
    #define USE_CERT_BUFFERS_2048

    #undef  USE_CERT_BUFFERS_1024
    #define USE_CERT_BUFFERS_1024

    #undef  USE_CERT_BUFFERS_256
    #define USE_CERT_BUFFERS_256
#endif

/* Essentially need  */
#if 0
    #undef WOLFSSL_NEED_DYNAMIC_TYPE_FIX_UEFI
    #define WOLFSSL_NEED_DYNAMIC_TYPE_FIX_UEFI
#endif

/* Debugging UEFI (for example code only not wolfSSL proper) */
#if 0
    #undef WOLFSSL_UEFI_VERBOSE_DEBUG
    #define WOLFSSL_UEFI_VERBOSE_DEBUG
#endif

#undef  HAVE_FIPS
#if 1

    #define HAVE_FIPS

    #undef  HAVE_FIPS_VERSION
    #define HAVE_FIPS_VERSION 5

    #undef HAVE_FIPS_VERSION_MINOR
    #define HAVE_FIPS_VERSION_MINOR 2

    #undef WC_RNG_SEED_CB
    #define WC_RNG_SEED_CB

    #undef WOLFCRYPT_FIPS_CORE_HASH_VALUE
    #define WOLFCRYPT_FIPS_CORE_HASH_VALUE A6A081C13719C5B6960A60CC4E2FCB231E60453430CBB92D546899FC9666C65B

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

    #undef  TFM_TIMING_RESISTANT
    #define TFM_TIMING_RESISTANT


#endif

/* Wolf Single Precision Math */
#undef WOLFSSL_SP
#if 0
    #define WOLFSSL_SP_MATH_ALL
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

    #ifdef HAVE_FIPS
        #undef  HAVE_ECC_CDH
        #define HAVE_ECC_CDH /* Enable cofactor support */

        #undef NO_STRICT_ECDSA_LEN
        #define NO_STRICT_ECDSA_LEN /* Do not force fixed len w/ FIPS */

        #undef  WOLFSSL_VALIDATE_ECC_IMPORT
        #define WOLFSSL_VALIDATE_ECC_IMPORT /* Validate import */

        #undef WOLFSSL_VALIDATE_ECC_KEYGEN
        #define WOLFSSL_VALIDATE_ECC_KEYGEN /* Validate generated keys */

        #undef WOLFSSL_ECDSA_SET_K
        #define WOLFSSL_ECDSA_SET_K

        #define WOLFCRYPT_HAVE_SAKKE

    #endif

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
#if 0
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

    #ifdef HAVE_FIPS
        #define WOLFSSL_VALIDATE_FFC_IMPORT
        #define HAVE_FFDHE_Q
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
#if 0
    #if 1
        #undef WOLFSSL_DES_ECB
        #define WOLFSSL_DES_ECB
    #endif
#else
    #define NO_DES3
#endif

/* ChaCha20 / Poly1305 */
#undef HAVE_CHACHA
#undef HAVE_POLY1305
#if 0
    #define HAVE_CHACHA
    #define HAVE_POLY1305

    /* Needed for Poly1305 */
    #undef  HAVE_ONE_TIME_AUTH
    #define HAVE_ONE_TIME_AUTH
#endif

/* Curve25519 */
#undef HAVE_CURVE25519
#if 1
    #define HAVE_CURVE25519

    /* Optionally use small math (less flash usage, but much slower) */
    #if 1
        #define CURVE25519_SMALL
    #endif
#endif

/* Ed25519 */
#undef HAVE_ED25519
#if 1
    #define HAVE_ED25519 /* ED25519 Requires SHA512 */

    /* Optionally use small math (less flash usage, but much slower) */
    #if 1
        #define ED25519_SMALL
    #endif
#endif


/* ------------------------------------------------------------------------- */
/* Hashing */
/* ------------------------------------------------------------------------- */
/* Sha */
#undef NO_SHA
#if 1
    /* 1k smaller, but 25% slower */
    #define USE_SLOW_SHA
#else
    #define NO_SHA
#endif

/* Sha256 */
#undef NO_SHA256
#if 1
    /* not unrolled - ~2k smaller and ~25% slower */
    #define USE_SLOW_SHA256
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

    #define  WOLFSSL_NOSHA512_224 /* Not in FIPS mode */
    #define  WOLFSSL_NOSHA512_256 /* Not in FIPS mode */

    /* Sha384 */
    #undef  WOLFSSL_SHA384
    #if 1
        #define WOLFSSL_SHA384
    #endif

    /* over twice as small, but 50% slower */
    #define USE_SLOW_SHA512
#endif

/* Sha3 */
#undef WOLFSSL_SHA3
#if 1
    #define WOLFSSL_SHA3
#endif

/* MD5 */
#undef  NO_MD5
#if 0

#else
    #define NO_MD5
#endif

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

/* SHAKE 128/256 */
#define WOLFSSL_SHAKE128
#define WOLFSSL_SHAKE256




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
#else
    #define NO_RSA
#endif




/* ------------------------------------------------------------------------- */
/* RNG */
/* ------------------------------------------------------------------------- */

//#define WC_RNG_SEED_CB

#if 0 /* the no RNG engine section */
    #define WOLFSSL_GENSEED_FORTEST
#endif

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



/* Section for FIPS validation testing only, disable for production */
#if 0
    //#define NO_CAVP_TDES
    //#define USE_NORMAL_PRINTF
    //#define USE_NORMAL_SCAN

    //#define WORKING_WITH_AEGISOLVE
    #define NO_MAIN_OPTEST_DRIVER
    #define OPTEST_LOGGING_ENABLED
    #define OPTEST_INVALID_LOGGING_ENABLED
    #define OPTEST_RUNNING_ORGANIC
    #define HAVE_FORCE_FIPS_FAILURE
// Defined above
    #define DEBUG_FIPS_VERBOSE
    #define DEBUG_WOLFSSL

    //#define FORCE_BUFFER_TEST
    //#define DEEPLY_EMBEDDED
    //#define USE_CERT_BUFFERS_2048
    //#define USE_CERT_BUFFERS_256
    //#define NO_WRITE_TEMP_FILES
    //#define PROBLEMATIC_DEC_PRIM_VECTOR_FILE
#endif

#endif /* USER_H */