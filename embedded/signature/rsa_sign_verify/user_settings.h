#define WOLFCRYPT_ONLY
#define NO_SIG_WRAPPER
#define WOLFSSL_PUBLIC_MP

/* hash */
#define NO_MD4
#define NO_MD5
#define NO_SHA

/* rsa */
#define WOLFSSL_RSA_VERIFY_INLINE
#define WC_NO_RSA_OAEP
#define WC_NO_HARDEN
#define WOLFSSL_KEY_GEN

/* sp_int */
#define NO_DH
#define NO_DSA
#define NO_DES3
#define NO_AES

/* asn */
#define NO_ASN_TIME
#define IGNORE_NAME_CONSTRAINTS
#define WOLFSSL_NO_ASN_STRICT 


#ifdef DEBUG_MEMORY
    #define WOLFSSL_TRACK_MEMORY
    #define HAVE_STACK_SIZE
    // #define WOLFSSL_DEBUG_MEMORY
    // #define WOLFSSL_DEBUG_MEMORY_PRINT
#endif


#ifdef SP_C32_FLAG 
    #define WOLFSSL_HAVE_SP_RSA
    #define WOLFSSL_SP_MATH_ALL
    #undef USE_FAST_MATH
#endif 

#ifdef SP_C64_FLAG
    #define WOLFSSL_HAVE_SP_RSA
    #define WOLFSSL_SP_MATH_ALL
    #define SP_WORD_SIZE 64
    #define HAVE___UINT128_T
    #undef USE_FAST_MATH

#endif

#ifdef SP_ARM64_FLAG
    #define WOLFSSL_HAVE_SP_RSA
    #define WOLFSSL_SP_MATH_ALL
    #define WOLFSSL_SP_ARM64
    #define WOLFSSL_SP_ARM64_ASM
#endif /*SP_ARM64_FLAG*/


#ifdef SP_X86_64_FLAG
    #define WOLFSSL_HAVE_SP_RSA
    #define WOLFSSL_SP_MATH_ALL
    #define WOLFSSL_SP_X86_64
    #define WOLFSSL_SP_X86_64_ASM
#endif /*SP_X86_64_FLAG*/

#ifdef TFM_FLAG
    #define USE_FAST_MATH
    #undef WOLFSSL_HAVE_SP_RSA
    #undef WOLFSSL_SP_ARM64
    #undef WOLFSSL_SP_ARM64_ASM
    #undef WOLFSSL_SP_X86_64
    #undef WOLFSSL_SP_X86_64_ASM
#endif /* TFM_FLAG*/

#ifdef BENCHMARK
    #undef DEBUG_MEMORY
#endif

// #define PSS_PADDING
#ifdef PSS_PADDING
    #define WC_RSA_PSS
    #define WC_RSA_BLINDING
#endif

/* Non-blocking */
#if defined(NONBLOCK)
    #define WC_RSA_NONBLOCK
    #define TFM_TIMING_RESISTANT
    #define WOLFSSL_SP_NONBLOCK
    #define WOLFSSL_SP_SMALL
    #define WOLFSSL_SP_NO_MALLOC
    #undef BENCHMARK
#endif /* NONBLOCK */
