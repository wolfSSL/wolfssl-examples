
#define NO_SIG_WRAPPER // -0

#define WOLFCRYPT_ONLY
#define XMALLOC_USER


#define WOLFSSL_PUBLIC_MP

#define NO_WRITEV // -0
#define NO_SESSION_CACHE // -0
#define NO_OLD_TLS // -0
#define NO_RC4 // -0

/* hash */
#define NO_MD4
#define NO_MD5
#define NO_SHA

/* rsa */
#define WOLFSSL_RSA_VERIFY_INLINE
#define WC_NO_RSA_OAEP
#define WC_NO_HARDEN



/* sp_int */
#define NO_DH
#define NO_DSA
#define NO_DES3
#define NO_AES
#define WOLFSSL_SP_NO_DYN_STACK
#define WOLFSSL_SP_SMALL


/* asn */
#define NO_ASN_TIME
#define IGNORE_NAME_CONSTRAINTS
// #define ASN_DUMP_OID // +212
// #define RSA_DECODE_EXTRA // -0
// #define WOLFSSL_CERT_GEN // +10000
#define WOLFSSL_NO_ASN_STRICT // -200
// #define WOLFSSL_NO_OCSP_OPTIONAL_CERTS // -0
// #define WOLFSSL_NO_TRUSTED_CERTS_VERIFY +115
// #define WOLFSSL_NO_OCSP_ISSUER_CHECK // -0
// #define WOLFSSL_SMALL_CERT_VERIFY // +2000
// #define WOLFSSL_NO_OCSP_DATE_CHECK // -0
// #define WOLFSSL_FORCE_OCSP_NONCE_CHECK // -0
// #define WOLFSSL_ASN_TEMPLATE // It can't be defined
// #define NO_CERTS
// #define WOLFSSL_ASN_TEMPLATE_TYPE_CHECK // -0
// #define CRLDP_VALIDATE_DATA //-0
// #define WOLFSSL_AKID_NAME // -0
// #define WOLFSSL_CUSTOM_OID // -0
// #define WOLFSSL_HAVE_ISSUER_NAMES // -0
// #define WOLFSSL_SUBJ_DIR_ATTR // +500
// #define WOLFSSL_SUBJ_INFO_ACC // + 500
// #define WOLFSSL_FPKI // -0
// #define WOLFSSL_CERT_NAME_ALL // -0

/* wc_port */

// #define WOLFSSL_NUCLEUS
// #define WOLFSSL_USER_MUTEX
// #define SINGLE_THREADED  // It may be bad macro
// #define WOLFSSL_USER_FILESYSTEM
