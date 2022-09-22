
#define NO_SIG_WRAPPER // -0

#define WOLFCRYPT_ONLY
#define XMALLOC_USER


#define WOLFSSL_PUBLIC_MP

#define NO_WRITEV // -0
#define NO_SESSION_CACHE // -0
#define NO_OLD_TLS // -0
#define NO_RC4 // -0

#define WC_NO_HARDEN

/* hash */
#define NO_MD4
#define NO_MD5
#define NO_SHA


/* rsa */
#define WOLFSSL_RSA_VERIFY_ONLY
#define WOLFSSL_RSA_PUBLIC_ONLY
#define WOLFSSL_RSA_VERIFY_INLINE
#define WC_NO_RSA_OAEP

/* math library */
#define NO_ASN
#define NO_DH
#define NO_DSA
#define NO_DES3
#define NO_AES
// #define WOLFSSL_SP_NO_256
// #define WOLFSSL_SP_NO_3072
// #define WOLFSSL_SP_NO_2048  // -0
// #define WOLFSSL_SP_FAST_MODEXP
#define WOLFSSL_SP_NO_DYN_STACK
#define WOLFSSL_SP_SMALL
// #define WOLFSSL_HAVE_SP_DH
// #define WOLFSSL_HAVE_SP_RSA

/* random */