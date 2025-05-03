#define WOLFCRYPT_ONLY
#define HAVE_ENTROPY_MEMUSE
#define ENTROPY_MEMUSE_FORCE_FAILURE
#define USE_FAST_MATH
#define FP_MAX_BITS 8192
#define WOLFSSL_SHA3
#define HAVE_HASHDRBG
#define SINGLE_THREADED
#define XMALLOC_USER
#define NO_MD5
#define NO_SHA
#define NO_PKCS8
#define NO_PKCS12
#define NO_DSA
#define NO_ASN_TIME
#define NO_FILESYSTEM
#define NO_INLINE
#define NO_DH
#define NO_DES3
#define NO_MD4
#define NO_SIG_WRAPPER
#define NO_PWDBASED
#define NO_ERROR_STRINGS
#define XPRINTF(_f_, ...) Print(L##_f_, ##__VA_ARGS__)
