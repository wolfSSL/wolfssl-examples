/*
 * Copyright (C) 2024 wolfSSL Inc.
 *
 * This file is part of wolfHSM.
 *
 * user_settings.h - wolfSSL configuration for DTLS server demo
 *
 * Based on the posix server user_settings.h with TLS enabled.
 */

#ifndef USER_SETTINGS_H
#define USER_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/** wolfHSM required settings for wolfCrypt */
#define WOLF_CRYPTO_CB
#define WOLFSSL_KEY_GEN
#define WOLFSSL_ASN_TEMPLATE
#define WOLFSSL_BASE64_ENCODE
#define HAVE_ANONYMOUS_INLINE_AGGREGATES 1

/** Math library */
#define USE_FAST_MATH
#define FP_MAX_BITS 8192

/** General recommended settings */
#define WOLFSSL_USE_ALIGN
#define WOLFSSL_IGNORE_FILE_WARN
#define TFM_TIMING_RESISTANT
#define ECC_TIMING_RESISTANT
#define WC_RSA_BLINDING

/** Remove unneeded features */
#define NO_MAIN_DRIVER
#define NO_INLINE
#define NO_DO178

/** Remove unneeded namespace */
#define NO_OLD_RNGNAME
#define NO_OLD_WC_NAMES
#define NO_OLD_SSL_NAMES
#define NO_OLD_SHA_NAMES
#define NO_OLD_MD5_NAME

/** RSA Options */
#define RSA_MIN_SIZE 1024
#define WC_RSA_PSS
#define WOLFSSL_PSS_LONG_SALT

/** ECC Options */
#define HAVE_ECC
#define TFM_ECC256
#define ECC_SHAMIR
#define HAVE_ECC_DHE  /* Required for ECDH key agreement (TLS 1.3 key share) */

/** Curve25519 Options - disabled for this demo
 * TLS 1.3 will use ECC (P-256) for key share instead */
/* #define HAVE_CURVE25519 */

/** Ed25519 Options - disabled for this demo */
/* #define HAVE_ED25519 */

/** DH Options */
#define HAVE_DH_DEFAULT_PARAMS
#define HAVE_FFDHE_2048

/** AES Options */
#define HAVE_AES_CBC
#define HAVE_AESGCM
#define WOLFSSL_AES_COUNTER
#define GCM_TABLE_4BIT
#define WOLFSSL_AES_DIRECT
#define HAVE_AES_ECB
#define WOLFSSL_CMAC

/** SHA Options */
/* Keep SHA-1 enabled - required for DTLS 1.2 cipher suites */
#define WOLFSSL_SHA224
#define WOLFSSL_SHA384
#define WOLFSSL_SHA512
#define WOLFSSL_SHA512_HASHTYPE

/** Composite features */
#define HAVE_HKDF

/** RNG configuration */
/* Use Hash DRBG (NIST SP 800-90A compliant) */
#define HAVE_HASHDRBG

/** Remove unneeded crypto */
#define NO_DSA
#define NO_RC4
#define NO_MD4
/* Keep MD5 for TLS PRF */
#define NO_DES3
#define NO_PWDBASED

/** TLS/DTLS settings */
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_EXTENDED_MASTER
#define HAVE_ENCRYPT_THEN_MAC
#define HAVE_SERVER_RENEGOTIATION_INFO

/* DTLS support */
#define WOLFSSL_DTLS
#define WOLFSSL_DTLS13
#define WOLFSSL_TLS13
#define WOLFSSL_SEND_HRR_COOKIE  /* Required for DTLS 1.3 server */

/* Disable features we don't need */
#define NO_PSK
#define NO_OLD_TLS

/* Test certificate buffers for demo */
#define USE_CERT_BUFFERS_256
#define USE_CERT_BUFFERS_2048

/* Debug (optional - enabled via Makefile DEBUG=1) */
#ifdef WOLFHSM_CFG_DEBUG
#define DEBUG_WOLFSSL
#endif

/* Include for POSIX extensions needed by wolfSSL */
#include <sys/time.h>   /* for struct timeval, gettimeofday */
#include <strings.h>    /* for strcasecmp */

#ifdef __cplusplus
}
#endif

#endif /* USER_SETTINGS_H */
