/* user_settings.h
 * For use with UnaliWear
*/


#ifndef WOLFSSL_USER_SETTINGS_H
#define WOLFSSL_USER_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------*/
/* Platform */
/*--------------------------------------------------------------------------*/

#undef NO_FILE_SYSTEM
#define NO_FILE_SYSTEM

#undef NO_MAIN_DRIVER
#define NO_MAIN_DRIVER

#undef BENCH_EMBEDDED
#define BENCH_EMBEDDED

#undef SINGLE_THREADED
#define SINGLE_THREADED

#undef WOLFSSL_NO_HEAP
#define WOLFSSL_NO_HEAP

/* defined in nano-libc disable in wolfSSL to avoid math mismatch in
 * wolfssl/wolfcrypt/types.h */
#undef __SIZEOF_LONG_LONG__

/*--------------------------------------------------------------------------*/
/* Functionality */
/*--------------------------------------------------------------------------*/
#undef NO_RC4
#define NO_RC4

#undef NO_RABBIT
#define NO_RABBIT

#undef NO_PWDBASED
#define NO_PWDBASED

#undef NO_RSA
#define NO_RSA

#undef NO_MD4
#define NO_MD4

#undef WOLFCRYPT_ONLY
#define WOLFCRYPT_ONLY

#undef NO_ERROR_STRINGS
#define NO_ERROR_STRINGS

#undef NO_BIG_INT
#define NO_BIG_INT

#undef NO_DES3
#define NO_DES3

#undef NO_SIG_WRAPPER
#define NO_SIG_WRAPPER

#undef NO_MD5
#define NO_MD5

#undef NO_ASN
#define NO_ASN

#undef NO_SHA
#define NO_SHA

#undef NO_SHA512
#undef WOLFSSL_SHA512
#define NO_SHA512

#undef NO_SHA384
#undef WOLFSSL_SHA384
#define NO_SHA384

#undef NO_RNG
#define NO_RNG

#undef NO_64BIT
#define NO_64BIT

#undef NO_DSA
#define NO_DSA

#undef NO_DH
#define NO_DH

#undef HAVE_AESGCM
#undef HAVE_HASHDRBG

#undef NO_HMAC
#define NO_HMAC

#undef NO_SHA256
#define NO_SHA256

#define MICROCHIP_PIC32
#define MICROCHIP_PIC24

#ifdef __cplusplus
}
#endif

#endif /* WOLFSSL_USER_SETTINGS_H */
