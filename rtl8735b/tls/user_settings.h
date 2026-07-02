/* user_settings.h -- wolfSSL config for the AmebaPro2 (RTL8735B) HUK-in-TLS
 * example (RealTek FreeRTOS SDK). In-memory TLS 1.2 ECDHE-ECDSA handshake with
 * the server's ECDSA P-256 auth routed to the HUK crypto-callback device. */

#ifndef RTL8735B_HUK_TLS_USER_SETTINGS_H
#define RTL8735B_HUK_TLS_USER_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- HUK crypto-callback device (our RealTek port) ---- */
#define WOLFSSL_RTL8735B_HUK
#define WOLF_CRYPTO_CB

/* ---- platform / RTOS ---- */
#define WOLFSSL_GENERAL_ALIGNMENT 4
#define SIZEOF_LONG_LONG 8
#define SINGLE_THREADED            /* HW crypto serialized by the SDK device_lock */
#define NO_FILESYSTEM
#define NO_WRITEV
#define NO_MAIN_DRIVER
#define WOLFSSL_USER_IO            /* custom SetIORecv/Send; no sockets/netinet */
#define WOLFSSL_NO_SOCK           /* no built-in socket headers (bare metal) */
#define WOLFSSL_SMALL_STACK
#define WOLFSSL_IGNORE_FILE_WARN
#define NO_ERROR_STRINGS

/* ---- TLS layer: TLS 1.2 only, ECDHE-ECDSA-AES-GCM ---- */
#define NO_OLD_TLS                 /* TLS 1.2+ only (no SSLv3/TLS1.0/1.1) */
#define WOLFSSL_TLS13              /* harmless; harness uses TLSv1_2 methods */
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES      /* ECDHE curve negotiation */
#define HAVE_EXTENDED_MASTER
#define HAVE_ENCRYPT_THEN_MAC
#define HAVE_HKDF                  /* TLS 1.3 key schedule (kdf.c) */
#define WOLFSSL_NO_TLS12_RENEGOTIATION
#define NO_SESSION_CACHE           /* save RAM (no resumption cache) */
#define WOLFSSL_AEAD_ONLY          /* AES-GCM suites only (no CBC-HMAC) */
#define HAVE_PK_CALLBACKS          /* route ONLY the server ECDSA sign to the HUK
                                    * (whole-CTX devId would send TLS's own 32-byte
                                    * HMAC/PRF + record AES to the HUK too, whose
                                    * key==HUK-seed semantics break TLS) */

/* ---- test certificate buffers (serv_ecc/ecc_key/ca_ecc, P-256) ---- */
#define USE_CERT_BUFFERS_256
#define NO_ASN_TIME                /* no RTC on the board -> skip cert date checks */

/* ---- AES modes: GCM for TLS records; ECB/CBC/CTR for the HUK device ---- */
#define HAVE_AESGCM
#define WOLFSSL_AES_DIRECT
#define HAVE_AES_ECB
#define HAVE_AES_CBC
#define WOLFSSL_AES_COUNTER
#define WOLFSSL_AES_256
#define WOLFSSL_AES_128
#define GCM_TABLE_4BIT

/* ---- hashing + DRBG ---- */
#undef  NO_SHA256
#define WOLFSSL_SHA256
#define WOLFSSL_SHA384             /* some TLS sigalg/PRF paths */
#define HAVE_HASHDRBG

/* ---- ECC / ECDSA / ECDHE (P-256) ---- */
#define HAVE_ECC
#define HAVE_ECC_SIGN
#define HAVE_ECC_VERIFY
#define HAVE_ECC_DHE               /* ECDHE key agreement (software) */
#define ECC_USER_CURVES
#define HAVE_ECC256                /* P-256 only */
#define ECC_TIMING_RESISTANT
#define WOLFSSL_SP_MATH_ALL
#define HAVE_SUPPORTED_CURVES

/* ---- trims ---- */
#define NO_RSA
#define NO_DSA
#define NO_DH
#define NO_DES3
#define NO_RC4
#define NO_MD4
#define NO_MD5
#define NO_PWDBASED
#define NO_PKCS12
#define NO_PKCS8

/* ---- custom RNG seed hook (provided in main.c via the SDK TRNG) ---- */
#define CUSTOM_RAND_GENERATE_SEED  rtl8735b_rand_seed
#ifndef __ASSEMBLER__
    #include <stddef.h>
    int rtl8735b_rand_seed(unsigned char* output, unsigned int sz);
#endif

#ifdef __cplusplus
}
#endif

#endif /* RTL8735B_HUK_TLS_USER_SETTINGS_H */
