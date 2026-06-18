/* user_settings.h -- wolfCrypt config for the AmebaPro2 (RTL8735B) HUK example
 * built inside the RealTek FreeRTOS SDK. HUK crypto-callback demo: AES
 * (GCM/ECB/CBC/CTR), HMAC-SHA256, and ECDSA P-256. */

#ifndef RTL8735B_HUK_USER_SETTINGS_H
#define RTL8735B_HUK_USER_SETTINGS_H

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
#define WOLFCRYPT_ONLY             /* no TLS layer */
#define WOLFSSL_SMALL_STACK
#define WOLFSSL_IGNORE_FILE_WARN
#define NO_ERROR_STRINGS

/* ---- AES modes exercised by the HUK device ---- */
#define HAVE_AESGCM
#define WOLFSSL_AES_DIRECT
#define HAVE_AES_ECB
#define HAVE_AES_CBC
#define WOLFSSL_AES_COUNTER
#define WOLFSSL_AES_256
#define WOLFSSL_AES_128
#define GCM_TABLE_4BIT

/* ---- hashing + DRBG (seeded from the SDK TRNG, see main.c) ---- */
#undef  NO_SHA256
#define WOLFSSL_SHA256
#define HAVE_HASHDRBG
/* HMAC-SHA256 is enabled by default (no NO_HMAC) for the HUK HMAC demo. */

/* ---- ECC / ECDSA P-256 (HUK-bound sign demo) ---- */
#define HAVE_ECC
#define HAVE_ECC_SIGN
#define HAVE_ECC_VERIFY
#define ECC_USER_CURVES            /* limit curve tables to those enabled below */
#define HAVE_ECC256                /* P-256 only */
#define ECC_TIMING_RESISTANT
#define WOLFSSL_SP_MATH_ALL        /* generic SP big-integer math backend (ECC) */

/* ---- trims ---- */
#define NO_RSA
#define NO_DSA
#define NO_DH
#define NO_DES3
#define NO_RC4
#define NO_MD4
#define NO_MD5
#define NO_SHA
#define NO_PWDBASED
#define NO_PKCS12
#define NO_PKCS8
#define NO_CERTS
#define NO_OLD_TLS

/* ---- custom RNG seed hook (provided in main.c via the SDK TRNG) ---- */
#define CUSTOM_RAND_GENERATE_SEED  rtl8735b_rand_seed
#ifndef __ASSEMBLER__
    #include <stddef.h>
    int rtl8735b_rand_seed(unsigned char* output, unsigned int sz);
#endif

#ifdef __cplusplus
}
#endif

#endif /* RTL8735B_HUK_USER_SETTINGS_H */
