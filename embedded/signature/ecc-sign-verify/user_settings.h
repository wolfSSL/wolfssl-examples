#define WOLFCRYPT_ONLY
#define WOLFSSL_PUBLIC_MP

/* hash */
#define NO_MD4
#define NO_MD5
#define NO_SHA

/* rsa */
#define WOLFSSL_RSA_VERIFY_INLINE
#define WC_NO_RSA_OAEP
#define WC_NO_HARDEN
#define NO_RSA
/* sp_int */
#define NO_DH
#define NO_DSA
#define NO_DES3
#define NO_AES

/* asn */
#define NO_ASN_TIME
#define IGNORE_NAME_CONSTRAINTS
#define WOLFSSL_NO_ASN_STRICT

/* ecc */
#define HAVE_ECC
#define HAVE_ALL_CURVES


#ifdef DEBUG_MEMORY
    // #define WOLFSSL_TRACK_MEMORY
    #define HAVE_STACK_SIZE
    #define WOLFSSL_DEBUG_MEMORY
    // #define WOLFSSL_DEBUG_MEMORY_PRINT
#endif


#ifdef SP_FLAG
    #define WOLFSSL_HAVE_SP_ECC
    #define WOLFSSL_SP_MATH_ALL
    #undef USE_FAST_MATH
#endif

#ifdef SP_ARM64_FLAG
    #define WOLFSSL_HAVE_SP_ECC
    #define WOLFSSL_SP_MATH_ALL
    #define WOLFSSL_SP_ARM64
    #define WOLFSSL_SP_ARM64_ASM
#endif


#ifdef SP_X86_64_FLAG
    #define WOLFSSL_HAVE_SP_ECC
    #define WOLFSSL_SP_MATH_ALL
    #define WOLFSSL_SP_X86_64
    #define WOLFSSL_SP_X86_64_ASM
#endif

#ifdef TFM_FLAG
    #define USE_FAST_MATH
    #undef WOLFSSL_HAVE_SP_ECC
    #undef WOLFSSL_SP_ARM64
    #undef WOLFSSL_SP_ARM64_ASM
    #undef WOLFSSL_SP_X86_64
    #undef WOLFSSL_SP_X86_64_ASM
#endif

#ifdef BENCHMARK
    #undef DEBUG_MEMORY
#endif
