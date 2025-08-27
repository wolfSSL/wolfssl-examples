/* test.c
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */


#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <wolfssl/wolfcrypt/settings.h>

#ifdef XMALLOC_USER
    #include <stdlib.h>  /* we're using malloc / free direct here */
#endif

#ifndef NO_CRYPT_TEST

#include <wolfssl/wolfcrypt/memory.h>
#ifdef WOLFSSL_STATIC_MEMORY
    static WOLFSSL_HEAP_HINT* HEAP_HINT;
#else
    #define HEAP_HINT NULL
#endif /* WOLFSSL_STATIC_MEMORY */

#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/types.h>

#ifdef WOLFSSL_TEST_CERT
    #include <wolfssl/wolfcrypt/asn.h>
#else
    #include <wolfssl/wolfcrypt/asn_public.h>
#endif
#include <wolfssl/wolfcrypt/md2.h>
#include <wolfssl/wolfcrypt/md5.h>
#include <wolfssl/wolfcrypt/md4.h>
#include <wolfssl/wolfcrypt/sha.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/sha512.h>
#include <wolfssl/wolfcrypt/arc4.h>

#if defined(WC_NO_RNG) && defined(USE_FAST_MATH)
    #include <wolfssl/wolfcrypt/tfm.h>
#else
    #include <wolfssl/wolfcrypt/random.h>
#endif

#include <wolfssl/wolfcrypt/coding.h>
#include <wolfssl/wolfcrypt/signature.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/des3.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/wc_encrypt.h>
#include <wolfssl/wolfcrypt/cmac.h>
#include <wolfssl/wolfcrypt/poly1305.h>
#include <wolfssl/wolfcrypt/camellia.h>
#include <wolfssl/wolfcrypt/hmac.h>
#include <wolfssl/wolfcrypt/dh.h>
#include <wolfssl/wolfcrypt/dsa.h>
#include <wolfssl/wolfcrypt/srp.h>
#include <wolfssl/wolfcrypt/idea.h>
#include <wolfssl/wolfcrypt/hc128.h>
#include <wolfssl/wolfcrypt/rabbit.h>
#include <wolfssl/wolfcrypt/chacha.h>
#include <wolfssl/wolfcrypt/chacha20_poly1305.h>
#include <wolfssl/wolfcrypt/pwdbased.h>
#include <wolfssl/wolfcrypt/ripemd.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#ifdef HAVE_ECC
    #include <wolfssl/wolfcrypt/ecc.h>
#endif
#ifdef HAVE_CURVE25519
    #include <wolfssl/wolfcrypt/curve25519.h>
#endif
#ifdef HAVE_ED25519
    #include <wolfssl/wolfcrypt/ed25519.h>
#endif
#ifdef HAVE_BLAKE2
    #include <wolfssl/wolfcrypt/blake2.h>
#endif
#ifdef WOLFSSL_SHA3
    #include <wolfssl/wolfcrypt/sha3.h>
#endif
#ifdef HAVE_LIBZ
    #include <wolfssl/wolfcrypt/compress.h>
#endif
#ifdef HAVE_PKCS7
    #include <wolfssl/wolfcrypt/pkcs7.h>
#endif
#ifdef HAVE_FIPS
    #include <wolfssl/wolfcrypt/fips_test.h>
#endif
#ifdef WOLFSSL_ASYNC_CRYPT
    #include <wolfssl/wolfcrypt/async.h>
#endif
#if defined(OPENSSL_EXTRA) || defined(DEBUG_WOLFSSL_VERBOSE)
    #include <wolfssl/wolfcrypt/logging.h>
#endif

/* only for stack size check */
#ifdef HAVE_STACK_SIZE
    #include <wolfssl/ssl.h>
    #define err_sys err_sys_remap /* remap err_sys */
    #include <wolfssl/test.h>
    #undef err_sys
#endif

#ifdef _MSC_VER
    /* 4996 warning to use MS extensions e.g., strcpy_s instead of strncpy */
    #pragma warning(disable: 4996)
#endif

#ifdef OPENSSL_EXTRA
  #ifndef WOLFCRYPT_ONLY
    #include <wolfssl/openssl/evp.h>
  #endif
    #include <wolfssl/openssl/rand.h>
    #include <wolfssl/openssl/hmac.h>
    #include <wolfssl/openssl/aes.h>
    #include <wolfssl/openssl/des.h>
#endif


#if defined(NO_FILESYSTEM)
    #if !defined(USE_CERT_BUFFERS_1024) && !defined(USE_CERT_BUFFERS_2048) && \
        !defined(USE_CERT_BUFFERS_4096)
        #define USE_CERT_BUFFERS_2048
    #endif
    #if !defined(USE_CERT_BUFFERS_256)
        #define USE_CERT_BUFFERS_256
    #endif
#endif

#include <wolfssl/certs_test.h>

#if defined(WOLFSSL_MDK_ARM)
        #include <stdio.h>
        #include <stdlib.h>
    extern FILE * wolfSSL_fopen(const char *fname, const char *mode) ;
    #define fopen wolfSSL_fopen
#endif

#ifdef HAVE_NTRU
    #include "libntruencrypt/ntru_crypto.h"
#endif

#if defined(FREESCALE_MQX) || defined(FREESCALE_KSDK_MQX)
    #include <mqx.h>
    #include <stdlib.h>
    #if MQX_USE_IO_OLD
        #include <fio.h>
    #else
        #include <nio.h>
    #endif
#elif defined(FREESCALE_KSDK_BM)
    #include "fsl_debug_console.h"
    #undef printf
    #define printf PRINTF
#else
    #include <stdio.h>
#endif

#if defined(WOLFSSL_CERT_GEN) && (defined(HAVE_ECC384) || defined(HAVE_ALL_CURVES))
    #define ENABLE_ECC384_CERT_GEN_TEST
#endif


#ifdef THREADX
    /* since just testing, use THREADX log printf instead */
    int dc_log_printf(char*, ...);
        #undef printf
        #define printf dc_log_printf
#endif


#ifdef MICRIUM
    #include <bsp_ser.h>
    void BSP_Ser_Printf (CPU_CHAR* format, ...);
    #undef printf
    #define printf BSP_Ser_Printf
#endif

#include "wolfcrypt/test/test.h"


/* for async devices */
static int devId = INVALID_DEVID;

#ifdef HAVE_WNR
    const char* wnrConfigFile = "wnr-example.conf";
#endif

typedef struct testVector {
    const char*  input;
    const char*  output;
    size_t inLen;
    size_t outLen;
} testVector;

int  error_test(void);
int  base64_test(void);
int  asn_test(void);
int  md2_test(void);
int  md5_test(void);
int  md4_test(void);
int  sha_test(void);
int  sha224_test(void);
int  sha256_test(void);
int  sha512_test(void);
int  sha384_test(void);
int  sha3_test(void);
int  hash_test(void);
int  hmac_md5_test(void);
int  hmac_sha_test(void);
int  hmac_sha224_test(void);
int  hmac_sha256_test(void);
int  hmac_sha384_test(void);
int  hmac_sha512_test(void);
int  hmac_blake2b_test(void);
int  hkdf_test(void);
int  x963kdf_test(void);
int  arc4_test(void);
int  hc128_test(void);
int  rabbit_test(void);
int  chacha_test(void);
int  chacha20_poly1305_aead_test(void);
int  des_test(void);
int  des3_test(void);
int  aes_test(void);
int  aes192_test(void);
int  aes256_test(void);
int  cmac_test(void);
int  poly1305_test(void);
int  aesgcm_test(void);
int  gmac_test(void);
int  aesccm_test(void);
int  aeskeywrap_test(void);
int  camellia_test(void);
int  rsa_test(void);
int  rsa4096_test(void);
int  dh_test(void);
int  dsa_test(void);
int  srp_test(void);
#ifndef WC_NO_RNG
int  random_test(void);
#endif /* WC_NO_RNG */
int  pwdbased_test(void);
int  ripemd_test(void);
#if defined(OPENSSL_EXTRA) && !defined(WOLFCRYPT_ONLY)
    int  openssl_test(void);   /* test mini api */
#endif
int pbkdf1_test(void);
int pkcs12_test(void);
int pbkdf2_test(void);
int scrypt_test(void);
#ifdef HAVE_ECC
    int  ecc_test(void);
    #ifdef HAVE_ECC_ENCRYPT
        int  ecc_encrypt_test(void);
    #endif
    #ifdef USE_CERT_BUFFERS_256
        int ecc_test_buffers(void);
    #endif
#endif
#ifdef HAVE_CURVE25519
    int  curve25519_test(void);
#endif
#ifdef HAVE_ED25519
    int  ed25519_test(void);
#endif
#ifdef HAVE_BLAKE2
    int  blake2b_test(void);
#endif
#ifdef HAVE_LIBZ
    int compress_test(void);
#endif
#ifdef HAVE_PKCS7
    int pkcs7enveloped_test(void);
    int pkcs7signed_test(void);
    int pkcs7encrypted_test(void);
#endif
#if !defined(NO_ASN_TIME) && !defined(NO_RSA) && defined(WOLFSSL_TEST_CERT)
int cert_test(void);
#endif
#if defined(WOLFSSL_CERT_EXT) && defined(WOLFSSL_TEST_CERT)
int  certext_test(void);
#endif
#ifdef HAVE_IDEA
int idea_test(void);
#endif
#ifdef WOLFSSL_STATIC_MEMORY
int memory_test(void);
#endif
#ifdef HAVE_VALGRIND
int mp_test(void);
#endif
int logging_test(void);
int mutex_test(void);
#if defined(USE_WOLFSSL_MEMORY) && !defined(FREERTOS)
int memcb_test(void);
#endif

#if defined(DEBUG_WOLFSSL) && !defined(HAVE_VALGRIND) && \
        !defined(OPENSSL_EXTRA) && !defined(HAVE_STACK_SIZE)
#ifdef __cplusplus
    extern "C" {
#endif
    WOLFSSL_API int wolfSSL_Debugging_ON(void);
#ifdef __cplusplus
    }  /* extern "C" */
#endif
#endif

/* General big buffer size for many tests. */
#define FOURK_BUF 4096


#define ERROR_OUT(err, eLabel) { ret = (err); goto eLabel; }

#ifdef HAVE_STACK_SIZE
static THREAD_RETURN err_sys(const char* msg, int es)
#else
static int err_sys(const char* msg, int es)
#endif
{
    printf("%s error = %d\n", msg, es);

    EXIT_TEST(-1);
}

#ifndef HAVE_STACK_SIZE
/* func_args from test.h, so don't have to pull in other stuff */
typedef struct func_args {
    int    argc;
    char** argv;
    int    return_code;
} func_args;
#endif /* !HAVE_STACK_SIZE */

#ifdef HAVE_FIPS

static void myFipsCb(int ok, int err, const char* hash)
{
    printf("in my Fips callback, ok = %d, err = %d\n", ok, err);
    printf("message = %s\n", wc_GetErrorString(err));
    printf("hash = %s\n", hash);

    if (err == IN_CORE_FIPS_E) {
        printf("In core integrity hash check failure, copy above hash\n");
        printf("into verifyCore[] in fips_test.c and rebuild\n");
    }
}

#endif /* HAVE_FIPS */

#ifdef WOLFSSL_STATIC_MEMORY
    #ifdef BENCH_EMBEDDED
        static byte gTestMemory[10000];
    #elif defined(USE_FAST_MATH) && !defined(ALT_ECC_SIZE)
        static byte gTestMemory[130000];
    #else
        static byte gTestMemory[80000];
    #endif
#endif

#ifdef HAVE_STACK_SIZE
THREAD_RETURN WOLFSSL_THREAD wolfcrypt_test(void* args)
#else
int wolfcrypt_test(void* args)
#endif
{
    int ret;

    ((func_args*)args)->return_code = -1; /* error state */

#ifdef WOLFSSL_STATIC_MEMORY
    if (wc_LoadStaticMemory(&HEAP_HINT, gTestMemory, sizeof(gTestMemory),
                                                WOLFMEM_GENERAL, 1) != 0) {
        printf("unable to load static memory");
        exit(EXIT_FAILURE);
    }
#endif

#if defined(DEBUG_WOLFSSL) && !defined(HAVE_VALGRIND)
    wolfSSL_Debugging_ON();
#endif

#if defined(OPENSSL_EXTRA) || defined(DEBUG_WOLFSSL_VERBOSE)
    wc_SetLoggingHeap(HEAP_HINT);
#endif

#ifdef HAVE_FIPS
    wolfCrypt_SetCb_fips(myFipsCb);
#endif

#if !defined(NO_BIG_INT)
    if (CheckCtcSettings() != 1)
        return err_sys("Build vs runtime math mismatch\n", -1000);

#if defined(USE_FAST_MATH) && \
	(!defined(NO_RSA) || !defined(NO_DH) || defined(HAVE_ECC))
    if (CheckFastMathSettings() != 1)
        return err_sys("Build vs runtime fastmath FP_MAX_BITS mismatch\n",
                       -1001);
#endif /* USE_FAST_MATH */
#endif /* !NO_BIG_INT */

#ifdef WOLFSSL_ASYNC_CRYPT
    ret = wolfAsync_DevOpen(&devId);
    if (ret < 0) {
        printf("Async device open failed\nRunning without async\n");
    }
#else
    (void)devId;
#endif /* WOLFSSL_ASYNC_CRYPT */

    if ( (ret = error_test()) != 0)
        return err_sys("error    test failed!\n", ret);
    else
        printf( "error    test passed!\n");

#if !defined(NO_CODING) && defined(WOLFSSL_BASE64_ENCODE)
    if ( (ret = base64_test()) != 0)
        return err_sys("base64   test failed!\n", ret);
    else
        printf( "base64   test passed!\n");
#endif

#ifndef NO_ASN
    if ( (ret = asn_test()) != 0)
        return err_sys("base64   test failed!\n", ret);
    else
        printf( "base64   test passed!\n");
#endif

#ifndef NO_MD5
    if ( (ret = md5_test()) != 0)
        return err_sys("MD5      test failed!\n", ret);
    else
        printf( "MD5      test passed!\n");
#endif

#ifdef WOLFSSL_MD2
    if ( (ret = md2_test()) != 0)
        return err_sys("MD2      test failed!\n", ret);
    else
        printf( "MD2      test passed!\n");
#endif

#ifndef NO_MD4
    if ( (ret = md4_test()) != 0)
        return err_sys("MD4      test failed!\n", ret);
    else
        printf( "MD4      test passed!\n");
#endif

#ifndef NO_SHA
    if ( (ret = sha_test()) != 0)
        return err_sys("SHA      test failed!\n", ret);
    else
        printf( "SHA      test passed!\n");
#endif

#ifdef WOLFSSL_SHA224
    if ( (ret = sha224_test()) != 0)
        return err_sys("SHA-224  test failed!\n", ret);
    else
        printf( "SHA-224  test passed!\n");
#endif

#ifndef NO_SHA256
    if ( (ret = sha256_test()) != 0)
        return err_sys("SHA-256  test failed!\n", ret);
    else
        printf( "SHA-256  test passed!\n");
#endif

#ifdef WOLFSSL_SHA384
    if ( (ret = sha384_test()) != 0)
        return err_sys("SHA-384  test failed!\n", ret);
    else
        printf( "SHA-384  test passed!\n");
#endif

#ifdef WOLFSSL_SHA512
    if ( (ret = sha512_test()) != 0)
        return err_sys("SHA-512  test failed!\n", ret);
    else
        printf( "SHA-512  test passed!\n");
#endif

#ifdef WOLFSSL_SHA3
    if ( (ret = sha3_test()) != 0)
        return err_sys("SHA-3    test failed!\n", ret);
    else
        printf( "SHA-3    test passed!\n");
#endif

    if ( (ret = hash_test()) != 0)
        return err_sys("Hash     test failed!\n", ret);
    else
        printf( "Hash     test passed!\n");

#ifdef WOLFSSL_RIPEMD
    if ( (ret = ripemd_test()) != 0)
        return err_sys("RIPEMD   test failed!\n", ret);
    else
        printf( "RIPEMD   test passed!\n");
#endif

#ifdef HAVE_BLAKE2
    if ( (ret = blake2b_test()) != 0)
        return err_sys("BLAKE2b  test failed!\n", ret);
    else
        printf( "BLAKE2b  test passed!\n");
#endif

#ifndef NO_HMAC
    #ifndef NO_MD5
        if ( (ret = hmac_md5_test()) != 0)
            return err_sys("HMAC-MD5 test failed!\n", ret);
        else
            printf( "HMAC-MD5 test passed!\n");
    #endif

    #ifndef NO_SHA
    if ( (ret = hmac_sha_test()) != 0)
        return err_sys("HMAC-SHA test failed!\n", ret);
    else
        printf( "HMAC-SHA test passed!\n");
    #endif

    #ifdef WOLFSSL_SHA224
        if ( (ret = hmac_sha224_test()) != 0)
            return err_sys("HMAC-SHA224 test failed!\n", ret);
        else
            printf( "HMAC-SHA224 test passed!\n");
    #endif

    #ifndef NO_SHA256
        if ( (ret = hmac_sha256_test()) != 0)
            return err_sys("HMAC-SHA256 test failed!\n", ret);
        else
            printf( "HMAC-SHA256 test passed!\n");
    #endif

    #ifdef WOLFSSL_SHA384
        if ( (ret = hmac_sha384_test()) != 0)
            return err_sys("HMAC-SHA384 test failed!\n", ret);
        else
            printf( "HMAC-SHA384 test passed!\n");
    #endif

    #ifdef WOLFSSL_SHA512
        if ( (ret = hmac_sha512_test()) != 0)
            return err_sys("HMAC-SHA512 test failed!\n", ret);
        else
            printf( "HMAC-SHA512 test passed!\n");
    #endif

    #ifdef HAVE_BLAKE2
        if ( (ret = hmac_blake2b_test()) != 0)
            return err_sys("HMAC-BLAKE2 test failed!\n", ret);
        else
            printf( "HMAC-BLAKE2 test passed!\n");
    #endif

    #ifdef HAVE_HKDF
        if ( (ret = hkdf_test()) != 0)
            return err_sys("HMAC-KDF    test failed!\n", ret);
        else
            printf( "HMAC-KDF    test passed!\n");
    #endif
#endif /* !NO_HMAC */

#if defined(HAVE_X963_KDF) && defined(HAVE_ECC)
    if ( (ret = x963kdf_test()) != 0)
        return err_sys("X963-KDF    test failed!\n", ret);
    else
        printf( "X963-KDF    test passed!\n");
#endif

#ifdef HAVE_AESGCM
    if ( (ret = gmac_test()) != 0)
        return err_sys("GMAC     test failed!\n", ret);
    else
        printf( "GMAC     test passed!\n");
#endif

#ifndef NO_RC4
    if ( (ret = arc4_test()) != 0)
        return err_sys("ARC4     test failed!\n", ret);
    else
        printf( "ARC4     test passed!\n");
#endif

#ifndef NO_HC128
    if ( (ret = hc128_test()) != 0)
        return err_sys("HC-128   test failed!\n", ret);
    else
        printf( "HC-128   test passed!\n");
#endif

#ifndef NO_RABBIT
    if ( (ret = rabbit_test()) != 0)
        return err_sys("Rabbit   test failed!\n", ret);
    else
        printf( "Rabbit   test passed!\n");
#endif

#ifdef HAVE_CHACHA
    if ( (ret = chacha_test()) != 0)
        return err_sys("Chacha   test failed!\n", ret);
    else
        printf( "Chacha   test passed!\n");
#endif

#ifdef HAVE_POLY1305
    if ( (ret = poly1305_test()) != 0)
        return err_sys("POLY1305 test failed!\n", ret);
    else
        printf( "POLY1305 test passed!\n");
#endif

#if defined(HAVE_CHACHA) && defined(HAVE_POLY1305)
    if ( (ret = chacha20_poly1305_aead_test()) != 0)
        return err_sys("ChaCha20-Poly1305 AEAD test failed!\n", ret);
    else
        printf( "ChaCha20-Poly1305 AEAD test passed!\n");
#endif

#ifndef NO_DES3
    if ( (ret = des_test()) != 0)
        return err_sys("DES      test failed!\n", ret);
    else
        printf( "DES      test passed!\n");
#endif

#ifndef NO_DES3
    if ( (ret = des3_test()) != 0)
        return err_sys("DES3     test failed!\n", ret);
    else
        printf( "DES3     test passed!\n");
#endif

#ifndef NO_AES
    if ( (ret = aes_test()) != 0)
        return err_sys("AES      test failed!\n", ret);
    else
        printf( "AES      test passed!\n");

    if ( (ret = aes192_test()) != 0)
        return err_sys("AES192   test failed!\n", ret);
    else
        printf( "AES192   test passed!\n");

    if ( (ret = aes256_test()) != 0)
        return err_sys("AES256   test failed!\n", ret);
    else
        printf( "AES256   test passed!\n");

#ifdef HAVE_AESGCM
    if ( (ret = aesgcm_test()) != 0)
        return err_sys("AES-GCM  test failed!\n", ret);
    else
        printf( "AES-GCM  test passed!\n");
#endif

#ifdef HAVE_AESCCM
    if ( (ret = aesccm_test()) != 0)
        return err_sys("AES-CCM  test failed!\n", ret);
    else
        printf( "AES-CCM  test passed!\n");
#endif
#ifdef HAVE_AES_KEYWRAP
    if ( (ret = aeskeywrap_test()) != 0)
        return err_sys("AES Key Wrap test failed!\n", ret);
    else
        printf( "AES Key Wrap test passed!\n");
#endif
#endif

#ifdef HAVE_CAMELLIA
    if ( (ret = camellia_test()) != 0)
        return err_sys("CAMELLIA test failed!\n", ret);
    else
        printf( "CAMELLIA test passed!\n");
#endif

#ifdef HAVE_IDEA
    if ( (ret = idea_test()) != 0)
        return err_sys("IDEA     test failed!\n", ret);
    else
        printf( "IDEA     test passed!\n");
#endif

#ifndef WC_NO_RNG
    if ( (ret = random_test()) != 0)
        return err_sys("RANDOM   test failed!\n", ret);
    else
        printf( "RANDOM   test passed!\n");
#endif /* WC_NO_RNG */

#ifdef WOLFSSL_STATIC_MEMORY
    if ( (ret = memory_test()) != 0)
        return err_sys("MEMORY   test failed!\n", ret);
    else
        printf( "MEMORY   test passed!\n");
#endif

#ifndef NO_RSA
    if ( (ret = rsa_test()) != 0)
        return err_sys("RSA      test failed!\n", ret);
    else
        printf( "RSA      test passed!\n");

    #if !defined(USE_FAST_MATH) || \
        (defined(USE_FAST_MATH) && (FP_MAX_BITS >= 8192)) \
        && !defined(NO_ASN)
    if ( (ret = rsa4096_test()) != 0)
        return err_sys("RSA 4096 test failed!\n", ret);
    else
        printf( "RSA 4096 test passed!\n");
    #endif
#endif

#ifndef NO_DH
    if ( (ret = dh_test()) != 0)
        return err_sys("DH       test failed!\n", ret);
    else
        printf( "DH       test passed!\n");
#endif

#ifndef NO_DSA
    if ( (ret = dsa_test()) != 0)
        return err_sys("DSA      test failed!\n", ret);
    else
        printf( "DSA      test passed!\n");
#endif

#ifdef WOLFCRYPT_HAVE_SRP
    if ( (ret = srp_test()) != 0)
        return err_sys("SRP      test failed!\n", ret);
    else
        printf( "SRP      test passed!\n");
#endif

#ifndef NO_PWDBASED
    if ( (ret = pwdbased_test()) != 0)
        return err_sys("PWDBASED test failed!\n", ret);
    else
        printf( "PWDBASED test passed!\n");
#endif

#if defined(OPENSSL_EXTRA) && !defined(WOLFCRYPT_ONLY)
    if ( (ret = openssl_test()) != 0)
        return err_sys("OPENSSL  test failed!\n", ret);
    else
        printf( "OPENSSL  test passed!\n");
#endif

#ifdef HAVE_ECC
    if ( (ret = ecc_test()) != 0)
        return err_sys("ECC      test failed!\n", ret);
    else
        printf( "ECC      test passed!\n");
    #ifdef HAVE_ECC_ENCRYPT
        if ( (ret = ecc_encrypt_test()) != 0)
            return err_sys("ECC Enc  test failed!\n", ret);
        else
            printf( "ECC Enc  test passed!\n");
    #endif
    #ifdef USE_CERT_BUFFERS_256
        if ( (ret = ecc_test_buffers()) != 0)
            return err_sys("ECC buffer test failed!\n", ret);
        else
            printf( "ECC buffer test passed!\n");
    #endif
#endif

#if !defined(NO_ASN_TIME) && !defined(NO_RSA) && defined(WOLFSSL_TEST_CERT)
    if ( (ret = cert_test()) != 0)
        return err_sys("CERT     test failed!\n", ret);
    else
        printf( "CERT     test passed!\n");
#endif

#if defined(WOLFSSL_CERT_EXT) && defined(WOLFSSL_TEST_CERT)
    if ( (ret = certext_test()) != 0)
        return err_sys("CERT EXT test failed!\n", ret);
    else
        printf( "CERT EXT test passed!\n");
#endif

#ifdef HAVE_CURVE25519
    if ( (ret = curve25519_test()) != 0)
        return err_sys("CURVE25519 test failed!\n", ret);
    else
        printf( "CURVE25519 test passed!\n");
#endif

#ifdef HAVE_ED25519
    if ( (ret = ed25519_test()) != 0)
        return err_sys("ED25519  test failed!\n", ret);
    else
        printf( "ED25519  test passed!\n");
#endif

#if defined(WOLFSSL_CMAC) && !defined(NO_AES)
    if ( (ret = cmac_test()) != 0)
        return err_sys("CMAC     test failed!\n", ret);
    else
        printf( "CMAC     test passed!\n");
#endif

#ifdef HAVE_LIBZ
    if ( (ret = compress_test()) != 0)
        return err_sys("COMPRESS test failed!\n", ret);
    else
        printf( "COMPRESS test passed!\n");
#endif

#ifdef HAVE_PKCS7
    if ( (ret = pkcs7enveloped_test()) != 0)
        return err_sys("PKCS7enveloped test failed!\n", ret);
    else
        printf( "PKCS7enveloped test passed!\n");

    if ( (ret = pkcs7signed_test()) != 0)
        return err_sys("PKCS7signed    test failed!\n", ret);
    else
        printf( "PKCS7signed    test passed!\n");

    if ( (ret = pkcs7encrypted_test()) != 0)
        return err_sys("PKCS7encrypted test failed!\n", ret);
    else
        printf( "PKCS7encrypted test passed!\n");
#endif

#ifdef HAVE_VALGRIND
    if ( (ret = mp_test()) != 0)
        return err_sys("mp       test failed!\n", ret);
    else
        printf( "mp       test passed!\n");
#endif

#ifdef HAVE_VALGRIND
    if ( (ret = logging_test()) != 0)
        return err_sys("logging  test failed!\n", ret);
    else
        printf( "logging  test passed!\n");
#endif

    if ( (ret = mutex_test()) != 0)
        return err_sys("mutex    test failed!\n", ret);
    else
        printf( "mutex    test passed!\n");

#if defined(USE_WOLFSSL_MEMORY) && !defined(FREERTOS)
    if ( (ret = memcb_test()) != 0)
        return err_sys("memcb    test failed!\n", ret);
    else
        printf( "memcb    test passed!\n");
#endif

#ifdef WOLFSSL_ASYNC_CRYPT
    wolfAsync_DevClose(&devId);
#endif

    ((func_args*)args)->return_code = ret;

    EXIT_TEST(ret);
}


#ifndef NO_MAIN_DRIVER

    /* so overall tests can pull in test function */
    int main(int argc, char** argv)
    {
        func_args args;

#ifdef HAVE_WNR
        if (wc_InitNetRandom(wnrConfigFile, NULL, 5000) != 0) {
            err_sys("Whitewood netRandom global config failed", -1002);
            return -1002;
        }
#endif

        args.argc = argc;
        args.argv = argv;

        wolfCrypt_Init();

    #ifdef HAVE_STACK_SIZE
        StackSizeCheck(&args, wolfcrypt_test);
    #else
        wolfcrypt_test(&args);
    #endif

        if (wolfCrypt_Cleanup() != 0) {
            err_sys("Error with wolfCrypt_Cleanup!\n", -1003);
        }

#ifdef HAVE_WNR
        if (wc_FreeNetRandom() < 0)
            err_sys("Failed to free netRandom context", -1004);
#endif /* HAVE_WNR */

        return args.return_code;
    }

#endif /* NO_MAIN_DRIVER */

/* helper to save DER, convert to PEM and save PEM */
#if !defined(NO_ASN) && (!defined(NO_RSA) || defined(HAVE_ECC)) && \
    (defined(WOLFSSL_KEY_GEN) || defined(WOLFSSL_CERT_GEN))

#if !defined(NO_FILESYSTEM) && !defined(NO_WRITE_TEMP_FILES)
#define SaveDerAndPem(d, dSz, p, pSz, fD, fP, pT, eB) _SaveDerAndPem(d, dSz, p, pSz, fD, fP, pT, eB)
#else
#define SaveDerAndPem(d, dSz, p, pSz, fD, fP, pT, eB) _SaveDerAndPem(d, dSz, p, pSz, NULL, NULL, pT, eB)
#endif

static int _SaveDerAndPem(const byte* der, int derSz,
    byte* pem, int pemSz, const char* fileDer,
    const char* filePem, int pemType, int errBase)
{
    int ret;
#if !defined(NO_FILESYSTEM) && !defined(NO_WRITE_TEMP_FILES)
    FILE* derFile;

    derFile = fopen(fileDer, "wb");
    if (!derFile) {
        return errBase + 0;
    }
    ret = (int)fwrite(der, 1, derSz, derFile);
    fclose(derFile);
    if (ret != derSz) {
        return errBase + 1;
    }
#endif

    if (pem && filePem) {
    #if !defined(NO_FILESYSTEM) && !defined(NO_WRITE_TEMP_FILES)
        FILE* pemFile;
    #endif

        pemSz = wc_DerToPem(der, derSz, pem, pemSz, pemType);
        if (pemSz < 0) {
            return errBase + 2;
        }

    #if !defined(NO_FILESYSTEM) && !defined(NO_WRITE_TEMP_FILES)
        pemFile = fopen(filePem, "wb");
        if (!pemFile) {
            return errBase + 3;
        }
        ret = (int)fwrite(pem, 1, pemSz, pemFile);
        fclose(pemFile);
        if (ret != pemSz) {
            return errBase + 4;
        }
    #endif
    }

    /* suppress unused variable warnings */
    (void)filePem;
    (void)fileDer;

    return 0;
}
#endif /* WOLFSSL_KEY_GEN || WOLFSSL_CERT_GEN */

int error_test(void)
{
    const char* errStr;
    char        out[WOLFSSL_MAX_ERROR_SZ];
    const char* unknownStr = wc_GetErrorString(0);

#ifdef NO_ERROR_STRINGS
    /* Ensure a valid error code's string matches an invalid code's.
     * The string is that error strings are not available.
     */
    errStr = wc_GetErrorString(OPEN_RAN_E);
    wc_ErrorString(OPEN_RAN_E, out);
    if (XSTRNCMP(errStr, unknownStr, XSTRLEN(unknownStr)) != 0)
        return -1100;
    if (XSTRNCMP(out, unknownStr, XSTRLEN(unknownStr)) != 0)
        return -1101;
#else
    int i;
    int j = 0;
    /* Values that are not or no longer error codes. */
    int missing[] = { -122, -123, -124,       -127, -128, -129,
                      -161, -162, -163, -164, -165, -166, -167, -168, -169,
                      -179,       -233,
                      0 };

    /* Check that all errors have a string and it's the same through the two
     * APIs. Check that the values that are not errors map to the unknown
     * string.
     */
    for (i = MAX_CODE_E-1; i >= WC_LAST_E; i--) {
        errStr = wc_GetErrorString(i);
        wc_ErrorString(i, out);

        if (i != missing[j]) {
            if (XSTRNCMP(errStr, unknownStr, XSTRLEN(unknownStr)) == 0)
                return -1102;
            if (XSTRNCMP(out, unknownStr, XSTRLEN(unknownStr)) == 0)
                return -1103;
            if (XSTRNCMP(errStr, out, XSTRLEN(errStr)) != 0)
                return -1104;
        }
        else {
            j++;
            if (XSTRNCMP(errStr, unknownStr, XSTRLEN(unknownStr)) != 0)
                return -1105;
            if (XSTRNCMP(out, unknownStr, XSTRLEN(unknownStr)) != 0)
                return -1106;
        }
    }

    /* Check if the next possible value has been given a string. */
    errStr = wc_GetErrorString(i);
    wc_ErrorString(i, out);
    if (XSTRNCMP(errStr, unknownStr, XSTRLEN(unknownStr)) != 0)
        return -1107;
    if (XSTRNCMP(out, unknownStr, XSTRLEN(unknownStr)) != 0)
        return -1108;
#endif

    return 0;
}

#if !defined(NO_CODING) && defined(WOLFSSL_BASE64_ENCODE)
int base64_test(void)
{
    int        ret;
    const byte good[] = "A+Gd\0\0\0";
    const byte goodEnd[] = "A+Gd \r\n";
    byte       out[128];
    word32     outLen;
    byte       data[3];
    word32     dataLen;
    byte       longData[79] = { 0 };
    const byte symbols[] = "+/A=";
    const byte badSmall[] = "AAA Gdj=";
    const byte badLarge[] = "AAA~Gdj=";
    const byte badEOL[] = "A+Gd ";
    int        i;

    /* Good Base64 encodings. */
    outLen = sizeof(out);
    ret = Base64_Decode(good, sizeof(good), out, &outLen);
    if (ret != 0)
        return -1200;
    outLen = sizeof(out);
    ret = Base64_Decode(goodEnd, sizeof(goodEnd), out, &outLen);
    if (ret != 0)
        return -1201;

    /* Bad parameters. */
    outLen = 1;
    ret = Base64_Decode(good, sizeof(good), out, &outLen);
    if (ret != BAD_FUNC_ARG)
        return -1202;

    outLen = sizeof(out);
    ret = Base64_Decode(badEOL, sizeof(badEOL), out, &outLen);
    if (ret != ASN_INPUT_E)
        return -1203;
    /* Bad character at each offset 0-3. */
    for (i = 0; i < 4; i++) {
        outLen = sizeof(out);
        ret = Base64_Decode(badSmall + i, 4, out, &outLen);
        if (ret != ASN_INPUT_E)
            return -1204 - i;
        ret = Base64_Decode(badLarge + i, 4, out, &outLen);
        if (ret != ASN_INPUT_E)
            return -1214 - i;
    }

    /* Decode and encode all symbols - non-alphanumeric. */
    dataLen = sizeof(data);
    ret = Base64_Decode(symbols, sizeof(symbols), data, &dataLen);
    if (ret != 0)
        return -1224;
    outLen = sizeof(out);
    ret = Base64_Encode(data, dataLen, NULL, &outLen);
    if (ret != LENGTH_ONLY_E)
        return -1225;
    outLen = sizeof(out);
    ret = Base64_Encode(data, dataLen, out, &outLen);
    if (ret != 0)
        return -1226;
    outLen = 7;
    ret = Base64_EncodeEsc(data, dataLen, out, &outLen);
    if (ret != BUFFER_E)
        return -1227;
    outLen = sizeof(out);
    ret = Base64_EncodeEsc(data, dataLen, NULL, &outLen);
    if (ret != LENGTH_ONLY_E)
        return -1228;
    outLen = sizeof(out);
    ret = Base64_EncodeEsc(data, dataLen, out, &outLen);
    if (ret != 0)
        return -1229;
    outLen = sizeof(out);
    ret = Base64_Encode_NoNl(data, dataLen, out, &outLen);
    if (ret != 0)
        return -1230;

    /* Data that results in an encoding longer than one line. */
    outLen = sizeof(out);
    dataLen = sizeof(longData);
    ret = Base64_Encode(longData, dataLen, out, &outLen);
    if (ret != 0)
        return -1231;
    outLen = sizeof(out);
    ret = Base64_EncodeEsc(longData, dataLen, out, &outLen);
    if (ret != 0)
        return -1232;
    outLen = sizeof(out);
    ret = Base64_Encode_NoNl(longData, dataLen, out, &outLen);
    if (ret != 0)
        return -1233;

    return 0;
}
#endif

#ifndef NO_ASN
int asn_test(void)
{
#ifndef NO_ASN_TIME
    #ifdef WORD64_AVAILABLE
        word64 now;
    #else
        word32 now;
    #endif

    /* Parameter Validation tests. */
    if (wc_GetTime(NULL, sizeof(now)) != BAD_FUNC_ARG)
        return -1300;
    if (wc_GetTime(&now, 0) != BUFFER_E)
        return -1301;

    now = 0;
    if (wc_GetTime(&now, sizeof(now)) != 0) {
        return -1302;
    }
    if (now == 0) {
        printf("RTC/Time not set!\n");
        return -1303;
    }
#endif

    return 0;
}
#endif

#ifdef WOLFSSL_MD2
int md2_test(void)
{
    Md2  md2;
    byte hash[MD2_DIGEST_SIZE];

    testVector a, b, c, d, e, f, g;
    testVector test_md2[7];
    int times = sizeof(test_md2) / sizeof(testVector), i;

    a.input  = "";
    a.output = "\x83\x50\xe5\xa3\xe2\x4c\x15\x3d\xf2\x27\x5c\x9f\x80\x69"
               "\x27\x73";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = MD2_DIGEST_SIZE;

    b.input  = "a";
    b.output = "\x32\xec\x01\xec\x4a\x6d\xac\x72\xc0\xab\x96\xfb\x34\xc0"
               "\xb5\xd1";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = MD2_DIGEST_SIZE;

    c.input  = "abc";
    c.output = "\xda\x85\x3b\x0d\x3f\x88\xd9\x9b\x30\x28\x3a\x69\xe6\xde"
               "\xd6\xbb";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = MD2_DIGEST_SIZE;

    d.input  = "message digest";
    d.output = "\xab\x4f\x49\x6b\xfb\x2a\x53\x0b\x21\x9f\xf3\x30\x31\xfe"
               "\x06\xb0";
    d.inLen  = XSTRLEN(d.input);
    d.outLen = MD2_DIGEST_SIZE;

    e.input  = "abcdefghijklmnopqrstuvwxyz";
    e.output = "\x4e\x8d\xdf\xf3\x65\x02\x92\xab\x5a\x41\x08\xc3\xaa\x47"
               "\x94\x0b";
    e.inLen  = XSTRLEN(e.input);
    e.outLen = MD2_DIGEST_SIZE;

    f.input  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345"
               "6789";
    f.output = "\xda\x33\xde\xf2\xa4\x2d\xf1\x39\x75\x35\x28\x46\xc3\x03"
               "\x38\xcd";
    f.inLen  = XSTRLEN(f.input);
    f.outLen = MD2_DIGEST_SIZE;

    g.input  = "1234567890123456789012345678901234567890123456789012345678"
               "9012345678901234567890";
    g.output = "\xd5\x97\x6f\x79\xd8\x3d\x3a\x0d\xc9\x80\x6c\x3c\x66\xf3"
               "\xef\xd8";
    g.inLen  = XSTRLEN(g.input);
    g.outLen = MD2_DIGEST_SIZE;

    test_md2[0] = a;
    test_md2[1] = b;
    test_md2[2] = c;
    test_md2[3] = d;
    test_md2[4] = e;
    test_md2[5] = f;
    test_md2[6] = g;

    wc_InitMd2(&md2);

    for (i = 0; i < times; ++i) {
        wc_Md2Update(&md2, (byte*)test_md2[i].input, (word32)test_md2[i].inLen);
        wc_Md2Final(&md2, hash);

        if (XMEMCMP(hash, test_md2[i].output, MD2_DIGEST_SIZE) != 0)
            return -1400 - i;
    }

    return 0;
}
#endif

#ifndef NO_MD5
int md5_test(void)
{
    int ret = 0;
    wc_Md5 md5;
    byte hash[WC_MD5_DIGEST_SIZE];
    byte hashcopy[WC_MD5_DIGEST_SIZE];
    testVector a, b, c, d, e, f;
    testVector test_md5[6];
    int times = sizeof(test_md5) / sizeof(testVector), i;

    a.input  = "";
    a.output = "\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42"
               "\x7e";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = WC_MD5_DIGEST_SIZE;

    b.input  = "abc";
    b.output = "\x90\x01\x50\x98\x3c\xd2\x4f\xb0\xd6\x96\x3f\x7d\x28\xe1\x7f"
               "\x72";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = WC_MD5_DIGEST_SIZE;

    c.input  = "message digest";
    c.output = "\xf9\x6b\x69\x7d\x7c\xb7\x93\x8d\x52\x5a\x2f\x31\xaa\xf1\x61"
               "\xd0";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = WC_MD5_DIGEST_SIZE;

    d.input  = "abcdefghijklmnopqrstuvwxyz";
    d.output = "\xc3\xfc\xd3\xd7\x61\x92\xe4\x00\x7d\xfb\x49\x6c\xca\x67\xe1"
               "\x3b";
    d.inLen  = XSTRLEN(d.input);
    d.outLen = WC_MD5_DIGEST_SIZE;

    e.input  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345"
               "6789";
    e.output = "\xd1\x74\xab\x98\xd2\x77\xd9\xf5\xa5\x61\x1c\x2c\x9f\x41\x9d"
               "\x9f";
    e.inLen  = XSTRLEN(e.input);
    e.outLen = WC_MD5_DIGEST_SIZE;

    f.input  = "1234567890123456789012345678901234567890123456789012345678"
               "9012345678901234567890";
    f.output = "\x57\xed\xf4\xa2\x2b\xe3\xc9\x55\xac\x49\xda\x2e\x21\x07\xb6"
               "\x7a";
    f.inLen  = XSTRLEN(f.input);
    f.outLen = WC_MD5_DIGEST_SIZE;

    test_md5[0] = a;
    test_md5[1] = b;
    test_md5[2] = c;
    test_md5[3] = d;
    test_md5[4] = e;
    test_md5[5] = f;

    ret = wc_InitMd5_ex(&md5, HEAP_HINT, devId);
    if (ret != 0)
        return -1500;

    for (i = 0; i < times; ++i) {
        ret = wc_Md5Update(&md5, (byte*)test_md5[i].input,
            (word32)test_md5[i].inLen);
        if (ret != 0)
            ERROR_OUT(-1510 - i, exit);

        ret = wc_Md5GetHash(&md5, hashcopy);
        if (ret != 0)
            ERROR_OUT(-1520 - i, exit);

        ret = wc_Md5Final(&md5, hash);
        if (ret != 0)
            ERROR_OUT(-1530 - i, exit);

        if (XMEMCMP(hash, test_md5[i].output, WC_MD5_DIGEST_SIZE) != 0)
            ERROR_OUT(-1540 - i, exit);

        if (XMEMCMP(hash, hashcopy, WC_MD5_DIGEST_SIZE) != 0)
            ERROR_OUT(-1550 - i, exit);
    }

    /* BEGIN LARGE HASH TEST */ {
    byte large_input[1024];
    const char* large_digest =
        "\x44\xd0\x88\xce\xf1\x36\xd1\x78\xe9\xc8\xba\x84\xc3\xfd\xf6\xca";

    for (i = 0; i < (int)sizeof(large_input); i++) {
        large_input[i] = (byte)(i & 0xFF);
    }
    times = 100;
#ifdef WOLFSSL_PIC32MZ_HASH
    wc_Md5SizeSet(&md5, times * sizeof(large_input));
#endif
    for (i = 0; i < times; ++i) {
        ret = wc_Md5Update(&md5, (byte*)large_input,
            (word32)sizeof(large_input));
        if (ret != 0)
            ERROR_OUT(-1560, exit);
    }
    ret = wc_Md5Final(&md5, hash);
    if (ret != 0)
        ERROR_OUT(-1561, exit);
    if (XMEMCMP(hash, large_digest, WC_MD5_DIGEST_SIZE) != 0)
        ERROR_OUT(-1562, exit);
    } /* END LARGE HASH TEST */

exit:

    wc_Md5Free(&md5);

    return ret;
}
#endif /* NO_MD5 */


#ifndef NO_MD4

int md4_test(void)
{
    Md4  md4;
    byte hash[MD4_DIGEST_SIZE];

    testVector a, b, c, d, e, f, g;
    testVector test_md4[7];
    int times = sizeof(test_md4) / sizeof(testVector), i;

    a.input  = "";
    a.output = "\x31\xd6\xcf\xe0\xd1\x6a\xe9\x31\xb7\x3c\x59\xd7\xe0\xc0\x89"
               "\xc0";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = MD4_DIGEST_SIZE;

    b.input  = "a";
    b.output = "\xbd\xe5\x2c\xb3\x1d\xe3\x3e\x46\x24\x5e\x05\xfb\xdb\xd6\xfb"
               "\x24";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = MD4_DIGEST_SIZE;

    c.input  = "abc";
    c.output = "\xa4\x48\x01\x7a\xaf\x21\xd8\x52\x5f\xc1\x0a\xe8\x7a\xa6\x72"
               "\x9d";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = MD4_DIGEST_SIZE;

    d.input  = "message digest";
    d.output = "\xd9\x13\x0a\x81\x64\x54\x9f\xe8\x18\x87\x48\x06\xe1\xc7\x01"
               "\x4b";
    d.inLen  = XSTRLEN(d.input);
    d.outLen = MD4_DIGEST_SIZE;

    e.input  = "abcdefghijklmnopqrstuvwxyz";
    e.output = "\xd7\x9e\x1c\x30\x8a\xa5\xbb\xcd\xee\xa8\xed\x63\xdf\x41\x2d"
               "\xa9";
    e.inLen  = XSTRLEN(e.input);
    e.outLen = MD4_DIGEST_SIZE;

    f.input  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345"
               "6789";
    f.output = "\x04\x3f\x85\x82\xf2\x41\xdb\x35\x1c\xe6\x27\xe1\x53\xe7\xf0"
               "\xe4";
    f.inLen  = XSTRLEN(f.input);
    f.outLen = MD4_DIGEST_SIZE;

    g.input  = "1234567890123456789012345678901234567890123456789012345678"
               "9012345678901234567890";
    g.output = "\xe3\x3b\x4d\xdc\x9c\x38\xf2\x19\x9c\x3e\x7b\x16\x4f\xcc\x05"
               "\x36";
    g.inLen  = XSTRLEN(g.input);
    g.outLen = MD4_DIGEST_SIZE;

    test_md4[0] = a;
    test_md4[1] = b;
    test_md4[2] = c;
    test_md4[3] = d;
    test_md4[4] = e;
    test_md4[5] = f;
    test_md4[6] = g;

    wc_InitMd4(&md4);

    for (i = 0; i < times; ++i) {
        wc_Md4Update(&md4, (byte*)test_md4[i].input, (word32)test_md4[i].inLen);
        wc_Md4Final(&md4, hash);

        if (XMEMCMP(hash, test_md4[i].output, MD4_DIGEST_SIZE) != 0)
            return -1600 - i;
    }

    return 0;
}

#endif /* NO_MD4 */

#ifndef NO_SHA

int sha_test(void)
{
    int ret = 0;
    wc_Sha sha;
    byte hash[WC_SHA_DIGEST_SIZE];
    byte hashcopy[WC_SHA_DIGEST_SIZE];
    testVector a, b, c, d, e;
    testVector test_sha[5];
    int times = sizeof(test_sha) / sizeof(struct testVector), i;

    a.input  = "";
    a.output = "\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18"
               "\x90\xaf\xd8\x07\x09";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = WC_SHA_DIGEST_SIZE;

    b.input  = "abc";
    b.output = "\xA9\x99\x3E\x36\x47\x06\x81\x6A\xBA\x3E\x25\x71\x78\x50\xC2"
               "\x6C\x9C\xD0\xD8\x9D";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = WC_SHA_DIGEST_SIZE;

    c.input  = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    c.output = "\x84\x98\x3E\x44\x1C\x3B\xD2\x6E\xBA\xAE\x4A\xA1\xF9\x51\x29"
               "\xE5\xE5\x46\x70\xF1";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = WC_SHA_DIGEST_SIZE;

    d.input  = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
               "aaaaaa";
    d.output = "\x00\x98\xBA\x82\x4B\x5C\x16\x42\x7B\xD7\xA1\x12\x2A\x5A\x44"
               "\x2A\x25\xEC\x64\x4D";
    d.inLen  = XSTRLEN(d.input);
    d.outLen = WC_SHA_DIGEST_SIZE;

    e.input  = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
               "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
               "aaaaaaaaaa";
    e.output = "\xAD\x5B\x3F\xDB\xCB\x52\x67\x78\xC2\x83\x9D\x2F\x15\x1E\xA7"
               "\x53\x99\x5E\x26\xA0";
    e.inLen  = XSTRLEN(e.input);
    e.outLen = WC_SHA_DIGEST_SIZE;

    test_sha[0] = a;
    test_sha[1] = b;
    test_sha[2] = c;
    test_sha[3] = d;
    test_sha[4] = e;

    ret = wc_InitSha_ex(&sha, HEAP_HINT, devId);
    if (ret != 0)
        return -1700;

    for (i = 0; i < times; ++i) {
        ret = wc_ShaUpdate(&sha, (byte*)test_sha[i].input,
            (word32)test_sha[i].inLen);
        if (ret != 0)
            ERROR_OUT(-1710 - i, exit);
        ret = wc_ShaGetHash(&sha, hashcopy);
        if (ret != 0)
            ERROR_OUT(-1720 - i, exit);
        ret = wc_ShaFinal(&sha, hash);
        if (ret != 0)
            ERROR_OUT(-1730 - i, exit);

        if (XMEMCMP(hash, test_sha[i].output, WC_SHA_DIGEST_SIZE) != 0)
            ERROR_OUT(-1740 - i, exit);
        if (XMEMCMP(hash, hashcopy, WC_SHA_DIGEST_SIZE) != 0)
            ERROR_OUT(-1750 - i, exit);
    }

    /* BEGIN LARGE HASH TEST */ {
    byte large_input[1024];
    const char* large_digest =
        "\x8b\x77\x02\x48\x39\xe8\xdb\xd3\x9a\xf4\x05\x24\x66\x12\x2d\x9e"
        "\xc5\xd9\x0a\xac";

    for (i = 0; i < (int)sizeof(large_input); i++) {
        large_input[i] = (byte)(i & 0xFF);
    }
    times = 100;
#ifdef WOLFSSL_PIC32MZ_HASH
    wc_ShaSizeSet(&sha, times * sizeof(large_input));
#endif
    for (i = 0; i < times; ++i) {
        ret = wc_ShaUpdate(&sha, (byte*)large_input,
            (word32)sizeof(large_input));
        if (ret != 0)
            ERROR_OUT(-1760, exit);
    }
    ret = wc_ShaFinal(&sha, hash);
    if (ret != 0)
        ERROR_OUT(-1761, exit);
    if (XMEMCMP(hash, large_digest, WC_SHA_DIGEST_SIZE) != 0)
        ERROR_OUT(-1762, exit);
    } /* END LARGE HASH TEST */

exit:

    wc_ShaFree(&sha);

    return ret;
}

#endif /* NO_SHA */

#ifdef WOLFSSL_RIPEMD
int ripemd_test(void)
{
    RipeMd  ripemd;
    int ret;
    byte hash[RIPEMD_DIGEST_SIZE];

    testVector a, b, c, d;
    testVector test_ripemd[4];
    int times = sizeof(test_ripemd) / sizeof(struct testVector), i;

    a.input  = "abc";
    a.output = "\x8e\xb2\x08\xf7\xe0\x5d\x98\x7a\x9b\x04\x4a\x8e\x98\xc6"
               "\xb0\x87\xf1\x5a\x0b\xfc";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = RIPEMD_DIGEST_SIZE;

    b.input  = "message digest";
    b.output = "\x5d\x06\x89\xef\x49\xd2\xfa\xe5\x72\xb8\x81\xb1\x23\xa8"
               "\x5f\xfa\x21\x59\x5f\x36";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = RIPEMD_DIGEST_SIZE;

    c.input  = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    c.output = "\x12\xa0\x53\x38\x4a\x9c\x0c\x88\xe4\x05\xa0\x6c\x27\xdc"
               "\xf4\x9a\xda\x62\xeb\x2b";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = RIPEMD_DIGEST_SIZE;

    d.input  = "12345678901234567890123456789012345678901234567890123456"
               "789012345678901234567890";
    d.output = "\x9b\x75\x2e\x45\x57\x3d\x4b\x39\xf4\xdb\xd3\x32\x3c\xab"
               "\x82\xbf\x63\x32\x6b\xfb";
    d.inLen  = XSTRLEN(d.input);
    d.outLen = RIPEMD_DIGEST_SIZE;

    test_ripemd[0] = a;
    test_ripemd[1] = b;
    test_ripemd[2] = c;
    test_ripemd[3] = d;

    ret = wc_InitRipeMd(&ripemd);
    if (ret != 0) {
        return -1800;
    }

    for (i = 0; i < times; ++i) {
        ret = wc_RipeMdUpdate(&ripemd, (byte*)test_ripemd[i].input,
                              (word32)test_ripemd[i].inLen);
        if (ret != 0) {
            return -1810 - i;
        }

        ret = wc_RipeMdFinal(&ripemd, hash);
        if (ret != 0) {
            return -1820 - i;
        }

        if (XMEMCMP(hash, test_ripemd[i].output, RIPEMD_DIGEST_SIZE) != 0)
            return -1830 - i;
    }

    return 0;
}
#endif /* WOLFSSL_RIPEMD */


#ifdef HAVE_BLAKE2


#define BLAKE2_TESTS 3

static const byte blake2b_vec[BLAKE2_TESTS][BLAKE2B_OUTBYTES] =
{
  {
    0x78, 0x6A, 0x02, 0xF7, 0x42, 0x01, 0x59, 0x03,
    0xC6, 0xC6, 0xFD, 0x85, 0x25, 0x52, 0xD2, 0x72,
    0x91, 0x2F, 0x47, 0x40, 0xE1, 0x58, 0x47, 0x61,
    0x8A, 0x86, 0xE2, 0x17, 0xF7, 0x1F, 0x54, 0x19,
    0xD2, 0x5E, 0x10, 0x31, 0xAF, 0xEE, 0x58, 0x53,
    0x13, 0x89, 0x64, 0x44, 0x93, 0x4E, 0xB0, 0x4B,
    0x90, 0x3A, 0x68, 0x5B, 0x14, 0x48, 0xB7, 0x55,
    0xD5, 0x6F, 0x70, 0x1A, 0xFE, 0x9B, 0xE2, 0xCE
  },
  {
    0x2F, 0xA3, 0xF6, 0x86, 0xDF, 0x87, 0x69, 0x95,
    0x16, 0x7E, 0x7C, 0x2E, 0x5D, 0x74, 0xC4, 0xC7,
    0xB6, 0xE4, 0x8F, 0x80, 0x68, 0xFE, 0x0E, 0x44,
    0x20, 0x83, 0x44, 0xD4, 0x80, 0xF7, 0x90, 0x4C,
    0x36, 0x96, 0x3E, 0x44, 0x11, 0x5F, 0xE3, 0xEB,
    0x2A, 0x3A, 0xC8, 0x69, 0x4C, 0x28, 0xBC, 0xB4,
    0xF5, 0xA0, 0xF3, 0x27, 0x6F, 0x2E, 0x79, 0x48,
    0x7D, 0x82, 0x19, 0x05, 0x7A, 0x50, 0x6E, 0x4B
  },
  {
    0x1C, 0x08, 0x79, 0x8D, 0xC6, 0x41, 0xAB, 0xA9,
    0xDE, 0xE4, 0x35, 0xE2, 0x25, 0x19, 0xA4, 0x72,
    0x9A, 0x09, 0xB2, 0xBF, 0xE0, 0xFF, 0x00, 0xEF,
    0x2D, 0xCD, 0x8E, 0xD6, 0xF8, 0xA0, 0x7D, 0x15,
    0xEA, 0xF4, 0xAE, 0xE5, 0x2B, 0xBF, 0x18, 0xAB,
    0x56, 0x08, 0xA6, 0x19, 0x0F, 0x70, 0xB9, 0x04,
    0x86, 0xC8, 0xA7, 0xD4, 0x87, 0x37, 0x10, 0xB1,
    0x11, 0x5D, 0x3D, 0xEB, 0xBB, 0x43, 0x27, 0xB5
  }
};



int blake2b_test(void)
{
    Blake2b b2b;
    byte    digest[64];
    byte    input[64];
    int     i, ret;

    for (i = 0; i < (int)sizeof(input); i++)
        input[i] = (byte)i;

    for (i = 0; i < BLAKE2_TESTS; i++) {
        ret = wc_InitBlake2b(&b2b, 64);
        if (ret != 0)
            return -1900 - i;

        ret = wc_Blake2bUpdate(&b2b, input, i);
        if (ret != 0)
            return -1910 - 1;

        ret = wc_Blake2bFinal(&b2b, digest, 64);
        if (ret != 0)
            return -1920 - i;

        if (XMEMCMP(digest, blake2b_vec[i], 64) != 0) {
            return -1930 - i;
        }
    }

    return 0;
}
#endif /* HAVE_BLAKE2 */


#ifdef WOLFSSL_SHA224
int sha224_test(void)
{
    wc_Sha224 sha;
    byte      hash[WC_SHA224_DIGEST_SIZE];
    byte      hashcopy[WC_SHA224_DIGEST_SIZE];
    int       ret = 0;

    testVector a, b, c;
    testVector test_sha[3];
    int times = sizeof(test_sha) / sizeof(struct testVector), i;

    a.input  = "";
    a.output = "\xd1\x4a\x02\x8c\x2a\x3a\x2b\xc9\x47\x61\x02\xbb\x28\x82\x34"
               "\xc4\x15\xa2\xb0\x1f\x82\x8e\xa6\x2a\xc5\xb3\xe4\x2f";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = WC_SHA224_DIGEST_SIZE;

    b.input  = "abc";
    b.output = "\x23\x09\x7d\x22\x34\x05\xd8\x22\x86\x42\xa4\x77\xbd\xa2\x55"
               "\xb3\x2a\xad\xbc\xe4\xbd\xa0\xb3\xf7\xe3\x6c\x9d\xa7";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = WC_SHA224_DIGEST_SIZE;

    c.input  = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    c.output = "\x75\x38\x8b\x16\x51\x27\x76\xcc\x5d\xba\x5d\xa1\xfd\x89\x01"
               "\x50\xb0\xc6\x45\x5c\xb4\xf5\x8b\x19\x52\x52\x25\x25";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = WC_SHA224_DIGEST_SIZE;

    test_sha[0] = a;
    test_sha[1] = b;
    test_sha[2] = c;

    ret = wc_InitSha224_ex(&sha, HEAP_HINT, devId);
    if (ret != 0)
        return -2000;

    for (i = 0; i < times; ++i) {
        ret = wc_Sha224Update(&sha, (byte*)test_sha[i].input,
            (word32)test_sha[i].inLen);
        if (ret != 0)
            ERROR_OUT(-2010 - i, exit);
        ret = wc_Sha224GetHash(&sha, hashcopy);
        if (ret != 0)
            ERROR_OUT(-2020 - i, exit);
        ret = wc_Sha224Final(&sha, hash);
        if (ret != 0)
            ERROR_OUT(-2030 - i, exit);

        if (XMEMCMP(hash, test_sha[i].output, WC_SHA224_DIGEST_SIZE) != 0)
            ERROR_OUT(-2040 - i, exit);
        if (XMEMCMP(hash, hashcopy, WC_SHA224_DIGEST_SIZE) != 0)
            ERROR_OUT(-2050 - i, exit);
    }

exit:
    wc_Sha224Free(&sha);

    return ret;
}
#endif


#ifndef NO_SHA256
int sha256_test(void)
{
    wc_Sha256 sha;
    byte      hash[WC_SHA256_DIGEST_SIZE];
    byte      hashcopy[WC_SHA256_DIGEST_SIZE];
    int       ret = 0;

    testVector a, b, c;
    testVector test_sha[3];
    int times = sizeof(test_sha) / sizeof(struct testVector), i;

    a.input  = "";
    a.output = "\xe3\xb0\xc4\x42\x98\xfc\x1c\x14\x9a\xfb\xf4\xc8\x99\x6f\xb9"
               "\x24\x27\xae\x41\xe4\x64\x9b\x93\x4c\xa4\x95\x99\x1b\x78\x52"
               "\xb8\x55";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = WC_SHA256_DIGEST_SIZE;

    b.input  = "abc";
    b.output = "\xBA\x78\x16\xBF\x8F\x01\xCF\xEA\x41\x41\x40\xDE\x5D\xAE\x22"
               "\x23\xB0\x03\x61\xA3\x96\x17\x7A\x9C\xB4\x10\xFF\x61\xF2\x00"
               "\x15\xAD";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = WC_SHA256_DIGEST_SIZE;

    c.input  = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    c.output = "\x24\x8D\x6A\x61\xD2\x06\x38\xB8\xE5\xC0\x26\x93\x0C\x3E\x60"
               "\x39\xA3\x3C\xE4\x59\x64\xFF\x21\x67\xF6\xEC\xED\xD4\x19\xDB"
               "\x06\xC1";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = WC_SHA256_DIGEST_SIZE;

    test_sha[0] = a;
    test_sha[1] = b;
    test_sha[2] = c;

    ret = wc_InitSha256_ex(&sha, HEAP_HINT, devId);
    if (ret != 0)
        return -2100;

    for (i = 0; i < times; ++i) {
        ret = wc_Sha256Update(&sha, (byte*)test_sha[i].input,
            (word32)test_sha[i].inLen);
        if (ret != 0)
            ERROR_OUT(-2110 - i, exit);
        ret = wc_Sha256GetHash(&sha, hashcopy);
        if (ret != 0)
            ERROR_OUT(-2120 - i, exit);
        ret = wc_Sha256Final(&sha, hash);
        if (ret != 0)
            ERROR_OUT(-2130 - i, exit);

        if (XMEMCMP(hash, test_sha[i].output, WC_SHA256_DIGEST_SIZE) != 0)
            ERROR_OUT(-2140 - i, exit);
        if (XMEMCMP(hash, hashcopy, WC_SHA256_DIGEST_SIZE) != 0)
            ERROR_OUT(-2150 - i, exit);
    }

    /* BEGIN LARGE HASH TEST */ {
    byte large_input[1024];
    const char* large_digest =
        "\x27\x78\x3e\x87\x96\x3a\x4e\xfb\x68\x29\xb5\x31\xc9\xba\x57\xb4"
        "\x4f\x45\x79\x7f\x67\x70\xbd\x63\x7f\xbf\x0d\x80\x7c\xbd\xba\xe0";

    for (i = 0; i < (int)sizeof(large_input); i++) {
        large_input[i] = (byte)(i & 0xFF);
    }
    times = 100;
#ifdef WOLFSSL_PIC32MZ_HASH
    wc_Sha256SizeSet(&sha, times * sizeof(large_input));
#endif
    for (i = 0; i < times; ++i) {
        ret = wc_Sha256Update(&sha, (byte*)large_input,
            (word32)sizeof(large_input));
        if (ret != 0)
            ERROR_OUT(-2160, exit);
    }
    ret = wc_Sha256Final(&sha, hash);
    if (ret != 0)
        ERROR_OUT(-2161, exit);
    if (XMEMCMP(hash, large_digest, WC_SHA256_DIGEST_SIZE) != 0)
        ERROR_OUT(-2162, exit);
    } /* END LARGE HASH TEST */

exit:

    wc_Sha256Free(&sha);

    return ret;
}
#endif


#ifdef WOLFSSL_SHA512
int sha512_test(void)
{
    wc_Sha512 sha;
    byte      hash[WC_SHA512_DIGEST_SIZE];
    byte      hashcopy[WC_SHA512_DIGEST_SIZE];
    int       ret = 0;

    testVector a, b, c;
    testVector test_sha[3];
    int times = sizeof(test_sha) / sizeof(struct testVector), i;

    a.input  = "";
    a.output = "\xcf\x83\xe1\x35\x7e\xef\xb8\xbd\xf1\x54\x28\x50\xd6\x6d\x80"
               "\x07\xd6\x20\xe4\x05\x0b\x57\x15\xdc\x83\xf4\xa9\x21\xd3\x6c"
               "\xe9\xce\x47\xd0\xd1\x3c\x5d\x85\xf2\xb0\xff\x83\x18\xd2\x87"
               "\x7e\xec\x2f\x63\xb9\x31\xbd\x47\x41\x7a\x81\xa5\x38\x32\x7a"
               "\xf9\x27\xda\x3e";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = WC_SHA512_DIGEST_SIZE;

    b.input  = "abc";
    b.output = "\xdd\xaf\x35\xa1\x93\x61\x7a\xba\xcc\x41\x73\x49\xae\x20\x41"
               "\x31\x12\xe6\xfa\x4e\x89\xa9\x7e\xa2\x0a\x9e\xee\xe6\x4b\x55"
               "\xd3\x9a\x21\x92\x99\x2a\x27\x4f\xc1\xa8\x36\xba\x3c\x23\xa3"
               "\xfe\xeb\xbd\x45\x4d\x44\x23\x64\x3c\xe8\x0e\x2a\x9a\xc9\x4f"
               "\xa5\x4c\xa4\x9f";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = WC_SHA512_DIGEST_SIZE;

    c.input  = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhi"
               "jklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    c.output = "\x8e\x95\x9b\x75\xda\xe3\x13\xda\x8c\xf4\xf7\x28\x14\xfc\x14"
               "\x3f\x8f\x77\x79\xc6\xeb\x9f\x7f\xa1\x72\x99\xae\xad\xb6\x88"
               "\x90\x18\x50\x1d\x28\x9e\x49\x00\xf7\xe4\x33\x1b\x99\xde\xc4"
               "\xb5\x43\x3a\xc7\xd3\x29\xee\xb6\xdd\x26\x54\x5e\x96\xe5\x5b"
               "\x87\x4b\xe9\x09";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = WC_SHA512_DIGEST_SIZE;

    test_sha[0] = a;
    test_sha[1] = b;
    test_sha[2] = c;

    ret = wc_InitSha512_ex(&sha, HEAP_HINT, devId);
    if (ret != 0)
        return -2200;

    for (i = 0; i < times; ++i) {
        ret = wc_Sha512Update(&sha, (byte*)test_sha[i].input,
            (word32)test_sha[i].inLen);
        if (ret != 0)
            ERROR_OUT(-2210 - i, exit);
        ret = wc_Sha512GetHash(&sha, hashcopy);
        if (ret != 0)
            ERROR_OUT(-2220 - i, exit);
        ret = wc_Sha512Final(&sha, hash);
        if (ret != 0)
            ERROR_OUT(-2230 - i, exit);

        if (XMEMCMP(hash, test_sha[i].output, WC_SHA512_DIGEST_SIZE) != 0)
            ERROR_OUT(-2240 - i, exit);
        if (XMEMCMP(hash, hashcopy, WC_SHA512_DIGEST_SIZE) != 0)
            ERROR_OUT(-2250 - i, exit);
    }

    /* BEGIN LARGE HASH TEST */ {
    byte large_input[1024];
    const char* large_digest =
        "\x5a\x1f\x73\x90\xbd\x8c\xe4\x63\x54\xce\xa0\x9b\xef\x32\x78\x2d"
        "\x2e\xe7\x0d\x5e\x2f\x9d\x15\x1b\xdd\x2d\xde\x65\x0c\x7b\xfa\x83"
        "\x5e\x80\x02\x13\x84\xb8\x3f\xff\x71\x62\xb5\x09\x89\x63\xe1\xdc"
        "\xa5\xdc\xfc\xfa\x9d\x1a\x4d\xc0\xfa\x3a\x14\xf6\x01\x51\x90\xa4";

    for (i = 0; i < (int)sizeof(large_input); i++) {
        large_input[i] = (byte)(i & 0xFF);
    }
    times = 100;
    for (i = 0; i < times; ++i) {
        ret = wc_Sha512Update(&sha, (byte*)large_input,
            (word32)sizeof(large_input));
        if (ret != 0)
            ERROR_OUT(-2260, exit);
    }
    ret = wc_Sha512Final(&sha, hash);
    if (ret != 0)
        ERROR_OUT(-2261, exit);
    if (XMEMCMP(hash, large_digest, WC_SHA512_DIGEST_SIZE) != 0)
        ERROR_OUT(-2262, exit);
    } /* END LARGE HASH TEST */

exit:
    wc_Sha512Free(&sha);

    return ret;
}
#endif


#ifdef WOLFSSL_SHA384
int sha384_test(void)
{
    wc_Sha384 sha;
    byte      hash[WC_SHA384_DIGEST_SIZE];
    byte      hashcopy[WC_SHA384_DIGEST_SIZE];
    int       ret = 0;

    testVector a, b, c;
    testVector test_sha[3];
    int times = sizeof(test_sha) / sizeof(struct testVector), i;

    a.input  = "";

    a.output = "\x38\xb0\x60\xa7\x51\xac\x96\x38\x4c\xd9\x32\x7e\xb1\xb1\xe3"
               "\x6a\x21\xfd\xb7\x11\x14\xbe\x07\x43\x4c\x0c\xc7\xbf\x63\xf6"
               "\xe1\xda\x27\x4e\xde\xbf\xe7\x6f\x65\xfb\xd5\x1a\xd2\xf1\x48"
               "\x98\xb9\x5b";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = WC_SHA384_DIGEST_SIZE;

    b.input  = "abc";
    b.output = "\xcb\x00\x75\x3f\x45\xa3\x5e\x8b\xb5\xa0\x3d\x69\x9a\xc6\x50"
               "\x07\x27\x2c\x32\xab\x0e\xde\xd1\x63\x1a\x8b\x60\x5a\x43\xff"
               "\x5b\xed\x80\x86\x07\x2b\xa1\xe7\xcc\x23\x58\xba\xec\xa1\x34"
               "\xc8\x25\xa7";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = WC_SHA384_DIGEST_SIZE;

    c.input  = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhi"
               "jklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    c.output = "\x09\x33\x0c\x33\xf7\x11\x47\xe8\x3d\x19\x2f\xc7\x82\xcd\x1b"
               "\x47\x53\x11\x1b\x17\x3b\x3b\x05\xd2\x2f\xa0\x80\x86\xe3\xb0"
               "\xf7\x12\xfc\xc7\xc7\x1a\x55\x7e\x2d\xb9\x66\xc3\xe9\xfa\x91"
               "\x74\x60\x39";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = WC_SHA384_DIGEST_SIZE;

    test_sha[0] = a;
    test_sha[1] = b;
    test_sha[2] = c;

    ret = wc_InitSha384_ex(&sha, HEAP_HINT, devId);
    if (ret != 0)
        return -2300;

    for (i = 0; i < times; ++i) {
        ret = wc_Sha384Update(&sha, (byte*)test_sha[i].input,
            (word32)test_sha[i].inLen);
        if (ret != 0)
            ERROR_OUT(-2310 - i, exit);
        ret = wc_Sha384GetHash(&sha, hashcopy);
        if (ret != 0)
            ERROR_OUT(-2320 - i, exit);
        ret = wc_Sha384Final(&sha, hash);
        if (ret != 0)
            ERROR_OUT(-2330 - i, exit);

        if (XMEMCMP(hash, test_sha[i].output, WC_SHA384_DIGEST_SIZE) != 0)
            ERROR_OUT(-2340 - i, exit);
        if (XMEMCMP(hash, hashcopy, WC_SHA384_DIGEST_SIZE) != 0)
            ERROR_OUT(-2350 - i, exit);
    }

    /* BEGIN LARGE HASH TEST */ {
    byte large_input[1024];
    const char* large_digest =
        "\x37\x01\xdb\xff\x1e\x40\x4f\xe1\xe2\xea\x0b\x40\xbb\x3b\x39\x9a"
        "\xcc\xe8\x44\x8e\x7e\xe5\x64\xb5\x6b\x7f\x56\x64\xa7\x2b\x84\xe3"
        "\xc5\xd7\x79\x03\x25\x90\xf7\xa4\x58\xcb\x97\xa8\x8b\xb1\xa4\x81";

    for (i = 0; i < (int)sizeof(large_input); i++) {
        large_input[i] = (byte)(i & 0xFF);
    }
    times = 100;
    for (i = 0; i < times; ++i) {
        ret = wc_Sha384Update(&sha, (byte*)large_input,
            (word32)sizeof(large_input));
        if (ret != 0)
            ERROR_OUT(-2360, exit);
    }
    ret = wc_Sha384Final(&sha, hash);
    if (ret != 0)
        ERROR_OUT(-2361, exit);
    if (XMEMCMP(hash, large_digest, WC_SHA384_DIGEST_SIZE) != 0)
        ERROR_OUT(-2362, exit);
    } /* END LARGE HASH TEST */

exit:

    wc_Sha384Free(&sha);

    return ret;
}
#endif /* WOLFSSL_SHA384 */

#ifdef WOLFSSL_SHA3
#ifndef WOLFSSL_NOSHA3_224
static int sha3_224_test(void)
{
    Sha3  sha;
    byte  hash[SHA3_224_DIGEST_SIZE];
    byte  hashcopy[SHA3_224_DIGEST_SIZE];

    testVector a, b, c;
    testVector test_sha[3];
    int ret = 0;
    int times = sizeof(test_sha) / sizeof(struct testVector), i;

    a.input  = "";
    a.output = "\x6b\x4e\x03\x42\x36\x67\xdb\xb7\x3b\x6e\x15\x45\x4f\x0e\xb1"
               "\xab\xd4\x59\x7f\x9a\x1b\x07\x8e\x3f\x5b\x5a\x6b\xc7";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = SHA3_224_DIGEST_SIZE;

    b.input  = "abc";
    b.output = "\xe6\x42\x82\x4c\x3f\x8c\xf2\x4a\xd0\x92\x34\xee\x7d\x3c\x76"
               "\x6f\xc9\xa3\xa5\x16\x8d\x0c\x94\xad\x73\xb4\x6f\xdf";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = SHA3_224_DIGEST_SIZE;

    c.input  = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    c.output = "\x8a\x24\x10\x8b\x15\x4a\xda\x21\xc9\xfd\x55\x74\x49\x44\x79"
               "\xba\x5c\x7e\x7a\xb7\x6e\xf2\x64\xea\xd0\xfc\xce\x33";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = SHA3_224_DIGEST_SIZE;

    test_sha[0] = a;
    test_sha[1] = b;
    test_sha[2] = c;

    ret = wc_InitSha3_224(&sha, HEAP_HINT, devId);
    if (ret != 0)
        return -2000;

    for (i = 0; i < times; ++i) {
        ret = wc_Sha3_224_Update(&sha, (byte*)test_sha[i].input,
            (word32)test_sha[i].inLen);
        if (ret != 0)
            ERROR_OUT(-2010 - i, exit);
        ret = wc_Sha3_224_GetHash(&sha, hashcopy);
        if (ret != 0)
            ERROR_OUT(-2020 - i, exit);
        ret = wc_Sha3_224_Final(&sha, hash);
        if (ret != 0)
            ERROR_OUT(-2030 - i, exit);

        if (XMEMCMP(hash, test_sha[i].output, SHA3_224_DIGEST_SIZE) != 0)
            ERROR_OUT(-2040 - i, exit);
        if (XMEMCMP(hash, hashcopy, SHA3_224_DIGEST_SIZE) != 0)
            ERROR_OUT(-2050 - i, exit);
    }

    /* BEGIN LARGE HASH TEST */ {
    byte large_input[1024];
    const char* large_digest =
        "\x13\xe5\xd3\x98\x7b\x94\xda\x41\x12\xc7\x1e\x92\x3a\x19"
        "\x21\x20\x86\x6f\x24\xbf\x0a\x31\xbc\xfd\xd6\x70\x36\xf3";

    for (i = 0; i < (int)sizeof(large_input); i++) {
        large_input[i] = (byte)(i & 0xFF);
    }
    times = 100;
    for (i = 0; i < times; ++i) {
        ret = wc_Sha3_224_Update(&sha, (byte*)large_input,
            (word32)sizeof(large_input));
        if (ret != 0)
            ERROR_OUT(-2060, exit);
    }
    ret = wc_Sha3_224_Final(&sha, hash);
    if (ret != 0)
        ERROR_OUT(-2061, exit);
    if (XMEMCMP(hash, large_digest, SHA3_224_DIGEST_SIZE) != 0)
        ERROR_OUT(-2062, exit);
    } /* END LARGE HASH TEST */

exit:
    wc_Sha3_224_Free(&sha);

    return ret;
}
#endif /* WOLFSSL_NOSHA3_224 */

#ifndef WOLFSSL_NOSHA3_256
static int sha3_256_test(void)
{
    Sha3  sha;
    byte  hash[SHA3_256_DIGEST_SIZE];
    byte  hashcopy[SHA3_256_DIGEST_SIZE];

    testVector a, b, c;
    testVector test_sha[3];
    int ret = 0;
    int times = sizeof(test_sha) / sizeof(struct testVector), i;

    a.input  = "";
    a.output = "\xa7\xff\xc6\xf8\xbf\x1e\xd7\x66\x51\xc1\x47\x56\xa0\x61\xd6"
               "\x62\xf5\x80\xff\x4d\xe4\x3b\x49\xfa\x82\xd8\x0a\x4b\x80\xf8"
               "\x43\x4a";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = SHA3_256_DIGEST_SIZE;

    b.input  = "abc";
    b.output = "\x3a\x98\x5d\xa7\x4f\xe2\x25\xb2\x04\x5c\x17\x2d\x6b\xd3\x90"
               "\xbd\x85\x5f\x08\x6e\x3e\x9d\x52\x5b\x46\xbf\xe2\x45\x11\x43"
               "\x15\x32";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = SHA3_256_DIGEST_SIZE;

    c.input  = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    c.output = "\x41\xc0\xdb\xa2\xa9\xd6\x24\x08\x49\x10\x03\x76\xa8\x23\x5e"
               "\x2c\x82\xe1\xb9\x99\x8a\x99\x9e\x21\xdb\x32\xdd\x97\x49\x6d"
               "\x33\x76";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = SHA3_256_DIGEST_SIZE;

    test_sha[0] = a;
    test_sha[1] = b;
    test_sha[2] = c;

    ret = wc_InitSha3_256(&sha, HEAP_HINT, devId);
    if (ret != 0)
        return -2100;

    for (i = 0; i < times; ++i) {
        ret = wc_Sha3_256_Update(&sha, (byte*)test_sha[i].input,
            (word32)test_sha[i].inLen);
        if (ret != 0)
            ERROR_OUT(-2110 - i, exit);
        ret = wc_Sha3_256_GetHash(&sha, hashcopy);
        if (ret != 0)
            ERROR_OUT(-2120 - i, exit);
        ret = wc_Sha3_256_Final(&sha, hash);
        if (ret != 0)
            ERROR_OUT(-2130 - i, exit);

        if (XMEMCMP(hash, test_sha[i].output, SHA3_256_DIGEST_SIZE) != 0)
            ERROR_OUT(-2140 - i, exit);
        if (XMEMCMP(hash, hashcopy, SHA3_256_DIGEST_SIZE) != 0)
            ERROR_OUT(-2150 - i, exit);
    }

    /* BEGIN LARGE HASH TEST */ {
    byte large_input[1024];
    const char* large_digest =
        "\xdc\x90\xc0\xb1\x25\xdb\x2c\x34\x81\xa3\xff\xbc\x1e\x2e\x87\xeb"
        "\x6d\x70\x85\x61\xe0\xe9\x63\x61\xff\xe5\x84\x4b\x1f\x68\x05\x15";

    for (i = 0; i < (int)sizeof(large_input); i++) {
        large_input[i] = (byte)(i & 0xFF);
    }
    times = 100;
    for (i = 0; i < times; ++i) {
        ret = wc_Sha3_256_Update(&sha, (byte*)large_input,
            (word32)sizeof(large_input));
        if (ret != 0)
            ERROR_OUT(-2160, exit);
    }
    ret = wc_Sha3_256_Final(&sha, hash);
    if (ret != 0)
        ERROR_OUT(-2161, exit);
    if (XMEMCMP(hash, large_digest, SHA3_256_DIGEST_SIZE) != 0)
        ERROR_OUT(-2162, exit);
    } /* END LARGE HASH TEST */

exit:
    wc_Sha3_256_Free(&sha);

    return ret;
}
#endif /* WOLFSSL_NOSHA3_256 */

#ifndef WOLFSSL_NOSHA3_384
static int sha3_384_test(void)
{
    Sha3  sha;
    byte  hash[SHA3_384_DIGEST_SIZE];
    byte  hashcopy[SHA3_384_DIGEST_SIZE];

    testVector a, b, c;
    testVector test_sha[3];
    int ret;
    int times = sizeof(test_sha) / sizeof(struct testVector), i;

    ret = wc_InitSha3_384(&sha, HEAP_HINT, devId);
    if (ret != 0)
        return -2200;

#ifndef WOLFSSL_XILINX_CRYPT
    a.input  = "";
    a.output = "\x0c\x63\xa7\x5b\x84\x5e\x4f\x7d\x01\x10\x7d\x85\x2e\x4c\x24"
               "\x85\xc5\x1a\x50\xaa\xaa\x94\xfc\x61\x99\x5e\x71\xbb\xee\x98"
               "\x3a\x2a\xc3\x71\x38\x31\x26\x4a\xdb\x47\xfb\x6b\xd1\xe0\x58"
               "\xd5\xf0\x04";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = SHA3_384_DIGEST_SIZE;

    b.input  = "abc";
    b.output = "\xec\x01\x49\x82\x88\x51\x6f\xc9\x26\x45\x9f\x58\xe2\xc6\xad"
               "\x8d\xf9\xb4\x73\xcb\x0f\xc0\x8c\x25\x96\xda\x7c\xf0\xe4\x9b"
               "\xe4\xb2\x98\xd8\x8c\xea\x92\x7a\xc7\xf5\x39\xf1\xed\xf2\x28"
               "\x37\x6d\x25";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = SHA3_384_DIGEST_SIZE;

    c.input  = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    c.output = "\x99\x1c\x66\x57\x55\xeb\x3a\x4b\x6b\xbd\xfb\x75\xc7\x8a\x49"
               "\x2e\x8c\x56\xa2\x2c\x5c\x4d\x7e\x42\x9b\xfd\xbc\x32\xb9\xd4"
               "\xad\x5a\xa0\x4a\x1f\x07\x6e\x62\xfe\xa1\x9e\xef\x51\xac\xd0"
               "\x65\x7c\x22";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = SHA3_384_DIGEST_SIZE;

    test_sha[0] = a;
    test_sha[1] = b;
    test_sha[2] = c;

    for (i = 0; i < times; ++i) {
        ret = wc_Sha3_384_Update(&sha, (byte*)test_sha[i].input,
            (word32)test_sha[i].inLen);
        if (ret != 0)
            ERROR_OUT(-2210 - i, exit);
        ret = wc_Sha3_384_GetHash(&sha, hashcopy);
        if (ret != 0)
            ERROR_OUT(-2220 - i, exit);
        ret = wc_Sha3_384_Final(&sha, hash);
        if (ret != 0)
            ERROR_OUT(-2230 - i, exit);

        if (XMEMCMP(hash, test_sha[i].output, SHA3_384_DIGEST_SIZE) != 0)
            ERROR_OUT(-2240 - i, exit);
        if (XMEMCMP(hash, hashcopy, SHA3_384_DIGEST_SIZE) != 0)
            ERROR_OUT(-2250 - i, exit);
    }
#endif

    /* BEGIN LARGE HASH TEST */ {
    byte large_input[1024];
    const char* large_digest =
        "\x30\x44\xec\x17\xef\x47\x9f\x55\x36\x11\xd6\x3f\x8a\x31\x5a\x71"
        "\x8a\x71\xa7\x1d\x8e\x84\xe8\x6c\x24\x02\x2f\x7a\x08\x4e\xea\xd7"
        "\x42\x36\x5d\xa8\xc2\xb7\x42\xad\xec\x19\xfb\xca\xc6\x64\xb3\xa4";

    for (i = 0; i < (int)sizeof(large_input); i++) {
        large_input[i] = (byte)(i & 0xFF);
    }
    times = 100;
    for (i = 0; i < times; ++i) {
        ret = wc_Sha3_384_Update(&sha, (byte*)large_input,
            (word32)sizeof(large_input));
        if (ret != 0)
            ERROR_OUT(-2260, exit);
    }
    ret = wc_Sha3_384_Final(&sha, hash);
    if (ret != 0)
        ERROR_OUT(-2261, exit);
    if (XMEMCMP(hash, large_digest, SHA3_384_DIGEST_SIZE) != 0)
        ERROR_OUT(-2262, exit);
    } /* END LARGE HASH TEST */


    /* BEGIN MORE NIST VECTORS */ {
    a.input  = "\x11\x58\x7d\xcb";
    a.output = "\xcb\x6e\x6c\xe4\xa2\x66\xd4\x38\xdd\xd5\x28\x67\xf2\xe1\x83"
               "\x02\x1b\xe5\x02\x23\xc7\xd5\x7f\x8f\xdc\xaa\x18\x09\x3a\x9d"
               "\x01\x26\x60\x7d\xf0\x26\xc0\x25\xbf\xf4\x0b\xc3\x14\xaf\x43"
               "\xfd\x8a\x08";
    a.inLen  = 4;
    a.outLen = SHA3_384_DIGEST_SIZE;

    b.input  = "\x04\x59\xdc\xbc\x14\x93\x33\xea\x2f\x93\x7b\x77\x9a\x5f\x37"
               "\x28\x14\x84\x49\xa9\xae\xa3\x66\x2c\xdd\x2c\xc6\x53\xce\x6a"
               "\x20\x50\xf9\xc0\xd5\x4b\xf9\x32\x6c\x03\x9b\x26\x3e\xb9";
    b.output = "\x46\x4b\xa4\x09\xfb\xb4\x5e\x98\x5f\x84\xee\x24\x66\x2e\xb7"
               "\xc0\x42\xc3\xc2\xad\x96\x49\xf1\xac\x4a\x8b\x2b\xe9\xc0\x7d"
               "\x37\xed\x2e\x42\x84\x36\x20\x57\x49\x3f\x6a\x7e\x52\xc3\x56"
               "\xb0\x5b\xc5";
    b.inLen  = 44;
    b.outLen = SHA3_384_DIGEST_SIZE;

    c.input  = "\xc0\x4b\x70\x1f\x68\x80\x92\xbb\xd1\xcf\x42\x17\xbc\x4b\x58"
               "\x77\xf2\xe6\x0c\x08\x7b\xda\xc4\x66\x11\x48\x2a\x61\xd5\x1f"
               "\x82\x01\x40\x40\x3b\xc8\x5b\xe0\xc3\x36\x33\x2d\xa0\x93\x87"
               "\x34\xbd\xe8\xc5\x02\x01\x4f\x35\x09\x26\x6c\x73\xc6\xc9\x3c"
               "\x22\xa1\xbd\x0d\xdf\x15\xa5\xce\x74\x10\xc2\x89\x4e\x9d\x09"
               "\x2e\x32\xc0\x79\x92\x2b\xa1\xab\xb7";
    c.output = "\x75\xc5\x85\x50\x3f\x15\xa5\x26\x11\x36\x08\xbc\x18\x31\x80"
               "\xb1\xcb\x80\xf4\xd1\xb4\x66\xc5\x76\xbf\x02\x1b\x1c\xe7\xa1"
               "\x52\x83\x91\xf7\x0e\x10\x44\x66\x81\x84\x9f\xa8\xa6\x43\xcb"
               "\x2b\x68\x28";
    c.inLen  = 84;
    c.outLen = SHA3_384_DIGEST_SIZE;

    test_sha[0] = a;
    test_sha[1] = b;
    test_sha[2] = c;

    wc_Sha3_384_Free(&sha);
    ret = wc_InitSha3_384(&sha, HEAP_HINT, devId);
    if (ret != 0)
        return -2263;

    for (i = 0; i < 3; ++i) {
        ret = wc_Sha3_384_Update(&sha, (byte*)test_sha[i].input,
            (word32)test_sha[i].inLen);
        if (ret != 0)
            ERROR_OUT(-2264 - i, exit);
        ret = wc_Sha3_384_Final(&sha, hash);
        if (ret != 0)
            ERROR_OUT(-2284 - i, exit);

        if (XMEMCMP(hash, test_sha[i].output, SHA3_384_DIGEST_SIZE) != 0)
            ERROR_OUT(-2294 - i, exit);
    }
    } /* END MORE NIST VECTORS */
exit:
    wc_Sha3_384_Free(&sha);

    return ret;
}
#endif /* WOLFSSL_NOSHA3_384 */

#ifndef WOLFSSL_NOSHA3_512
static int sha3_512_test(void)
{
    Sha3  sha;
    byte  hash[SHA3_512_DIGEST_SIZE];
    byte  hashcopy[SHA3_512_DIGEST_SIZE];

    testVector a, b, c;
    testVector test_sha[3];
    int ret;
    int times = sizeof(test_sha) / sizeof(struct testVector), i;

    a.input  = "";
    a.output = "\xa6\x9f\x73\xcc\xa2\x3a\x9a\xc5\xc8\xb5\x67\xdc\x18\x5a\x75"
               "\x6e\x97\xc9\x82\x16\x4f\xe2\x58\x59\xe0\xd1\xdc\xc1\x47\x5c"
               "\x80\xa6\x15\xb2\x12\x3a\xf1\xf5\xf9\x4c\x11\xe3\xe9\x40\x2c"
               "\x3a\xc5\x58\xf5\x00\x19\x9d\x95\xb6\xd3\xe3\x01\x75\x85\x86"
               "\x28\x1d\xcd\x26";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = SHA3_512_DIGEST_SIZE;

    b.input  = "abc";
    b.output = "\xb7\x51\x85\x0b\x1a\x57\x16\x8a\x56\x93\xcd\x92\x4b\x6b\x09"
               "\x6e\x08\xf6\x21\x82\x74\x44\xf7\x0d\x88\x4f\x5d\x02\x40\xd2"
               "\x71\x2e\x10\xe1\x16\xe9\x19\x2a\xf3\xc9\x1a\x7e\xc5\x76\x47"
               "\xe3\x93\x40\x57\x34\x0b\x4c\xf4\x08\xd5\xa5\x65\x92\xf8\x27"
               "\x4e\xec\x53\xf0";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = SHA3_512_DIGEST_SIZE;

    c.input  = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    c.output = "\x04\xa3\x71\xe8\x4e\xcf\xb5\xb8\xb7\x7c\xb4\x86\x10\xfc\xa8"
               "\x18\x2d\xd4\x57\xce\x6f\x32\x6a\x0f\xd3\xd7\xec\x2f\x1e\x91"
               "\x63\x6d\xee\x69\x1f\xbe\x0c\x98\x53\x02\xba\x1b\x0d\x8d\xc7"
               "\x8c\x08\x63\x46\xb5\x33\xb4\x9c\x03\x0d\x99\xa2\x7d\xaf\x11"
               "\x39\xd6\xe7\x5e";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = SHA3_512_DIGEST_SIZE;

    test_sha[0] = a;
    test_sha[1] = b;
    test_sha[2] = c;

    ret = wc_InitSha3_512(&sha, HEAP_HINT, devId);
    if (ret != 0)
        return -2300;

    for (i = 0; i < times; ++i) {
        ret = wc_Sha3_512_Update(&sha, (byte*)test_sha[i].input,
            (word32)test_sha[i].inLen);
        if (ret != 0)
            ERROR_OUT(-2310 - i, exit);
        ret = wc_Sha3_512_GetHash(&sha, hashcopy);
        if (ret != 0)
            ERROR_OUT(-2320 - i, exit);
        ret = wc_Sha3_512_Final(&sha, hash);
        if (ret != 0)
            ERROR_OUT(-2330 - i, exit);

        if (XMEMCMP(hash, test_sha[i].output, SHA3_512_DIGEST_SIZE) != 0)
            ERROR_OUT(-2340 - i, exit);
        if (XMEMCMP(hash, hashcopy, SHA3_512_DIGEST_SIZE) != 0)
            ERROR_OUT(-2350 - i, exit);
    }

    /* BEGIN LARGE HASH TEST */ {
    byte large_input[1024];
    const char* large_digest =
        "\x9c\x13\x26\xb6\x26\xb2\x94\x31\xbc\xf4\x34\xe9\x6f\xf2\xd6\x29"
        "\x9a\xd0\x9b\x32\x63\x2f\x18\xa7\x5f\x23\xc9\x60\xc2\x32\x0c\xbc"
        "\x57\x77\x33\xf1\x83\x81\x8a\xd3\x15\x7c\x93\xdc\x80\x9f\xed\x61"
        "\x41\xa7\x5b\xfd\x32\x0e\x38\x15\xb0\x46\x3b\x7a\x4f\xfd\x44\x88";

    for (i = 0; i < (int)sizeof(large_input); i++) {
        large_input[i] = (byte)(i & 0xFF);
    }
    times = 100;
    for (i = 0; i < times; ++i) {
        ret = wc_Sha3_512_Update(&sha, (byte*)large_input,
            (word32)sizeof(large_input));
        if (ret != 0)
            ERROR_OUT(-2360, exit);
    }
    ret = wc_Sha3_512_Final(&sha, hash);
    if (ret != 0)
        ERROR_OUT(-2361, exit);
    if (XMEMCMP(hash, large_digest, SHA3_512_DIGEST_SIZE) != 0)
        ERROR_OUT(-2362, exit);
    } /* END LARGE HASH TEST */

exit:
    wc_Sha3_512_Free(&sha);

    return ret;
}
#endif /* WOLFSSL_NOSHA3_512 */

int sha3_test(void)
{
    int ret;

#ifndef WOLFSSL_NOSHA3_224
    if ((ret = sha3_224_test()) != 0)
        return ret;
#endif
#ifndef WOLFSSL_NOSHA3_256
    if ((ret = sha3_256_test()) != 0)
        return ret;
#endif
#ifndef WOLFSSL_NOSHA3_384
    if ((ret = sha3_384_test()) != 0)
        return ret;
#endif
#ifndef WOLFSSL_NOSHA3_512
    if ((ret = sha3_512_test()) != 0)
        return ret;
#endif

    return 0;
}
#endif


int hash_test(void)
{
    wc_HashAlg       hash;
    int              ret, exp_ret;
    int              i, j;
    byte             data[] = "0123456789abcdef0123456789abcdef012345";
    byte             out[MAX_DIGEST_SIZE];
    enum wc_HashType typesGood[] = { WC_HASH_TYPE_MD5, WC_HASH_TYPE_SHA,
                                     WC_HASH_TYPE_SHA224, WC_HASH_TYPE_SHA256,
                                     WC_HASH_TYPE_SHA384, WC_HASH_TYPE_SHA512 };
    enum wc_HashType typesNoImpl[] = {
#ifdef NO_MD5
                                        WC_HASH_TYPE_MD5,
#endif
#ifdef NO_SHA
                                        WC_HASH_TYPE_SHA,
#endif
#ifndef WOLFSSL_SHA224
                                        WC_HASH_TYPE_SHA224,
#endif
#ifdef NO_SHA256
                                        WC_HASH_TYPE_SHA256,
#endif
#ifndef WOLFSSL_SHA384
                                        WC_HASH_TYPE_SHA384,
#endif
#ifndef WOLFSSL_SHA512
                                        WC_HASH_TYPE_SHA512,
#endif
                                        WC_HASH_TYPE_NONE
                                     };
    enum wc_HashType typesBad[]  = { WC_HASH_TYPE_NONE, WC_HASH_TYPE_MD5_SHA,
                                     WC_HASH_TYPE_MD2, WC_HASH_TYPE_MD4 };

    /* Parameter Validation testing. */
    ret = wc_HashInit(NULL, WC_HASH_TYPE_SHA256);
    if (ret != BAD_FUNC_ARG)
        return -2400;
    ret = wc_HashUpdate(NULL, WC_HASH_TYPE_SHA256, NULL, sizeof(data));
    if (ret != BAD_FUNC_ARG)
        return -2401;
    ret = wc_HashUpdate(&hash, WC_HASH_TYPE_SHA256, NULL, sizeof(data));
    if (ret != BAD_FUNC_ARG)
        return -2402;
    ret = wc_HashUpdate(NULL, WC_HASH_TYPE_SHA256, data, sizeof(data));
    if (ret != BAD_FUNC_ARG)
        return -2403;
    ret = wc_HashFinal(NULL, WC_HASH_TYPE_SHA256, NULL);
    if (ret != BAD_FUNC_ARG)
        return -2404;
    ret = wc_HashFinal(&hash, WC_HASH_TYPE_SHA256, NULL);
    if (ret != BAD_FUNC_ARG)
        return -2405;
    ret = wc_HashFinal(NULL, WC_HASH_TYPE_SHA256, out);
    if (ret != BAD_FUNC_ARG)
        return -2406;

    /* Try invalid hash algorithms. */
    for (i = 0; i < (int)(sizeof(typesBad)/sizeof(*typesBad)); i++) {
        ret = wc_HashInit(&hash, typesBad[i]);
        if (ret != BAD_FUNC_ARG)
            return -2407 - i;
        ret = wc_HashUpdate(&hash, typesBad[i], data, sizeof(data));
        if (ret != BAD_FUNC_ARG)
            return -2417 - i;
        ret = wc_HashFinal(&hash, typesBad[i], out);
        if (ret != BAD_FUNC_ARG)
            return -2427 - i;
    }

    /* Try valid hash algorithms. */
    for (i = 0, j = 0; i < (int)(sizeof(typesGood)/sizeof(*typesGood)); i++) {
        exp_ret = 0;
        if (typesGood[i] == typesNoImpl[j]) {
            /* Recognized but no implementation compiled in. */
            exp_ret = HASH_TYPE_E;
            j++;
        }
        ret = wc_HashInit(&hash, typesGood[i]);
        if (ret != exp_ret)
            return -2437 - i;
        ret = wc_HashUpdate(&hash, typesGood[i], data, sizeof(data));
        if (ret != exp_ret)
            return -2447 - i;
        ret = wc_HashFinal(&hash, typesGood[i], out);
        if (ret != exp_ret)
            return -2457 - i;
#if !defined(NO_ASN) || !defined(NO_DH) || defined(HAVE_ECC)
        ret = wc_HashGetOID(typesGood[i]);
        if (ret == BAD_FUNC_ARG ||
                (exp_ret == 0 && ret == HASH_TYPE_E) ||
                (exp_ret != 0 && ret != HASH_TYPE_E)) {
            return -2467 - i;
        }
#endif /* !defined(NO_ASN) || !defined(NO_DH) || defined(HAVE_ECC) */
    }

#if !defined(NO_ASN) || !defined(NO_DH) || defined(HAVE_ECC)
    ret = wc_HashGetOID(WC_HASH_TYPE_MD2);
#ifdef WOLFSSL_MD2
    if (ret == HASH_TYPE_E || ret == BAD_FUNC_ARG)
        return -2477;
#else
    if (ret != HASH_TYPE_E)
        return -2478;
#endif
    ret = wc_HashGetOID(WC_HASH_TYPE_MD5_SHA);
#ifndef NO_MD5
    if (ret == HASH_TYPE_E || ret == BAD_FUNC_ARG)
        return -2479;
#else
    if (ret != HASH_TYPE_E)
        return -2480;
#endif
    ret = wc_HashGetOID(WC_HASH_TYPE_MD4);
    if (ret != BAD_FUNC_ARG)
        return -2481;
    ret = wc_HashGetOID(WC_HASH_TYPE_NONE);
    if (ret != BAD_FUNC_ARG)
        return -2482;
#endif /* !defined(NO_ASN) || !defined(NO_DH) || defined(HAVE_ECC) */

#ifndef NO_ASN
#ifdef WOLFSSL_MD2
    ret = wc_GetCTC_HashOID(MD2);
    if (ret == 0)
        return -2483;
#endif
#ifndef NO_MD5
    ret = wc_GetCTC_HashOID(WC_MD5);
    if (ret == 0)
        return -2484;
#endif
#ifndef NO_SHA
    ret = wc_GetCTC_HashOID(WC_SHA);
    if (ret == 0)
        return -2485;
#endif
#ifdef WOLFSSL_SHA224
    ret = wc_GetCTC_HashOID(WC_SHA224);
    if (ret == 0)
        return -2486;
#endif
#ifndef NO_SHA256
    ret = wc_GetCTC_HashOID(WC_SHA256);
    if (ret == 0)
        return -2487;
#endif
#ifdef WOLFSSL_SHA384
    ret = wc_GetCTC_HashOID(WC_SHA384);
    if (ret == 0)
        return -2488;
#endif
#ifdef WOLFSSL_SHA512
    ret = wc_GetCTC_HashOID(WC_SHA512);
    if (ret == 0)
        return -2489;
#endif
    ret = wc_GetCTC_HashOID(-1);
    if (ret != 0)
        return -2490;
#endif

    return 0;
}

#if !defined(NO_HMAC) && !defined(NO_MD5)
int hmac_md5_test(void)
{
    Hmac hmac;
    byte hash[WC_MD5_DIGEST_SIZE];

    const char* keys[]=
    {
        "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b",
        "Jefe",
        "\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
    };

    testVector a, b, c;
    testVector test_hmac[3];

    int ret;
    int times = sizeof(test_hmac) / sizeof(testVector), i;

    a.input  = "Hi There";
    a.output = "\x92\x94\x72\x7a\x36\x38\xbb\x1c\x13\xf4\x8e\xf8\x15\x8b\xfc"
               "\x9d";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = WC_MD5_DIGEST_SIZE;

    b.input  = "what do ya want for nothing?";
    b.output = "\x75\x0c\x78\x3e\x6a\xb0\xb5\x03\xea\xa8\x6e\x31\x0a\x5d\xb7"
               "\x38";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = WC_MD5_DIGEST_SIZE;

    c.input  = "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD";
    c.output = "\x56\xbe\x34\x52\x1d\x14\x4c\x88\xdb\xb8\xc7\x33\xf0\xe8\xb3"
               "\xf6";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = WC_MD5_DIGEST_SIZE;

    test_hmac[0] = a;
    test_hmac[1] = b;
    test_hmac[2] = c;

    for (i = 0; i < times; ++i) {
    #if defined(HAVE_FIPS) || defined(HAVE_CAVIUM)
        if (i == 1) {
            continue; /* cavium can't handle short keys, fips not allowed */
        }
    #endif

        if (wc_HmacInit(&hmac, HEAP_HINT, devId) != 0) {
            return -2500;
        }

        ret = wc_HmacSetKey(&hmac, WC_MD5, (byte*)keys[i],
            (word32)XSTRLEN(keys[i]));
        if (ret != 0)
            return -2501;
        ret = wc_HmacUpdate(&hmac, (byte*)test_hmac[i].input,
                   (word32)test_hmac[i].inLen);
        if (ret != 0)
            return -2502;
        ret = wc_HmacFinal(&hmac, hash);
        if (ret != 0)
            return -2503;

        if (XMEMCMP(hash, test_hmac[i].output, WC_MD5_DIGEST_SIZE) != 0)
            return -2504 - i;

        wc_HmacFree(&hmac);
    }

#ifndef HAVE_FIPS
    if (wc_HmacSizeByType(WC_MD5) != WC_MD5_DIGEST_SIZE)
        return -2514;
#endif

    return 0;
}
#endif /* NO_HMAC && NO_MD5 */

#if !defined(NO_HMAC) && !defined(NO_SHA)
int hmac_sha_test(void)
{
    Hmac hmac;
    byte hash[WC_SHA_DIGEST_SIZE];

    const char* keys[]=
    {
        "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b"
                                                                "\x0b\x0b\x0b",
        "Jefe",
        "\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
                                                                "\xAA\xAA\xAA"
    };

    testVector a, b, c;
    testVector test_hmac[3];

    int ret;
    int times = sizeof(test_hmac) / sizeof(testVector), i;

    a.input  = "Hi There";
    a.output = "\xb6\x17\x31\x86\x55\x05\x72\x64\xe2\x8b\xc0\xb6\xfb\x37\x8c"
               "\x8e\xf1\x46\xbe\x00";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = WC_SHA_DIGEST_SIZE;

    b.input  = "what do ya want for nothing?";
    b.output = "\xef\xfc\xdf\x6a\xe5\xeb\x2f\xa2\xd2\x74\x16\xd5\xf1\x84\xdf"
               "\x9c\x25\x9a\x7c\x79";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = WC_SHA_DIGEST_SIZE;

    c.input  = "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD";
    c.output = "\x12\x5d\x73\x42\xb9\xac\x11\xcd\x91\xa3\x9a\xf4\x8a\xa1\x7b"
               "\x4f\x63\xf1\x75\xd3";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = WC_SHA_DIGEST_SIZE;

    test_hmac[0] = a;
    test_hmac[1] = b;
    test_hmac[2] = c;

    for (i = 0; i < times; ++i) {
#if defined(HAVE_FIPS) || defined(HAVE_CAVIUM)
        if (i == 1)
            continue; /* cavium can't handle short keys, fips not allowed */
#endif

        if (wc_HmacInit(&hmac, HEAP_HINT, devId) != 0)
            return -20010;

        ret = wc_HmacSetKey(&hmac, WC_SHA, (byte*)keys[i],
            (word32)XSTRLEN(keys[i]));
        if (ret != 0)
            return -2601;
        ret = wc_HmacUpdate(&hmac, (byte*)test_hmac[i].input,
                   (word32)test_hmac[i].inLen);
        if (ret != 0)
            return -2602;
        ret = wc_HmacFinal(&hmac, hash);
        if (ret != 0)
            return -2603;

        if (XMEMCMP(hash, test_hmac[i].output, WC_SHA_DIGEST_SIZE) != 0)
            return -2604 - i;

        wc_HmacFree(&hmac);
    }

#ifndef HAVE_FIPS
    if (wc_HmacSizeByType(WC_SHA) != WC_SHA_DIGEST_SIZE)
        return -2614;
#endif

    return 0;
}
#endif


#if !defined(NO_HMAC) && defined(WOLFSSL_SHA224)
int hmac_sha224_test(void)
{
    Hmac hmac;
    byte hash[WC_SHA224_DIGEST_SIZE];

    const char* keys[]=
    {
        "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b"
                                                                "\x0b\x0b\x0b",
        "Jefe",
        "\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
                                                                "\xAA\xAA\xAA"
    };

    testVector a, b, c;
    testVector test_hmac[3];

    int ret;
    int times = sizeof(test_hmac) / sizeof(testVector), i;

    a.input  = "Hi There";
    a.output = "\x89\x6f\xb1\x12\x8a\xbb\xdf\x19\x68\x32\x10\x7c\xd4\x9d\xf3"
               "\x3f\x47\xb4\xb1\x16\x99\x12\xba\x4f\x53\x68\x4b\x22";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = WC_SHA224_DIGEST_SIZE;

    b.input  = "what do ya want for nothing?";
    b.output = "\xa3\x0e\x01\x09\x8b\xc6\xdb\xbf\x45\x69\x0f\x3a\x7e\x9e\x6d"
               "\x0f\x8b\xbe\xa2\xa3\x9e\x61\x48\x00\x8f\xd0\x5e\x44";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = WC_SHA224_DIGEST_SIZE;

    c.input  = "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD";
    c.output = "\x7f\xb3\xcb\x35\x88\xc6\xc1\xf6\xff\xa9\x69\x4d\x7d\x6a\xd2"
               "\x64\x93\x65\xb0\xc1\xf6\x5d\x69\xd1\xec\x83\x33\xea";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = WC_SHA224_DIGEST_SIZE;

    test_hmac[0] = a;
    test_hmac[1] = b;
    test_hmac[2] = c;

    for (i = 0; i < times; ++i) {
#if defined(HAVE_FIPS) || defined(HAVE_CAVIUM)
        if (i == 1)
            continue; /* cavium can't handle short keys, fips not allowed */
#endif

        if (wc_HmacInit(&hmac, HEAP_HINT, devId) != 0)
            return -2700;

        ret = wc_HmacSetKey(&hmac, WC_SHA224, (byte*)keys[i],
            (word32)XSTRLEN(keys[i]));
        if (ret != 0)
            return -2701;
        ret = wc_HmacUpdate(&hmac, (byte*)test_hmac[i].input,
                   (word32)test_hmac[i].inLen);
        if (ret != 0)
            return -2702;
        ret = wc_HmacFinal(&hmac, hash);
        if (ret != 0)
            return -2703;

        if (XMEMCMP(hash, test_hmac[i].output, WC_SHA224_DIGEST_SIZE) != 0)
            return -2704 - i;

        wc_HmacFree(&hmac);
    }

#ifndef HAVE_FIPS
    if (wc_HmacSizeByType(WC_SHA224) != WC_SHA224_DIGEST_SIZE)
        return -2714;
#endif

    return 0;
}
#endif


#if !defined(NO_HMAC) && !defined(NO_SHA256)
int hmac_sha256_test(void)
{
    Hmac hmac;
    byte hash[WC_SHA256_DIGEST_SIZE];

    const char* keys[]=
    {
        "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b"
                                                                "\x0b\x0b\x0b",
        "Jefe",
        "\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
                                                                "\xAA\xAA\xAA"
    };

    testVector a, b, c;
    testVector test_hmac[3];

    int ret;
    int times = sizeof(test_hmac) / sizeof(testVector), i;

    a.input  = "Hi There";
    a.output = "\xb0\x34\x4c\x61\xd8\xdb\x38\x53\x5c\xa8\xaf\xce\xaf\x0b\xf1"
               "\x2b\x88\x1d\xc2\x00\xc9\x83\x3d\xa7\x26\xe9\x37\x6c\x2e\x32"
               "\xcf\xf7";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = WC_SHA256_DIGEST_SIZE;

    b.input  = "what do ya want for nothing?";
    b.output = "\x5b\xdc\xc1\x46\xbf\x60\x75\x4e\x6a\x04\x24\x26\x08\x95\x75"
               "\xc7\x5a\x00\x3f\x08\x9d\x27\x39\x83\x9d\xec\x58\xb9\x64\xec"
               "\x38\x43";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = WC_SHA256_DIGEST_SIZE;

    c.input  = "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD";
    c.output = "\x77\x3e\xa9\x1e\x36\x80\x0e\x46\x85\x4d\xb8\xeb\xd0\x91\x81"
               "\xa7\x29\x59\x09\x8b\x3e\xf8\xc1\x22\xd9\x63\x55\x14\xce\xd5"
               "\x65\xfe";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = WC_SHA256_DIGEST_SIZE;

    test_hmac[0] = a;
    test_hmac[1] = b;
    test_hmac[2] = c;

    for (i = 0; i < times; ++i) {
#if defined(HAVE_FIPS) || defined(HAVE_CAVIUM)
        if (i == 1)
            continue; /* cavium can't handle short keys, fips not allowed */
#endif

        if (wc_HmacInit(&hmac, HEAP_HINT, devId) != 0)
            return -2800;

        ret = wc_HmacSetKey(&hmac, WC_SHA256, (byte*)keys[i],
            (word32)XSTRLEN(keys[i]));
        if (ret != 0)
            return -2801;
        ret = wc_HmacUpdate(&hmac, (byte*)test_hmac[i].input,
                   (word32)test_hmac[i].inLen);
        if (ret != 0)
            return -2802;
        ret = wc_HmacFinal(&hmac, hash);
        if (ret != 0)
            return -2803;

        if (XMEMCMP(hash, test_hmac[i].output, WC_SHA256_DIGEST_SIZE) != 0)
            return -2804 - i;

        wc_HmacFree(&hmac);
    }

#ifndef HAVE_FIPS
    if (wc_HmacSizeByType(WC_SHA256) != WC_SHA256_DIGEST_SIZE)
        return -2814;
    if (wc_HmacSizeByType(20) != BAD_FUNC_ARG)
        return -2815;
#endif
    if (wolfSSL_GetHmacMaxSize() != MAX_DIGEST_SIZE)
        return -2816;

    return 0;
}
#endif


#if !defined(NO_HMAC) && defined(HAVE_BLAKE2)
int hmac_blake2b_test(void)
{
    Hmac hmac;
    byte hash[BLAKE2B_256];

    const char* keys[]=
    {
        "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b"
                                                                "\x0b\x0b\x0b",
        "Jefe",
        "\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
                                                                "\xAA\xAA\xAA"
    };

    testVector a, b, c;
    testVector test_hmac[3];

    int ret;
    int times = sizeof(test_hmac) / sizeof(testVector), i;

    a.input  = "Hi There";
    a.output = "\x72\x93\x0d\xdd\xf5\xf7\xe1\x78\x38\x07\x44\x18\x0b\x3f\x51"
               "\x37\x25\xb5\x82\xc2\x08\x83\x2f\x1c\x99\xfd\x03\xa0\x16\x75"
               "\xac\xfd";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = BLAKE2B_256;

    b.input  = "what do ya want for nothing?";
    b.output = "\x3d\x20\x50\x71\x05\xc0\x8c\x0c\x38\x44\x1e\xf7\xf9\xd1\x67"
               "\x21\xff\x64\xf5\x94\x00\xcf\xf9\x75\x41\xda\x88\x61\x9d\x7c"
               "\xda\x2b";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = BLAKE2B_256;

    c.input  = "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD";
    c.output = "\xda\xfe\x2a\x24\xfc\xe7\xea\x36\x34\xbe\x41\x92\xc7\x11\xa7"
               "\x00\xae\x53\x9c\x11\x9c\x80\x74\x55\x22\x25\x4a\xb9\x55\xd3"
               "\x0f\x87";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = BLAKE2B_256;

    test_hmac[0] = a;
    test_hmac[1] = b;
    test_hmac[2] = c;

    for (i = 0; i < times; ++i) {
#if defined(HAVE_FIPS) || defined(HAVE_CAVIUM)
        if (i == 1)
            continue; /* cavium can't handle short keys, fips not allowed */
#endif

    #if defined(HAVE_CAVIUM) && !defined(HAVE_CAVIUM_V)
        /* Blake2 only supported on Cavium Nitrox III */
        if (wc_HmacInit(&hmac, HEAP_HINT, devId) != 0)
            return -2900;
    #endif

        ret = wc_HmacSetKey(&hmac, BLAKE2B_ID, (byte*)keys[i],
                         (word32)XSTRLEN(keys[i]));
        if (ret != 0)
            return -2901;
        ret = wc_HmacUpdate(&hmac, (byte*)test_hmac[i].input,
                   (word32)test_hmac[i].inLen);
        if (ret != 0)
            return -2902;
        ret = wc_HmacFinal(&hmac, hash);
        if (ret != 0)
            return -2903;

        if (XMEMCMP(hash, test_hmac[i].output, BLAKE2B_256) != 0)
            return -2904 - i;

        wc_HmacFree(&hmac);
    }

#ifndef HAVE_FIPS
    if (wc_HmacSizeByType(BLAKE2B_ID) != BLAKE2B_OUTBYTES)
        return -2914;
#endif

    return 0;
}
#endif


#if !defined(NO_HMAC) && defined(WOLFSSL_SHA384)
int hmac_sha384_test(void)
{
    Hmac hmac;
    byte hash[WC_SHA384_DIGEST_SIZE];

    const char* keys[]=
    {
        "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b"
                                                                "\x0b\x0b\x0b",
        "Jefe",
        "\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
                                                                "\xAA\xAA\xAA"
    };

    testVector a, b, c;
    testVector test_hmac[3];

    int ret;
    int times = sizeof(test_hmac) / sizeof(testVector), i;

    a.input  = "Hi There";
    a.output = "\xaf\xd0\x39\x44\xd8\x48\x95\x62\x6b\x08\x25\xf4\xab\x46\x90"
               "\x7f\x15\xf9\xda\xdb\xe4\x10\x1e\xc6\x82\xaa\x03\x4c\x7c\xeb"
               "\xc5\x9c\xfa\xea\x9e\xa9\x07\x6e\xde\x7f\x4a\xf1\x52\xe8\xb2"
               "\xfa\x9c\xb6";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = WC_SHA384_DIGEST_SIZE;

    b.input  = "what do ya want for nothing?";
    b.output = "\xaf\x45\xd2\xe3\x76\x48\x40\x31\x61\x7f\x78\xd2\xb5\x8a\x6b"
               "\x1b\x9c\x7e\xf4\x64\xf5\xa0\x1b\x47\xe4\x2e\xc3\x73\x63\x22"
               "\x44\x5e\x8e\x22\x40\xca\x5e\x69\xe2\xc7\x8b\x32\x39\xec\xfa"
               "\xb2\x16\x49";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = WC_SHA384_DIGEST_SIZE;

    c.input  = "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD";
    c.output = "\x88\x06\x26\x08\xd3\xe6\xad\x8a\x0a\xa2\xac\xe0\x14\xc8\xa8"
               "\x6f\x0a\xa6\x35\xd9\x47\xac\x9f\xeb\xe8\x3e\xf4\xe5\x59\x66"
               "\x14\x4b\x2a\x5a\xb3\x9d\xc1\x38\x14\xb9\x4e\x3a\xb6\xe1\x01"
               "\xa3\x4f\x27";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = WC_SHA384_DIGEST_SIZE;

    test_hmac[0] = a;
    test_hmac[1] = b;
    test_hmac[2] = c;

    for (i = 0; i < times; ++i) {
#if defined(HAVE_FIPS)
        if (i == 1)
            continue; /* fips not allowed */
#endif

        if (wc_HmacInit(&hmac, HEAP_HINT, devId) != 0)
            return -3000;

        ret = wc_HmacSetKey(&hmac, WC_SHA384, (byte*)keys[i],
            (word32)XSTRLEN(keys[i]));
        if (ret != 0)
            return -3001;
        ret = wc_HmacUpdate(&hmac, (byte*)test_hmac[i].input,
                   (word32)test_hmac[i].inLen);
        if (ret != 0)
            return -3002;
        ret = wc_HmacFinal(&hmac, hash);
        if (ret != 0)
            return -3003;

        if (XMEMCMP(hash, test_hmac[i].output, WC_SHA384_DIGEST_SIZE) != 0)
            return -3004 - i;

        wc_HmacFree(&hmac);
    }

#ifndef HAVE_FIPS
    if (wc_HmacSizeByType(WC_SHA384) != WC_SHA384_DIGEST_SIZE)
        return -3013;
#endif

    return 0;
}
#endif


#if !defined(NO_HMAC) && defined(WOLFSSL_SHA512)
int hmac_sha512_test(void)
{
    Hmac hmac;
    byte hash[WC_SHA512_DIGEST_SIZE];

    const char* keys[]=
    {
        "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b"
                                                                "\x0b\x0b\x0b",
        "Jefe",
        "\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
                                                                "\xAA\xAA\xAA"
    };

    testVector a, b, c;
    testVector test_hmac[3];

    int ret;
    int times = sizeof(test_hmac) / sizeof(testVector), i;

    a.input  = "Hi There";
    a.output = "\x87\xaa\x7c\xde\xa5\xef\x61\x9d\x4f\xf0\xb4\x24\x1a\x1d\x6c"
               "\xb0\x23\x79\xf4\xe2\xce\x4e\xc2\x78\x7a\xd0\xb3\x05\x45\xe1"
               "\x7c\xde\xda\xa8\x33\xb7\xd6\xb8\xa7\x02\x03\x8b\x27\x4e\xae"
               "\xa3\xf4\xe4\xbe\x9d\x91\x4e\xeb\x61\xf1\x70\x2e\x69\x6c\x20"
               "\x3a\x12\x68\x54";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = WC_SHA512_DIGEST_SIZE;

    b.input  = "what do ya want for nothing?";
    b.output = "\x16\x4b\x7a\x7b\xfc\xf8\x19\xe2\xe3\x95\xfb\xe7\x3b\x56\xe0"
               "\xa3\x87\xbd\x64\x22\x2e\x83\x1f\xd6\x10\x27\x0c\xd7\xea\x25"
               "\x05\x54\x97\x58\xbf\x75\xc0\x5a\x99\x4a\x6d\x03\x4f\x65\xf8"
               "\xf0\xe6\xfd\xca\xea\xb1\xa3\x4d\x4a\x6b\x4b\x63\x6e\x07\x0a"
               "\x38\xbc\xe7\x37";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = WC_SHA512_DIGEST_SIZE;

    c.input  = "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
               "\xDD\xDD\xDD\xDD\xDD\xDD";
    c.output = "\xfa\x73\xb0\x08\x9d\x56\xa2\x84\xef\xb0\xf0\x75\x6c\x89\x0b"
               "\xe9\xb1\xb5\xdb\xdd\x8e\xe8\x1a\x36\x55\xf8\x3e\x33\xb2\x27"
               "\x9d\x39\xbf\x3e\x84\x82\x79\xa7\x22\xc8\x06\xb4\x85\xa4\x7e"
               "\x67\xc8\x07\xb9\x46\xa3\x37\xbe\xe8\x94\x26\x74\x27\x88\x59"
               "\xe1\x32\x92\xfb";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = WC_SHA512_DIGEST_SIZE;

    test_hmac[0] = a;
    test_hmac[1] = b;
    test_hmac[2] = c;

    for (i = 0; i < times; ++i) {
#if defined(HAVE_FIPS)
        if (i == 1)
            continue; /* fips not allowed */
#endif

        if (wc_HmacInit(&hmac, HEAP_HINT, devId) != 0)
            return -3100;

        ret = wc_HmacSetKey(&hmac, WC_SHA512, (byte*)keys[i],
            (word32)XSTRLEN(keys[i]));
        if (ret != 0)
            return -3101;
        ret = wc_HmacUpdate(&hmac, (byte*)test_hmac[i].input,
                   (word32)test_hmac[i].inLen);
        if (ret != 0)
            return -3102;
        ret = wc_HmacFinal(&hmac, hash);
        if (ret != 0)
            return -3103;

        if (XMEMCMP(hash, test_hmac[i].output, WC_SHA512_DIGEST_SIZE) != 0)
            return -3104 - i;

        wc_HmacFree(&hmac);
    }

#ifndef HAVE_FIPS
    if (wc_HmacSizeByType(WC_SHA512) != WC_SHA512_DIGEST_SIZE)
        return -3113;
#endif

    return 0;
}
#endif


#ifndef NO_RC4
int arc4_test(void)
{
    byte cipher[16];
    byte plain[16];

    const char* keys[] =
    {
        "\x01\x23\x45\x67\x89\xab\xcd\xef",
        "\x01\x23\x45\x67\x89\xab\xcd\xef",
        "\x00\x00\x00\x00\x00\x00\x00\x00",
        "\xef\x01\x23\x45"
    };

    testVector a, b, c, d;
    testVector test_arc4[4];

    int times = sizeof(test_arc4) / sizeof(testVector), i;

    a.input  = "\x01\x23\x45\x67\x89\xab\xcd\xef";
    a.output = "\x75\xb7\x87\x80\x99\xe0\xc5\x96";
    a.inLen  = 8;
    a.outLen = 8;

    b.input  = "\x00\x00\x00\x00\x00\x00\x00\x00";
    b.output = "\x74\x94\xc2\xe7\x10\x4b\x08\x79";
    b.inLen  = 8;
    b.outLen = 8;

    c.input  = "\x00\x00\x00\x00\x00\x00\x00\x00";
    c.output = "\xde\x18\x89\x41\xa3\x37\x5d\x3a";
    c.inLen  = 8;
    c.outLen = 8;

    d.input  = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    d.output = "\xd6\xa1\x41\xa7\xec\x3c\x38\xdf\xbd\x61";
    d.inLen  = 10;
    d.outLen = 10;

    test_arc4[0] = a;
    test_arc4[1] = b;
    test_arc4[2] = c;
    test_arc4[3] = d;

    for (i = 0; i < times; ++i) {
        Arc4 enc;
        Arc4 dec;
        int  keylen = 8;  /* XSTRLEN with key 0x00 not good */
        if (i == 3)
            keylen = 4;

        if (wc_Arc4Init(&enc, HEAP_HINT, devId) != 0)
            return -3200;
        if (wc_Arc4Init(&dec, HEAP_HINT, devId) != 0)
            return -3201;

        wc_Arc4SetKey(&enc, (byte*)keys[i], keylen);
        wc_Arc4SetKey(&dec, (byte*)keys[i], keylen);

        wc_Arc4Process(&enc, cipher, (byte*)test_arc4[i].input,
                    (word32)test_arc4[i].outLen);
        wc_Arc4Process(&dec, plain,  cipher, (word32)test_arc4[i].outLen);

        if (XMEMCMP(plain, test_arc4[i].input, test_arc4[i].outLen))
            return -3202 - i;

        if (XMEMCMP(cipher, test_arc4[i].output, test_arc4[i].outLen))
            return -3212 - i;

        wc_Arc4Free(&enc);
        wc_Arc4Free(&dec);
    }

    return 0;
}
#endif


int hc128_test(void)
{
#ifdef HAVE_HC128
    byte cipher[16];
    byte plain[16];

    const char* keys[] =
    {
        "\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
        "\x00\x53\xA6\xF9\x4C\x9F\xF2\x45\x98\xEB\x3E\x91\xE4\x37\x8A\xDD",
        "\x0F\x62\xB5\x08\x5B\xAE\x01\x54\xA7\xFA\x4D\xA0\xF3\x46\x99\xEC"
    };

    const char* ivs[] =
    {
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
        "\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
        "\x0D\x74\xDB\x42\xA9\x10\x77\xDE\x45\xAC\x13\x7A\xE1\x48\xAF\x16",
        "\x28\x8F\xF6\x5D\xC4\x2B\x92\xF9\x60\xC7\x2E\x95\xFC\x63\xCA\x31"
    };


    testVector a, b, c, d;
    testVector test_hc128[4];

    int times = sizeof(test_hc128) / sizeof(testVector), i;

    a.input  = "\x00\x00\x00\x00\x00\x00\x00\x00";
    a.output = "\x37\x86\x02\xB9\x8F\x32\xA7\x48";
    a.inLen  = 8;
    a.outLen = 8;

    b.input  = "\x00\x00\x00\x00\x00\x00\x00\x00";
    b.output = "\x33\x7F\x86\x11\xC6\xED\x61\x5F";
    b.inLen  = 8;
    b.outLen = 8;

    c.input  = "\x00\x00\x00\x00\x00\x00\x00\x00";
    c.output = "\x2E\x1E\xD1\x2A\x85\x51\xC0\x5A";
    c.inLen  = 8;
    c.outLen = 8;

    d.input  = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    d.output = "\x1C\xD8\xAE\xDD\xFE\x52\xE2\x17\xE8\x35\xD0\xB7\xE8\x4E\x29";
    d.inLen  = 15;
    d.outLen = 15;

    test_hc128[0] = a;
    test_hc128[1] = b;
    test_hc128[2] = c;
    test_hc128[3] = d;

    for (i = 0; i < times; ++i) {
        HC128 enc;
        HC128 dec;

        /* align keys/ivs in plain/cipher buffers */
        XMEMCPY(plain,  keys[i], 16);
        XMEMCPY(cipher, ivs[i],  16);

        wc_Hc128_SetKey(&enc, plain, cipher);
        wc_Hc128_SetKey(&dec, plain, cipher);

        /* align input */
        XMEMCPY(plain, test_hc128[i].input, test_hc128[i].outLen);
        if (wc_Hc128_Process(&enc, cipher, plain,
                                           (word32)test_hc128[i].outLen) != 0) {
            return -3300;
        }
        if (wc_Hc128_Process(&dec, plain, cipher,
                                           (word32)test_hc128[i].outLen) != 0) {
            return -3301;
        }

        if (XMEMCMP(plain, test_hc128[i].input, test_hc128[i].outLen))
            return -3302 - i;

        if (XMEMCMP(cipher, test_hc128[i].output, test_hc128[i].outLen))
            return -3312 - i;
    }

#endif /* HAVE_HC128 */
    return 0;
}


#ifndef NO_RABBIT
int rabbit_test(void)
{
    byte cipher[16];
    byte plain[16];

    const char* keys[] =
    {
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
        "\xAC\xC3\x51\xDC\xF1\x62\xFC\x3B\xFE\x36\x3D\x2E\x29\x13\x28\x91"
    };

    const char* ivs[] =
    {
        "\x00\x00\x00\x00\x00\x00\x00\x00",
        "\x59\x7E\x26\xC1\x75\xF5\x73\xC3",
        0
    };

    testVector a, b, c;
    testVector test_rabbit[3];

    int times = sizeof(test_rabbit) / sizeof(testVector), i;

    a.input  = "\x00\x00\x00\x00\x00\x00\x00\x00";
    a.output = "\xED\xB7\x05\x67\x37\x5D\xCD\x7C";
    a.inLen  = 8;
    a.outLen = 8;

    b.input  = "\x00\x00\x00\x00\x00\x00\x00\x00";
    b.output = "\x6D\x7D\x01\x22\x92\xCC\xDC\xE0";
    b.inLen  = 8;
    b.outLen = 8;

    c.input  = "\x00\x00\x00\x00\x00\x00\x00\x00";
    c.output = "\x04\xCE\xCA\x7A\x1A\x86\x6E\x77";
    c.inLen  = 8;
    c.outLen = 8;

    test_rabbit[0] = a;
    test_rabbit[1] = b;
    test_rabbit[2] = c;

    for (i = 0; i < times; ++i) {
        Rabbit enc;
        Rabbit dec;
        byte*  iv;

        /* align keys/ivs in plain/cipher buffers */
        XMEMCPY(plain,  keys[i], 16);
        if (ivs[i]) {
            XMEMCPY(cipher, ivs[i],   8);
            iv = cipher;
        } else
            iv = NULL;
        wc_RabbitSetKey(&enc, plain, iv);
        wc_RabbitSetKey(&dec, plain, iv);

        /* align input */
        XMEMCPY(plain, test_rabbit[i].input, test_rabbit[i].outLen);
        wc_RabbitProcess(&enc, cipher, plain,  (word32)test_rabbit[i].outLen);
        wc_RabbitProcess(&dec, plain,  cipher, (word32)test_rabbit[i].outLen);

        if (XMEMCMP(plain, test_rabbit[i].input, test_rabbit[i].outLen))
            return -3400 - i;

        if (XMEMCMP(cipher, test_rabbit[i].output, test_rabbit[i].outLen))
            return -3410 - i;
    }

    return 0;
}
#endif /* NO_RABBIT */


#ifdef HAVE_CHACHA
int chacha_test(void)
{
    ChaCha enc;
    ChaCha dec;
    byte   cipher[128];
    byte   plain[128];
    byte   sliver[64];
    byte   input[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    word32 keySz = 32;
    int    ret = 0;
    int    i;
    int    times = 4;

    static const byte key1[] =
    {
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };

    static const byte key2[] =
    {
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01
    };

    static const byte key3[] =
    {
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };

    /* 128 bit key */
    static const byte key4[] =
    {
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };


    const byte* keys[] = {key1, key2, key3, key4};

    static const byte ivs1[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const byte ivs2[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const byte ivs3[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
    static const byte ivs4[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};


    const byte* ivs[] = {ivs1, ivs2, ivs3, ivs4};


    byte a[] = {0x76,0xb8,0xe0,0xad,0xa0,0xf1,0x3d,0x90};
    byte b[] = {0x45,0x40,0xf0,0x5a,0x9f,0x1f,0xb2,0x96};
    byte c[] = {0xde,0x9c,0xba,0x7b,0xf3,0xd6,0x9e,0xf5};
    byte d[] = {0x89,0x67,0x09,0x52,0x60,0x83,0x64,0xfd};

    byte* test_chacha[4];

    test_chacha[0] = a;
    test_chacha[1] = b;
    test_chacha[2] = c;
    test_chacha[3] = d;

    for (i = 0; i < times; ++i) {
        if (i < 3) {
            keySz = 32;
        }
        else {
            keySz = 16;
        }

        XMEMCPY(plain, keys[i], keySz);
        XMEMSET(cipher, 0, 32);
        XMEMCPY(cipher + 4, ivs[i], 8);

        ret |= wc_Chacha_SetKey(&enc, keys[i], keySz);
        ret |= wc_Chacha_SetKey(&dec, keys[i], keySz);
        if (ret != 0)
            return ret;

        ret |= wc_Chacha_SetIV(&enc, cipher, 0);
        ret |= wc_Chacha_SetIV(&dec, cipher, 0);
        if (ret != 0)
            return ret;
        XMEMCPY(plain, input, 8);

        ret |= wc_Chacha_Process(&enc, cipher, plain,  (word32)8);
        ret |= wc_Chacha_Process(&dec, plain,  cipher, (word32)8);
        if (ret != 0)
            return ret;

        if (XMEMCMP(test_chacha[i], cipher, 8))
            return -3500 - i;

        if (XMEMCMP(plain, input, 8))
            return -3510 - i;
    }

    /* test of starting at a different counter
       encrypts all of the information and decrypts starting at 2nd chunk */
    XMEMSET(plain,  0, sizeof(plain));
    XMEMSET(sliver, 1, sizeof(sliver)); /* set as 1's to not match plain */
    XMEMSET(cipher, 0, sizeof(cipher));
    XMEMCPY(cipher + 4, ivs[0], 8);

    ret |= wc_Chacha_SetKey(&enc, keys[0], keySz);
    ret |= wc_Chacha_SetKey(&dec, keys[0], keySz);
    if (ret != 0)
        return ret;

    ret |= wc_Chacha_SetIV(&enc, cipher, 0);
    ret |= wc_Chacha_SetIV(&dec, cipher, 1);
    if (ret != 0)
        return ret;

    ret |= wc_Chacha_Process(&enc, cipher, plain,  sizeof(plain));
    ret |= wc_Chacha_Process(&dec, sliver,  cipher + 64, sizeof(sliver));
    if (ret != 0)
        return ret;

    if (XMEMCMP(plain + 64, sliver, 64))
        return -3520;

    return 0;
}
#endif /* HAVE_CHACHA */


#ifdef HAVE_POLY1305
int poly1305_test(void)
{
    int      ret = 0;
    int      i;
    byte     tag[16];
    Poly1305 enc;

    static const byte msg1[] =
    {
        0x43,0x72,0x79,0x70,0x74,0x6f,0x67,0x72,
        0x61,0x70,0x68,0x69,0x63,0x20,0x46,0x6f,
        0x72,0x75,0x6d,0x20,0x52,0x65,0x73,0x65,
        0x61,0x72,0x63,0x68,0x20,0x47,0x72,0x6f,
        0x75,0x70
    };

    static const byte msg2[] =
    {
        0x48,0x65,0x6c,0x6c,0x6f,0x20,0x77,0x6f,0x72,
        0x6c,0x64,0x21
    };

    static const byte msg3[] =
    {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };

    static const byte msg4[] =
    {
        0xd3,0x1a,0x8d,0x34,0x64,0x8e,0x60,0xdb,
        0x7b,0x86,0xaf,0xbc,0x53,0xef,0x7e,0xc2,
        0xa4,0xad,0xed,0x51,0x29,0x6e,0x08,0xfe,
        0xa9,0xe2,0xb5,0xa7,0x36,0xee,0x62,0xd6,
        0x3d,0xbe,0xa4,0x5e,0x8c,0xa9,0x67,0x12,
        0x82,0xfa,0xfb,0x69,0xda,0x92,0x72,0x8b,
        0x1a,0x71,0xde,0x0a,0x9e,0x06,0x0b,0x29,
        0x05,0xd6,0xa5,0xb6,0x7e,0xcd,0x3b,0x36,
        0x92,0xdd,0xbd,0x7f,0x2d,0x77,0x8b,0x8c,
        0x98,0x03,0xae,0xe3,0x28,0x09,0x1b,0x58,
        0xfa,0xb3,0x24,0xe4,0xfa,0xd6,0x75,0x94,
        0x55,0x85,0x80,0x8b,0x48,0x31,0xd7,0xbc,
        0x3f,0xf4,0xde,0xf0,0x8e,0x4b,0x7a,0x9d,
        0xe5,0x76,0xd2,0x65,0x86,0xce,0xc6,0x4b,
        0x61,0x16
    };

    static const byte msg5[] =
    {
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    };

    byte additional[] =
    {
        0x50,0x51,0x52,0x53,0xc0,0xc1,0xc2,0xc3,
        0xc4,0xc5,0xc6,0xc7
    };

    static const byte correct0[] =
    {
        0x01,0x03,0x80,0x8a,0xfb,0x0d,0xb2,0xfd,
        0x4a,0xbf,0xf6,0xaf,0x41,0x49,0xf5,0x1b
    };

    static const byte correct1[] =
    {
        0xa8,0x06,0x1d,0xc1,0x30,0x51,0x36,0xc6,
        0xc2,0x2b,0x8b,0xaf,0x0c,0x01,0x27,0xa9
    };

    static const byte correct2[] =
    {
        0xa6,0xf7,0x45,0x00,0x8f,0x81,0xc9,0x16,
        0xa2,0x0d,0xcc,0x74,0xee,0xf2,0xb2,0xf0
    };

    static const byte correct3[] =
    {
        0x49,0xec,0x78,0x09,0x0e,0x48,0x1e,0xc6,
        0xc2,0x6b,0x33,0xb9,0x1c,0xcc,0x03,0x07
    };

    static const byte correct4[] =
    {
        0x1a,0xe1,0x0b,0x59,0x4f,0x09,0xe2,0x6a,
        0x7e,0x90,0x2e,0xcb,0xd0,0x60,0x06,0x91
    };

    static const byte correct5[] =
    {
        0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };

    static const byte key[] = {
        0x85,0xd6,0xbe,0x78,0x57,0x55,0x6d,0x33,
        0x7f,0x44,0x52,0xfe,0x42,0xd5,0x06,0xa8,
        0x01,0x03,0x80,0x8a,0xfb,0x0d,0xb2,0xfd,
        0x4a,0xbf,0xf6,0xaf,0x41,0x49,0xf5,0x1b
    };

    static const byte key2[] = {
        0x74,0x68,0x69,0x73,0x20,0x69,0x73,0x20,
        0x33,0x32,0x2d,0x62,0x79,0x74,0x65,0x20,
        0x6b,0x65,0x79,0x20,0x66,0x6f,0x72,0x20,
        0x50,0x6f,0x6c,0x79,0x31,0x33,0x30,0x35
    };

    static const byte key4[] = {
        0x7b,0xac,0x2b,0x25,0x2d,0xb4,0x47,0xaf,
        0x09,0xb6,0x7a,0x55,0xa4,0xe9,0x55,0x84,
        0x0a,0xe1,0xd6,0x73,0x10,0x75,0xd9,0xeb,
        0x2a,0x93,0x75,0x78,0x3e,0xd5,0x53,0xff
    };

    static const byte key5[] = {
        0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };

    const byte* msgs[]  = {NULL, msg1, msg2, msg3, msg5};
    word32      szm[]   = {0, sizeof(msg1), sizeof(msg2),
                           sizeof(msg3), sizeof(msg5)};
    const byte* keys[]  = {key, key, key2, key2, key5};
    const byte* tests[] = {correct0, correct1, correct2, correct3, correct5};

    for (i = 0; i < 5; i++) {
        ret = wc_Poly1305SetKey(&enc, keys[i], 32);
        if (ret != 0)
            return -3600 + i;

        ret = wc_Poly1305Update(&enc, msgs[i], szm[i]);
        if (ret != 0)
            return -3605 + i;

        ret = wc_Poly1305Final(&enc, tag);
        if (ret != 0)
            return -36108 + i;

        if (XMEMCMP(tag, tests[i], sizeof(tag)))
            return -3615 + i;
    }

    /* Check TLS MAC function from 2.8.2 https://tools.ietf.org/html/rfc7539 */
    XMEMSET(tag, 0, sizeof(tag));
    ret = wc_Poly1305SetKey(&enc, key4, sizeof(key4));
    if (ret != 0)
        return -3614;

    ret = wc_Poly1305_MAC(&enc, additional, sizeof(additional),
                                   (byte*)msg4, sizeof(msg4), tag, sizeof(tag));
    if (ret != 0)
        return -3615;

    if (XMEMCMP(tag, correct4, sizeof(tag)))
        return -3616;

    /* Check fail of TLS MAC function if altering additional data */
    XMEMSET(tag, 0, sizeof(tag));
	additional[0]++;
    ret = wc_Poly1305_MAC(&enc, additional, sizeof(additional),
                                   (byte*)msg4, sizeof(msg4), tag, sizeof(tag));
    if (ret != 0)
        return -3617;

    if (XMEMCMP(tag, correct4, sizeof(tag)) == 0)
        return -3618;


    return 0;
}
#endif /* HAVE_POLY1305 */


#if defined(HAVE_CHACHA) && defined(HAVE_POLY1305)
int chacha20_poly1305_aead_test(void)
{
    /* Test #1 from Section 2.8.2 of draft-irtf-cfrg-chacha20-poly1305-10 */
    /* https://tools.ietf.org/html/draft-irtf-cfrg-chacha20-poly1305-10  */

    const byte key1[] = {
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f
    };

    const byte plaintext1[] = {
        0x4c, 0x61, 0x64, 0x69, 0x65, 0x73, 0x20, 0x61,
        0x6e, 0x64, 0x20, 0x47, 0x65, 0x6e, 0x74, 0x6c,
        0x65, 0x6d, 0x65, 0x6e, 0x20, 0x6f, 0x66, 0x20,
        0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x61, 0x73,
        0x73, 0x20, 0x6f, 0x66, 0x20, 0x27, 0x39, 0x39,
        0x3a, 0x20, 0x49, 0x66, 0x20, 0x49, 0x20, 0x63,
        0x6f, 0x75, 0x6c, 0x64, 0x20, 0x6f, 0x66, 0x66,
        0x65, 0x72, 0x20, 0x79, 0x6f, 0x75, 0x20, 0x6f,
        0x6e, 0x6c, 0x79, 0x20, 0x6f, 0x6e, 0x65, 0x20,
        0x74, 0x69, 0x70, 0x20, 0x66, 0x6f, 0x72, 0x20,
        0x74, 0x68, 0x65, 0x20, 0x66, 0x75, 0x74, 0x75,
        0x72, 0x65, 0x2c, 0x20, 0x73, 0x75, 0x6e, 0x73,
        0x63, 0x72, 0x65, 0x65, 0x6e, 0x20, 0x77, 0x6f,
        0x75, 0x6c, 0x64, 0x20, 0x62, 0x65, 0x20, 0x69,
        0x74, 0x2e
    };

    const byte iv1[] = {
        0x07, 0x00, 0x00, 0x00, 0x40, 0x41, 0x42, 0x43,
        0x44, 0x45, 0x46, 0x47
    };

    const byte aad1[] = { /* additional data */
        0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7
    };

    const byte cipher1[] = { /* expected output from operation */
        0xd3, 0x1a, 0x8d, 0x34, 0x64, 0x8e, 0x60, 0xdb,
        0x7b, 0x86, 0xaf, 0xbc, 0x53, 0xef, 0x7e, 0xc2,
        0xa4, 0xad, 0xed, 0x51, 0x29, 0x6e, 0x08, 0xfe,
        0xa9, 0xe2, 0xb5, 0xa7, 0x36, 0xee, 0x62, 0xd6,
        0x3d, 0xbe, 0xa4, 0x5e, 0x8c, 0xa9, 0x67, 0x12,
        0x82, 0xfa, 0xfb, 0x69, 0xda, 0x92, 0x72, 0x8b,
        0x1a, 0x71, 0xde, 0x0a, 0x9e, 0x06, 0x0b, 0x29,
        0x05, 0xd6, 0xa5, 0xb6, 0x7e, 0xcd, 0x3b, 0x36,
        0x92, 0xdd, 0xbd, 0x7f, 0x2d, 0x77, 0x8b, 0x8c,
        0x98, 0x03, 0xae, 0xe3, 0x28, 0x09, 0x1b, 0x58,
        0xfa, 0xb3, 0x24, 0xe4, 0xfa, 0xd6, 0x75, 0x94,
        0x55, 0x85, 0x80, 0x8b, 0x48, 0x31, 0xd7, 0xbc,
        0x3f, 0xf4, 0xde, 0xf0, 0x8e, 0x4b, 0x7a, 0x9d,
        0xe5, 0x76, 0xd2, 0x65, 0x86, 0xce, 0xc6, 0x4b,
        0x61, 0x16
    };

    const byte authTag1[] = { /* expected output from operation */
        0x1a, 0xe1, 0x0b, 0x59, 0x4f, 0x09, 0xe2, 0x6a,
        0x7e, 0x90, 0x2e, 0xcb, 0xd0, 0x60, 0x06, 0x91
    };

    /* Test #2 from Appendix A.2 in draft-irtf-cfrg-chacha20-poly1305-10 */
    /* https://tools.ietf.org/html/draft-irtf-cfrg-chacha20-poly1305-10  */

    const byte key2[] = {
        0x1c, 0x92, 0x40, 0xa5, 0xeb, 0x55, 0xd3, 0x8a,
        0xf3, 0x33, 0x88, 0x86, 0x04, 0xf6, 0xb5, 0xf0,
        0x47, 0x39, 0x17, 0xc1, 0x40, 0x2b, 0x80, 0x09,
        0x9d, 0xca, 0x5c, 0xbc, 0x20, 0x70, 0x75, 0xc0
    };

    const byte plaintext2[] = {
        0x49, 0x6e, 0x74, 0x65, 0x72, 0x6e, 0x65, 0x74,
        0x2d, 0x44, 0x72, 0x61, 0x66, 0x74, 0x73, 0x20,
        0x61, 0x72, 0x65, 0x20, 0x64, 0x72, 0x61, 0x66,
        0x74, 0x20, 0x64, 0x6f, 0x63, 0x75, 0x6d, 0x65,
        0x6e, 0x74, 0x73, 0x20, 0x76, 0x61, 0x6c, 0x69,
        0x64, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x61, 0x20,
        0x6d, 0x61, 0x78, 0x69, 0x6d, 0x75, 0x6d, 0x20,
        0x6f, 0x66, 0x20, 0x73, 0x69, 0x78, 0x20, 0x6d,
        0x6f, 0x6e, 0x74, 0x68, 0x73, 0x20, 0x61, 0x6e,
        0x64, 0x20, 0x6d, 0x61, 0x79, 0x20, 0x62, 0x65,
        0x20, 0x75, 0x70, 0x64, 0x61, 0x74, 0x65, 0x64,
        0x2c, 0x20, 0x72, 0x65, 0x70, 0x6c, 0x61, 0x63,
        0x65, 0x64, 0x2c, 0x20, 0x6f, 0x72, 0x20, 0x6f,
        0x62, 0x73, 0x6f, 0x6c, 0x65, 0x74, 0x65, 0x64,
        0x20, 0x62, 0x79, 0x20, 0x6f, 0x74, 0x68, 0x65,
        0x72, 0x20, 0x64, 0x6f, 0x63, 0x75, 0x6d, 0x65,
        0x6e, 0x74, 0x73, 0x20, 0x61, 0x74, 0x20, 0x61,
        0x6e, 0x79, 0x20, 0x74, 0x69, 0x6d, 0x65, 0x2e,
        0x20, 0x49, 0x74, 0x20, 0x69, 0x73, 0x20, 0x69,
        0x6e, 0x61, 0x70, 0x70, 0x72, 0x6f, 0x70, 0x72,
        0x69, 0x61, 0x74, 0x65, 0x20, 0x74, 0x6f, 0x20,
        0x75, 0x73, 0x65, 0x20, 0x49, 0x6e, 0x74, 0x65,
        0x72, 0x6e, 0x65, 0x74, 0x2d, 0x44, 0x72, 0x61,
        0x66, 0x74, 0x73, 0x20, 0x61, 0x73, 0x20, 0x72,
        0x65, 0x66, 0x65, 0x72, 0x65, 0x6e, 0x63, 0x65,
        0x20, 0x6d, 0x61, 0x74, 0x65, 0x72, 0x69, 0x61,
        0x6c, 0x20, 0x6f, 0x72, 0x20, 0x74, 0x6f, 0x20,
        0x63, 0x69, 0x74, 0x65, 0x20, 0x74, 0x68, 0x65,
        0x6d, 0x20, 0x6f, 0x74, 0x68, 0x65, 0x72, 0x20,
        0x74, 0x68, 0x61, 0x6e, 0x20, 0x61, 0x73, 0x20,
        0x2f, 0xe2, 0x80, 0x9c, 0x77, 0x6f, 0x72, 0x6b,
        0x20, 0x69, 0x6e, 0x20, 0x70, 0x72, 0x6f, 0x67,
        0x72, 0x65, 0x73, 0x73, 0x2e, 0x2f, 0xe2, 0x80,
        0x9d
    };

    const byte iv2[] = {
        0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08
    };

    const byte aad2[] = { /* additional data */
        0xf3, 0x33, 0x88, 0x86, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x4e, 0x91
    };

    const byte cipher2[] = { /* expected output from operation */
        0x64, 0xa0, 0x86, 0x15, 0x75, 0x86, 0x1a, 0xf4,
        0x60, 0xf0, 0x62, 0xc7, 0x9b, 0xe6, 0x43, 0xbd,
        0x5e, 0x80, 0x5c, 0xfd, 0x34, 0x5c, 0xf3, 0x89,
        0xf1, 0x08, 0x67, 0x0a, 0xc7, 0x6c, 0x8c, 0xb2,
        0x4c, 0x6c, 0xfc, 0x18, 0x75, 0x5d, 0x43, 0xee,
        0xa0, 0x9e, 0xe9, 0x4e, 0x38, 0x2d, 0x26, 0xb0,
        0xbd, 0xb7, 0xb7, 0x3c, 0x32, 0x1b, 0x01, 0x00,
        0xd4, 0xf0, 0x3b, 0x7f, 0x35, 0x58, 0x94, 0xcf,
        0x33, 0x2f, 0x83, 0x0e, 0x71, 0x0b, 0x97, 0xce,
        0x98, 0xc8, 0xa8, 0x4a, 0xbd, 0x0b, 0x94, 0x81,
        0x14, 0xad, 0x17, 0x6e, 0x00, 0x8d, 0x33, 0xbd,
        0x60, 0xf9, 0x82, 0xb1, 0xff, 0x37, 0xc8, 0x55,
        0x97, 0x97, 0xa0, 0x6e, 0xf4, 0xf0, 0xef, 0x61,
        0xc1, 0x86, 0x32, 0x4e, 0x2b, 0x35, 0x06, 0x38,
        0x36, 0x06, 0x90, 0x7b, 0x6a, 0x7c, 0x02, 0xb0,
        0xf9, 0xf6, 0x15, 0x7b, 0x53, 0xc8, 0x67, 0xe4,
        0xb9, 0x16, 0x6c, 0x76, 0x7b, 0x80, 0x4d, 0x46,
        0xa5, 0x9b, 0x52, 0x16, 0xcd, 0xe7, 0xa4, 0xe9,
        0x90, 0x40, 0xc5, 0xa4, 0x04, 0x33, 0x22, 0x5e,
        0xe2, 0x82, 0xa1, 0xb0, 0xa0, 0x6c, 0x52, 0x3e,
        0xaf, 0x45, 0x34, 0xd7, 0xf8, 0x3f, 0xa1, 0x15,
        0x5b, 0x00, 0x47, 0x71, 0x8c, 0xbc, 0x54, 0x6a,
        0x0d, 0x07, 0x2b, 0x04, 0xb3, 0x56, 0x4e, 0xea,
        0x1b, 0x42, 0x22, 0x73, 0xf5, 0x48, 0x27, 0x1a,
        0x0b, 0xb2, 0x31, 0x60, 0x53, 0xfa, 0x76, 0x99,
        0x19, 0x55, 0xeb, 0xd6, 0x31, 0x59, 0x43, 0x4e,
        0xce, 0xbb, 0x4e, 0x46, 0x6d, 0xae, 0x5a, 0x10,
        0x73, 0xa6, 0x72, 0x76, 0x27, 0x09, 0x7a, 0x10,
        0x49, 0xe6, 0x17, 0xd9, 0x1d, 0x36, 0x10, 0x94,
        0xfa, 0x68, 0xf0, 0xff, 0x77, 0x98, 0x71, 0x30,
        0x30, 0x5b, 0xea, 0xba, 0x2e, 0xda, 0x04, 0xdf,
        0x99, 0x7b, 0x71, 0x4d, 0x6c, 0x6f, 0x2c, 0x29,
        0xa6, 0xad, 0x5c, 0xb4, 0x02, 0x2b, 0x02, 0x70,
        0x9b
    };

    const byte authTag2[] = { /* expected output from operation */
        0xee, 0xad, 0x9d, 0x67, 0x89, 0x0c, 0xbb, 0x22,
        0x39, 0x23, 0x36, 0xfe, 0xa1, 0x85, 0x1f, 0x38
    };

    byte generatedCiphertext[272];
    byte generatedPlaintext[272];
    byte generatedAuthTag[CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE];
    int err;

    XMEMSET(generatedCiphertext, 0, sizeof(generatedCiphertext));
    XMEMSET(generatedAuthTag, 0, sizeof(generatedAuthTag));
    XMEMSET(generatedPlaintext, 0, sizeof(generatedPlaintext));

    /* Parameter Validation testing */
    /* Encrypt */
    err = wc_ChaCha20Poly1305_Encrypt(NULL, iv1, aad1, sizeof(aad1), plaintext1,
            sizeof(plaintext1), generatedCiphertext, generatedAuthTag);
    if (err != BAD_FUNC_ARG)
        return -3700;
    err = wc_ChaCha20Poly1305_Encrypt(key1, NULL, aad1, sizeof(aad1),
            plaintext1, sizeof(plaintext1), generatedCiphertext,
            generatedAuthTag);
    if (err != BAD_FUNC_ARG)
        return -3701;
    err = wc_ChaCha20Poly1305_Encrypt(key1, iv1, aad1, sizeof(aad1), NULL,
            sizeof(plaintext1), generatedCiphertext, generatedAuthTag);
    if (err != BAD_FUNC_ARG)
        return -3702;
    err = wc_ChaCha20Poly1305_Encrypt(key1, iv1, aad1, sizeof(aad1), plaintext1,
            sizeof(plaintext1), NULL, generatedAuthTag);
    if (err != BAD_FUNC_ARG)
        return -3703;
    err = wc_ChaCha20Poly1305_Encrypt(key1, iv1, aad1, sizeof(aad1), plaintext1,
            sizeof(plaintext1), generatedCiphertext, NULL);
    if (err != BAD_FUNC_ARG)
        return -3704;
    err = wc_ChaCha20Poly1305_Encrypt(key1, iv1, aad1, sizeof(aad1), plaintext1,
            0, generatedCiphertext, generatedAuthTag);
    if (err != BAD_FUNC_ARG)
        return -3705;
    /* Decrypt */
    err = wc_ChaCha20Poly1305_Decrypt(NULL, iv2, aad2, sizeof(aad2), cipher2,
            sizeof(cipher2), authTag2, generatedPlaintext);
    if (err != BAD_FUNC_ARG)
        return -3706;
    err = wc_ChaCha20Poly1305_Decrypt(key2, NULL, aad2, sizeof(aad2), cipher2,
            sizeof(cipher2), authTag2, generatedPlaintext);
    if (err != BAD_FUNC_ARG)
        return -3707;
    err = wc_ChaCha20Poly1305_Decrypt(key2, iv2, aad2, sizeof(aad2), NULL,
            sizeof(cipher2), authTag2, generatedPlaintext);
    if (err != BAD_FUNC_ARG)
        return -3708;
    err = wc_ChaCha20Poly1305_Decrypt(key2, iv2, aad2, sizeof(aad2), cipher2,
            sizeof(cipher2), NULL, generatedPlaintext);
    if (err != BAD_FUNC_ARG)
        return -3709;
    err = wc_ChaCha20Poly1305_Decrypt(key2, iv2, aad2, sizeof(aad2), cipher2,
            sizeof(cipher2), authTag2, NULL);
    if (err != BAD_FUNC_ARG)
        return -3710;
    err = wc_ChaCha20Poly1305_Decrypt(key2, iv2, aad2, sizeof(aad2), cipher2,
            0, authTag2, generatedPlaintext);
    if (err != BAD_FUNC_ARG)
        return -3711;

    /* Test #1 */

    err = wc_ChaCha20Poly1305_Encrypt(key1, iv1,
                                       aad1, sizeof(aad1),
                                       plaintext1, sizeof(plaintext1),
                                       generatedCiphertext, generatedAuthTag);
    if (err) {
        return err;
    }

    /* -- Check the ciphertext and authtag */

    if (XMEMCMP(generatedCiphertext, cipher1, sizeof(cipher1))) {
        return -3712;
    }

    if (XMEMCMP(generatedAuthTag, authTag1, sizeof(authTag1))) {
        return -3713;
    }

    /* -- Verify decryption works */

    err = wc_ChaCha20Poly1305_Decrypt(key1, iv1,
                                       aad1, sizeof(aad1),
                                       cipher1, sizeof(cipher1),
                                       authTag1, generatedPlaintext);
    if (err) {
        return err;
    }

    if (XMEMCMP(generatedPlaintext, plaintext1, sizeof( plaintext1))) {
        return -3714;
    }

    XMEMSET(generatedCiphertext, 0, sizeof(generatedCiphertext));
    XMEMSET(generatedAuthTag, 0, sizeof(generatedAuthTag));
    XMEMSET(generatedPlaintext, 0, sizeof(generatedPlaintext));

    /* Test #2 */

    err = wc_ChaCha20Poly1305_Encrypt(key2, iv2,
                                       aad2, sizeof(aad2),
                                       plaintext2, sizeof(plaintext2),
                                       generatedCiphertext, generatedAuthTag);
    if (err) {
        return err;
    }

    /* -- Check the ciphertext and authtag */

    if (XMEMCMP(generatedCiphertext, cipher2, sizeof(cipher2))) {
        return -3715;
    }

    if (XMEMCMP(generatedAuthTag, authTag2, sizeof(authTag2))) {
        return -3716;
    }

    /* -- Verify decryption works */

    err = wc_ChaCha20Poly1305_Decrypt(key2, iv2,
                                      aad2, sizeof(aad2),
                                      cipher2, sizeof(cipher2),
                                      authTag2, generatedPlaintext);
    if (err) {
        return err;
    }

    if (XMEMCMP(generatedPlaintext, plaintext2, sizeof(plaintext2))) {
        return -3717;
    }

    return err;
}
#endif /* HAVE_CHACHA && HAVE_POLY1305 */


#ifndef NO_DES3
int des_test(void)
{
    const byte vector[] = { /* "now is the time for all " w/o trailing 0 */
        0x6e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,
        0x68,0x65,0x20,0x74,0x69,0x6d,0x65,0x20,
        0x66,0x6f,0x72,0x20,0x61,0x6c,0x6c,0x20
    };

    byte plain[24];
    byte cipher[24];

    Des enc;
    Des dec;

    const byte key[] =
    {
        0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef
    };

    const byte iv[] =
    {
        0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef
    };

    const byte verify[] =
    {
        0x8b,0x7c,0x52,0xb0,0x01,0x2b,0x6c,0xb8,
        0x4f,0x0f,0xeb,0xf3,0xfb,0x5f,0x86,0x73,
        0x15,0x85,0xb3,0x22,0x4b,0x86,0x2b,0x4b
    };

    int ret;

    ret = wc_Des_SetKey(&enc, key, iv, DES_ENCRYPTION);
    if (ret != 0)
        return -3800;

    ret = wc_Des_CbcEncrypt(&enc, cipher, vector, sizeof(vector));
    if (ret != 0)
        return -3801;

    ret = wc_Des_SetKey(&dec, key, iv, DES_DECRYPTION);
    if (ret != 0)
        return -3802;

    ret = wc_Des_CbcDecrypt(&dec, plain, cipher, sizeof(cipher));
    if (ret != 0)
        return -3803;

    if (XMEMCMP(plain, vector, sizeof(plain)))
        return -3804;

    if (XMEMCMP(cipher, verify, sizeof(cipher)))
        return -3805;

    return 0;
}
#endif /* NO_DES3 */


#ifndef NO_DES3
int des3_test(void)
{
    const byte vector[] = { /* "Now is the time for all " w/o trailing 0 */
        0x4e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,
        0x68,0x65,0x20,0x74,0x69,0x6d,0x65,0x20,
        0x66,0x6f,0x72,0x20,0x61,0x6c,0x6c,0x20
    };

    byte plain[24];
    byte cipher[24];

    Des3 enc;
    Des3 dec;

    const byte key3[] =
    {
        0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
        0xfe,0xde,0xba,0x98,0x76,0x54,0x32,0x10,
        0x89,0xab,0xcd,0xef,0x01,0x23,0x45,0x67
    };
    const byte iv3[] =
    {
        0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef,
        0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
        0x11,0x21,0x31,0x41,0x51,0x61,0x71,0x81

    };

    const byte verify3[] =
    {
        0x43,0xa0,0x29,0x7e,0xd1,0x84,0xf8,0x0e,
        0x89,0x64,0x84,0x32,0x12,0xd5,0x08,0x98,
        0x18,0x94,0x15,0x74,0x87,0x12,0x7d,0xb0
    };

    int ret;


    if (wc_Des3Init(&enc, HEAP_HINT, devId) != 0)
        return -3900;
    if (wc_Des3Init(&dec, HEAP_HINT, devId) != 0)
        return -3901;

    ret = wc_Des3_SetKey(&enc, key3, iv3, DES_ENCRYPTION);
    if (ret != 0)
        return -3902;
    ret = wc_Des3_SetKey(&dec, key3, iv3, DES_DECRYPTION);
    if (ret != 0)
        return -3903;
    ret = wc_Des3_CbcEncrypt(&enc, cipher, vector, sizeof(vector));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -3904;
    ret = wc_Des3_CbcDecrypt(&dec, plain, cipher, sizeof(cipher));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &dec.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -3905;

    if (XMEMCMP(plain, vector, sizeof(plain)))
        return -3906;

    if (XMEMCMP(cipher, verify3, sizeof(cipher)))
        return -3907;

    wc_Des3Free(&enc);
    wc_Des3Free(&dec);

    return 0;
}
#endif /* NO_DES */


#ifndef NO_AES
static int aes_key_size_test(void)
{
    int    ret;
    Aes    aes;
    byte   key16[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                       0x38, 0x39, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66 };
    byte   key24[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                       0x38, 0x39, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
                       0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37 };
    byte   key32[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                       0x38, 0x39, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
                       0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                       0x38, 0x39, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66 };
    byte   iv[]    = "1234567890abcdef";
#ifndef HAVE_FIPS
    word32 keySize;
#endif

#ifdef WC_INITAES_H
    ret = wc_InitAes_h(NULL, NULL);
    if (ret != BAD_FUNC_ARG)
        return -4000;
    ret = wc_InitAes_h(&aes, NULL);
    if (ret != 0)
        return -4001;
#endif

#ifndef HAVE_FIPS
    /* Parameter Validation testing. */
    ret = wc_AesGetKeySize(NULL, NULL);
    if (ret != BAD_FUNC_ARG)
        return -4002;
    ret = wc_AesGetKeySize(&aes, NULL);
    if (ret != BAD_FUNC_ARG)
        return -4003;
    ret = wc_AesGetKeySize(NULL, &keySize);
    if (ret != BAD_FUNC_ARG)
        return -4004;
    /* Crashes in FIPS */
    ret = wc_AesSetKey(NULL, key16, sizeof(key16), iv, AES_ENCRYPTION);
    if (ret != BAD_FUNC_ARG)
        return -4005;
#endif
    /* NULL IV indicates to use all zeros IV. */
    ret = wc_AesSetKey(&aes, key16, sizeof(key16), NULL, AES_ENCRYPTION);
    if (ret != 0)
        return -4006;
    ret = wc_AesSetKey(&aes, key32, sizeof(key32) - 1, iv, AES_ENCRYPTION);
    if (ret != BAD_FUNC_ARG)
        return -4007;
#ifndef HAVE_FIPS
    /* Force invalid rounds */
    aes.rounds = 16;
    ret = wc_AesGetKeySize(&aes, &keySize);
    if (ret != BAD_FUNC_ARG)
        return -4008;
#endif

    ret = wc_AesSetKey(&aes, key16, sizeof(key16), iv, AES_ENCRYPTION);
    if (ret != 0)
        return -4009;
#ifndef HAVE_FIPS
    ret = wc_AesGetKeySize(&aes, &keySize);
    if (ret != 0 || keySize != sizeof(key16))
        return -4010;
#endif

    ret = wc_AesSetKey(&aes, key24, sizeof(key24), iv, AES_ENCRYPTION);
    if (ret != 0)
        return -4011;
#ifndef HAVE_FIPS
    ret = wc_AesGetKeySize(&aes, &keySize);
    if (ret != 0 || keySize != sizeof(key24))
        return -4012;
#endif

    ret = wc_AesSetKey(&aes, key32, sizeof(key32), iv, AES_ENCRYPTION);
    if (ret != 0)
        return -4013;
#ifndef HAVE_FIPS
    ret = wc_AesGetKeySize(&aes, &keySize);
    if (ret != 0 || keySize != sizeof(key32))
        return -4014;
#endif

    return 0;
}

#if defined(WOLFSSL_AES_XTS)
/* test vectors from http://csrc.nist.gov/groups/STM/cavp/block-cipher-modes.html */
static int aes_xts_128_test(void)
{
    XtsAes aes;
    int ret = 0;
    unsigned char buf[AES_BLOCK_SIZE * 2];
    unsigned char cipher[AES_BLOCK_SIZE * 2];

    /* 128 key tests */
    static unsigned char k1[] = {
        0xa1, 0xb9, 0x0c, 0xba, 0x3f, 0x06, 0xac, 0x35,
        0x3b, 0x2c, 0x34, 0x38, 0x76, 0x08, 0x17, 0x62,
        0x09, 0x09, 0x23, 0x02, 0x6e, 0x91, 0x77, 0x18,
        0x15, 0xf2, 0x9d, 0xab, 0x01, 0x93, 0x2f, 0x2f
    };

    static unsigned char i1[] = {
        0x4f, 0xae, 0xf7, 0x11, 0x7c, 0xda, 0x59, 0xc6,
        0x6e, 0x4b, 0x92, 0x01, 0x3e, 0x76, 0x8a, 0xd5
    };

    static unsigned char p1[] = {
        0xeb, 0xab, 0xce, 0x95, 0xb1, 0x4d, 0x3c, 0x8d,
        0x6f, 0xb3, 0x50, 0x39, 0x07, 0x90, 0x31, 0x1c
    };

    /* plain text test of partial block is not from NIST test vector list */
    static unsigned char pp[] = {
        0xeb, 0xab, 0xce, 0x95, 0xb1, 0x4d, 0x3c, 0x8d,
        0x6f, 0xb3, 0x50, 0x39, 0x07, 0x90, 0x31, 0x1c,
        0x6e, 0x4b, 0x92, 0x01, 0x3e, 0x76, 0x8a, 0xd5
    };

    static unsigned char c1[] = {
        0x77, 0x8a, 0xe8, 0xb4, 0x3c, 0xb9, 0x8d, 0x5a,
        0x82, 0x50, 0x81, 0xd5, 0xbe, 0x47, 0x1c, 0x63
    };

    static unsigned char k2[] = {
        0x39, 0x25, 0x79, 0x05, 0xdf, 0xcc, 0x77, 0x76,
        0x6c, 0x87, 0x0a, 0x80, 0x6a, 0x60, 0xe3, 0xc0,
        0x93, 0xd1, 0x2a, 0xcf, 0xcb, 0x51, 0x42, 0xfa,
        0x09, 0x69, 0x89, 0x62, 0x5b, 0x60, 0xdb, 0x16
    };

    static unsigned char i2[] = {
        0x5c, 0xf7, 0x9d, 0xb6, 0xc5, 0xcd, 0x99, 0x1a,
        0x1c, 0x78, 0x81, 0x42, 0x24, 0x95, 0x1e, 0x84
    };

    static unsigned char p2[] = {
        0xbd, 0xc5, 0x46, 0x8f, 0xbc, 0x8d, 0x50, 0xa1,
        0x0d, 0x1c, 0x85, 0x7f, 0x79, 0x1c, 0x5c, 0xba,
        0xb3, 0x81, 0x0d, 0x0d, 0x73, 0xcf, 0x8f, 0x20,
        0x46, 0xb1, 0xd1, 0x9e, 0x7d, 0x5d, 0x8a, 0x56
    };

    static unsigned char c2[] = {
        0xd6, 0xbe, 0x04, 0x6d, 0x41, 0xf2, 0x3b, 0x5e,
        0xd7, 0x0b, 0x6b, 0x3d, 0x5c, 0x8e, 0x66, 0x23,
        0x2b, 0xe6, 0xb8, 0x07, 0xd4, 0xdc, 0xc6, 0x0e,
        0xff, 0x8d, 0xbc, 0x1d, 0x9f, 0x7f, 0xc8, 0x22
    };

    XMEMSET(buf, 0, sizeof(buf));
    if (wc_AesXtsSetKey(&aes, k2, sizeof(k2), AES_ENCRYPTION,
            HEAP_HINT, devId) != 0)
        return -4000;
    ret = wc_AesXtsEncrypt(&aes, buf, p2, sizeof(p2), i2, sizeof(i2));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4001;
    if (XMEMCMP(c2, buf, sizeof(c2)))
        return -4002;

    XMEMSET(buf, 0, sizeof(buf));
    if (wc_AesXtsSetKey(&aes, k1, sizeof(k1), AES_ENCRYPTION,
            HEAP_HINT, devId) != 0)
        return -4003;
    ret = wc_AesXtsEncrypt(&aes, buf, p1, sizeof(p1), i1, sizeof(i1));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4004;
    if (XMEMCMP(c1, buf, AES_BLOCK_SIZE))
        return -4005;

    /* partial block encryption test */
    XMEMSET(cipher, 0, sizeof(cipher));
    ret = wc_AesXtsEncrypt(&aes, cipher, pp, sizeof(pp), i1, sizeof(i1));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4006;
    wc_AesXtsFree(&aes);

    /* partial block decrypt test */
    XMEMSET(buf, 0, sizeof(buf));
    if (wc_AesXtsSetKey(&aes, k1, sizeof(k1), AES_DECRYPTION,
            HEAP_HINT, devId) != 0)
        return -4007;
    ret = wc_AesXtsDecrypt(&aes, buf, cipher, sizeof(pp), i1, sizeof(i1));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4008;
    if (XMEMCMP(pp, buf, sizeof(pp)))
        return -4009;

    /* NIST decrypt test vector */
    XMEMSET(buf, 0, sizeof(buf));
    ret = wc_AesXtsDecrypt(&aes, buf, c1, sizeof(c1), i1, sizeof(i1));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4010;
    if (XMEMCMP(p1, buf, AES_BLOCK_SIZE))
        return -4011;

    /* fail case with decrypting using wrong key */
    XMEMSET(buf, 0, sizeof(buf));
    ret = wc_AesXtsDecrypt(&aes, buf, c2, sizeof(c2), i2, sizeof(i2));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4012;
    if (XMEMCMP(p2, buf, sizeof(p2)) == 0) /* fail case with wrong key */
        return -4013;

    /* set correct key and retest */
    XMEMSET(buf, 0, sizeof(buf));
    if (wc_AesXtsSetKey(&aes, k2, sizeof(k2), AES_DECRYPTION,
            HEAP_HINT, devId) != 0)
        return -4014;
    ret = wc_AesXtsDecrypt(&aes, buf, c2, sizeof(c2), i2, sizeof(i2));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4015;
    if (XMEMCMP(p2, buf, sizeof(p2)))
        return -4016;
    wc_AesXtsFree(&aes);

    return ret;
}


static int aes_xts_256_test(void)
{
    XtsAes aes;
    int ret = 0;
    unsigned char buf[AES_BLOCK_SIZE * 3];
    unsigned char cipher[AES_BLOCK_SIZE * 3];

    /* 256 key tests */
    static unsigned char k1[] = {
        0x1e, 0xa6, 0x61, 0xc5, 0x8d, 0x94, 0x3a, 0x0e,
        0x48, 0x01, 0xe4, 0x2f, 0x4b, 0x09, 0x47, 0x14,
        0x9e, 0x7f, 0x9f, 0x8e, 0x3e, 0x68, 0xd0, 0xc7,
        0x50, 0x52, 0x10, 0xbd, 0x31, 0x1a, 0x0e, 0x7c,
        0xd6, 0xe1, 0x3f, 0xfd, 0xf2, 0x41, 0x8d, 0x8d,
        0x19, 0x11, 0xc0, 0x04, 0xcd, 0xa5, 0x8d, 0xa3,
        0xd6, 0x19, 0xb7, 0xe2, 0xb9, 0x14, 0x1e, 0x58,
        0x31, 0x8e, 0xea, 0x39, 0x2c, 0xf4, 0x1b, 0x08
    };

    static unsigned char i1[] = {
        0xad, 0xf8, 0xd9, 0x26, 0x27, 0x46, 0x4a, 0xd2,
        0xf0, 0x42, 0x8e, 0x84, 0xa9, 0xf8, 0x75, 0x64
    };

    static unsigned char p1[] = {
        0x2e, 0xed, 0xea, 0x52, 0xcd, 0x82, 0x15, 0xe1,
        0xac, 0xc6, 0x47, 0xe8, 0x10, 0xbb, 0xc3, 0x64,
        0x2e, 0x87, 0x28, 0x7f, 0x8d, 0x2e, 0x57, 0xe3,
        0x6c, 0x0a, 0x24, 0xfb, 0xc1, 0x2a, 0x20, 0x2e
    };

    /* plain text test of partial block is not from NIST test vector list */
    static unsigned char pp[] = {
        0xeb, 0xab, 0xce, 0x95, 0xb1, 0x4d, 0x3c, 0x8d,
        0x6f, 0xb3, 0x50, 0x39, 0x07, 0x90, 0x31, 0x1c,
        0x6e, 0x4b, 0x92, 0x01, 0x3e, 0x76, 0x8a, 0xd5
    };

    static unsigned char c1[] = {
        0xcb, 0xaa, 0xd0, 0xe2, 0xf6, 0xce, 0xa3, 0xf5,
        0x0b, 0x37, 0xf9, 0x34, 0xd4, 0x6a, 0x9b, 0x13,
        0x0b, 0x9d, 0x54, 0xf0, 0x7e, 0x34, 0xf3, 0x6a,
        0xf7, 0x93, 0xe8, 0x6f, 0x73, 0xc6, 0xd7, 0xdb
    };

    static unsigned char k2[] = {
        0xad, 0x50, 0x4b, 0x85, 0xd7, 0x51, 0xbf, 0xba,
        0x69, 0x13, 0xb4, 0xcc, 0x79, 0xb6, 0x5a, 0x62,
        0xf7, 0xf3, 0x9d, 0x36, 0x0f, 0x35, 0xb5, 0xec,
        0x4a, 0x7e, 0x95, 0xbd, 0x9b, 0xa5, 0xf2, 0xec,
        0xc1, 0xd7, 0x7e, 0xa3, 0xc3, 0x74, 0xbd, 0x4b,
        0x13, 0x1b, 0x07, 0x83, 0x87, 0xdd, 0x55, 0x5a,
        0xb5, 0xb0, 0xc7, 0xe5, 0x2d, 0xb5, 0x06, 0x12,
        0xd2, 0xb5, 0x3a, 0xcb, 0x47, 0x8a, 0x53, 0xb4
    };

    static unsigned char i2[] = {
        0xe6, 0x42, 0x19, 0xed, 0xe0, 0xe1, 0xc2, 0xa0,
        0x0e, 0xf5, 0x58, 0x6a, 0xc4, 0x9b, 0xeb, 0x6f
    };

    static unsigned char p2[] = {
        0x24, 0xcb, 0x76, 0x22, 0x55, 0xb5, 0xa8, 0x00,
        0xf4, 0x6e, 0x80, 0x60, 0x56, 0x9e, 0x05, 0x53,
        0xbc, 0xfe, 0x86, 0x55, 0x3b, 0xca, 0xd5, 0x89,
        0xc7, 0x54, 0x1a, 0x73, 0xac, 0xc3, 0x9a, 0xbd,
        0x53, 0xc4, 0x07, 0x76, 0xd8, 0xe8, 0x22, 0x61,
        0x9e, 0xa9, 0xad, 0x77, 0xa0, 0x13, 0x4c, 0xfc
    };

    static unsigned char c2[] = {
        0xa3, 0xc6, 0xf3, 0xf3, 0x82, 0x79, 0x5b, 0x10,
        0x87, 0xd7, 0x02, 0x50, 0xdb, 0x2c, 0xd3, 0xb1,
        0xa1, 0x62, 0xa8, 0xb6, 0xdc, 0x12, 0x60, 0x61,
        0xc1, 0x0a, 0x84, 0xa5, 0x85, 0x3f, 0x3a, 0x89,
        0xe6, 0x6c, 0xdb, 0xb7, 0x9a, 0xb4, 0x28, 0x9b,
        0xc3, 0xea, 0xd8, 0x10, 0xe9, 0xc0, 0xaf, 0x92
    };

    XMEMSET(buf, 0, sizeof(buf));
    if (wc_AesXtsSetKey(&aes, k2, sizeof(k2), AES_ENCRYPTION,
            HEAP_HINT, devId) != 0)
        return -4017;
    ret = wc_AesXtsEncrypt(&aes, buf, p2, sizeof(p2), i2, sizeof(i2));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4018;
    if (XMEMCMP(c2, buf, sizeof(c2)))
        return -4019;

    XMEMSET(buf, 0, sizeof(buf));
    if (wc_AesXtsSetKey(&aes, k1, sizeof(k1), AES_ENCRYPTION,
            HEAP_HINT, devId) != 0)
        return -4020;
    ret = wc_AesXtsEncrypt(&aes, buf, p1, sizeof(p1), i1, sizeof(i1));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4021;
    if (XMEMCMP(c1, buf, AES_BLOCK_SIZE))
        return -4022;

    /* partial block encryption test */
    XMEMSET(cipher, 0, sizeof(cipher));
    ret = wc_AesXtsEncrypt(&aes, cipher, pp, sizeof(pp), i1, sizeof(i1));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4023;
    wc_AesXtsFree(&aes);

    /* partial block decrypt test */
    XMEMSET(buf, 0, sizeof(buf));
    if (wc_AesXtsSetKey(&aes, k1, sizeof(k1), AES_DECRYPTION,
            HEAP_HINT, devId) != 0)
        return -4024;
    ret = wc_AesXtsDecrypt(&aes, buf, cipher, sizeof(pp), i1, sizeof(i1));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4025;
    if (XMEMCMP(pp, buf, sizeof(pp)))
        return -4026;

    /* NIST decrypt test vector */
    XMEMSET(buf, 0, sizeof(buf));
    ret = wc_AesXtsDecrypt(&aes, buf, c1, sizeof(c1), i1, sizeof(i1));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4027;
    if (XMEMCMP(p1, buf, AES_BLOCK_SIZE))
        return -4028;

    XMEMSET(buf, 0, sizeof(buf));
    if (wc_AesXtsSetKey(&aes, k2, sizeof(k2), AES_DECRYPTION,
            HEAP_HINT, devId) != 0)
        return -4029;
    ret = wc_AesXtsDecrypt(&aes, buf, c2, sizeof(c2), i2, sizeof(i2));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4030;
    if (XMEMCMP(p2, buf, sizeof(p2)))
        return -4031;
    wc_AesXtsFree(&aes);

    return ret;
}


/* both 128 and 256 bit key test */
static int aes_xts_sector_test(void)
{
    XtsAes aes;
    int ret = 0;
    unsigned char buf[AES_BLOCK_SIZE * 2];

    /* 128 key tests */
    static unsigned char k1[] = {
        0xa3, 0xe4, 0x0d, 0x5b, 0xd4, 0xb6, 0xbb, 0xed,
        0xb2, 0xd1, 0x8c, 0x70, 0x0a, 0xd2, 0xdb, 0x22,
        0x10, 0xc8, 0x11, 0x90, 0x64, 0x6d, 0x67, 0x3c,
        0xbc, 0xa5, 0x3f, 0x13, 0x3e, 0xab, 0x37, 0x3c
    };

    static unsigned char p1[] = {
        0x20, 0xe0, 0x71, 0x94, 0x05, 0x99, 0x3f, 0x09,
        0xa6, 0x6a, 0xe5, 0xbb, 0x50, 0x0e, 0x56, 0x2c
    };

    static unsigned char c1[] = {
        0x74, 0x62, 0x35, 0x51, 0x21, 0x02, 0x16, 0xac,
        0x92, 0x6b, 0x96, 0x50, 0xb6, 0xd3, 0xfa, 0x52
    };
    word64 s1 = 141;

    /* 256 key tests */
    static unsigned char k2[] = {
        0xef, 0x01, 0x0c, 0xa1, 0xa3, 0x66, 0x3e, 0x32,
        0x53, 0x43, 0x49, 0xbc, 0x0b, 0xae, 0x62, 0x23,
        0x2a, 0x15, 0x73, 0x34, 0x85, 0x68, 0xfb, 0x9e,
        0xf4, 0x17, 0x68, 0xa7, 0x67, 0x4f, 0x50, 0x7a,
        0x72, 0x7f, 0x98, 0x75, 0x53, 0x97, 0xd0, 0xe0,
        0xaa, 0x32, 0xf8, 0x30, 0x33, 0x8c, 0xc7, 0xa9,
        0x26, 0xc7, 0x73, 0xf0, 0x9e, 0x57, 0xb3, 0x57,
        0xcd, 0x15, 0x6a, 0xfb, 0xca, 0x46, 0xe1, 0xa0
    };

    static unsigned char p2[] = {
        0xed, 0x98, 0xe0, 0x17, 0x70, 0xa8, 0x53, 0xb4,
        0x9d, 0xb9, 0xe6, 0xaa, 0xf8, 0x8f, 0x0a, 0x41,
        0xb9, 0xb5, 0x6e, 0x91, 0xa5, 0xa2, 0xb1, 0x1d,
        0x40, 0x52, 0x92, 0x54, 0xf5, 0x52, 0x3e, 0x75
    };

    static unsigned char c2[] = {
        0xca, 0x20, 0xc5, 0x5e, 0x8d, 0xc1, 0x49, 0x68,
        0x7d, 0x25, 0x41, 0xde, 0x39, 0xc3, 0xdf, 0x63,
        0x00, 0xbb, 0x5a, 0x16, 0x3c, 0x10, 0xce, 0xd3,
        0x66, 0x6b, 0x13, 0x57, 0xdb, 0x8b, 0xd3, 0x9d
    };
    word64 s2 = 187;

    XMEMSET(buf, 0, sizeof(buf));
    if (wc_AesXtsSetKey(&aes, k1, sizeof(k1), AES_ENCRYPTION,
            HEAP_HINT, devId) != 0)
        return -4032;
    ret = wc_AesXtsEncryptSector(&aes, buf, p1, sizeof(p1), s1);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4033;
    if (XMEMCMP(c1, buf, AES_BLOCK_SIZE))
        return -4034;

    /* decrypt test */
    XMEMSET(buf, 0, sizeof(buf));
    if (wc_AesXtsSetKey(&aes, k1, sizeof(k1), AES_DECRYPTION,
            HEAP_HINT, devId) != 0)
        return -4035;
    ret = wc_AesXtsDecryptSector(&aes, buf, c1, sizeof(c1), s1);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4036;
    if (XMEMCMP(p1, buf, AES_BLOCK_SIZE))
        return -4037;
    wc_AesXtsFree(&aes);

    /* 256 bit key tests */
    XMEMSET(buf, 0, sizeof(buf));
    if (wc_AesXtsSetKey(&aes, k2, sizeof(k2), AES_ENCRYPTION,
            HEAP_HINT, devId) != 0)
        return -4038;
    ret = wc_AesXtsEncryptSector(&aes, buf, p2, sizeof(p2), s2);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4039;
    if (XMEMCMP(c2, buf, sizeof(c2)))
        return -4040;

    /* decrypt test */
    XMEMSET(buf, 0, sizeof(buf));
    if (wc_AesXtsSetKey(&aes, k2, sizeof(k2), AES_DECRYPTION,
            HEAP_HINT, devId) != 0)
        return -4041;
    ret = wc_AesXtsDecryptSector(&aes, buf, c2, sizeof(c2), s2);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4042;
    if (XMEMCMP(p2, buf, sizeof(p2)))
        return -4043;
    wc_AesXtsFree(&aes);

    return ret;
}


/* testing of bad arguments */
static int aes_xts_args_test(void)
{
    XtsAes aes;
    int ret = 0;
    unsigned char buf[AES_BLOCK_SIZE * 2];

    /* 128 key tests */
    static unsigned char k1[] = {
        0xa3, 0xe4, 0x0d, 0x5b, 0xd4, 0xb6, 0xbb, 0xed,
        0xb2, 0xd1, 0x8c, 0x70, 0x0a, 0xd2, 0xdb, 0x22,
        0x10, 0xc8, 0x11, 0x90, 0x64, 0x6d, 0x67, 0x3c,
        0xbc, 0xa5, 0x3f, 0x13, 0x3e, 0xab, 0x37, 0x3c
    };

    static unsigned char p1[] = {
        0x20, 0xe0, 0x71, 0x94, 0x05, 0x99, 0x3f, 0x09,
        0xa6, 0x6a, 0xe5, 0xbb, 0x50, 0x0e, 0x56, 0x2c
    };

    static unsigned char c1[] = {
        0x74, 0x62, 0x35, 0x51, 0x21, 0x02, 0x16, 0xac,
        0x92, 0x6b, 0x96, 0x50, 0xb6, 0xd3, 0xfa, 0x52
    };
    word64 s1 = 141;

    if (wc_AesXtsSetKey(NULL, k1, sizeof(k1), AES_ENCRYPTION,
            HEAP_HINT, devId) == 0)
        return -4044;
    if (wc_AesXtsSetKey(&aes, NULL, sizeof(k1), AES_ENCRYPTION,
            HEAP_HINT, devId) == 0)
        return -4045;

    /* encryption operations */
    if (wc_AesXtsSetKey(&aes, k1, sizeof(k1), AES_ENCRYPTION,
            HEAP_HINT, devId) != 0)
        return -4046;
    ret = wc_AesXtsEncryptSector(NULL, buf, p1, sizeof(p1), s1);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret == 0)
        return -4047;

    ret = wc_AesXtsEncryptSector(&aes, NULL, p1, sizeof(p1), s1);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret == 0)
        return -4048;
    wc_AesXtsFree(&aes);

    /* decryption operations */
    if (wc_AesXtsSetKey(&aes, k1, sizeof(k1), AES_DECRYPTION,
            HEAP_HINT, devId) != 0)
        return -4046;
    ret = wc_AesXtsDecryptSector(NULL, buf, c1, sizeof(c1), s1);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret == 0)
        return -4049;

    ret = wc_AesXtsDecryptSector(&aes, NULL, c1, sizeof(c1), s1);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &aes.aes.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret == 0)
        return -4050;
    wc_AesXtsFree(&aes);

    return 0;
}
#endif /* WOLFSSL_AES_XTS */

#if defined(HAVE_AES_CBC)
static int aes_cbc_test(void)
{
    byte cipher[AES_BLOCK_SIZE];
    byte plain[AES_BLOCK_SIZE];
    int  ret;
    const byte msg[] = { /* "Now is the time for all " w/o trailing 0 */
        0x6e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,
        0x68,0x65,0x20,0x74,0x69,0x6d,0x65,0x20,
        0x66,0x6f,0x72,0x20,0x61,0x6c,0x6c,0x20
    };
    byte key[] = "0123456789abcdef   ";  /* align */
    byte iv[]  = "1234567890abcdef   ";  /* align */

    /* Parameter Validation testing. */
    ret = wc_AesCbcEncryptWithKey(cipher, msg, AES_BLOCK_SIZE, key, 17, NULL);
    if (ret != BAD_FUNC_ARG)
        return -4100;
    ret = wc_AesCbcDecryptWithKey(plain, cipher, AES_BLOCK_SIZE, key, 17, NULL);
    if (ret != BAD_FUNC_ARG)
        return -4101;

    ret = wc_AesCbcEncryptWithKey(cipher, msg, AES_BLOCK_SIZE, key,
                                  AES_BLOCK_SIZE, iv);
    if (ret != 0)
        return -4102;
    ret = wc_AesCbcDecryptWithKey(plain, cipher, AES_BLOCK_SIZE, key,
                                  AES_BLOCK_SIZE, iv);
    if (ret != 0)
        return -4103;

    if (XMEMCMP(plain, msg, AES_BLOCK_SIZE) != 0)
        return -4104;

    return 0;
}
#endif

int aes_test(void)
{
#if defined(HAVE_AES_CBC) || defined(WOLFSSL_AES_COUNTER)
    Aes enc;
    byte cipher[AES_BLOCK_SIZE * 4];
#if defined(HAVE_AES_DECRYPT) || defined(WOLFSSL_AES_COUNTER)
    Aes dec;
    byte plain [AES_BLOCK_SIZE * 4];
#endif
#endif /* HAVE_AES_CBC || WOLFSSL_AES_COUNTER */
    int  ret = 0;

#ifdef HAVE_AES_CBC
    const byte msg[] = { /* "Now is the time for all " w/o trailing 0 */
        0x6e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,
        0x68,0x65,0x20,0x74,0x69,0x6d,0x65,0x20,
        0x66,0x6f,0x72,0x20,0x61,0x6c,0x6c,0x20
    };

    const byte verify[] =
    {
        0x95,0x94,0x92,0x57,0x5f,0x42,0x81,0x53,
        0x2c,0xcc,0x9d,0x46,0x77,0xa2,0x33,0xcb
    };

    byte key[] = "0123456789abcdef   ";  /* align */
    byte iv[]  = "1234567890abcdef   ";  /* align */

#ifdef WOLFSSL_ASYNC_CRYPT
    if (wc_AesInit(&enc, HEAP_HINT, devId) != 0)
        return -4200;
    if (wc_AesInit(&dec, HEAP_HINT, devId) != 0)
        return -4201;
#endif

    ret = wc_AesSetKey(&enc, key, AES_BLOCK_SIZE, iv, AES_ENCRYPTION);
    if (ret != 0)
        return -4202;
#ifdef HAVE_AES_DECRYPT
    ret = wc_AesSetKey(&dec, key, AES_BLOCK_SIZE, iv, AES_DECRYPTION);
    if (ret != 0)
        return -4203;
#endif

    ret = wc_AesCbcEncrypt(&enc, cipher, msg, AES_BLOCK_SIZE);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4204;
#ifdef HAVE_AES_DECRYPT
    ret = wc_AesCbcDecrypt(&dec, plain, cipher, AES_BLOCK_SIZE);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &dec.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4205;

    if (XMEMCMP(plain, msg, AES_BLOCK_SIZE))
        return -4206;
#endif /* HAVE_AES_DECRYPT */
    if (XMEMCMP(cipher, verify, AES_BLOCK_SIZE))
        return -4207;

#if defined(WOLFSSL_AESNI) && defined(HAVE_AES_DECRYPT)
    {
        const byte bigMsg[] = {
            /* "All work and no play makes Jack a dull boy. " */
            0x41,0x6c,0x6c,0x20,0x77,0x6f,0x72,0x6b,
            0x20,0x61,0x6e,0x64,0x20,0x6e,0x6f,0x20,
            0x70,0x6c,0x61,0x79,0x20,0x6d,0x61,0x6b,
            0x65,0x73,0x20,0x4a,0x61,0x63,0x6b,0x20,
            0x61,0x20,0x64,0x75,0x6c,0x6c,0x20,0x62,
            0x6f,0x79,0x2e,0x20,0x41,0x6c,0x6c,0x20,
            0x77,0x6f,0x72,0x6b,0x20,0x61,0x6e,0x64,
            0x20,0x6e,0x6f,0x20,0x70,0x6c,0x61,0x79,
            0x20,0x6d,0x61,0x6b,0x65,0x73,0x20,0x4a,
            0x61,0x63,0x6b,0x20,0x61,0x20,0x64,0x75,
            0x6c,0x6c,0x20,0x62,0x6f,0x79,0x2e,0x20,
            0x41,0x6c,0x6c,0x20,0x77,0x6f,0x72,0x6b,
            0x20,0x61,0x6e,0x64,0x20,0x6e,0x6f,0x20,
            0x70,0x6c,0x61,0x79,0x20,0x6d,0x61,0x6b,
            0x65,0x73,0x20,0x4a,0x61,0x63,0x6b,0x20,
            0x61,0x20,0x64,0x75,0x6c,0x6c,0x20,0x62,
            0x6f,0x79,0x2e,0x20,0x41,0x6c,0x6c,0x20,
            0x77,0x6f,0x72,0x6b,0x20,0x61,0x6e,0x64,
            0x20,0x6e,0x6f,0x20,0x70,0x6c,0x61,0x79,
            0x20,0x6d,0x61,0x6b,0x65,0x73,0x20,0x4a,
            0x61,0x63,0x6b,0x20,0x61,0x20,0x64,0x75,
            0x6c,0x6c,0x20,0x62,0x6f,0x79,0x2e,0x20,
            0x41,0x6c,0x6c,0x20,0x77,0x6f,0x72,0x6b,
            0x20,0x61,0x6e,0x64,0x20,0x6e,0x6f,0x20,
            0x70,0x6c,0x61,0x79,0x20,0x6d,0x61,0x6b,
            0x65,0x73,0x20,0x4a,0x61,0x63,0x6b,0x20,
            0x61,0x20,0x64,0x75,0x6c,0x6c,0x20,0x62,
            0x6f,0x79,0x2e,0x20,0x41,0x6c,0x6c,0x20,
            0x77,0x6f,0x72,0x6b,0x20,0x61,0x6e,0x64,
            0x20,0x6e,0x6f,0x20,0x70,0x6c,0x61,0x79,
            0x20,0x6d,0x61,0x6b,0x65,0x73,0x20,0x4a,
            0x61,0x63,0x6b,0x20,0x61,0x20,0x64,0x75,
            0x6c,0x6c,0x20,0x62,0x6f,0x79,0x2e,0x20,
            0x41,0x6c,0x6c,0x20,0x77,0x6f,0x72,0x6b,
            0x20,0x61,0x6e,0x64,0x20,0x6e,0x6f,0x20,
            0x70,0x6c,0x61,0x79,0x20,0x6d,0x61,0x6b,
            0x65,0x73,0x20,0x4a,0x61,0x63,0x6b,0x20,
            0x61,0x20,0x64,0x75,0x6c,0x6c,0x20,0x62,
            0x6f,0x79,0x2e,0x20,0x41,0x6c,0x6c,0x20,
            0x77,0x6f,0x72,0x6b,0x20,0x61,0x6e,0x64,
            0x20,0x6e,0x6f,0x20,0x70,0x6c,0x61,0x79,
            0x20,0x6d,0x61,0x6b,0x65,0x73,0x20,0x4a,
            0x61,0x63,0x6b,0x20,0x61,0x20,0x64,0x75,
            0x6c,0x6c,0x20,0x62,0x6f,0x79,0x2e,0x20,
            0x41,0x6c,0x6c,0x20,0x77,0x6f,0x72,0x6b,
            0x20,0x61,0x6e,0x64,0x20,0x6e,0x6f,0x20,
            0x70,0x6c,0x61,0x79,0x20,0x6d,0x61,0x6b,
            0x65,0x73,0x20,0x4a,0x61,0x63,0x6b,0x20
        };
        const byte bigKey[] = "0123456789abcdeffedcba9876543210";
        byte bigCipher[sizeof(bigMsg)];
        byte bigPlain[sizeof(bigMsg)];
        word32 keySz, msgSz;

        /* Iterate from one AES_BLOCK_SIZE of bigMsg through the whole
         * message by AES_BLOCK_SIZE for each size of AES key. */
        for (keySz = 16; keySz <= 32; keySz += 8) {
            for (msgSz = AES_BLOCK_SIZE;
                 msgSz <= sizeof(bigMsg);
                 msgSz += AES_BLOCK_SIZE) {

                XMEMSET(bigCipher, 0, sizeof(bigCipher));
                XMEMSET(bigPlain, 0, sizeof(bigPlain));
                ret = wc_AesSetKey(&enc, bigKey, keySz, iv, AES_ENCRYPTION);
                if (ret != 0)
                    return -4208;
                ret = wc_AesSetKey(&dec, bigKey, keySz, iv, AES_DECRYPTION);
                if (ret != 0)
                    return -4209;

                ret = wc_AesCbcEncrypt(&enc, bigCipher, bigMsg, msgSz);
            #if defined(WOLFSSL_ASYNC_CRYPT)
                ret = wc_AsyncWait(ret, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
            #endif
                if (ret != 0)
                    return -4210;

                ret = wc_AesCbcDecrypt(&dec, bigPlain, bigCipher, msgSz);
            #if defined(WOLFSSL_ASYNC_CRYPT)
                ret = wc_AsyncWait(ret, &dec.asyncDev, WC_ASYNC_FLAG_NONE);
            #endif
                if (ret != 0)
                    return -4211;

                if (XMEMCMP(bigPlain, bigMsg, msgSz))
                    return -4212;
            }
        }
    }
#endif /* WOLFSSL_AESNI HAVE_AES_DECRYPT */

#endif /* HAVE_AES_CBC */

#ifdef WOLFSSL_AES_COUNTER
    {
        /* test vectors from "Recommendation for Block Cipher Modes of
         * Operation" NIST Special Publication 800-38A */

        const byte ctrIv[] =
        {
            0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
            0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
        };

        const byte ctrPlain[] =
        {
            0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
            0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a,
            0xae,0x2d,0x8a,0x57,0x1e,0x03,0xac,0x9c,
            0x9e,0xb7,0x6f,0xac,0x45,0xaf,0x8e,0x51,
            0x30,0xc8,0x1c,0x46,0xa3,0x5c,0xe4,0x11,
            0xe5,0xfb,0xc1,0x19,0x1a,0x0a,0x52,0xef,
            0xf6,0x9f,0x24,0x45,0xdf,0x4f,0x9b,0x17,
            0xad,0x2b,0x41,0x7b,0xe6,0x6c,0x37,0x10
        };

        const byte oddCipher[] =
        {
            0xb9,0xd7,0xcb,0x08,0xb0,0xe1,0x7b,0xa0,
            0xc2
        };

        const byte ctr128Key[] =
        {
            0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
            0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c
        };

        const byte ctr128Cipher[] =
        {
            0x87,0x4d,0x61,0x91,0xb6,0x20,0xe3,0x26,
            0x1b,0xef,0x68,0x64,0x99,0x0d,0xb6,0xce,
            0x98,0x06,0xf6,0x6b,0x79,0x70,0xfd,0xff,
            0x86,0x17,0x18,0x7b,0xb9,0xff,0xfd,0xff,
            0x5a,0xe4,0xdf,0x3e,0xdb,0xd5,0xd3,0x5e,
            0x5b,0x4f,0x09,0x02,0x0d,0xb0,0x3e,0xab,
            0x1e,0x03,0x1d,0xda,0x2f,0xbe,0x03,0xd1,
            0x79,0x21,0x70,0xa0,0xf3,0x00,0x9c,0xee
        };

        const byte ctr192Key[] =
        {
            0x8e,0x73,0xb0,0xf7,0xda,0x0e,0x64,0x52,
            0xc8,0x10,0xf3,0x2b,0x80,0x90,0x79,0xe5,
            0x62,0xf8,0xea,0xd2,0x52,0x2c,0x6b,0x7b
        };

        const byte ctr192Cipher[] =
        {
            0x1a,0xbc,0x93,0x24,0x17,0x52,0x1c,0xa2,
            0x4f,0x2b,0x04,0x59,0xfe,0x7e,0x6e,0x0b,
            0x09,0x03,0x39,0xec,0x0a,0xa6,0xfa,0xef,
            0xd5,0xcc,0xc2,0xc6,0xf4,0xce,0x8e,0x94,
            0x1e,0x36,0xb2,0x6b,0xd1,0xeb,0xc6,0x70,
            0xd1,0xbd,0x1d,0x66,0x56,0x20,0xab,0xf7,
            0x4f,0x78,0xa7,0xf6,0xd2,0x98,0x09,0x58,
            0x5a,0x97,0xda,0xec,0x58,0xc6,0xb0,0x50
        };

        const byte ctr256Key[] =
        {
            0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,
            0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
            0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,
            0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4
        };

        const byte ctr256Cipher[] =
        {
            0x60,0x1e,0xc3,0x13,0x77,0x57,0x89,0xa5,
            0xb7,0xa7,0xf5,0x04,0xbb,0xf3,0xd2,0x28,
            0xf4,0x43,0xe3,0xca,0x4d,0x62,0xb5,0x9a,
            0xca,0x84,0xe9,0x90,0xca,0xca,0xf5,0xc5,
            0x2b,0x09,0x30,0xda,0xa2,0x3d,0xe9,0x4c,
            0xe8,0x70,0x17,0xba,0x2d,0x84,0x98,0x8d,
            0xdf,0xc9,0xc5,0x8d,0xb6,0x7a,0xad,0xa6,
            0x13,0xc2,0xdd,0x08,0x45,0x79,0x41,0xa6
        };

        wc_AesSetKeyDirect(&enc, ctr128Key, sizeof(ctr128Key),
                           ctrIv, AES_ENCRYPTION);
        /* Ctr only uses encrypt, even on key setup */
        wc_AesSetKeyDirect(&dec, ctr128Key, sizeof(ctr128Key),
                           ctrIv, AES_ENCRYPTION);

        ret = wc_AesCtrEncrypt(&enc, cipher, ctrPlain, sizeof(ctrPlain));
        if (ret != 0) {
            return -4227;
        }
        ret = wc_AesCtrEncrypt(&dec, plain, cipher, sizeof(ctrPlain));
        if (ret != 0) {
            return -4228;
        }
        if (XMEMCMP(plain, ctrPlain, sizeof(ctrPlain)))
            return -4213;

        if (XMEMCMP(cipher, ctr128Cipher, sizeof(ctr128Cipher)))
            return -4214;

        /* let's try with just 9 bytes, non block size test */
        wc_AesSetKeyDirect(&enc, ctr128Key, AES_BLOCK_SIZE,
                           ctrIv, AES_ENCRYPTION);
        /* Ctr only uses encrypt, even on key setup */
        wc_AesSetKeyDirect(&dec, ctr128Key, AES_BLOCK_SIZE,
                           ctrIv, AES_ENCRYPTION);

        ret = wc_AesCtrEncrypt(&enc, cipher, ctrPlain, sizeof(oddCipher));
        if (ret != 0) {
            return -4229;
        }
        ret = wc_AesCtrEncrypt(&dec, plain, cipher, sizeof(oddCipher));
        if (ret != 0) {
            return -4230;
        }

        if (XMEMCMP(plain, ctrPlain, sizeof(oddCipher)))
            return -4215;

        if (XMEMCMP(cipher, ctr128Cipher, sizeof(oddCipher)))
            return -4216;

        /* and an additional 9 bytes to reuse tmp left buffer */
        ret = wc_AesCtrEncrypt(&enc, cipher, ctrPlain, sizeof(oddCipher));
        if (ret != 0) {
            return -4231;
        }
        ret = wc_AesCtrEncrypt(&dec, plain, cipher, sizeof(oddCipher));
        if (ret != 0) {
            return -4232;
        }

        if (XMEMCMP(plain, ctrPlain, sizeof(oddCipher)))
            return -4217;

        if (XMEMCMP(cipher, oddCipher, sizeof(oddCipher)))
            return -4218;

        /* 192 bit key */
        wc_AesSetKeyDirect(&enc, ctr192Key, sizeof(ctr192Key),
                           ctrIv, AES_ENCRYPTION);
        /* Ctr only uses encrypt, even on key setup */
        wc_AesSetKeyDirect(&dec, ctr192Key, sizeof(ctr192Key),
                           ctrIv, AES_ENCRYPTION);

        XMEMSET(plain, 0, sizeof(plain));
        ret = wc_AesCtrEncrypt(&enc, plain, ctr192Cipher, sizeof(ctr192Cipher));
        if (ret != 0) {
            return -4233;
        }

        if (XMEMCMP(plain, ctrPlain, sizeof(ctr192Cipher)))
            return -4219;

        ret = wc_AesCtrEncrypt(&dec, cipher, ctrPlain, sizeof(ctrPlain));
        if (ret != 0) {
            return -4234;
        }
        if (XMEMCMP(ctr192Cipher, cipher, sizeof(ctr192Cipher)))
            return -4220;

        /* 256 bit key */
        wc_AesSetKeyDirect(&enc, ctr256Key, sizeof(ctr256Key),
                           ctrIv, AES_ENCRYPTION);
        /* Ctr only uses encrypt, even on key setup */
        wc_AesSetKeyDirect(&dec, ctr256Key, sizeof(ctr256Key),
                           ctrIv, AES_ENCRYPTION);

        XMEMSET(plain, 0, sizeof(plain));
        ret = wc_AesCtrEncrypt(&enc, plain, ctr256Cipher, sizeof(ctr256Cipher));
        if (ret != 0) {
            return -4235;
        }

        if (XMEMCMP(plain, ctrPlain, sizeof(ctrPlain)))
            return -4221;

        ret = wc_AesCtrEncrypt(&dec, cipher, ctrPlain, sizeof(ctrPlain));
        if (ret != 0) {
            return -4236;
        }
        if (XMEMCMP(ctr256Cipher, cipher, sizeof(ctr256Cipher)))
            return -4222;
    }
#endif /* WOLFSSL_AES_COUNTER */

#ifdef WOLFSSL_AES_DIRECT
    {
        const byte niPlain[] =
        {
            0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
            0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a
        };

        const byte niCipher[] =
        {
            0xf3,0xee,0xd1,0xbd,0xb5,0xd2,0xa0,0x3c,
            0x06,0x4b,0x5a,0x7e,0x3d,0xb1,0x81,0xf8
        };

        const byte niKey[] =
        {
            0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,
            0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
            0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,
            0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4
        };

        XMEMSET(cipher, 0, AES_BLOCK_SIZE);
        ret = wc_AesSetKey(&enc, niKey, sizeof(niKey), cipher, AES_ENCRYPTION);
        if (ret != 0)
            return -4223;
        wc_AesEncryptDirect(&enc, cipher, niPlain);
        if (XMEMCMP(cipher, niCipher, AES_BLOCK_SIZE) != 0)
            return -4224;

        XMEMSET(plain, 0, AES_BLOCK_SIZE);
        ret = wc_AesSetKey(&dec, niKey, sizeof(niKey), plain, AES_DECRYPTION);
        if (ret != 0)
            return -4225;
        wc_AesDecryptDirect(&dec, plain, niCipher);
        if (XMEMCMP(plain, niPlain, AES_BLOCK_SIZE) != 0)
            return -4226;
    }
#endif /* WOLFSSL_AES_DIRECT */

    ret = aes_key_size_test();
    if (ret != 0)
        return ret;

#if defined(HAVE_AES_CBC)
    ret = aes_cbc_test();
    if (ret != 0)
        return ret;
#endif

#if defined(WOLFSSL_AES_XTS)
    ret = aes_xts_128_test();
    if (ret != 0)
        return ret;
    ret = aes_xts_256_test();
    if (ret != 0)
        return ret;
    ret = aes_xts_sector_test();
    if (ret != 0)
        return ret;
    ret = aes_xts_args_test();
    if (ret != 0)
        return ret;
#endif

    wc_AesFree(&enc);
#ifdef HAVE_AES_DECRYPT
    wc_AesFree(&dec);
#endif

    return ret;
}

int aes192_test(void)
{
#ifdef HAVE_AES_CBC
    Aes enc;
    byte cipher[AES_BLOCK_SIZE];
#ifdef HAVE_AES_DECRYPT
    Aes dec;
    byte plain[AES_BLOCK_SIZE];
#endif
#endif /* HAVE_AES_CBC */
    int  ret = 0;

#ifdef HAVE_AES_CBC
    /* Test vectors from NIST Special Publication 800-38A, 2001 Edition
     * Appendix F.2.3  */

    const byte msg[] = {
        0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
        0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a
    };

    const byte verify[] =
    {
        0x4f,0x02,0x1d,0xb2,0x43,0xbc,0x63,0x3d,
        0x71,0x78,0x18,0x3a,0x9f,0xa0,0x71,0xe8
    };

    byte key[] = {
        0x8e,0x73,0xb0,0xf7,0xda,0x0e,0x64,0x52,
        0xc8,0x10,0xf3,0x2b,0x80,0x90,0x79,0xe5,
        0x62,0xf8,0xea,0xd2,0x52,0x2c,0x6b,0x7b
    };
    byte iv[]  = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F
    };


    if (wc_AesInit(&enc, HEAP_HINT, devId) != 0)
        return -4230;
#ifdef HAVE_AES_DECRYPT
    if (wc_AesInit(&dec, HEAP_HINT, devId) != 0)
        return -4231;
#endif

    ret = wc_AesSetKey(&enc, key, (int) sizeof(key), iv, AES_ENCRYPTION);
    if (ret != 0)
        return -4232;
#ifdef HAVE_AES_DECRYPT
    ret = wc_AesSetKey(&dec, key, (int) sizeof(key), iv, AES_DECRYPTION);
    if (ret != 0)
        return -4233;
#endif

    ret = wc_AesCbcEncrypt(&enc, cipher, msg, (int) sizeof(msg));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4234;
#ifdef HAVE_AES_DECRYPT
    ret = wc_AesCbcDecrypt(&dec, plain, cipher, (int) sizeof(cipher));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &dec.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4235;
    if (XMEMCMP(plain, msg, (int) sizeof(plain))) {
        return -4236;
    }
#endif

    if (XMEMCMP(cipher, verify, (int) sizeof(cipher)))
        return -4237;

    wc_AesFree(&enc);
#ifdef HAVE_AES_DECRYPT
    wc_AesFree(&dec);
#endif

#endif /* HAVE_AES_CBC */

    return ret;
}

int aes256_test(void)
{
#ifdef HAVE_AES_CBC
    Aes enc;
    byte cipher[AES_BLOCK_SIZE];
#ifdef HAVE_AES_DECRYPT
    Aes dec;
    byte plain[AES_BLOCK_SIZE];
#endif
#endif /* HAVE_AES_CBC */
    int  ret = 0;

#ifdef HAVE_AES_CBC
    /* Test vectors from NIST Special Publication 800-38A, 2001 Edition,
     * Appendix F.2.5  */
    const byte msg[] = {
        0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
        0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a
    };

    const byte verify[] =
    {
        0xf5,0x8c,0x4c,0x04,0xd6,0xe5,0xf1,0xba,
        0x77,0x9e,0xab,0xfb,0x5f,0x7b,0xfb,0xd6
    };

    byte key[] = {
        0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,
        0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
        0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,
        0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4
    };
    byte iv[]  = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F
    };


    if (wc_AesInit(&enc, HEAP_HINT, devId) != 0)
        return -4240;
#ifdef HAVE_AES_DECRYPT
    if (wc_AesInit(&dec, HEAP_HINT, devId) != 0)
        return -4241;
#endif

    ret = wc_AesSetKey(&enc, key, (int) sizeof(key), iv, AES_ENCRYPTION);
    if (ret != 0)
        return -4242;
#ifdef HAVE_AES_DECRYPT
    ret = wc_AesSetKey(&dec, key, (int) sizeof(key), iv, AES_DECRYPTION);
    if (ret != 0)
        return -4243;
#endif

    ret = wc_AesCbcEncrypt(&enc, cipher, msg, (int) sizeof(msg));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4244;
#ifdef HAVE_AES_DECRYPT
    ret = wc_AesCbcDecrypt(&dec, plain, cipher, (int) sizeof(cipher));
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &dec.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0)
        return -4245;
    if (XMEMCMP(plain, msg, (int) sizeof(plain))) {
        return -4246;
    }
#endif

    if (XMEMCMP(cipher, verify, (int) sizeof(cipher)))
        return -4247;

    wc_AesFree(&enc);
#ifdef HAVE_AES_DECRYPT
    wc_AesFree(&dec);
#endif

#endif /* HAVE_AES_CBC */

    return 0;
}


#ifdef HAVE_AESGCM
int aesgcm_test(void)
{
    Aes enc;

    /*
     * This is Test Case 16 from the document Galois/
     * Counter Mode of Operation (GCM) by McGrew and
     * Viega.
     */
    const byte p[] =
    {
        0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
        0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
        0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
        0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
        0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
        0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
        0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
        0xba, 0x63, 0x7b, 0x39
    };

    const byte a[] =
    {
        0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
        0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
        0xab, 0xad, 0xda, 0xd2
    };

    const byte k1[] =
    {
        0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
        0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
        0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
        0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08
    };

    const byte iv1[] =
    {
        0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
        0xde, 0xca, 0xf8, 0x88
    };

    const byte c1[] =
    {
        0x52, 0x2d, 0xc1, 0xf0, 0x99, 0x56, 0x7d, 0x07,
        0xf4, 0x7f, 0x37, 0xa3, 0x2a, 0x84, 0x42, 0x7d,
        0x64, 0x3a, 0x8c, 0xdc, 0xbf, 0xe5, 0xc0, 0xc9,
        0x75, 0x98, 0xa2, 0xbd, 0x25, 0x55, 0xd1, 0xaa,
        0x8c, 0xb0, 0x8e, 0x48, 0x59, 0x0d, 0xbb, 0x3d,
        0xa7, 0xb0, 0x8b, 0x10, 0x56, 0x82, 0x88, 0x38,
        0xc5, 0xf6, 0x1e, 0x63, 0x93, 0xba, 0x7a, 0x0a,
        0xbc, 0xc9, 0xf6, 0x62
    };

    const byte t1[] =
    {
        0x76, 0xfc, 0x6e, 0xce, 0x0f, 0x4e, 0x17, 0x68,
        0xcd, 0xdf, 0x88, 0x53, 0xbb, 0x2d, 0x55, 0x1b
    };

    /* FIPS, QAT and STM32F2/4 HW Crypto only support 12-byte IV */
#if !defined(HAVE_FIPS) && !defined(HAVE_INTEL_QA) && \
        !defined(STM32_CRYPTO) && !defined(WOLFSSL_PIC32MZ_CRYPT) && \
        !defined(WOLFSSL_XILINX_CRYPT)

    #define ENABLE_NON_12BYTE_IV_TEST

    /* Test Case 12, uses same plaintext and AAD data. */
    const byte k2[] =
    {
        0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
        0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
        0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c
    };

    const byte iv2[] =
    {
        0x93, 0x13, 0x22, 0x5d, 0xf8, 0x84, 0x06, 0xe5,
        0x55, 0x90, 0x9c, 0x5a, 0xff, 0x52, 0x69, 0xaa,
        0x6a, 0x7a, 0x95, 0x38, 0x53, 0x4f, 0x7d, 0xa1,
        0xe4, 0xc3, 0x03, 0xd2, 0xa3, 0x18, 0xa7, 0x28,
        0xc3, 0xc0, 0xc9, 0x51, 0x56, 0x80, 0x95, 0x39,
        0xfc, 0xf0, 0xe2, 0x42, 0x9a, 0x6b, 0x52, 0x54,
        0x16, 0xae, 0xdb, 0xf5, 0xa0, 0xde, 0x6a, 0x57,
        0xa6, 0x37, 0xb3, 0x9b
    };

    const byte c2[] =
    {
        0xd2, 0x7e, 0x88, 0x68, 0x1c, 0xe3, 0x24, 0x3c,
        0x48, 0x30, 0x16, 0x5a, 0x8f, 0xdc, 0xf9, 0xff,
        0x1d, 0xe9, 0xa1, 0xd8, 0xe6, 0xb4, 0x47, 0xef,
        0x6e, 0xf7, 0xb7, 0x98, 0x28, 0x66, 0x6e, 0x45,
        0x81, 0xe7, 0x90, 0x12, 0xaf, 0x34, 0xdd, 0xd9,
        0xe2, 0xf0, 0x37, 0x58, 0x9b, 0x29, 0x2d, 0xb3,
        0xe6, 0x7c, 0x03, 0x67, 0x45, 0xfa, 0x22, 0xe7,
        0xe9, 0xb7, 0x37, 0x3b
    };

    const byte t2[] =
    {
        0xdc, 0xf5, 0x66, 0xff, 0x29, 0x1c, 0x25, 0xbb,
        0xb8, 0x56, 0x8f, 0xc3, 0xd3, 0x76, 0xa6, 0xd9
    };
#endif

    byte resultT[sizeof(t1)];
    byte resultP[sizeof(p)];
#ifdef WOLFSSL_XILINX_CRYPT
    byte resultC[sizeof(p) + 16];
#else
    byte resultC[sizeof(p)];
#endif
    int  result;
#if !defined(HAVE_FIPS) && !defined(STM32_CRYPTO)
    int  ivlen;
#endif
    int  alen, plen;

#if !defined(BENCH_EMBEDDED)
    #ifndef BENCH_AESGCM_LARGE
        #define BENCH_AESGCM_LARGE 1024
    #endif
    byte large_input[BENCH_AESGCM_LARGE];
    byte large_output[BENCH_AESGCM_LARGE];
    byte large_outdec[BENCH_AESGCM_LARGE];

    XMEMSET(large_input, 0, sizeof(large_input));
    XMEMSET(large_output, 0, sizeof(large_output));
    XMEMSET(large_outdec, 0, sizeof(large_outdec));
#endif

    XMEMSET(resultT, 0, sizeof(resultT));
    XMEMSET(resultC, 0, sizeof(resultC));
    XMEMSET(resultP, 0, sizeof(resultP));

    if (wc_AesInit(&enc, HEAP_HINT, devId) != 0) {
        return -4300;
    }

    result = wc_AesGcmSetKey(&enc, k1, sizeof(k1));
    if (result != 0)
        return -4301;

    /* AES-GCM encrypt and decrypt both use AES encrypt internally */
    result = wc_AesGcmEncrypt(&enc, resultC, p, sizeof(p), iv1, sizeof(iv1),
                                        resultT, sizeof(resultT), a, sizeof(a));
#if defined(WOLFSSL_ASYNC_CRYPT)
    result = wc_AsyncWait(result, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (result != 0)
        return -4302;
    if (XMEMCMP(c1, resultC, sizeof(p)))
        return -4303;
    if (XMEMCMP(t1, resultT, sizeof(resultT)))
        return -4304;

    result = wc_AesGcmDecrypt(&enc, resultP, resultC, sizeof(p),
                      iv1, sizeof(iv1), resultT, sizeof(resultT), a, sizeof(a));
#if defined(WOLFSSL_ASYNC_CRYPT)
    result = wc_AsyncWait(result, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (result != 0)
        return -4305;
    if (XMEMCMP(p, resultP, sizeof(resultP)))
        return -4306;

    /* Large buffer test */
#ifdef BENCH_AESGCM_LARGE
    /* setup test buffer */
    for (alen=0; alen<BENCH_AESGCM_LARGE; alen++)
        large_input[alen] = (byte)alen;

    /* AES-GCM encrypt and decrypt both use AES encrypt internally */
    result = wc_AesGcmEncrypt(&enc, large_output, large_input,
                              BENCH_AESGCM_LARGE, iv1, sizeof(iv1),
                              resultT, sizeof(resultT), a, sizeof(a));
#if defined(WOLFSSL_ASYNC_CRYPT)
    result = wc_AsyncWait(result, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (result != 0)
        return -4307;

    result = wc_AesGcmDecrypt(&enc, large_outdec, large_output,
                              BENCH_AESGCM_LARGE, iv1, sizeof(iv1), resultT,
                              sizeof(resultT), a, sizeof(a));
#if defined(WOLFSSL_ASYNC_CRYPT)
    result = wc_AsyncWait(result, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (result != 0)
        return -4308;
    if (XMEMCMP(large_input, large_outdec, BENCH_AESGCM_LARGE))
        return -4309;
#endif /* BENCH_AESGCM_LARGE */

#if !defined(HAVE_FIPS) && !defined(STM32_CRYPTO) && !defined(WOLFSSL_XILINX_CRYPT)
    /* Variable IV length test */
    for (ivlen=0; ivlen<(int)sizeof(k1); ivlen++) {
         /* AES-GCM encrypt and decrypt both use AES encrypt internally */
         result = wc_AesGcmEncrypt(&enc, resultC, p, sizeof(p), k1,
                         (word32)ivlen, resultT, sizeof(resultT), a, sizeof(a));
#if defined(WOLFSSL_ASYNC_CRYPT)
        result = wc_AsyncWait(result, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
        if (result != 0)
            return -4310;
        result = wc_AesGcmDecrypt(&enc, resultP, resultC, sizeof(resultC), k1,
                         (word32)ivlen, resultT, sizeof(resultT), a, sizeof(a));
#if defined(WOLFSSL_ASYNC_CRYPT)
        result = wc_AsyncWait(result, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
        if (result != 0)
            return -4311;
    }
#endif

    /* Variable authenticed data length test */
    for (alen=0; alen<(int)sizeof(p); alen++) {
         /* AES-GCM encrypt and decrypt both use AES encrypt internally */
         result = wc_AesGcmEncrypt(&enc, resultC, p, sizeof(p), iv1,
                        sizeof(iv1), resultT, sizeof(resultT), p, (word32)alen);
#if defined(WOLFSSL_ASYNC_CRYPT)
        result = wc_AsyncWait(result, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
        if (result != 0)
            return -4312;
        result = wc_AesGcmDecrypt(&enc, resultP, resultC, sizeof(p), iv1,
                        sizeof(iv1), resultT, sizeof(resultT), p, (word32)alen);
#if defined(WOLFSSL_ASYNC_CRYPT)
        result = wc_AsyncWait(result, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
        if (result != 0)
            return -4313;
    }

#ifndef WOLFSSL_XILINX_CRYPT
    /* Variable plain text length test */
    for (plen=1; plen<(int)sizeof(p); plen++) {
         /* AES-GCM encrypt and decrypt both use AES encrypt internally */
         result = wc_AesGcmEncrypt(&enc, resultC, p, (word32)plen, iv1,
                           sizeof(iv1), resultT, sizeof(resultT), a, sizeof(a));
#if defined(WOLFSSL_ASYNC_CRYPT)
        result = wc_AsyncWait(result, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
        if (result != 0)
            return -4314;
        result = wc_AesGcmDecrypt(&enc, resultP, resultC, (word32)plen, iv1,
                           sizeof(iv1), resultT, sizeof(resultT), a, sizeof(a));
#if defined(WOLFSSL_ASYNC_CRYPT)
        result = wc_AsyncWait(result, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
        if (result != 0)
            return -4315;
    }
#endif

    /* test with IV != 12 bytes */
#ifdef ENABLE_NON_12BYTE_IV_TEST
    XMEMSET(resultT, 0, sizeof(resultT));
    XMEMSET(resultC, 0, sizeof(resultC));
    XMEMSET(resultP, 0, sizeof(resultP));

    wc_AesGcmSetKey(&enc, k2, sizeof(k2));
    /* AES-GCM encrypt and decrypt both use AES encrypt internally */
    result = wc_AesGcmEncrypt(&enc, resultC, p, sizeof(p), iv2, sizeof(iv2),
                                        resultT, sizeof(resultT), a, sizeof(a));
#if defined(WOLFSSL_ASYNC_CRYPT)
    result = wc_AsyncWait(result, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (result != 0)
        return -4316;
    if (XMEMCMP(c2, resultC, sizeof(resultC)))
        return -4317;
    if (XMEMCMP(t2, resultT, sizeof(resultT)))
        return -4318;

    result = wc_AesGcmDecrypt(&enc, resultP, resultC, sizeof(resultC),
                      iv2, sizeof(iv2), resultT, sizeof(resultT), a, sizeof(a));
#if defined(WOLFSSL_ASYNC_CRYPT)
    result = wc_AsyncWait(result, &enc.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (result != 0)
        return -4319;
    if (XMEMCMP(p, resultP, sizeof(resultP)))
        return -4320;
#endif /* ENABLE_NON_12BYTE_IV_TEST */

    wc_AesFree(&enc);

    return 0;
}

int gmac_test(void)
{
    Gmac gmac;

    const byte k1[] =
    {
        0x89, 0xc9, 0x49, 0xe9, 0xc8, 0x04, 0xaf, 0x01,
        0x4d, 0x56, 0x04, 0xb3, 0x94, 0x59, 0xf2, 0xc8
    };
    const byte iv1[] =
    {
        0xd1, 0xb1, 0x04, 0xc8, 0x15, 0xbf, 0x1e, 0x94,
        0xe2, 0x8c, 0x8f, 0x16
    };
    const byte a1[] =
    {
       0x82, 0xad, 0xcd, 0x63, 0x8d, 0x3f, 0xa9, 0xd9,
       0xf3, 0xe8, 0x41, 0x00, 0xd6, 0x1e, 0x07, 0x77
    };
    const byte t1[] =
    {
        0x88, 0xdb, 0x9d, 0x62, 0x17, 0x2e, 0xd0, 0x43,
        0xaa, 0x10, 0xf1, 0x6d, 0x22, 0x7d, 0xc4, 0x1b
    };

    const byte k2[] =
    {
        0x40, 0xf7, 0xec, 0xb2, 0x52, 0x6d, 0xaa, 0xd4,
        0x74, 0x25, 0x1d, 0xf4, 0x88, 0x9e, 0xf6, 0x5b
    };
    const byte iv2[] =
    {
        0xee, 0x9c, 0x6e, 0x06, 0x15, 0x45, 0x45, 0x03,
        0x1a, 0x60, 0x24, 0xa7
    };
    const byte a2[] =
    {
        0x94, 0x81, 0x2c, 0x87, 0x07, 0x4e, 0x15, 0x18,
        0x34, 0xb8, 0x35, 0xaf, 0x1c, 0xa5, 0x7e, 0x56
    };
    const byte t2[] =
    {
        0xc6, 0x81, 0x79, 0x8e, 0x3d, 0xda, 0xb0, 0x9f,
        0x8d, 0x83, 0xb0, 0xbb, 0x14, 0xb6, 0x91
    };

    byte tag[16];

    XMEMSET(&gmac, 0, sizeof(Gmac)); /* clear context */
    XMEMSET(tag, 0, sizeof(tag));
    wc_GmacSetKey(&gmac, k1, sizeof(k1));
    wc_GmacUpdate(&gmac, iv1, sizeof(iv1), a1, sizeof(a1), tag, sizeof(t1));
    if (XMEMCMP(t1, tag, sizeof(t1)) != 0)
        return -4400;

    XMEMSET(tag, 0, sizeof(tag));
    wc_GmacSetKey(&gmac, k2, sizeof(k2));
    wc_GmacUpdate(&gmac, iv2, sizeof(iv2), a2, sizeof(a2), tag, sizeof(t2));
    if (XMEMCMP(t2, tag, sizeof(t2)) != 0)
        return -4401;

    return 0;
}
#endif /* HAVE_AESGCM */

#ifdef HAVE_AESCCM
int aesccm_test(void)
{
    Aes enc;

    /* key */
    const byte k[] =
    {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf
    };

    /* nonce */
    const byte iv[] =
    {
        0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0xa0,
        0xa1, 0xa2, 0xa3, 0xa4, 0xa5
    };

    /* plaintext */
    const byte p[] =
    {
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e
    };

    const byte a[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    };

    const byte c[] =
    {
        0x58, 0x8c, 0x97, 0x9a, 0x61, 0xc6, 0x63, 0xd2,
        0xf0, 0x66, 0xd0, 0xc2, 0xc0, 0xf9, 0x89, 0x80,
        0x6d, 0x5f, 0x6b, 0x61, 0xda, 0xc3, 0x84
    };

    const byte t[] =
    {
        0x17, 0xe8, 0xd1, 0x2c, 0xfd, 0xf9, 0x26, 0xe0
    };

    byte t2[sizeof(t)];
    byte p2[sizeof(p)];
    byte c2[sizeof(c)];

    int result;

    XMEMSET(&enc, 0, sizeof(Aes)); /* clear context */
    XMEMSET(t2, 0, sizeof(t2));
    XMEMSET(c2, 0, sizeof(c2));
    XMEMSET(p2, 0, sizeof(p2));

    result = wc_AesCcmSetKey(&enc, k, sizeof(k));
    if (result != 0)
        return -4500;

    /* AES-CCM encrypt and decrypt both use AES encrypt internally */
    result = wc_AesCcmEncrypt(&enc, c2, p, sizeof(c2), iv, sizeof(iv),
                                                 t2, sizeof(t2), a, sizeof(a));
    if (result != 0)
        return -4501;
    if (XMEMCMP(c, c2, sizeof(c2)))
        return -4502;
    if (XMEMCMP(t, t2, sizeof(t2)))
        return -4503;

    result = wc_AesCcmDecrypt(&enc, p2, c2, sizeof(p2), iv, sizeof(iv),
                                                 t2, sizeof(t2), a, sizeof(a));
    if (result != 0)
        return -4504;
    if (XMEMCMP(p, p2, sizeof(p2)))
        return -4505;

    /* Test the authentication failure */
    t2[0]++; /* Corrupt the authentication tag. */
    result = wc_AesCcmDecrypt(&enc, p2, c, sizeof(p2), iv, sizeof(iv),
                                                 t2, sizeof(t2), a, sizeof(a));
    if (result == 0)
        return -4506;

    /* Clear c2 to compare against p2. p2 should be set to zero in case of
     * authentication fail. */
    XMEMSET(c2, 0, sizeof(c2));
    if (XMEMCMP(p2, c2, sizeof(p2)))
        return -4507;

    return 0;
}
#endif /* HAVE_AESCCM */


#ifdef HAVE_AES_KEYWRAP

#define MAX_KEYWRAP_TEST_OUTLEN 40
#define MAX_KEYWRAP_TEST_PLAINLEN 32

typedef struct keywrapVector {
    const byte* kek;
    const byte* data;
    const byte* verify;
    word32 kekLen;
    word32 dataLen;
    word32 verifyLen;
} keywrapVector;

int aeskeywrap_test(void)
{
    int wrapSz, plainSz, testSz, i;

    /* test vectors from RFC 3394 (kek, data, verify) */

    /* Wrap 128 bits of Key Data with a 128-bit KEK */
    const byte k1[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    const byte d1[] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };

    const byte v1[] = {
        0x1F, 0xA6, 0x8B, 0x0A, 0x81, 0x12, 0xB4, 0x47,
        0xAE, 0xF3, 0x4B, 0xD8, 0xFB, 0x5A, 0x7B, 0x82,
        0x9D, 0x3E, 0x86, 0x23, 0x71, 0xD2, 0xCF, 0xE5
    };

    /* Wrap 128 bits of Key Data with a 192-bit KEK */
    const byte k2[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
    };

    const byte d2[] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };

    const byte v2[] = {
        0x96, 0x77, 0x8B, 0x25, 0xAE, 0x6C, 0xA4, 0x35,
        0xF9, 0x2B, 0x5B, 0x97, 0xC0, 0x50, 0xAE, 0xD2,
        0x46, 0x8A, 0xB8, 0xA1, 0x7A, 0xD8, 0x4E, 0x5D
    };

    /* Wrap 128 bits of Key Data with a 256-bit KEK */
    const byte k3[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
    };

    const byte d3[] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };

    const byte v3[] = {
        0x64, 0xE8, 0xC3, 0xF9, 0xCE, 0x0F, 0x5B, 0xA2,
        0x63, 0xE9, 0x77, 0x79, 0x05, 0x81, 0x8A, 0x2A,
        0x93, 0xC8, 0x19, 0x1E, 0x7D, 0x6E, 0x8A, 0xE7
    };

    /* Wrap 192 bits of Key Data with a 192-bit KEK */
    const byte k4[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
    };

    const byte d4[] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    };

    const byte v4[] = {
        0x03, 0x1D, 0x33, 0x26, 0x4E, 0x15, 0xD3, 0x32,
        0x68, 0xF2, 0x4E, 0xC2, 0x60, 0x74, 0x3E, 0xDC,
        0xE1, 0xC6, 0xC7, 0xDD, 0xEE, 0x72, 0x5A, 0x93,
        0x6B, 0xA8, 0x14, 0x91, 0x5C, 0x67, 0x62, 0xD2
    };

    /* Wrap 192 bits of Key Data with a 256-bit KEK */
    const byte k5[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
    };

    const byte d5[] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    };

    const byte v5[] = {
        0xA8, 0xF9, 0xBC, 0x16, 0x12, 0xC6, 0x8B, 0x3F,
        0xF6, 0xE6, 0xF4, 0xFB, 0xE3, 0x0E, 0x71, 0xE4,
        0x76, 0x9C, 0x8B, 0x80, 0xA3, 0x2C, 0xB8, 0x95,
        0x8C, 0xD5, 0xD1, 0x7D, 0x6B, 0x25, 0x4D, 0xA1
    };

    /* Wrap 256 bits of Key Data with a 256-bit KEK */
    const byte k6[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
    };

    const byte d6[] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    const byte v6[] = {
        0x28, 0xC9, 0xF4, 0x04, 0xC4, 0xB8, 0x10, 0xF4,
        0xCB, 0xCC, 0xB3, 0x5C, 0xFB, 0x87, 0xF8, 0x26,
        0x3F, 0x57, 0x86, 0xE2, 0xD8, 0x0E, 0xD3, 0x26,
        0xCB, 0xC7, 0xF0, 0xE7, 0x1A, 0x99, 0xF4, 0x3B,
        0xFB, 0x98, 0x8B, 0x9B, 0x7A, 0x02, 0xDD, 0x21
    };

    byte output[MAX_KEYWRAP_TEST_OUTLEN];
    byte plain [MAX_KEYWRAP_TEST_PLAINLEN];

    const keywrapVector test_wrap[] =
    {
        {k1, d1, v1, sizeof(k1), sizeof(d1), sizeof(v1)},
        {k2, d2, v2, sizeof(k2), sizeof(d2), sizeof(v2)},
        {k3, d3, v3, sizeof(k3), sizeof(d3), sizeof(v3)},
        {k4, d4, v4, sizeof(k4), sizeof(d4), sizeof(v4)},
        {k5, d5, v5, sizeof(k5), sizeof(d5), sizeof(v5)},
        {k6, d6, v6, sizeof(k6), sizeof(d6), sizeof(v6)}
    };
    testSz = sizeof(test_wrap) / sizeof(keywrapVector);

    XMEMSET(output, 0, sizeof(output));
    XMEMSET(plain,  0, sizeof(plain));

    for (i = 0; i < testSz; i++) {

        wrapSz = wc_AesKeyWrap(test_wrap[i].kek, test_wrap[i].kekLen,
                               test_wrap[i].data, test_wrap[i].dataLen,
                               output, sizeof(output), NULL);

        if ( (wrapSz < 0) || (wrapSz != (int)test_wrap[i].verifyLen) )
            return -4600;

        if (XMEMCMP(output, test_wrap[i].verify, test_wrap[i].verifyLen) != 0)
            return -4601;

        plainSz = wc_AesKeyUnWrap((byte*)test_wrap[i].kek, test_wrap[i].kekLen,
                                  output, wrapSz,
                                  plain, sizeof(plain), NULL);

        if ( (plainSz < 0) || (plainSz != (int)test_wrap[i].dataLen) )
            return -4602;

        if (XMEMCMP(plain, test_wrap[i].data, test_wrap[i].dataLen) != 0)
            return -4610 - i;
    }

    return 0;
}
#endif /* HAVE_AES_KEYWRAP */


#endif /* NO_AES */


#ifdef HAVE_CAMELLIA

enum {
    CAM_ECB_ENC, CAM_ECB_DEC, CAM_CBC_ENC, CAM_CBC_DEC
};

typedef struct {
    int type;
    const byte* plaintext;
    const byte* iv;
    const byte* ciphertext;
    const byte* key;
    word32 keySz;
    int errorCode;
} test_vector_t;

int camellia_test(void)
{
    /* Camellia ECB Test Plaintext */
    static const byte pte[] =
    {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
    };

    /* Camellia ECB Test Initialization Vector */
    static const byte ive[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    /* Test 1: Camellia ECB 128-bit key */
    static const byte k1[] =
    {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
    };
    static const byte c1[] =
    {
        0x67, 0x67, 0x31, 0x38, 0x54, 0x96, 0x69, 0x73,
        0x08, 0x57, 0x06, 0x56, 0x48, 0xea, 0xbe, 0x43
    };

    /* Test 2: Camellia ECB 192-bit key */
    static const byte k2[] =
    {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
    };
    static const byte c2[] =
    {
        0xb4, 0x99, 0x34, 0x01, 0xb3, 0xe9, 0x96, 0xf8,
        0x4e, 0xe5, 0xce, 0xe7, 0xd7, 0x9b, 0x09, 0xb9
    };

    /* Test 3: Camellia ECB 256-bit key */
    static const byte k3[] =
    {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };
    static const byte c3[] =
    {
        0x9a, 0xcc, 0x23, 0x7d, 0xff, 0x16, 0xd7, 0x6c,
        0x20, 0xef, 0x7c, 0x91, 0x9e, 0x3a, 0x75, 0x09
    };

    /* Camellia CBC Test Plaintext */
    static const byte ptc[] =
    {
        0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96,
        0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A
    };

    /* Camellia CBC Test Initialization Vector */
    static const byte ivc[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    /* Test 4: Camellia-CBC 128-bit key */
    static const byte k4[] =
    {
        0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
        0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
    };
    static const byte c4[] =
    {
        0x16, 0x07, 0xCF, 0x49, 0x4B, 0x36, 0xBB, 0xF0,
        0x0D, 0xAE, 0xB0, 0xB5, 0x03, 0xC8, 0x31, 0xAB
    };

    /* Test 5: Camellia-CBC 192-bit key */
    static const byte k5[] =
    {
        0x8E, 0x73, 0xB0, 0xF7, 0xDA, 0x0E, 0x64, 0x52,
        0xC8, 0x10, 0xF3, 0x2B, 0x80, 0x90, 0x79, 0xE5,
        0x62, 0xF8, 0xEA, 0xD2, 0x52, 0x2C, 0x6B, 0x7B
    };
    static const byte c5[] =
    {
        0x2A, 0x48, 0x30, 0xAB, 0x5A, 0xC4, 0xA1, 0xA2,
        0x40, 0x59, 0x55, 0xFD, 0x21, 0x95, 0xCF, 0x93
    };

    /* Test 6: CBC 256-bit key */
    static const byte k6[] =
    {
        0x60, 0x3D, 0xEB, 0x10, 0x15, 0xCA, 0x71, 0xBE,
        0x2B, 0x73, 0xAE, 0xF0, 0x85, 0x7D, 0x77, 0x81,
        0x1F, 0x35, 0x2C, 0x07, 0x3B, 0x61, 0x08, 0xD7,
        0x2D, 0x98, 0x10, 0xA3, 0x09, 0x14, 0xDF, 0xF4
    };
    static const byte c6[] =
    {
        0xE6, 0xCF, 0xA3, 0x5F, 0xC0, 0x2B, 0x13, 0x4A,
        0x4D, 0x2C, 0x0B, 0x67, 0x37, 0xAC, 0x3E, 0xDA
    };

    byte out[CAMELLIA_BLOCK_SIZE];
    Camellia cam;
    int i, testsSz, ret;
    const test_vector_t testVectors[] =
    {
        {CAM_ECB_ENC, pte, ive, c1, k1, sizeof(k1), -114},
        {CAM_ECB_ENC, pte, ive, c2, k2, sizeof(k2), -115},
        {CAM_ECB_ENC, pte, ive, c3, k3, sizeof(k3), -116},
        {CAM_ECB_DEC, pte, ive, c1, k1, sizeof(k1), -117},
        {CAM_ECB_DEC, pte, ive, c2, k2, sizeof(k2), -118},
        {CAM_ECB_DEC, pte, ive, c3, k3, sizeof(k3), -119},
        {CAM_CBC_ENC, ptc, ivc, c4, k4, sizeof(k4), -120},
        {CAM_CBC_ENC, ptc, ivc, c5, k5, sizeof(k5), -121},
        {CAM_CBC_ENC, ptc, ivc, c6, k6, sizeof(k6), -122},
        {CAM_CBC_DEC, ptc, ivc, c4, k4, sizeof(k4), -123},
        {CAM_CBC_DEC, ptc, ivc, c5, k5, sizeof(k5), -124},
        {CAM_CBC_DEC, ptc, ivc, c6, k6, sizeof(k6), -125}
    };

    testsSz = sizeof(testVectors)/sizeof(test_vector_t);
    for (i = 0; i < testsSz; i++) {
        if (wc_CamelliaSetKey(&cam, testVectors[i].key, testVectors[i].keySz,
                                                        testVectors[i].iv) != 0)
            return testVectors[i].errorCode;

        switch (testVectors[i].type) {
            case CAM_ECB_ENC:
                ret = wc_CamelliaEncryptDirect(&cam, out,
                                                testVectors[i].plaintext);
                if (ret != 0 || XMEMCMP(out, testVectors[i].ciphertext,
                                                        CAMELLIA_BLOCK_SIZE))
                    return testVectors[i].errorCode;
                break;
            case CAM_ECB_DEC:
                ret = wc_CamelliaDecryptDirect(&cam, out,
                                                    testVectors[i].ciphertext);
                if (ret != 0 || XMEMCMP(out, testVectors[i].plaintext,
                                                        CAMELLIA_BLOCK_SIZE))
                    return testVectors[i].errorCode;
                break;
            case CAM_CBC_ENC:
                ret = wc_CamelliaCbcEncrypt(&cam, out, testVectors[i].plaintext,
                                                           CAMELLIA_BLOCK_SIZE);
                if (ret != 0 || XMEMCMP(out, testVectors[i].ciphertext,
                                                        CAMELLIA_BLOCK_SIZE))
                    return testVectors[i].errorCode;
                break;
            case CAM_CBC_DEC:
                ret = wc_CamelliaCbcDecrypt(&cam, out,
                                testVectors[i].ciphertext, CAMELLIA_BLOCK_SIZE);
                if (ret != 0 || XMEMCMP(out, testVectors[i].plaintext,
                                                           CAMELLIA_BLOCK_SIZE))
                    return testVectors[i].errorCode;
                break;
            default:
                break;
        }
    }

    /* Setting the IV and checking it was actually set. */
    ret = wc_CamelliaSetIV(&cam, ivc);
    if (ret != 0 || XMEMCMP(cam.reg, ivc, CAMELLIA_BLOCK_SIZE))
        return -4700;

    /* Setting the IV to NULL should be same as all zeros IV */
    if (wc_CamelliaSetIV(&cam, NULL) != 0 ||
                                    XMEMCMP(cam.reg, ive, CAMELLIA_BLOCK_SIZE))
        return -4701;

    /* First parameter should never be null */
    if (wc_CamelliaSetIV(NULL, NULL) == 0)
        return -4702;

    /* First parameter should never be null, check it fails */
    if (wc_CamelliaSetKey(NULL, k1, sizeof(k1), NULL) == 0)
        return -4703;

    /* Key should have a size of 16, 24, or 32 */
    if (wc_CamelliaSetKey(&cam, k1, 0, NULL) == 0)
        return -4704;

    return 0;
}
#endif /* HAVE_CAMELLIA */

#ifdef HAVE_IDEA
int idea_test(void)
{
    int ret;
    word16 i, j;

    Idea idea;
    byte data[IDEA_BLOCK_SIZE];

    /* Project NESSIE test vectors */
#define IDEA_NB_TESTS   6
#define IDEA_NB_TESTS_EXTRA 4

    const byte v_key[IDEA_NB_TESTS][IDEA_KEY_SIZE] = {
        { 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37,
            0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
        { 0x57, 0x57, 0x57, 0x57, 0x57, 0x57, 0x57, 0x57,
            0x57, 0x57, 0x57, 0x57, 0x57, 0x57, 0x57, 0x57 },
        { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F },
        { 0x2B, 0xD6, 0x45, 0x9F, 0x82, 0xC5, 0xB3, 0x00,
            0x95, 0x2C, 0x49, 0x10, 0x48, 0x81, 0xFF, 0x48 },
        { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F },
        { 0x2B, 0xD6, 0x45, 0x9F, 0x82, 0xC5, 0xB3, 0x00,
            0x95, 0x2C, 0x49, 0x10, 0x48, 0x81, 0xFF, 0x48 },
    };

    const byte v1_plain[IDEA_NB_TESTS][IDEA_BLOCK_SIZE] = {
        { 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37, 0x37 },
        { 0x57, 0x57, 0x57, 0x57, 0x57, 0x57, 0x57, 0x57 },
        { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 },
        { 0xEA, 0x02, 0x47, 0x14, 0xAD, 0x5C, 0x4D, 0x84 },
        { 0xDB, 0x2D, 0x4A, 0x92, 0xAA, 0x68, 0x27, 0x3F },
        { 0xF1, 0x29, 0xA6, 0x60, 0x1E, 0xF6, 0x2A, 0x47 },
    };

    byte v1_cipher[IDEA_NB_TESTS][IDEA_BLOCK_SIZE] = {
        { 0x54, 0xCF, 0x21, 0xE3, 0x89, 0xD8, 0x73, 0xEC },
        { 0x85, 0x52, 0x4D, 0x41, 0x0E, 0xB4, 0x28, 0xAE },
        { 0xF5, 0x26, 0xAB, 0x9A, 0x62, 0xC0, 0xD2, 0x58 },
        { 0xC8, 0xFB, 0x51, 0xD3, 0x51, 0x66, 0x27, 0xA8 },
        { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 },
        { 0xEA, 0x02, 0x47, 0x14, 0xAD, 0x5C, 0x4D, 0x84 },
    };

    byte v1_cipher_100[IDEA_NB_TESTS_EXTRA][IDEA_BLOCK_SIZE]  = {
        { 0x12, 0x46, 0x2F, 0xD0, 0xFB, 0x3A, 0x63, 0x39 },
        { 0x15, 0x61, 0xE8, 0xC9, 0x04, 0x54, 0x8B, 0xE9 },
        { 0x42, 0x12, 0x2A, 0x94, 0xB0, 0xF6, 0xD2, 0x43 },
        { 0x53, 0x4D, 0xCD, 0x48, 0xDD, 0xD5, 0xF5, 0x9C },
    };

    byte v1_cipher_1000[IDEA_NB_TESTS_EXTRA][IDEA_BLOCK_SIZE] = {
        { 0x44, 0x1B, 0x38, 0x5C, 0x77, 0x29, 0x75, 0x34 },
        { 0xF0, 0x4E, 0x58, 0x88, 0x44, 0x99, 0x22, 0x2D },
        { 0xB3, 0x5F, 0x93, 0x7F, 0x6A, 0xA0, 0xCD, 0x1F },
        { 0x9A, 0xEA, 0x46, 0x8F, 0x42, 0x9B, 0xBA, 0x15 },
    };

    /* CBC test */
    const char *message = "International Data Encryption Algorithm";
    byte msg_enc[40], msg_dec[40];

    for (i = 0; i < IDEA_NB_TESTS; i++) {
        /* Set encryption key */
        XMEMSET(&idea, 0, sizeof(Idea));
        ret = wc_IdeaSetKey(&idea, v_key[i], IDEA_KEY_SIZE,
                            NULL, IDEA_ENCRYPTION);
        if (ret != 0) {
            printf("wc_IdeaSetKey (enc) failed\n");
            return -4800;
        }

        /* Data encryption */
        ret = wc_IdeaCipher(&idea, data, v1_plain[i]);
        if (ret != 0 || XMEMCMP(&v1_cipher[i], data, IDEA_BLOCK_SIZE)) {
            printf("Bad encryption\n");
            return -4801;
        }

        /* Set decryption key */
        XMEMSET(&idea, 0, sizeof(Idea));
        ret = wc_IdeaSetKey(&idea, v_key[i], IDEA_KEY_SIZE,
                            NULL, IDEA_DECRYPTION);
        if (ret != 0) {
            printf("wc_IdeaSetKey (dec) failed\n");
            return -4802;
        }

        /* Data decryption */
        ret = wc_IdeaCipher(&idea, data, data);
        if (ret != 0 || XMEMCMP(v1_plain[i], data, IDEA_BLOCK_SIZE)) {
            printf("Bad decryption\n");
            return -4803;
        }

        /* Set encryption key */
        XMEMSET(&idea, 0, sizeof(Idea));
        ret = wc_IdeaSetKey(&idea, v_key[i], IDEA_KEY_SIZE,
                            v_key[i], IDEA_ENCRYPTION);
        if (ret != 0) {
            printf("wc_IdeaSetKey (enc) failed\n");
            return -4804;
        }

        XMEMSET(msg_enc, 0, sizeof(msg_enc));
        ret = wc_IdeaCbcEncrypt(&idea, msg_enc, (byte *)message,
                                (word32)XSTRLEN(message)+1);
        if (ret != 0) {
            printf("wc_IdeaCbcEncrypt failed\n");
            return -4805;
        }

        /* Set decryption key */
        XMEMSET(&idea, 0, sizeof(Idea));
        ret = wc_IdeaSetKey(&idea, v_key[i], IDEA_KEY_SIZE,
                            v_key[i], IDEA_DECRYPTION);
        if (ret != 0) {
            printf("wc_IdeaSetKey (dec) failed\n");
            return -4806;
        }

        XMEMSET(msg_dec, 0, sizeof(msg_dec));
        ret = wc_IdeaCbcDecrypt(&idea, msg_dec, msg_enc,
                                (word32)XSTRLEN(message)+1);
        if (ret != 0) {
            printf("wc_IdeaCbcDecrypt failed\n");
            return -4807;
        }

        if (XMEMCMP(message, msg_dec, (word32)XSTRLEN(message))) {
            printf("Bad CBC decryption\n");
            return -4808;
        }
    }

    for (i = 0; i < IDEA_NB_TESTS_EXTRA; i++) {
        /* Set encryption key */
        XMEMSET(&idea, 0, sizeof(Idea));
        ret = wc_IdeaSetKey(&idea, v_key[i], IDEA_KEY_SIZE,
                            NULL, IDEA_ENCRYPTION);
        if (ret != 0) {
            printf("wc_IdeaSetKey (enc) failed\n");
            return -4809;
        }

        /* 100 times data encryption */
        XMEMCPY(data, v1_plain[i], IDEA_BLOCK_SIZE);
        for (j = 0; j < 100; j++) {
            ret = wc_IdeaCipher(&idea, data, data);
            if (ret != 0) {
                return -4821;
            }
        }

        if (XMEMCMP(v1_cipher_100[i], data, IDEA_BLOCK_SIZE)) {
            printf("Bad encryption (100 times)\n");
            return -4810;
        }

        /* 1000 times data encryption */
        XMEMCPY(data, v1_plain[i], IDEA_BLOCK_SIZE);
        for (j = 0; j < 1000; j++) {
            ret = wc_IdeaCipher(&idea, data, data);
            if (ret != 0) {
                return -4822;
            }
        }

        if (XMEMCMP(v1_cipher_1000[i], data, IDEA_BLOCK_SIZE)) {
            printf("Bad encryption (100 times)\n");
            return -4811;
        }
    }

#ifndef WC_NO_RNG
    /* random test for CBC */
    {
        WC_RNG rng;
        byte key[IDEA_KEY_SIZE], iv[IDEA_BLOCK_SIZE],
        rnd[1000], enc[1000], dec[1000];

        /* random values */
    #ifndef HAVE_FIPS
        ret = wc_InitRng_ex(&rng, HEAP_HINT, devId);
    #else
        ret = wc_InitRng(&rng);
    #endif
        if (ret != 0)
            return -4812;

        for (i = 0; i < 1000; i++) {
            /* random key */
            ret = wc_RNG_GenerateBlock(&rng, key, sizeof(key));
            if (ret != 0)
                return -4813;

            /* random iv */
            ret = wc_RNG_GenerateBlock(&rng, iv, sizeof(iv));
            if (ret != 0)
                return -4814;

            /* random data */
            ret = wc_RNG_GenerateBlock(&rng, rnd, sizeof(rnd));
            if (ret != 0)
                return -4815;

            /* Set encryption key */
            XMEMSET(&idea, 0, sizeof(Idea));
            ret = wc_IdeaSetKey(&idea, key, IDEA_KEY_SIZE, iv, IDEA_ENCRYPTION);
            if (ret != 0) {
                printf("wc_IdeaSetKey (enc) failed\n");
                return -4816;
            }

            /* Data encryption */
            XMEMSET(enc, 0, sizeof(enc));
            ret = wc_IdeaCbcEncrypt(&idea, enc, rnd, sizeof(rnd));
            if (ret != 0) {
                printf("wc_IdeaCbcEncrypt failed\n");
                return -4817;
            }

            /* Set decryption key */
            XMEMSET(&idea, 0, sizeof(Idea));
            ret = wc_IdeaSetKey(&idea, key, IDEA_KEY_SIZE, iv, IDEA_DECRYPTION);
            if (ret != 0) {
                printf("wc_IdeaSetKey (enc) failed\n");
                return -4818;
            }

            /* Data decryption */
            XMEMSET(dec, 0, sizeof(dec));
            ret = wc_IdeaCbcDecrypt(&idea, dec, enc, sizeof(enc));
            if (ret != 0) {
                printf("wc_IdeaCbcDecrypt failed\n");
                return -4819;
            }

            if (XMEMCMP(rnd, dec, sizeof(rnd))) {
                printf("Bad CBC decryption\n");
                return -4820;
            }
        }

        wc_FreeRng(&rng);
    }
#endif /* WC_NO_RNG */

    return 0;
}
#endif /* HAVE_IDEA */


#ifndef WC_NO_RNG
static int random_rng_test(void)
{
    WC_RNG rng;
    byte block[32];
    int ret, i;

#ifndef HAVE_FIPS
    ret = wc_InitRng_ex(&rng, HEAP_HINT, devId);
#else
    ret = wc_InitRng(&rng);
#endif
    if (ret != 0) return -4900;

    XMEMSET(block, 0, sizeof(block));

    ret = wc_RNG_GenerateBlock(&rng, block, sizeof(block));
    if (ret != 0) {
        ret = -4901;
        goto exit;
    }

    /* Check for 0's */
    for (i=0; i<(int)sizeof(block); i++) {
        if (block[i] == 0) {
            ret++;
        }
    }
    /* All zeros count check */
    if (ret >= (int)sizeof(block)) {
        ret = -4902;
        goto exit;
    }

    ret = wc_RNG_GenerateByte(&rng, block);
    if (ret != 0) {
        ret = -4903;
        goto exit;
    }

exit:
    /* Make sure and free RNG */
    wc_FreeRng(&rng);

    return ret;
}

#if defined(HAVE_HASHDRBG) && !defined(CUSTOM_RAND_GENERATE_BLOCK)

int random_test(void)
{
    const byte test1Entropy[] =
    {
        0xa6, 0x5a, 0xd0, 0xf3, 0x45, 0xdb, 0x4e, 0x0e, 0xff, 0xe8, 0x75, 0xc3,
        0xa2, 0xe7, 0x1f, 0x42, 0xc7, 0x12, 0x9d, 0x62, 0x0f, 0xf5, 0xc1, 0x19,
        0xa9, 0xef, 0x55, 0xf0, 0x51, 0x85, 0xe0, 0xfb, 0x85, 0x81, 0xf9, 0x31,
        0x75, 0x17, 0x27, 0x6e, 0x06, 0xe9, 0x60, 0x7d, 0xdb, 0xcb, 0xcc, 0x2e
    };
    const byte test1Output[] =
    {
        0xd3, 0xe1, 0x60, 0xc3, 0x5b, 0x99, 0xf3, 0x40, 0xb2, 0x62, 0x82, 0x64,
        0xd1, 0x75, 0x10, 0x60, 0xe0, 0x04, 0x5d, 0xa3, 0x83, 0xff, 0x57, 0xa5,
        0x7d, 0x73, 0xa6, 0x73, 0xd2, 0xb8, 0xd8, 0x0d, 0xaa, 0xf6, 0xa6, 0xc3,
        0x5a, 0x91, 0xbb, 0x45, 0x79, 0xd7, 0x3f, 0xd0, 0xc8, 0xfe, 0xd1, 0x11,
        0xb0, 0x39, 0x13, 0x06, 0x82, 0x8a, 0xdf, 0xed, 0x52, 0x8f, 0x01, 0x81,
        0x21, 0xb3, 0xfe, 0xbd, 0xc3, 0x43, 0xe7, 0x97, 0xb8, 0x7d, 0xbb, 0x63,
        0xdb, 0x13, 0x33, 0xde, 0xd9, 0xd1, 0xec, 0xe1, 0x77, 0xcf, 0xa6, 0xb7,
        0x1f, 0xe8, 0xab, 0x1d, 0xa4, 0x66, 0x24, 0xed, 0x64, 0x15, 0xe5, 0x1c,
        0xcd, 0xe2, 0xc7, 0xca, 0x86, 0xe2, 0x83, 0x99, 0x0e, 0xea, 0xeb, 0x91,
        0x12, 0x04, 0x15, 0x52, 0x8b, 0x22, 0x95, 0x91, 0x02, 0x81, 0xb0, 0x2d,
        0xd4, 0x31, 0xf4, 0xc9, 0xf7, 0x04, 0x27, 0xdf
    };
    const byte test2EntropyA[] =
    {
        0x63, 0x36, 0x33, 0x77, 0xe4, 0x1e, 0x86, 0x46, 0x8d, 0xeb, 0x0a, 0xb4,
        0xa8, 0xed, 0x68, 0x3f, 0x6a, 0x13, 0x4e, 0x47, 0xe0, 0x14, 0xc7, 0x00,
        0x45, 0x4e, 0x81, 0xe9, 0x53, 0x58, 0xa5, 0x69, 0x80, 0x8a, 0xa3, 0x8f,
        0x2a, 0x72, 0xa6, 0x23, 0x59, 0x91, 0x5a, 0x9f, 0x8a, 0x04, 0xca, 0x68
    };
    const byte test2EntropyB[] =
    {
        0xe6, 0x2b, 0x8a, 0x8e, 0xe8, 0xf1, 0x41, 0xb6, 0x98, 0x05, 0x66, 0xe3,
        0xbf, 0xe3, 0xc0, 0x49, 0x03, 0xda, 0xd4, 0xac, 0x2c, 0xdf, 0x9f, 0x22,
        0x80, 0x01, 0x0a, 0x67, 0x39, 0xbc, 0x83, 0xd3
    };
    const byte test2Output[] =
    {
        0x04, 0xee, 0xc6, 0x3b, 0xb2, 0x31, 0xdf, 0x2c, 0x63, 0x0a, 0x1a, 0xfb,
        0xe7, 0x24, 0x94, 0x9d, 0x00, 0x5a, 0x58, 0x78, 0x51, 0xe1, 0xaa, 0x79,
        0x5e, 0x47, 0x73, 0x47, 0xc8, 0xb0, 0x56, 0x62, 0x1c, 0x18, 0xbd, 0xdc,
        0xdd, 0x8d, 0x99, 0xfc, 0x5f, 0xc2, 0xb9, 0x20, 0x53, 0xd8, 0xcf, 0xac,
        0xfb, 0x0b, 0xb8, 0x83, 0x12, 0x05, 0xfa, 0xd1, 0xdd, 0xd6, 0xc0, 0x71,
        0x31, 0x8a, 0x60, 0x18, 0xf0, 0x3b, 0x73, 0xf5, 0xed, 0xe4, 0xd4, 0xd0,
        0x71, 0xf9, 0xde, 0x03, 0xfd, 0x7a, 0xea, 0x10, 0x5d, 0x92, 0x99, 0xb8,
        0xaf, 0x99, 0xaa, 0x07, 0x5b, 0xdb, 0x4d, 0xb9, 0xaa, 0x28, 0xc1, 0x8d,
        0x17, 0x4b, 0x56, 0xee, 0x2a, 0x01, 0x4d, 0x09, 0x88, 0x96, 0xff, 0x22,
        0x82, 0xc9, 0x55, 0xa8, 0x19, 0x69, 0xe0, 0x69, 0xfa, 0x8c, 0xe0, 0x07,
        0xa1, 0x80, 0x18, 0x3a, 0x07, 0xdf, 0xae, 0x17
    };

    byte output[WC_SHA256_DIGEST_SIZE * 4];
    int ret;

    ret = wc_RNG_HealthTest(0, test1Entropy, sizeof(test1Entropy), NULL, 0,
                            output, sizeof(output));
    if (ret != 0)
        return -5000;

    if (XMEMCMP(test1Output, output, sizeof(output)) != 0)
        return -5001;

    ret = wc_RNG_HealthTest(1, test2EntropyA, sizeof(test2EntropyA),
                            test2EntropyB, sizeof(test2EntropyB),
                            output, sizeof(output));
    if (ret != 0)
        return -5002;

    if (XMEMCMP(test2Output, output, sizeof(output)) != 0)
        return -5003;

    /* Basic RNG generate block test */
    if (random_rng_test() != 0)
        return -5004;

    return 0;
}

#else

int random_test(void)
{
    /* Basic RNG generate block test */
    return random_rng_test();
}

#endif /* HAVE_HASHDRBG && !CUSTOM_RAND_GENERATE_BLOCK */
#endif /* WC_NO_RNG */


#ifdef WOLFSSL_STATIC_MEMORY
int memory_test(void)
{
    int ret = 0;
    unsigned int i;
    word32 size[] = { WOLFMEM_BUCKETS };
    word32 dist[] = { WOLFMEM_DIST };
    byte buffer[30000]; /* make large enough to involve many bucket sizes */
    int pad = -(int)((wolfssl_word)&(buffer[0])) & (WOLFSSL_STATIC_ALIGN - 1);
              /* pad to account for if head of buffer is not at set memory
               * alignment when tests are ran */

    /* check macro settings */
    if (sizeof(size)/sizeof(word32) != WOLFMEM_MAX_BUCKETS) {
        return -5100;
    }

    if (sizeof(dist)/sizeof(word32) != WOLFMEM_MAX_BUCKETS) {
        return -5101;
    }

    for (i = 0; i < WOLFMEM_MAX_BUCKETS; i++) {
        if ((size[i] % WOLFSSL_STATIC_ALIGN) != 0) {
            /* each element in array should be divisable by alignment size */
            return -5102;
        }
    }

    for (i = 1; i < WOLFMEM_MAX_BUCKETS; i++) {
        if (size[i - 1] >= size[i]) {
            return -5103; /* sizes should be in increasing order  */
        }
    }

    /* check that padding size returned is possible */
    if (wolfSSL_MemoryPaddingSz() < WOLFSSL_STATIC_ALIGN) {
        return -5104; /* no room for wc_Memory struct */
    }

    if (wolfSSL_MemoryPaddingSz() < 0) {
        return -5105;
    }

    if (wolfSSL_MemoryPaddingSz() % WOLFSSL_STATIC_ALIGN != 0) {
        return -5106; /* not aligned! */
    }

    /* check function to return optimum buffer size (rounded down) */
    ret = wolfSSL_StaticBufferSz(buffer, sizeof(buffer), WOLFMEM_GENERAL);
    if ((ret - pad) % WOLFSSL_STATIC_ALIGN != 0) {
        return -5107; /* not aligned! */
    }

    if (ret < 0) {
        return -5108;
    }

    if ((unsigned int)ret > sizeof(buffer)) {
        return -5109; /* did not round down as expected */
    }

    if (ret != wolfSSL_StaticBufferSz(buffer, ret, WOLFMEM_GENERAL)) {
        return -5110; /* retrun value changed when using suggested value */
    }

    ret = wolfSSL_MemoryPaddingSz();
    ret += pad; /* add space that is going to be needed if buffer not aligned */
    if (wolfSSL_StaticBufferSz(buffer, size[0] + ret + 1, WOLFMEM_GENERAL) !=
            (ret + (int)size[0])) {
        return -5111; /* did not round down to nearest bucket value */
    }

    ret = wolfSSL_StaticBufferSz(buffer, sizeof(buffer), WOLFMEM_IO_POOL);
    if ((ret - pad) < 0) {
        return -5112;
    }

    if (((ret - pad) % (WOLFMEM_IO_SZ + wolfSSL_MemoryPaddingSz())) != 0) {
        return -5113; /* not even chunks of memory for IO size */
    }

    if (((ret - pad) % WOLFSSL_STATIC_ALIGN) != 0) {
        return -5114; /* memory not aligned */
    }

    /* check for passing bad or unknown argments to functions */
    if (wolfSSL_StaticBufferSz(NULL, 1, WOLFMEM_GENERAL) > 0) {
        return -5115;
    }

    if (wolfSSL_StaticBufferSz(buffer, 1, WOLFMEM_GENERAL) != 0) {
        return -5116; /* should round to 0 since struct + bucket will not fit */
    }

    (void)dist; /* avoid static analysis warning of variable not used */
    return 0;
}
#endif /* WOLFSSL_STATIC_MEMORY */


#ifdef HAVE_NTRU

byte GetEntropy(ENTROPY_CMD cmd, byte* out);

byte GetEntropy(ENTROPY_CMD cmd, byte* out)
{
    static WC_RNG rng;

    if (cmd == INIT)
        return (wc_InitRng(&rng) == 0) ? 1 : 0;

    if (out == NULL)
        return 0;

    if (cmd == GET_BYTE_OF_ENTROPY)
        return (wc_RNG_GenerateBlock(&rng, out, 1) == 0) ? 1 : 0;

    if (cmd == GET_NUM_BYTES_PER_BYTE_OF_ENTROPY) {
        *out = 1;
        return 1;
    }

    return 0;
}

#endif /* HAVE_NTRU */


#ifndef NO_FILESYSTEM

/* Cert Paths */
#ifdef FREESCALE_MQX
    #define CERT_PREFIX "a:\\"
    #define CERT_PATH_SEP "\\"
#elif defined(WOLFSSL_MKD_SHELL)
    #define CERT_PREFIX ""
    #define CERT_PATH_SEP "/"
#elif defined(WOLFSSL_uTKERNEL2)
    #define CERT_PREFIX "/uda/"
    #define CERT_PATH_SEP "/"
#else
    #define CERT_PREFIX "./"
    #define CERT_PATH_SEP "/"
#endif
#define CERT_ROOT CERT_PREFIX "certs" CERT_PATH_SEP

/* Generated Test Certs */
#if !defined(USE_CERT_BUFFERS_1024) && !defined(USE_CERT_BUFFERS_2048) && \
        !defined(NO_ASN)
    #ifndef NO_RSA
        static const char* clientKey  = CERT_ROOT "client-key.der";
        static const char* clientCert = CERT_ROOT "client-cert.der";
        #ifdef WOLFSSL_CERT_EXT
            static const char* clientKeyPub  = CERT_ROOT "client-keyPub.der";
        #endif
        #ifdef WOLFSSL_CERT_GEN
            static const char* rsaCaKeyFile  = CERT_ROOT "ca-key.der";
            static const char* rsaCaCertFile = CERT_ROOT "ca-cert.pem";
        #endif
    #endif /* !NO_RSA */
    #ifndef NO_DH
        static const char* dhKey = CERT_ROOT "dh2048.der";
    #endif
    #ifndef NO_DSA
        static const char* dsaKey = CERT_ROOT "dsa2048.der";
    #endif
#endif /* !USE_CERT_BUFFER_* */
#if !defined(USE_CERT_BUFFERS_256) && !defined(NO_ASN)
    #ifdef HAVE_ECC
        /* cert files to be used in rsa cert gen test, check if RSA enabled */
        #ifdef HAVE_ECC_KEY_IMPORT
            static const char* eccKeyDerFile = CERT_ROOT "ecc-key.der";
        #endif
        #ifdef WOLFSSL_CERT_GEN
            static const char* eccKeyPubFile = CERT_ROOT "ecc-keyPub.der";
            static const char* eccCaKeyFile  = CERT_ROOT "ca-ecc-key.der";
            static const char* eccCaCertFile = CERT_ROOT "ca-ecc-cert.pem";
            #ifdef ENABLE_ECC384_CERT_GEN_TEST
                static const char* eccCaKey384File =
                                               CERT_ROOT "ca-ecc384-key.der";
                static const char* eccCaCert384File =
                                               CERT_ROOT "ca-ecc384-cert.pem";
            #endif
        #endif
        #if defined(HAVE_PKCS7) && defined(HAVE_ECC)
            static const char* eccClientKey  = CERT_ROOT "ecc-client-key.der";
            static const char* eccClientCert = CERT_ROOT "client-ecc-cert.der";
        #endif
    #endif /* HAVE_ECC */
    #ifdef HAVE_ED25519
        #ifdef WOLFSSL_TEST_CERT
            static const char* serverEd25519Cert =
                                         CERT_ROOT "ed25519/server-ed25519.der";
            static const char* caEd25519Cert     =
                                             CERT_ROOT "ed25519/ca-ed25519.der";
        #endif
    #endif
#endif /* !USE_CERT_BUFFER_* */

#ifndef NO_WRITE_TEMP_FILES
#ifdef HAVE_ECC
    #ifdef WOLFSSL_CERT_GEN
         static const char* certEccPemFile =   CERT_PREFIX "certecc.pem";
    #endif
    #if defined(WOLFSSL_CERT_GEN) && !defined(NO_RSA)
        static const char* certEccRsaPemFile = CERT_PREFIX "certeccrsa.pem";
        static const char* certEccRsaDerFile = CERT_PREFIX "certeccrsa.der";
    #endif
    #ifdef WOLFSSL_KEY_GEN
        static const char* eccCaKeyPemFile  = CERT_PREFIX "ecc-key.pem";
        static const char* eccPubKeyDerFile = CERT_PREFIX "ecc-public-key.der";
        static const char* eccCaKeyTempFile = CERT_PREFIX "ecc-key.der";
    #endif
    #if defined(WOLFSSL_CERT_GEN) || \
            (defined(WOLFSSL_CERT_EXT) && defined(WOLFSSL_TEST_CERT))
        static const char* certEccDerFile = CERT_PREFIX "certecc.der";
    #endif
#endif /* HAVE_ECC */

#ifndef NO_RSA
    #if defined(WOLFSSL_CERT_GEN) || defined(WOLFSSL_CERT_EXT)
        static const char* otherCertDerFile = CERT_PREFIX "othercert.der";
        static const char* certDerFile = CERT_PREFIX "cert.der";
    #endif
    #ifdef WOLFSSL_CERT_GEN
        static const char* otherCertPemFile = CERT_PREFIX "othercert.pem";
        static const char* certPemFile = CERT_PREFIX "cert.pem";
    #endif
    #ifdef WOLFSSL_CERT_REQ
        static const char* certReqDerFile = CERT_PREFIX "certreq.der";
        static const char* certReqPemFile = CERT_PREFIX "certreq.pem";
    #endif
#endif /* !NO_RSA */

#if !defined(NO_RSA) || !defined(NO_DSA)
    #ifdef WOLFSSL_KEY_GEN
        static const char* keyDerFile = CERT_PREFIX "key.der";
        static const char* keyPemFile = CERT_PREFIX "key.pem";
    #endif
#endif

#endif /* !NO_WRITE_TEMP_FILES */
#endif /* !NO_FILESYSTEM */


#ifdef WOLFSSL_CERT_GEN
static const CertName certDefaultName = {
    "US",               CTC_PRINTABLE,  /* country */
    "Orgeon",           CTC_UTF8,       /* state */
    "Portland",         CTC_UTF8,       /* locality */
    "Test",             CTC_UTF8,       /* sur */
    "wolfSSL",          CTC_UTF8,       /* org */
    "Development",      CTC_UTF8,       /* unit */
    "www.wolfssl.com",  CTC_UTF8,       /* commonName */
    "info@wolfssl.com"                  /* email */
};

#ifdef WOLFSSL_CERT_EXT
    static const char certKeyUsage[] =
        "digitalSignature,nonRepudiation";
    #if defined(WOLFSSL_CERT_REQ) || defined(HAVE_NTRU)
        static const char certKeyUsage2[] =
        "digitalSignature,nonRepudiation,keyEncipherment,keyAgreement";
    #endif
#endif /* WOLFSSL_CERT_EXT */
#endif /* WOLFSSL_CERT_GEN */

#ifndef NO_RSA

#if !defined(NO_ASN_TIME) && defined(WOLFSSL_TEST_CERT)
int cert_test(void)
{
    DecodedCert cert;
    byte*       tmp;
    size_t      bytes;
    FILE        *file;
    int         ret;

    tmp = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (tmp == NULL)
        return -5200;

    /* Certificate with Name Constraints extension. */
#ifdef FREESCALE_MQX
    file = fopen(".\\certs\\test\\cert-ext-nc.der", "rb");
#else
    file = fopen("./certs/test/cert-ext-nc.der", "rb");
#endif
    if (!file) {
        ERROR_OUT(-5201, done);
    }
    bytes = fread(tmp, 1, FOURK_BUF, file);
    fclose(file);
    InitDecodedCert(&cert, tmp, (word32)bytes, 0);
    ret = ParseCert(&cert, CERT_TYPE, NO_VERIFY, NULL);
    if (ret != 0) {
        ERROR_OUT(-5202, done);
    }
    FreeDecodedCert(&cert);

    /* Certificate with Inhibit Any Policy extension. */
#ifdef FREESCALE_MQX
    file = fopen(".\\certs\\test\\cert-ext-ia.der", "rb");
#else
    file = fopen("./certs/test/cert-ext-ia.der", "rb");
#endif
    if (!file) {
        ERROR_OUT(-5203, done);
    }
    bytes = fread(tmp, 1, FOURK_BUF, file);
    fclose(file);
    InitDecodedCert(&cert, tmp, (word32)bytes, 0);
    ret = ParseCert(&cert, CERT_TYPE, NO_VERIFY, NULL);
    if (ret != 0) {
        ERROR_OUT(-5204, done);
    }

done:
    FreeDecodedCert(&cert);
    XFREE(tmp, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

    return ret;
}
#endif

#if defined(WOLFSSL_CERT_EXT) && defined(WOLFSSL_TEST_CERT)
int certext_test(void)
{
    DecodedCert cert;
    byte*       tmp;
    size_t      bytes;
    FILE        *file;
    int         ret;

    /* created from rsa_test : othercert.der */
    byte skid_rsa[]   = "\x33\xD8\x45\x66\xD7\x68\x87\x18\x7E\x54"
                        "\x0D\x70\x27\x91\xC7\x26\xD7\x85\x65\xC0";

    /* created from rsa_test : othercert.der */
    byte akid_rsa[] = "\x27\x8E\x67\x11\x74\xC3\x26\x1D\x3F\xED"
                      "\x33\x63\xB3\xA4\xD8\x1D\x30\xE5\xE8\xD5";

#ifdef HAVE_ECC
   /* created from ecc_test_cert_gen : certecc.der */
#ifdef ENABLE_ECC384_CERT_GEN_TEST
    /* Authority key id from ./certs/ca-ecc384-cert.pem */
    byte akid_ecc[] = "\xAB\xE0\xC3\x26\x4C\x18\xD4\x72\xBB\xD2"
                      "\x84\x8C\x9C\x0A\x05\x92\x80\x12\x53\x52";
#else
    /* Authority key id from ./certs/ca-ecc-cert.pem */
    byte akid_ecc[] = "\x56\x8E\x9A\xC3\xF0\x42\xDE\x18\xB9\x45"
                      "\x55\x6E\xF9\x93\xCF\xEA\xC3\xF3\xA5\x21";
#endif
#endif /* HAVE_ECC */

    /* created from rsa_test : cert.der */
    byte kid_ca[] = "\x33\xD8\x45\x66\xD7\x68\x87\x18\x7E\x54"
                    "\x0D\x70\x27\x91\xC7\x26\xD7\x85\x65\xC0";

    tmp = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (tmp == NULL)
        return -5300;

    /* load othercert.der (Cert signed by an authority) */
    file = fopen(otherCertDerFile, "rb");
    if (!file) {
        XFREE(tmp, HEAP_HINT ,DYNAMIC_TYPE_TMP_BUFFER);
        return -5301;
    }

    bytes = fread(tmp, 1, FOURK_BUF, file);
    fclose(file);

    InitDecodedCert(&cert, tmp, (word32)bytes, 0);

    ret = ParseCert(&cert, CERT_TYPE, NO_VERIFY, 0);
    if (ret != 0)
        return -5302;

    /* check the SKID from a RSA certificate */
    if (XMEMCMP(skid_rsa, cert.extSubjKeyId, sizeof(cert.extSubjKeyId)))
        return -5303;

    /* check the AKID from an RSA certificate */
    if (XMEMCMP(akid_rsa, cert.extAuthKeyId, sizeof(cert.extAuthKeyId)))
        return -5304;

    /* check the Key Usage from an RSA certificate */
    if (!cert.extKeyUsageSet)
        return -5305;

    if (cert.extKeyUsage != (KEYUSE_KEY_ENCIPHER|KEYUSE_KEY_AGREE))
        return -5306;

    /* check the CA Basic Constraints from an RSA certificate */
    if (cert.isCA)
        return -5307;

#ifndef WOLFSSL_SEP /* test only if not using SEP policies */
    /* check the Certificate Policies Id */
    if (cert.extCertPoliciesNb != 1)
        return -5308;

    if (strncmp(cert.extCertPolicies[0], "2.16.840.1.101.3.4.1.42", 23))
        return -5309;
#endif

    FreeDecodedCert(&cert);

#ifdef HAVE_ECC
    /* load certecc.der (Cert signed by our ECC CA test in ecc_test_cert_gen) */
    file = fopen(certEccDerFile, "rb");
    if (!file) {
        XFREE(tmp, HEAP_HINT ,DYNAMIC_TYPE_TMP_BUFFER);
        return -5310;
    }

    bytes = fread(tmp, 1, FOURK_BUF, file);
    fclose(file);

    InitDecodedCert(&cert, tmp, (word32)bytes, 0);

    ret = ParseCert(&cert, CERT_TYPE, NO_VERIFY, 0);
    if (ret != 0)
        return -5311;

    /* check the SKID from a ECC certificate - generated dynamically */

    /* check the AKID from an ECC certificate */
    if (XMEMCMP(akid_ecc, cert.extAuthKeyId, sizeof(cert.extAuthKeyId)))
        return -5313;

    /* check the Key Usage from an ECC certificate */
    if (!cert.extKeyUsageSet)
        return -5314;

    if (cert.extKeyUsage != (KEYUSE_DIGITAL_SIG|KEYUSE_CONTENT_COMMIT))
        return -5315;

    /* check the CA Basic Constraints from an ECC certificate */
    if (cert.isCA)
        return -5316;

#ifndef WOLFSSL_SEP /* test only if not using SEP policies */
    /* check the Certificate Policies Id */
    if (cert.extCertPoliciesNb != 2)
        return -5317;

    if (strncmp(cert.extCertPolicies[0], "2.4.589440.587.101.2.1.9632587.1", 32))
        return -5318;

    if (strncmp(cert.extCertPolicies[1], "1.2.13025.489.1.113549", 22))
        return -5319;
#endif

    FreeDecodedCert(&cert);
#endif /* HAVE_ECC */

    /* load cert.der (self signed certificate) */
    file = fopen(certDerFile, "rb");
    if (!file) {
        XFREE(tmp, HEAP_HINT ,DYNAMIC_TYPE_TMP_BUFFER);
        return -5320;
    }

    bytes = fread(tmp, 1, FOURK_BUF, file);
    fclose(file);

    InitDecodedCert(&cert, tmp, (word32)bytes, 0);

    ret = ParseCert(&cert, CERT_TYPE, NO_VERIFY, 0);
    if (ret != 0)
        return -5321;

    /* check the SKID from a CA certificate */
    if (XMEMCMP(kid_ca, cert.extSubjKeyId, sizeof(cert.extSubjKeyId)))
        return -5322;

    /* check the AKID from an CA certificate */
    if (XMEMCMP(kid_ca, cert.extAuthKeyId, sizeof(cert.extAuthKeyId)))
        return -5323;

    /* check the Key Usage from CA certificate */
    if (!cert.extKeyUsageSet)
        return -5324;

    if (cert.extKeyUsage != (KEYUSE_KEY_CERT_SIGN|KEYUSE_CRL_SIGN))
        return -5325;

    /* check the CA Basic Constraints CA certificate */
    if (!cert.isCA)
        return -5326;

#ifndef WOLFSSL_SEP /* test only if not using SEP policies */
    /* check the Certificate Policies Id */
    if (cert.extCertPoliciesNb != 2)
        return -5327;

    if (strncmp(cert.extCertPolicies[0], "2.16.840.1.101.3.4.1.42", 23))
        return -5328;

    if (strncmp(cert.extCertPolicies[1], "1.2.840.113549.1.9.16.6.5", 25))
        return -5329;
#endif

    FreeDecodedCert(&cert);
    XFREE(tmp, HEAP_HINT ,DYNAMIC_TYPE_TMP_BUFFER);

    return 0;
}
#endif /* WOLFSSL_CERT_EXT && WOLFSSL_TEST_CERT */

static int rsa_flatten_test(RsaKey* key)
{
    int    ret;
    byte   e[256];
    byte   n[256];
    word32 eSz = sizeof(e);
    word32 nSz = sizeof(n);

    /* Parameter Validation testing. */
    ret = wc_RsaFlattenPublicKey(NULL, e, &eSz, n, &nSz);
#ifdef HAVE_USER_RSA
    /* Implementation using IPP Libraries returns:
     *     -101 = USER_CRYPTO_ERROR
     */
    if (ret == 0)
#else
    if (ret != BAD_FUNC_ARG)
#endif
        return -5330;
    ret = wc_RsaFlattenPublicKey(key, NULL, &eSz, n, &nSz);
#ifdef HAVE_USER_RSA
    /* Implementation using IPP Libraries returns:
     *     -101 = USER_CRYPTO_ERROR
     */
    if (ret == 0)
#else
    if (ret != BAD_FUNC_ARG)
#endif
        return -5331;
    ret = wc_RsaFlattenPublicKey(key, e, NULL, n, &nSz);
#ifdef HAVE_USER_RSA
    /* Implementation using IPP Libraries returns:
     *     -101 = USER_CRYPTO_ERROR
     */
    if (ret == 0)
#else
    if (ret != BAD_FUNC_ARG)
#endif
        return -5332;
    ret = wc_RsaFlattenPublicKey(key, e, &eSz, NULL, &nSz);
#ifdef HAVE_USER_RSA
    /* Implementation using IPP Libraries returns:
     *     -101 = USER_CRYPTO_ERROR
     */
    if (ret == 0)
#else
    if (ret != BAD_FUNC_ARG)
#endif
        return -5333;
    ret = wc_RsaFlattenPublicKey(key, e, &eSz, n, NULL);
#ifdef HAVE_USER_RSA
    /* Implementation using IPP Libraries returns:
     *     -101 = USER_CRYPTO_ERROR
     */
    if (ret == 0)
#else
    if (ret != BAD_FUNC_ARG)
#endif
        return -5334;
    ret = wc_RsaFlattenPublicKey(key, e, &eSz, n, &nSz);
    if (ret != 0)
        return -5335;
    eSz = 0;
    ret = wc_RsaFlattenPublicKey(key, e, &eSz, n, &nSz);
#ifdef HAVE_USER_RSA
    /* Implementation using IPP Libraries returns:
     *     -101 = USER_CRYPTO_ERROR
     */
    if (ret == 0)
#elif defined(HAVE_FIPS)
    if (ret != 0)
#else
    if (ret != RSA_BUFFER_E)
#endif
        return -5336;
    eSz = sizeof(e);
    nSz = 0;
    ret = wc_RsaFlattenPublicKey(key, e, &eSz, n, &nSz);
#ifdef HAVE_USER_RSA
    /* Implementation using IPP Libraries returns:
     *     -101 = USER_CRYPTO_ERROR
     */
    if (ret == 0)
#else
    if (ret != RSA_BUFFER_E)
#endif
        return -5337;

    return 0;
}

#ifndef NO_SIG_WRAPPER
static int rsa_sig_test(RsaKey* key, word32 keyLen, int modLen, WC_RNG* rng)
{
    int ret;
    word32 sigSz;
    byte   in[] = "Everyone gets Friday off.";
    word32 inLen = (word32)XSTRLEN((char*)in);
    byte   out[256];

    /* Parameter Validation testing. */
    ret = wc_SignatureGetSize(WC_SIGNATURE_TYPE_NONE, key, keyLen);
    if (ret != BAD_FUNC_ARG)
        return -5338;
    ret = wc_SignatureGetSize(WC_SIGNATURE_TYPE_RSA, key, 0);
    if (ret != BAD_FUNC_ARG)
        return -5339;

    sigSz = (word32)modLen;
    ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, NULL,
                               inLen, out, &sigSz, key, keyLen, rng);
    if (ret != BAD_FUNC_ARG)
        return -5340;
    ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                               0, out, &sigSz, key, keyLen, rng);
    if (ret != BAD_FUNC_ARG)
        return -5341;
    ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                               inLen, NULL, &sigSz, key, keyLen, rng);
    if (ret != BAD_FUNC_ARG)
        return -5342;
    ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                               inLen, out, NULL, key, keyLen, rng);
    if (ret != BAD_FUNC_ARG)
        return -5343;
    ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                               inLen, out, &sigSz, NULL, keyLen, rng);
    if (ret != BAD_FUNC_ARG)
        return -5344;
    ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                               inLen, out, &sigSz, key, 0, rng);
    if (ret != BAD_FUNC_ARG)
        return -5345;
    ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                               inLen, out, &sigSz, key, keyLen, NULL);
#ifdef HAVE_USER_RSA
    /* Implementation using IPP Libraries returns:
     *     -101 = USER_CRYPTO_ERROR
     */
    if (ret == 0)
#elif defined(WOLFSSL_ASYNC_CRYPT)
    /* async may not require RNG */
    if (ret != 0 && ret != MISSING_RNG_E)
#elif defined(HAVE_FIPS) || defined(WOLFSSL_ASYNC_CRYPT) || \
     !defined(WC_RSA_BLINDING) || defined(WOLFSSL_XILINX_CRYPT)
    /* FIPS140 implementation does not do blinding */
    if (ret != 0)
#else
    if (ret != MISSING_RNG_E)
#endif
        return -5346;
    sigSz = 0;
    ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                               inLen, out, &sigSz, key, keyLen, rng);
    if (ret != BAD_FUNC_ARG)
        return -5347;

    ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, NULL,
                             inLen, out, (word32)modLen, key, keyLen);
    if (ret != BAD_FUNC_ARG)
        return -5348;
    ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                             0, out, (word32)modLen, key, keyLen);
    if (ret != BAD_FUNC_ARG)
        return -5349;
    ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                             inLen, NULL, (word32)modLen, key, keyLen);
    if (ret != BAD_FUNC_ARG)
        return -5350;
    ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                             inLen, out, 0, key, keyLen);
    if (ret != BAD_FUNC_ARG)
        return -5351;
    ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                             inLen, out, (word32)modLen, NULL, keyLen);
    if (ret != BAD_FUNC_ARG)
        return -5352;
    ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                             inLen, out, (word32)modLen, key, 0);
    if (ret != BAD_FUNC_ARG)
        return -5353;

#ifndef HAVE_ECC
    ret = wc_SignatureGetSize(WC_SIGNATURE_TYPE_ECC, key, keyLen);
    if (ret != SIG_TYPE_E)
        return -5354;
#endif

    /* Use APIs. */
    ret = wc_SignatureGetSize(WC_SIGNATURE_TYPE_RSA, key, keyLen);
    if (ret != modLen)
        return -5355;
    ret = wc_SignatureGetSize(WC_SIGNATURE_TYPE_RSA_W_ENC, key, keyLen);
    if (ret != modLen)
        return -5356;

    sigSz = (word32)ret;
    ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                               inLen, out, &sigSz, key, keyLen, rng);
    if (ret != 0)
        return -5357;

    ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                             inLen, out, (word32)modLen, key, keyLen);
    if (ret != 0)
        return -5358;

    sigSz = sizeof(out);
    ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA_W_ENC,
                               in, inLen, out, &sigSz, key, keyLen, rng);
    if (ret != 0)
        return -5359;

    ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA_W_ENC,
                             in, inLen, out, (word32)modLen, key, keyLen);
    if (ret != 0)
        return -5360;

    /* Wrong signature type. */
    ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA, in,
                             inLen, out, (word32)modLen, key, keyLen);
    if (ret == 0)
        return -5361;

    return 0;
}
#endif /* !NO_SIG_WRAPPER */

#ifndef HAVE_USER_RSA
static int rsa_decode_test(void)
{
    int        ret;
    word32     inSz;
    word32     inOutIdx;
    RsaKey     keyPub;
    const byte n[2] = { 0x00, 0x23 };
    const byte e[2] = { 0x00, 0x03 };
    const byte good[] = { 0x30, 0x06, 0x02, 0x01, 0x23, 0x02, 0x1, 0x03 };
    const byte goodAlgId[] = { 0x30, 0x0f, 0x30, 0x0d, 0x06, 0x00,
            0x03, 0x09, 0x00, 0x30, 0x06, 0x02, 0x01, 0x23, 0x02, 0x1, 0x03 };
    const byte goodAlgIdNull[] = { 0x30, 0x11, 0x30, 0x0f, 0x06, 0x00,
            0x05, 0x00, 0x03, 0x09, 0x00, 0x30, 0x06, 0x02, 0x01, 0x23,
            0x02, 0x1, 0x03 };
    const byte badAlgIdNull[] = { 0x30, 0x12, 0x30, 0x10, 0x06, 0x00,
            0x05, 0x01, 0x00, 0x03, 0x09, 0x00, 0x30, 0x06, 0x02, 0x01, 0x23,
            0x02, 0x1, 0x03 };
    const byte badNotBitString[] = { 0x30, 0x0f, 0x30, 0x0d, 0x06, 0x00,
            0x04, 0x09, 0x00, 0x30, 0x06, 0x02, 0x01, 0x23, 0x02, 0x1, 0x03 };
    const byte badBitStringLen[] = { 0x30, 0x0f, 0x30, 0x0d, 0x06, 0x00,
            0x03, 0x0a, 0x00, 0x30, 0x06, 0x02, 0x01, 0x23, 0x02, 0x1, 0x03 };
    const byte badNoSeq[] = { 0x30, 0x0d, 0x30, 0x0b, 0x06, 0x00, 0x03, 0x07,
            0x00, 0x02, 0x01, 0x23, 0x02, 0x1, 0x03 };
    const byte badNoObj[] = {
            0x30, 0x0f, 0x30, 0x0d, 0x05, 0x00, 0x03, 0x09, 0x00, 0x30, 0x06,
            0x02, 0x01, 0x23, 0x02, 0x1, 0x03 };
    const byte badIntN[] = { 0x30, 0x06, 0x02, 0x05, 0x23, 0x02, 0x1, 0x03 };
    const byte badNotIntE[] = { 0x30, 0x06, 0x02, 0x01, 0x23, 0x04, 0x1, 0x03 };
    const byte badLength[] = { 0x30, 0x04, 0x02, 0x01, 0x23, 0x02, 0x1, 0x03 };
    const byte badBitStrNoZero[] = { 0x30, 0x0e, 0x30, 0x0c, 0x06, 0x00,
            0x03, 0x08, 0x30, 0x06, 0x02, 0x01, 0x23, 0x02, 0x1, 0x03 };

    ret = wc_InitRsaKey(&keyPub, NULL);
    if (ret != 0)
        return -5400;

    /* Parameter Validation testing. */
    ret = wc_RsaPublicKeyDecodeRaw(NULL, sizeof(n), e, sizeof(e), &keyPub);
    if (ret != BAD_FUNC_ARG) {
        ret = -5401;
        goto done;
    }
    ret = wc_RsaPublicKeyDecodeRaw(n, sizeof(n), NULL, sizeof(e), &keyPub);
    if (ret != BAD_FUNC_ARG) {
        ret = -5402;
        goto done;
    }
    ret = wc_RsaPublicKeyDecodeRaw(n, sizeof(n), e, sizeof(e), NULL);
    if (ret != BAD_FUNC_ARG) {
        ret = -5403;
        goto done;
    }
    /* TODO: probably should fail when length is -1! */
    ret = wc_RsaPublicKeyDecodeRaw(n, (word32)-1, e, sizeof(e), &keyPub);
#ifdef WOLFSSL_XILINX_CRYPT
    if (ret == 0) {
#else
    if (ret != 0) {
#endif
        ret = -5404;
        goto done;
    }
    wc_FreeRsaKey(&keyPub);
    ret = wc_InitRsaKey(&keyPub, NULL);
    if (ret != 0)
        return -5405;

#ifndef WOLFSSL_XILINX_CRYPT
    ret = wc_RsaPublicKeyDecodeRaw(n, sizeof(n), e, (word32)-1, &keyPub);
    if (ret != 0) {
        ret = -5406;
        goto done;
    }
    wc_FreeRsaKey(&keyPub);
    ret = wc_InitRsaKey(&keyPub, NULL);
    if (ret != 0)
        return -5407;
#endif

    /* Use API. */
    ret = wc_RsaPublicKeyDecodeRaw(n, sizeof(n), e, sizeof(e), &keyPub);
    if (ret != 0) {
        ret = -5408;
        goto done;
    }
    wc_FreeRsaKey(&keyPub);
    ret = wc_InitRsaKey(&keyPub, NULL);
    if (ret != 0)
        return -5409;

    /* Parameter Validation testing. */
    inSz = sizeof(good);
    ret = wc_RsaPublicKeyDecode(NULL, &inOutIdx, &keyPub, inSz);
    if (ret != BAD_FUNC_ARG) {
        ret = -5410;
        goto done;
    }
    ret = wc_RsaPublicKeyDecode(good, NULL, &keyPub, inSz);
    if (ret != BAD_FUNC_ARG) {
        ret = -5411;
        goto done;
    }
    ret = wc_RsaPublicKeyDecode(good, &inOutIdx, NULL, inSz);
    if (ret != BAD_FUNC_ARG) {
        ret = -5412;
        goto done;
    }

    /* Use good data and offest to bad data. */
    inOutIdx = 2;
    inSz = sizeof(good) - inOutIdx;
    ret = wc_RsaPublicKeyDecode(good, &inOutIdx, &keyPub, inSz);
    if (ret != ASN_PARSE_E) {
        ret = -5413;
        goto done;
    }
    inOutIdx = 2;
    inSz = sizeof(goodAlgId) - inOutIdx;
    ret = wc_RsaPublicKeyDecode(goodAlgId, &inOutIdx, &keyPub, inSz);
    if (ret != ASN_PARSE_E) {
        ret = -5414;
        goto done;
    }
    /* Try different bad data. */
    inSz = sizeof(badAlgIdNull);
    inOutIdx = 0;
    ret = wc_RsaPublicKeyDecode(badAlgIdNull, &inOutIdx, &keyPub, inSz);
    if (ret != ASN_EXPECT_0_E) {
        ret = -5415;
        goto done;
    }
    inSz = sizeof(badNotBitString);
    inOutIdx = 0;
    ret = wc_RsaPublicKeyDecode(badNotBitString, &inOutIdx, &keyPub, inSz);
    if (ret != ASN_BITSTR_E) {
        ret = -5416;
        goto done;
    }
    inSz = sizeof(badBitStringLen);
    inOutIdx = 0;
    ret = wc_RsaPublicKeyDecode(badBitStringLen, &inOutIdx, &keyPub, inSz);
    if (ret != ASN_PARSE_E) {
        ret = -5417;
        goto done;
    }
    inSz = sizeof(badNoSeq);
    inOutIdx = 0;
    ret = wc_RsaPublicKeyDecode(badNoSeq, &inOutIdx, &keyPub, inSz);
    if (ret != ASN_PARSE_E) {
        ret = -5418;
        goto done;
    }
    inSz = sizeof(badNoObj);
    inOutIdx = 0;
    ret = wc_RsaPublicKeyDecode(badNoObj, &inOutIdx, &keyPub, inSz);
    if (ret != ASN_PARSE_E) {
        ret = -5419;
        goto done;
    }
    inSz = sizeof(badIntN);
    inOutIdx = 0;
    ret = wc_RsaPublicKeyDecode(badIntN, &inOutIdx, &keyPub, inSz);
    if (ret != ASN_RSA_KEY_E) {
        ret = -5420;
        goto done;
    }
    inSz = sizeof(badNotIntE);
    inOutIdx = 0;
    ret = wc_RsaPublicKeyDecode(badNotIntE, &inOutIdx, &keyPub, inSz);
    if (ret != ASN_RSA_KEY_E) {
        ret = -5421;
        goto done;
    }
    /* TODO: Shouldn't pass as the sequence length is too small. */
    inSz = sizeof(badLength);
    inOutIdx = 0;
    ret = wc_RsaPublicKeyDecode(badLength, &inOutIdx, &keyPub, inSz);
    if (ret != 0) {
        ret = -5422;
        goto done;
    }
    /* TODO: Shouldn't ignore object id's data. */
    wc_FreeRsaKey(&keyPub);
    ret = wc_InitRsaKey(&keyPub, NULL);
    if (ret != 0)
        return -5423;

    /* Valid data cases. */
    inSz = sizeof(good);
    inOutIdx = 0;
    ret = wc_RsaPublicKeyDecode(good, &inOutIdx, &keyPub, inSz);
    if (ret != 0) {
        ret = -5424;
        goto done;
    }
    if (inOutIdx != inSz) {
        ret = -5425;
        goto done;
    }
    wc_FreeRsaKey(&keyPub);
    ret = wc_InitRsaKey(&keyPub, NULL);
    if (ret != 0)
        return -5426;

    inSz = sizeof(goodAlgId);
    inOutIdx = 0;
    ret = wc_RsaPublicKeyDecode(goodAlgId, &inOutIdx, &keyPub, inSz);
    if (ret != 0) {
        ret = -5427;
        goto done;
    }
    if (inOutIdx != inSz) {
        ret = -5428;
        goto done;
    }
    wc_FreeRsaKey(&keyPub);
    ret = wc_InitRsaKey(&keyPub, NULL);
    if (ret != 0)
        return -5429;

    inSz = sizeof(goodAlgIdNull);
    inOutIdx = 0;
    ret = wc_RsaPublicKeyDecode(goodAlgIdNull, &inOutIdx, &keyPub, inSz);
    if (ret != 0) {
        ret = -5430;
        goto done;
    }
    if (inOutIdx != inSz) {
        ret = -5431;
        goto done;
    }
    wc_FreeRsaKey(&keyPub);
    ret = wc_InitRsaKey(&keyPub, NULL);
    if (ret != 0)
        return -5432;

    inSz = sizeof(badBitStrNoZero);
    inOutIdx = 0;
    ret = wc_RsaPublicKeyDecode(badBitStrNoZero, &inOutIdx, &keyPub, inSz);
    if (ret != ASN_EXPECT_0_E) {
        ret = -5433;
        goto done;
    }
    ret = 0;

done:
    wc_FreeRsaKey(&keyPub);
    return ret;
}
#endif

#if !defined(USE_FAST_MATH) || \
        (defined(USE_FAST_MATH) && (FP_MAX_BITS >= 8192)) \
        && !defined(NO_ASN)
static unsigned char key_4k[] =  {
    0x30, 0x82, 0x09, 0x28, 0x02, 0x01, 0x00, 0x02,
    0x82, 0x02, 0x01, 0x00, 0xBD, 0x8D, 0x2B, 0x48,
    0xA4, 0x51, 0xFA, 0x89, 0x4F, 0x84, 0x6B, 0x6F,
    0x49, 0xD0, 0xBC, 0x21, 0xD4, 0xE1, 0x60, 0x9C,
    0x5F, 0x4B, 0x0C, 0x9C, 0x6A, 0xA7, 0x79, 0x64,
    0x54, 0x91, 0x95, 0x02, 0x3D, 0x5A, 0x03, 0xDD,
    0xCE, 0xAB, 0x6B, 0xE0, 0x8B, 0x00, 0x02, 0x61,
    0xDE, 0xD8, 0x8B, 0x8E, 0x3C, 0x97, 0xA8, 0x8F,
    0x05, 0x6A, 0xCF, 0x69, 0x77, 0xC9, 0x38, 0x4E,
    0xCF, 0x89, 0xDF, 0x90, 0x84, 0xDD, 0x7D, 0xD8,
    0xDA, 0xB7, 0xD4, 0xA1, 0x1B, 0xDA, 0x55, 0x2B,
    0x43, 0xC3, 0xAE, 0x67, 0xC6, 0x84, 0x25, 0x3E,
    0x75, 0xA1, 0x8A, 0x18, 0x16, 0xD7, 0x0B, 0x2B,
    0x21, 0x22, 0xEA, 0x66, 0xCD, 0x50, 0x3B, 0x60,
    0x4B, 0x4D, 0x4A, 0xE8, 0x9A, 0x20, 0x44, 0x78,
    0xA5, 0x7B, 0x46, 0x43, 0xD2, 0x7A, 0x00, 0x2A,
    0xAF, 0xB7, 0xE0, 0x57, 0x02, 0xD8, 0xDE, 0x5F,
    0x23, 0x1D, 0xC8, 0xB3, 0x7C, 0xC9, 0xB2, 0xA0,
    0xEE, 0xCD, 0xD4, 0xE7, 0x18, 0x29, 0xEF, 0x82,
    0xEF, 0x18, 0x5C, 0x63, 0x28, 0xC4, 0x3C, 0x3E,
    0x56, 0xBE, 0x06, 0x75, 0x82, 0x25, 0x0A, 0x64,
    0x2B, 0x37, 0x5A, 0xFC, 0x36, 0x0B, 0x9D, 0xAA,
    0x6B, 0xBC, 0x57, 0x39, 0x08, 0xF2, 0xDE, 0x83,
    0x4C, 0x6E, 0x62, 0x27, 0x19, 0x38, 0x56, 0x6E,
    0x44, 0xE0, 0x13, 0x76, 0x07, 0x52, 0xE5, 0xA7,
    0x2F, 0x05, 0xFF, 0xBB, 0xAC, 0x41, 0x2B, 0x1C,
    0xF0, 0x44, 0x68, 0xFE, 0xAF, 0x26, 0xD7, 0xC3,
    0x94, 0xE4, 0x3C, 0x26, 0xC3, 0xF1, 0x66, 0xEE,
    0x7C, 0xEC, 0x37, 0x58, 0x4F, 0x56, 0xF7, 0x31,
    0x7F, 0x42, 0x8F, 0x1C, 0xE7, 0xCE, 0x11, 0x03,
    0xFD, 0xC6, 0xAF, 0x04, 0x81, 0xDF, 0x07, 0x83,
    0xC9, 0xFB, 0xA8, 0xCC, 0x87, 0xFF, 0xAF, 0xF0,
    0x3A, 0x0A, 0x3A, 0xF9, 0x30, 0x8B, 0xC3, 0x76,
    0xEB, 0xE4, 0x01, 0x01, 0x11, 0x26, 0xD8, 0xA8,
    0x70, 0xE1, 0x2E, 0x32, 0x26, 0x69, 0xB8, 0xF2,
    0x0C, 0x3D, 0x7F, 0x3A, 0xDF, 0x23, 0x95, 0x5A,
    0xAF, 0xFC, 0x8E, 0x61, 0x6E, 0xDB, 0xC1, 0x08,
    0x37, 0x24, 0x86, 0xAD, 0xC5, 0xDE, 0xFE, 0xE4,
    0xFA, 0x6F, 0x9A, 0x55, 0x68, 0x46, 0x29, 0xF0,
    0x01, 0xBD, 0x1E, 0x26, 0x18, 0xDE, 0xD1, 0xA6,
    0xE9, 0x27, 0x95, 0xAC, 0xF9, 0xEA, 0x6D, 0x4B,
    0xA2, 0x04, 0xEB, 0xF2, 0x6C, 0x44, 0xD3, 0x40,
    0xED, 0xC5, 0x50, 0x55, 0x64, 0xAC, 0x7E, 0xAE,
    0x51, 0x74, 0xCE, 0x33, 0xDB, 0xF6, 0x9F, 0xC8,
    0xEE, 0x94, 0x34, 0x05, 0x9F, 0xB3, 0x1C, 0xD7,
    0x62, 0xA5, 0x76, 0x52, 0x37, 0x30, 0xDA, 0x6B,
    0xAD, 0x7E, 0xA0, 0x1C, 0xC7, 0x11, 0xA0, 0x23,
    0x0C, 0xA6, 0x37, 0x17, 0x66, 0xC8, 0xF4, 0x94,
    0xA5, 0x2A, 0xF0, 0x6B, 0x8A, 0xF9, 0xE7, 0x40,
    0x6C, 0x90, 0x6C, 0x07, 0xB0, 0xCE, 0xD7, 0x15,
    0x05, 0x05, 0xE7, 0xC5, 0x41, 0x31, 0x76, 0x7A,
    0x11, 0xE0, 0x74, 0x8E, 0xD9, 0xF1, 0xE4, 0x5D,
    0xA3, 0x5F, 0xBF, 0x0F, 0x1D, 0x56, 0x06, 0xB2,
    0x0A, 0x32, 0x71, 0x79, 0xAE, 0x32, 0x02, 0x3A,
    0x67, 0xCC, 0x5A, 0x85, 0x2B, 0x24, 0xB4, 0xDB,
    0x38, 0xD7, 0xD5, 0x75, 0x70, 0xEC, 0x46, 0x2F,
    0x25, 0xD5, 0xF2, 0x54, 0x25, 0xD6, 0xE3, 0xFE,
    0x45, 0x18, 0xF2, 0x45, 0xC2, 0xB0, 0x7B, 0x1F,
    0x8C, 0x58, 0x11, 0x78, 0x80, 0x82, 0xEE, 0xC1,
    0x31, 0x79, 0xB3, 0xD6, 0x93, 0xAD, 0xD8, 0xCB,
    0x8F, 0xA1, 0xC6, 0x56, 0xBC, 0x22, 0x79, 0xAD,
    0x8A, 0x31, 0xF8, 0x72, 0xAC, 0xA9, 0xD8, 0xE8,
    0xD9, 0x11, 0x2D, 0xC3, 0x47, 0x91, 0xF8, 0x4F,
    0x08, 0x30, 0xD5, 0xFE, 0xC8, 0xFB, 0x92, 0x8F,
    0xBF, 0x20, 0x78, 0x85, 0x96, 0x84, 0x22, 0x22,
    0x9D, 0xA3, 0x8B, 0xDB, 0x02, 0x03, 0x01, 0x00,
    0x01, 0x02, 0x82, 0x02, 0x00, 0x7D, 0x3C, 0x5C,
    0x30, 0xAE, 0x80, 0x8B, 0x6B, 0x55, 0x62, 0x44,
    0x29, 0x4F, 0x37, 0xDE, 0x54, 0xEB, 0xF5, 0xCC,
    0x83, 0x04, 0x62, 0xED, 0x7E, 0x1A, 0xE9, 0x73,
    0xC6, 0xDA, 0x29, 0x25, 0xAE, 0x49, 0x22, 0xC4,
    0x13, 0x34, 0x8D, 0x05, 0xC5, 0x61, 0x91, 0x2C,
    0x01, 0xC5, 0xDE, 0x84, 0xB6, 0x01, 0x4A, 0xA2,
    0x4F, 0x7C, 0x60, 0xC9, 0x91, 0x01, 0x57, 0xAD,
    0x20, 0x44, 0xF6, 0xEF, 0x02, 0xE9, 0x05, 0xFE,
    0x33, 0xDC, 0xAE, 0x40, 0x8A, 0x01, 0x81, 0x0B,
    0x6C, 0x7E, 0x65, 0x37, 0x41, 0xD1, 0x63, 0x55,
    0xC8, 0x22, 0xA8, 0x0C, 0x39, 0x46, 0x3D, 0xE6,
    0x5F, 0x24, 0x91, 0x91, 0x15, 0x42, 0xE9, 0x93,
    0x91, 0x57, 0xAF, 0x45, 0xA2, 0xD0, 0xD2, 0x28,
    0xEB, 0x1D, 0x4A, 0x89, 0x6E, 0x1E, 0x0E, 0x05,
    0x36, 0x2E, 0xCA, 0xEA, 0x2A, 0x12, 0xF3, 0x7E,
    0x58, 0x95, 0x37, 0x04, 0x77, 0xB2, 0x27, 0x01,
    0x22, 0x5B, 0x25, 0x1B, 0x59, 0x64, 0x19, 0xB0,
    0xE9, 0x6D, 0x57, 0x47, 0x13, 0x2C, 0xC8, 0x1A,
    0x95, 0x85, 0x72, 0x1F, 0x8A, 0x5B, 0xE6, 0x65,
    0x2B, 0x31, 0x72, 0x6D, 0x66, 0xDD, 0xBE, 0x66,
    0xF9, 0x46, 0x1D, 0x35, 0xF3, 0xB4, 0xF9, 0xE2,
    0x12, 0x15, 0xD9, 0x88, 0x78, 0x14, 0xAD, 0xA8,
    0x4D, 0x4A, 0xAC, 0x51, 0xAF, 0x55, 0xBF, 0x6B,
    0x84, 0x3E, 0x09, 0x54, 0xA1, 0x86, 0x79, 0xB8,
    0x26, 0x72, 0x88, 0x80, 0x66, 0x65, 0x0A, 0xE0,
    0xDB, 0x93, 0xF0, 0x07, 0xDE, 0x04, 0xEF, 0xE5,
    0xF6, 0x04, 0x0B, 0x7D, 0xB5, 0x97, 0x15, 0xFA,
    0x54, 0x75, 0xA4, 0x60, 0x31, 0xD4, 0xE5, 0xCD,
    0x50, 0x39, 0xC7, 0x9F, 0xB2, 0xF6, 0xAB, 0x88,
    0xA7, 0x86, 0x38, 0x03, 0x9A, 0xBE, 0xF0, 0x2C,
    0x2A, 0x27, 0xEE, 0xC2, 0x70, 0x09, 0xA7, 0x0E,
    0x09, 0xF0, 0x68, 0xC0, 0x4F, 0xF3, 0xCC, 0x87,
    0x20, 0xA7, 0x17, 0x23, 0x46, 0xC8, 0x9E, 0x98,
    0x9C, 0xAC, 0xA5, 0xFA, 0x18, 0x54, 0x6F, 0xA1,
    0xD1, 0x7B, 0x2B, 0x63, 0x9F, 0xC5, 0x1F, 0xB7,
    0x0F, 0xD0, 0x46, 0x9B, 0xE0, 0xB9, 0x0C, 0xE7,
    0x20, 0x51, 0xBD, 0xE8, 0x9F, 0x2E, 0xC5, 0x4F,
    0x1A, 0xB4, 0x6A, 0x62, 0xE6, 0xB6, 0x7A, 0x0E,
    0xD6, 0xFD, 0x91, 0x65, 0x14, 0x89, 0xEC, 0x9B,
    0xF4, 0x4A, 0x33, 0xE6, 0x9F, 0x59, 0x54, 0x5F,
    0xEE, 0x5F, 0x91, 0x68, 0x17, 0xB2, 0x71, 0x35,
    0xE4, 0x1B, 0x8A, 0x5A, 0x10, 0x82, 0x3C, 0x58,
    0x44, 0x96, 0x0B, 0x77, 0xD2, 0xED, 0xB5, 0x18,
    0xDB, 0x7C, 0xE0, 0x91, 0x70, 0x7C, 0x90, 0x4F,
    0x4B, 0xA1, 0x33, 0xEF, 0x15, 0x5F, 0xC1, 0xD5,
    0x9B, 0xD8, 0xD7, 0xCC, 0xBE, 0xB3, 0xE6, 0xA9,
    0xB8, 0x35, 0x94, 0xED, 0x08, 0x22, 0x6B, 0xC8,
    0x7D, 0xE5, 0x5E, 0x49, 0x4A, 0x88, 0x08, 0x64,
    0xFF, 0x59, 0x36, 0x8C, 0xF1, 0xE5, 0xB1, 0x95,
    0x7C, 0xC9, 0x12, 0xD4, 0x90, 0x9E, 0xCB, 0x36,
    0x22, 0xF5, 0x74, 0x02, 0x1D, 0x09, 0x74, 0x6A,
    0x88, 0xDA, 0xE7, 0x9C, 0x6C, 0x17, 0x92, 0x3F,
    0x3D, 0x2E, 0x0A, 0x91, 0x85, 0x6F, 0x16, 0x8D,
    0xD4, 0x95, 0x0D, 0x32, 0xBA, 0x2A, 0x7A, 0x35,
    0xFC, 0x67, 0xEC, 0xA1, 0x4B, 0x0E, 0xB5, 0x99,
    0x47, 0xC2, 0xD0, 0x6C, 0xCC, 0x2D, 0x71, 0x65,
    0xE1, 0xB4, 0x35, 0xCA, 0x45, 0x37, 0x1D, 0x71,
    0x9B, 0xF9, 0xEF, 0x30, 0xFC, 0x0E, 0x56, 0x97,
    0x55, 0x7A, 0x51, 0x98, 0x38, 0x26, 0x98, 0xB6,
    0xC4, 0xAE, 0x27, 0x43, 0x1A, 0x7D, 0x1D, 0xE8,
    0x20, 0xE9, 0xA6, 0x67, 0x14, 0x72, 0x43, 0xE2,
    0x5E, 0xC3, 0x95, 0x84, 0x30, 0xE9, 0x7A, 0x67,
    0x5D, 0x84, 0x7B, 0xF8, 0x95, 0xA5, 0xB7, 0x9E,
    0x83, 0x98, 0xE4, 0x85, 0x01, 0x02, 0x82, 0x01,
    0x01, 0x00, 0xF1, 0xE4, 0xE3, 0x7B, 0x47, 0x3B,
    0xD8, 0x21, 0x2A, 0xCB, 0x7B, 0xDB, 0x81, 0x64,
    0x90, 0xDD, 0x15, 0x93, 0x4D, 0xD7, 0x48, 0x5F,
    0xEF, 0x42, 0x54, 0x86, 0x98, 0xC5, 0xAE, 0x8D,
    0xF8, 0x8C, 0x81, 0x6E, 0x00, 0x82, 0x7B, 0x69,
    0xB9, 0x86, 0x36, 0xC0, 0x70, 0xB5, 0xC8, 0x89,
    0xB7, 0xC4, 0xFF, 0xAF, 0x1E, 0x22, 0x56, 0x06,
    0x38, 0xDC, 0x03, 0xEA, 0xE0, 0x9D, 0x8A, 0x0C,
    0x01, 0x5E, 0xB3, 0xFB, 0xDA, 0x16, 0xDA, 0x0D,
    0x08, 0x99, 0xAC, 0xF2, 0xE8, 0x3F, 0x04, 0xA3,
    0x42, 0x76, 0xEE, 0x82, 0xC6, 0x71, 0x87, 0x93,
    0x05, 0xEF, 0x89, 0x9D, 0xC1, 0x02, 0xE3, 0x09,
    0x20, 0x35, 0x64, 0x27, 0x08, 0x7A, 0xC5, 0x10,
    0x62, 0xAE, 0x9E, 0xF2, 0xD8, 0x50, 0xF1, 0x75,
    0xED, 0xD3, 0x57, 0xE4, 0x4F, 0xBF, 0xDE, 0xA8,
    0xBB, 0x3C, 0xDD, 0x2B, 0x85, 0xB1, 0xEF, 0x84,
    0xE1, 0x27, 0x4D, 0x33, 0xBA, 0x8A, 0xFF, 0x17,
    0xDC, 0x79, 0xED, 0xFC, 0xB4, 0xC5, 0x18, 0xCB,
    0x96, 0x9F, 0x7E, 0x7A, 0x4F, 0xCA, 0x4E, 0x4F,
    0x7C, 0xB6, 0x03, 0x00, 0xDF, 0xB9, 0x9F, 0x55,
    0x3F, 0x52, 0x7C, 0xEB, 0xD4, 0x33, 0x97, 0x13,
    0x9A, 0x09, 0xD7, 0xC9, 0x82, 0x62, 0x43, 0xA7,
    0xDA, 0xFB, 0x90, 0xA2, 0x5E, 0xA8, 0x34, 0xFD,
    0x29, 0x43, 0x47, 0xD1, 0x4E, 0x67, 0x08, 0x72,
    0x10, 0xEA, 0x23, 0x5F, 0x1A, 0x7D, 0x4D, 0xB3,
    0x37, 0x23, 0x2F, 0xD6, 0x3A, 0x1A, 0xFA, 0xE4,
    0x71, 0x15, 0xB6, 0xDF, 0xA0, 0xDE, 0x34, 0x84,
    0xC2, 0xCD, 0x43, 0x2F, 0x60, 0xDB, 0xFC, 0x96,
    0xB4, 0x6E, 0x14, 0x92, 0xCD, 0xEA, 0x10, 0x2F,
    0x72, 0xD5, 0xAE, 0x3D, 0xC8, 0xEF, 0x7E, 0x9F,
    0x08, 0xBF, 0x8B, 0xBA, 0x2F, 0xED, 0x21, 0xD8,
    0xE5, 0x00, 0x4B, 0xD5, 0xCE, 0x5D, 0xAA, 0xC0,
    0x40, 0x7B, 0x02, 0x82, 0x01, 0x01, 0x00, 0xC8,
    0x9A, 0xE2, 0x58, 0x49, 0x07, 0x7D, 0x31, 0xAA,
    0x96, 0xAF, 0xF4, 0x93, 0x34, 0x14, 0xE6, 0x88,
    0xCA, 0x10, 0x67, 0x96, 0x74, 0x8B, 0x30, 0xBD,
    0x95, 0x9F, 0x8C, 0x4C, 0x6F, 0x39, 0xDF, 0x66,
    0x45, 0xFF, 0xB3, 0xAD, 0x8D, 0x0A, 0xA9, 0x5B,
    0xD3, 0xD3, 0x31, 0xA4, 0x46, 0xDD, 0xF7, 0x30,
    0xB0, 0x58, 0x03, 0x8B, 0x28, 0x89, 0x43, 0xCE,
    0x61, 0x23, 0x3F, 0x67, 0x5C, 0x5A, 0xF6, 0x19,
    0x8E, 0x0B, 0x61, 0xA7, 0xB1, 0x54, 0x54, 0xE6,
    0x91, 0x74, 0x10, 0xA6, 0x02, 0x04, 0x48, 0x92,
    0xA8, 0xBC, 0xA6, 0x54, 0x34, 0x0F, 0xC1, 0x0F,
    0x27, 0x75, 0x6A, 0x2F, 0xA7, 0x39, 0x8F, 0xE3,
    0x6C, 0x21, 0x30, 0x67, 0x06, 0xAD, 0xF1, 0xBF,
    0x13, 0x83, 0xFC, 0x4E, 0x1B, 0x5F, 0x02, 0x12,
    0xE1, 0xB9, 0xB8, 0x48, 0x55, 0xAF, 0x58, 0xBA,
    0xE2, 0x71, 0x1B, 0x91, 0x31, 0x63, 0x85, 0x12,
    0x97, 0x46, 0x35, 0xCB, 0xD0, 0x29, 0xA8, 0x27,
    0xBE, 0x0B, 0x31, 0xA2, 0x99, 0x7B, 0x09, 0xCC,
    0xF5, 0x9F, 0xB3, 0x8B, 0x6A, 0x38, 0xED, 0xF9,
    0xC2, 0x69, 0x15, 0xDA, 0x15, 0x15, 0x17, 0x1C,
    0x79, 0x0E, 0x44, 0xFB, 0x1A, 0x24, 0x11, 0x47,
    0xB2, 0x86, 0xFF, 0xBD, 0x63, 0x94, 0xEE, 0x63,
    0x2E, 0xF7, 0x2A, 0xD1, 0x1A, 0x91, 0xD2, 0xE2,
    0x22, 0x02, 0xAC, 0xE6, 0x18, 0x77, 0xF3, 0x6F,
    0x77, 0x7A, 0xDD, 0xA3, 0x76, 0x64, 0x42, 0x87,
    0x98, 0x6F, 0x29, 0x04, 0xB0, 0x4C, 0x43, 0xC3,
    0x76, 0x3C, 0x04, 0x9D, 0xE1, 0x3C, 0xE6, 0x28,
    0xE4, 0x8B, 0x75, 0xE2, 0x63, 0x6C, 0x4E, 0x30,
    0x38, 0xB0, 0x24, 0x96, 0xC0, 0x00, 0xA0, 0xDF,
    0xAE, 0xBB, 0xB0, 0xC2, 0x8F, 0xD7, 0x18, 0x23,
    0x3E, 0xDF, 0x9E, 0xD4, 0xD8, 0x89, 0xBE, 0x7C,
    0x25, 0xA8, 0x94, 0x42, 0x52, 0xF4, 0x21, 0x02,
    0x82, 0x01, 0x00, 0x6D, 0x92, 0xC8, 0x75, 0x14,
    0x7C, 0x88, 0x57, 0xE5, 0xE1, 0xCD, 0x16, 0x4E,
    0xD3, 0xB6, 0x47, 0x5F, 0xB9, 0x9D, 0x5E, 0x32,
    0xDB, 0xBA, 0x2E, 0xEA, 0xC9, 0x19, 0xB5, 0x1A,
    0x70, 0x17, 0x31, 0xD6, 0x83, 0x74, 0xD9, 0xA6,
    0x7B, 0xEA, 0xDD, 0x60, 0x39, 0xE6, 0x14, 0x82,
    0x65, 0xD8, 0x07, 0xF5, 0xE6, 0x66, 0x99, 0x19,
    0x2C, 0xB2, 0x5E, 0xFF, 0xAB, 0x4A, 0xDA, 0x32,
    0x29, 0x64, 0x09, 0x79, 0x30, 0x51, 0xC7, 0x5E,
    0x38, 0xDF, 0x37, 0x75, 0xBF, 0x86, 0xF2, 0xDC,
    0xEF, 0xF3, 0x70, 0x9A, 0x74, 0x44, 0xDE, 0x32,
    0x92, 0xA7, 0xF2, 0xAC, 0xFC, 0xF7, 0xEC, 0x8D,
    0xD9, 0x99, 0x6C, 0x76, 0x14, 0xB4, 0x63, 0xA0,
    0x96, 0xDE, 0x8C, 0xC9, 0x55, 0xAA, 0x25, 0x68,
    0x6B, 0x8A, 0x32, 0xDD, 0x32, 0x7F, 0xE9, 0xA3,
    0x3B, 0xFD, 0xAD, 0x01, 0x66, 0xF0, 0xA5, 0xFC,
    0xA0, 0xCD, 0xB9, 0x16, 0xE0, 0xA2, 0xD4, 0xC9,
    0xDF, 0x0E, 0x31, 0x71, 0x5B, 0xB2, 0xAB, 0x41,
    0xA3, 0x1F, 0x74, 0xDA, 0x3F, 0x1F, 0x9B, 0xBD,
    0xC3, 0x85, 0xF3, 0x91, 0x02, 0xB9, 0x55, 0x85,
    0x56, 0x81, 0x5B, 0xA5, 0x58, 0xD2, 0x84, 0x18,
    0x20, 0x79, 0xFB, 0x91, 0x73, 0x27, 0x8B, 0xC9,
    0xA7, 0xE4, 0xAB, 0x66, 0x06, 0x17, 0x28, 0xCC,
    0xF0, 0x0C, 0xDB, 0xFC, 0x51, 0x54, 0xD9, 0x99,
    0x5A, 0xCA, 0x4C, 0xB5, 0x7F, 0x24, 0x29, 0xB4,
    0xA3, 0xC5, 0x0B, 0x3C, 0x65, 0x27, 0x67, 0x07,
    0x55, 0xA9, 0xE0, 0x33, 0x39, 0xB3, 0xCE, 0x54,
    0x2D, 0xFB, 0x6C, 0x2D, 0xCD, 0xED, 0x6B, 0x59,
    0x4D, 0x0F, 0x28, 0x7A, 0x4B, 0x51, 0xD9, 0x80,
    0x41, 0x6E, 0x1C, 0x5B, 0x54, 0x83, 0x8B, 0xF1,
    0x68, 0xAA, 0xF6, 0xD1, 0x9D, 0xA2, 0x8E, 0x21,
    0x71, 0xF5, 0xB2, 0x1F, 0xA8, 0x17, 0x56, 0x18,
    0xBE, 0x03, 0xB7, 0x02, 0x82, 0x01, 0x01, 0x00,
    0x95, 0xF7, 0xC4, 0x32, 0xC0, 0x15, 0xCB, 0x30,
    0x30, 0x19, 0x8A, 0xDD, 0x4A, 0x96, 0xEA, 0xF1,
    0x15, 0x5E, 0x0A, 0x39, 0x86, 0xAF, 0x05, 0xC7,
    0xA0, 0x53, 0x94, 0xFD, 0x2B, 0xBF, 0x13, 0x02,
    0xC1, 0xD0, 0x9E, 0xC2, 0x52, 0x82, 0x58, 0x70,
    0x17, 0xE8, 0x19, 0xC0, 0xC8, 0xD0, 0xF9, 0xEB,
    0x6C, 0x08, 0xC9, 0x23, 0x8D, 0x0A, 0xF6, 0xE3,
    0x2C, 0x0C, 0x4E, 0x6F, 0x7D, 0xF3, 0xFB, 0x44,
    0x74, 0x02, 0x4E, 0xDB, 0xE8, 0xA3, 0x97, 0x2A,
    0x09, 0x5C, 0xBF, 0xEA, 0x1A, 0x50, 0xF3, 0xA6,
    0xD5, 0x8D, 0x33, 0xCB, 0x1C, 0xB3, 0xF6, 0xFB,
    0x1B, 0x3B, 0x22, 0x7A, 0x09, 0xB9, 0xF8, 0x3C,
    0x6B, 0x86, 0x50, 0xA1, 0x80, 0xC3, 0xEB, 0x92,
    0x08, 0x88, 0xAC, 0x69, 0x18, 0x73, 0x39, 0xE9,
    0xB0, 0xF7, 0xF3, 0x01, 0xA3, 0x81, 0xFB, 0x1E,
    0x58, 0x48, 0x24, 0xCA, 0xB9, 0x16, 0xB5, 0x11,
    0xAC, 0xBE, 0x4E, 0x8C, 0xB5, 0x1B, 0x9F, 0xA8,
    0xC8, 0x5D, 0x38, 0xBD, 0x18, 0x93, 0xA8, 0xF9,
    0xC0, 0xB1, 0xF7, 0x7C, 0x20, 0x43, 0x7E, 0xDF,
    0xD9, 0x91, 0xCF, 0x41, 0xB8, 0xD1, 0x7D, 0xFC,
    0xD3, 0x1A, 0x64, 0xE2, 0x88, 0xDE, 0x18, 0x0C,
    0x9B, 0x0F, 0x0A, 0x01, 0x72, 0x32, 0x5A, 0x0A,
    0x85, 0x37, 0x7D, 0x2C, 0x13, 0x3A, 0xCB, 0xBD,
    0x5D, 0xB4, 0x93, 0x4F, 0x72, 0x8C, 0xE4, 0x4F,
    0xA7, 0x0B, 0x29, 0x5D, 0x3C, 0x70, 0xAB, 0xAF,
    0x58, 0xF3, 0x4A, 0x56, 0xDD, 0xDC, 0x76, 0x87,
    0x4A, 0x3E, 0xBF, 0xCB, 0xAB, 0xC3, 0xD0, 0xE5,
    0x37, 0xB8, 0xF3, 0xD1, 0x63, 0x37, 0x74, 0xA6,
    0x57, 0x42, 0x69, 0xDC, 0x50, 0x9E, 0xA5, 0x59,
    0x9F, 0x1F, 0x5F, 0xCD, 0x5B, 0x06, 0x71, 0x84,
    0xE8, 0x04, 0x7E, 0xE4, 0x9C, 0xB6, 0x6C, 0xF3,
    0x6C, 0x98, 0x32, 0x98, 0xF3, 0xA4, 0x54, 0x41,
    0x02, 0x82, 0x01, 0x00, 0x33, 0x4A, 0xAB, 0xB3,
    0x75, 0x6C, 0xFB, 0xD1, 0x8F, 0x3F, 0x4C, 0xB2,
    0x5F, 0xB7, 0xCB, 0xE9, 0xA9, 0x6B, 0xF4, 0x12,
    0xB4, 0xCF, 0xC9, 0xC9, 0x16, 0x7D, 0x45, 0x37,
    0x26, 0x65, 0xA5, 0x04, 0xFE, 0xF8, 0xDC, 0x83,
    0xD7, 0x83, 0x2B, 0x57, 0x68, 0xBC, 0x3C, 0x18,
    0x97, 0x1A, 0x7B, 0x82, 0xD3, 0x17, 0x13, 0xE1,
    0xBF, 0xC4, 0x66, 0x81, 0x31, 0x96, 0xCD, 0x95,
    0xA7, 0xCE, 0x4D, 0x03, 0xFA, 0xDF, 0x6B, 0x8D,
    0xE1, 0x89, 0x5D, 0xF9, 0x97, 0xBE, 0x8C, 0xB7,
    0x7E, 0xE3, 0x2F, 0x98, 0xFE, 0x9C, 0x46, 0xAD,
    0xF2, 0x82, 0xCD, 0x8E, 0x9A, 0x23, 0x50, 0x48,
    0xEE, 0xA5, 0xDE, 0x51, 0x4D, 0x59, 0x9B, 0xBC,
    0xDE, 0xFD, 0xF4, 0x7C, 0x16, 0xE1, 0xA8, 0x9C,
    0x72, 0x2E, 0x54, 0x5C, 0xE0, 0x61, 0xBD, 0x66,
    0xCB, 0x64, 0x08, 0x45, 0xE0, 0x39, 0x8A, 0x58,
    0xC9, 0x56, 0xDC, 0x99, 0x1E, 0x27, 0xEB, 0x13,
    0xEA, 0x74, 0x63, 0x5D, 0x96, 0x23, 0xB8, 0xF9,
    0x3B, 0x36, 0xFA, 0x1F, 0x71, 0x4C, 0xF1, 0x03,
    0x5C, 0x42, 0x78, 0x48, 0xE0, 0xEA, 0xD4, 0x90,
    0xC8, 0xED, 0xD0, 0xBE, 0x41, 0x75, 0xE9, 0x34,
    0x0B, 0x79, 0xCF, 0x25, 0xA5, 0xCD, 0xC6, 0x07,
    0x5E, 0x1C, 0x3A, 0xFA, 0x88, 0x13, 0xCC, 0x18,
    0xD6, 0x39, 0x05, 0x14, 0xAE, 0xD8, 0x93, 0xD6,
    0x95, 0x5C, 0x3E, 0xC7, 0xFC, 0xDB, 0x45, 0xCF,
    0x50, 0x26, 0xEA, 0x19, 0x6B, 0xE5, 0xC5, 0x83,
    0x40, 0x0F, 0x53, 0x2A, 0x7F, 0xEE, 0x28, 0x83,
    0x17, 0xDE, 0xFC, 0xEE, 0x05, 0x6E, 0x2C, 0xF4,
    0x41, 0xB1, 0x88, 0xE9, 0x62, 0xA0, 0x3A, 0x72,
    0x89, 0x69, 0x48, 0x4B, 0x86, 0x49, 0x83, 0xD1,
    0xF3, 0x45, 0x69, 0x2D, 0xB5, 0xC2, 0x3B, 0x84,
    0x02, 0x67, 0x58, 0xE0, 0xFE, 0x9E, 0xE4, 0xFB,
    0x7D, 0xE7, 0x5A, 0x1A
};

static unsigned char sig_4k[] = {
    0x7d, 0xeb, 0x52, 0x97, 0x84, 0xc1, 0xf7, 0x67,
    0x24, 0xa8, 0xae, 0xdd, 0xd8, 0xaf, 0xe6, 0xd6,
    0xbc, 0x92, 0x3e, 0x51, 0xab, 0xa1, 0x05, 0xb6,
    0x78, 0x6c, 0xa3, 0x7c, 0x18, 0x2a, 0xd9, 0xbf,
    0x2b, 0xee, 0x5f, 0xe1, 0x3f, 0x16, 0xb9, 0xf3,
    0xd3, 0x49, 0xfd, 0x7e, 0x1d, 0xfe, 0x19, 0x66,
    0x51, 0xc8, 0x89, 0xe2, 0xc3, 0x80, 0xff, 0x53,
    0x14, 0x82, 0x3c, 0x0e, 0xdc, 0x3f, 0xab, 0x0a,
    0x2b, 0xc9, 0x81, 0x4b, 0x2d, 0xcd, 0xf2, 0xbe,
    0xd5, 0x4f, 0x65, 0xa7, 0xb9, 0xf5, 0x92, 0x7e,
    0xb1, 0xbe, 0xc5, 0x98, 0xf4, 0x95, 0x81, 0x86,
    0x66, 0xfc, 0x29, 0xd4, 0x56, 0x2a, 0x4a, 0x8a,
    0x7c, 0x67, 0x14, 0xc0, 0x2b, 0xb8, 0x9a, 0x6d,
    0x73, 0x5d, 0x58, 0x10, 0x18, 0x38, 0x9d, 0x63,
    0x09, 0x16, 0xfa, 0x83, 0x14, 0xef, 0xca, 0x9b,
    0x71, 0x8d, 0xcd, 0x33, 0x5d, 0xc4, 0xae, 0x0d,
    0x7c, 0xde, 0x82, 0x6f, 0x45, 0xdd, 0x2a, 0x7d,
    0x35, 0x83, 0xac, 0x28, 0x33, 0xc7, 0x5c, 0x52,
    0x6a, 0x4c, 0x08, 0x0b, 0x61, 0xb7, 0xc8, 0x39,
    0x25, 0xf0, 0xcb, 0xeb, 0xdb, 0x2c, 0x5f, 0x56,
    0x7d, 0x62, 0xed, 0x61, 0xe4, 0x01, 0x32, 0x65,
    0xad, 0xcc, 0x37, 0xa4, 0xcb, 0x96, 0x33, 0x6c,
    0x18, 0x63, 0xbd, 0x87, 0x17, 0x2a, 0x98, 0x8a,
    0xe0, 0xca, 0x5b, 0x77, 0x28, 0x84, 0x44, 0xe9,
    0x41, 0x6f, 0x16, 0xef, 0x29, 0x3f, 0x5d, 0xa7,
    0x99, 0x2a, 0x33, 0xf0, 0x0a, 0xf7, 0x3e, 0xba,
    0xd0, 0x5d, 0x44, 0x4c, 0xf7, 0x64, 0xfa, 0xe6,
    0x93, 0xb6, 0xd2, 0xe5, 0x07, 0xa3, 0xc2, 0x9f,
    0xee, 0x05, 0x02, 0xc6, 0x7d, 0x15, 0x31, 0x9b,
    0x32, 0x12, 0x30, 0x78, 0xda, 0xdc, 0xd1, 0x32,
    0xbe, 0xaf, 0x4b, 0x4e, 0xe0, 0x16, 0x6b, 0xc7,
    0x92, 0x35, 0x48, 0x86, 0x7d, 0xd6, 0xf9, 0x7e,
    0x29, 0x2b, 0x50, 0xbb, 0x27, 0x19, 0xd3, 0x1b,
    0x37, 0xc4, 0x8e, 0x1a, 0x28, 0x08, 0xdf, 0xf1,
    0xb6, 0x7e, 0xe0, 0x06, 0x93, 0x36, 0xd3, 0x13,
    0x64, 0x47, 0xf3, 0xad, 0x35, 0xf0, 0xae, 0xd1,
    0x6c, 0xc5, 0x75, 0x58, 0xac, 0xe0, 0xa7, 0x9b,
    0xbb, 0x61, 0xb9, 0xcf, 0xbe, 0x04, 0x9b, 0x1e,
    0xa4, 0xf4, 0x59, 0xa7, 0x17, 0xc4, 0x5a, 0x40,
    0x5c, 0x4a, 0x4f, 0x2d, 0x95, 0x25, 0x9b, 0x1c,
    0x02, 0x89, 0xd9, 0x88, 0x5d, 0xd5, 0x60, 0x2a,
    0x9a, 0xfd, 0xde, 0x8c, 0xcc, 0x6b, 0xa6, 0x25,
    0x14, 0xe7, 0x71, 0x85, 0xd0, 0xd0, 0xc4, 0x59,
    0xf1, 0x22, 0x23, 0xb5, 0x91, 0x3d, 0xd4, 0xdb,
    0x11, 0xe6, 0x32, 0x5f, 0x0c, 0x8b, 0xe6, 0x8d,
    0xf9, 0xff, 0x65, 0xac, 0x25, 0xb4, 0x55, 0x2d,
    0x60, 0x9a, 0x10, 0xcf, 0xfa, 0x4f, 0x3c, 0x3b,
    0x1b, 0xbe, 0x14, 0x49, 0x58, 0x31, 0x2c, 0x42,
    0xff, 0x98, 0xa2, 0xbc, 0x59, 0x54, 0x8b, 0x74,
    0x7b, 0xba, 0x7e, 0x52, 0xb3, 0xcd, 0xac, 0x29,
    0x69, 0xdc, 0x10, 0x59, 0x55, 0x3b, 0x17, 0xe6,
    0x99, 0xa2, 0x4b, 0x46, 0xbf, 0x23, 0x62, 0x5d,
    0x61, 0x8c, 0x62, 0x32, 0xfa, 0x73, 0x85, 0x03,
    0x5f, 0x9b, 0x6c, 0x6c, 0x9c, 0x55, 0x03, 0x4d,
    0x4f, 0x70, 0x16, 0x68, 0xe3, 0x04, 0x8b, 0xfb,
    0x94, 0x3e, 0xde, 0xb7, 0x01, 0x4a, 0xd1, 0x48,
    0xec, 0x84, 0xa5, 0x35, 0x4f, 0xa5, 0x75, 0x9f,
    0x29, 0xf0, 0x1d, 0x84, 0x30, 0x3a, 0xd4, 0x63,
    0x9a, 0x6c, 0x43, 0x8e, 0xd3, 0x4c, 0x24, 0xbb,
    0x92, 0x71, 0xa0, 0x64, 0x6a, 0x1e, 0xcd, 0xa9,
    0xfe, 0xa4, 0x87, 0x3a, 0x8e, 0x98, 0xee, 0x58,
    0x06, 0xc8, 0x50, 0xf9, 0x75, 0x65, 0xac, 0xc5,
    0xe7, 0xc3, 0xd1, 0x9a, 0x5e, 0x86, 0x4b, 0x84,
    0xeb, 0x89, 0x8c, 0x7f, 0x1f, 0x36, 0x57, 0x7a
};


int rsa4096_test(void)
{
    int    ret;
    WC_RNG rng;
    RsaKey key;
    const unsigned char* in = (const unsigned char*)"Everyone gets Friday off.";
    const char expected[] = {
        0x45, 0x76, 0x65, 0x72, 0x79, 0x6F, 0x6E, 0x65,
        0x20, 0x67, 0x65, 0x74, 0x73, 0x20, 0x46, 0x72,
        0x69, 0x64, 0x61, 0x79, 0x20, 0x6F, 0x66, 0x66,
        0x2E
    };
    const char expected_2[] = { /* interoperability test */
        0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x0a
    };
    unsigned int idx = 0;
    word32 inLen = (word32)XSTRLEN((char*)in);
    const word32 outSz   = 512;
    const word32 plainSz = 512;
    byte out[outSz];
    byte plain[plainSz];

    if (wc_InitRsaKey(&key, HEAP_HINT) != 0) {
        return -5434;
    }

    if (wc_InitRng(&rng) != 0) {
        return -5435;
    }

    ret = wc_RsaPrivateKeyDecode(key_4k, &idx, &key, (word32)sizeof(key_4k));
    if (ret != 0) {
        return -5436;
    }

    ret = wc_RsaSSL_Sign(in, inLen, out, outSz, &key, &rng);
    if (ret < 0) {
        wc_FreeRsaKey(&key);
        return -5437;
    }

    ret = wc_RsaSSL_Verify(out, outSz, plain, plainSz, &key);
    if (ret < 0) {
        wc_FreeRsaKey(&key);
        return -5438;
    }

    if (XMEMCMP(plain, expected, inLen) != 0) {
        wc_FreeRsaKey(&key);
        return -5439;
    }

    /* test of interoperability */
    XMEMSET(plain, 0, plainSz);
    ret = wc_RsaSSL_Verify(sig_4k, sizeof(sig_4k), plain, plainSz, &key);
    if (ret < 0) {
        wc_FreeRsaKey(&key);
        return -5440;
    }

    wc_FreeRsaKey(&key);
    if (XMEMCMP(plain, expected_2, sizeof(expected_2)) != 0) {
        return -5441;
    }

    return 0;
}
#endif /* 4096 bit RSA tests */

#define RSA_TEST_BYTES 256
int rsa_test(void)
{
    int    ret;
    byte*  tmp = NULL;
    byte*  der = NULL;
    byte*  pem = NULL;
    size_t bytes;
    WC_RNG rng;
    RsaKey key;
#ifdef WOLFSSL_CERT_EXT
    RsaKey keypub;
#endif
#ifdef WOLFSSL_KEY_GEN
    RsaKey genKey;
#endif
#if defined(WOLFSSL_CERT_GEN) || defined(HAVE_NTRU)
    RsaKey caKey;
#endif
#ifdef HAVE_ECC
    #ifdef WOLFSSL_CERT_GEN
        ecc_key caEccKey;
        ecc_key caEccKeyPub;
    #endif
#endif /* HAVE_ECC */
    word32 idx = 0;
    byte*  res;
    const char* inStr = "Everyone gets Friday off.";
    word32      inLen = (word32)XSTRLEN((char*)inStr);
    const word32 outSz   = RSA_TEST_BYTES;
    const word32 plainSz = RSA_TEST_BYTES;
#if !defined(USE_CERT_BUFFERS_1024) && !defined(USE_CERT_BUFFERS_2048) \
                                    && !defined(NO_FILESYSTEM)
    FILE    *file, *file2;
#endif
#ifdef WOLFSSL_TEST_CERT
    DecodedCert cert;
#endif

    DECLARE_VAR_INIT(in, byte, inLen, inStr, HEAP_HINT);
    DECLARE_VAR(out, byte, RSA_TEST_BYTES, HEAP_HINT);
    DECLARE_VAR(plain, byte, RSA_TEST_BYTES, HEAP_HINT);

#ifdef WOLFSSL_ASYNC_CRYPT
    if (in == NULL)
        return MEMORY_E;
#endif

    /* initialize stack structures */
    XMEMSET(&rng, 0, sizeof(rng));
    XMEMSET(&key, 0, sizeof(key));
#ifdef WOLFSSL_CERT_EXT
    XMEMSET(&keypub, 0, sizeof(keypub));
#endif
#ifdef WOLFSSL_KEY_GEN
    XMEMSET(&genKey, 0, sizeof(genKey));
#endif
#if defined(WOLFSSL_CERT_GEN) || defined(HAVE_NTRU)
    XMEMSET(&caKey, 0, sizeof(caKey));
#endif
#ifdef HAVE_ECC
    #ifdef WOLFSSL_CERT_GEN
        XMEMSET(&caEccKey, 0, sizeof(caEccKey));
        XMEMSET(&caEccKeyPub, 0, sizeof(caEccKeyPub));
    #endif
#endif /* HAVE_ECC */

#ifndef HAVE_USER_RSA
    ret = rsa_decode_test();
    if (ret != 0)
        return ret;
#endif

#ifdef USE_CERT_BUFFERS_1024
    bytes = (size_t)sizeof_client_key_der_1024;
	if (bytes < (size_t)sizeof_client_cert_der_1024)
		bytes = (size_t)sizeof_client_cert_der_1024;
#elif defined(USE_CERT_BUFFERS_2048)
    bytes = (size_t)sizeof_client_key_der_2048;
	if (bytes < (size_t)sizeof_client_cert_der_2048)
		bytes = (size_t)sizeof_client_cert_der_2048;
#else
	bytes = FOURK_BUF;
#endif

    tmp = (byte*)XMALLOC(bytes, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (tmp == NULL
    #ifdef WOLFSSL_ASYNC_CRYPT
        || out == NULL || plain == NULL
    #endif
    ) {
        return -5500;
    }

#ifdef USE_CERT_BUFFERS_1024
    XMEMCPY(tmp, client_key_der_1024, (size_t)sizeof_client_key_der_1024);
#elif defined(USE_CERT_BUFFERS_2048)
    XMEMCPY(tmp, client_key_der_2048, (size_t)sizeof_client_key_der_2048);
#elif !defined(NO_FILESYSTEM)
    file = fopen(clientKey, "rb");
    if (!file) {
        err_sys("can't open ./certs/client-key.der, "
                "Please run from wolfSSL home dir", -40);
        ERROR_OUT(-5501, exit_rsa);
    }

    bytes = fread(tmp, 1, FOURK_BUF, file);
    fclose(file);
#else
    /* No key to use. */
    ERROR_OUT(-5502, exit_rsa);
#endif /* USE_CERT_BUFFERS */

    ret = wc_InitRsaKey_ex(&key, HEAP_HINT, devId);
    if (ret != 0) {
        ERROR_OUT(-5503, exit_rsa);
    }
    ret = wc_RsaPrivateKeyDecode(tmp, &idx, &key, (word32)bytes);
    if (ret != 0) {
        ERROR_OUT(-5504, exit_rsa);
    }

#ifndef HAVE_FIPS
    ret = wc_InitRng_ex(&rng, HEAP_HINT, devId);
#else
    ret = wc_InitRng(&rng);
#endif
    if (ret != 0) {
        ERROR_OUT(-5505, exit_rsa);
    }

#ifndef NO_SIG_WRAPPER
    ret = rsa_sig_test(&key, sizeof(RsaKey), wc_RsaEncryptSize(&key), &rng);
    if (ret != 0)
        goto exit_rsa;
#endif

    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPublicEncrypt(in, inLen, out, outSz, &key, &rng);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5506, exit_rsa);
    }

#ifdef WC_RSA_BLINDING
    {
        int tmpret = ret;
        ret = wc_RsaSetRNG(&key, &rng);
        if (ret < 0) {
            ERROR_OUT(-5507, exit_rsa);
        }
        ret = tmpret;
    }
#endif

    idx = (word32)ret; /* save off encrypted length */
    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPrivateDecrypt(out, idx, plain, plainSz, &key);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5508, exit_rsa);
    }

    if (XMEMCMP(plain, in, inLen)) {
        ERROR_OUT(-5509, exit_rsa);
    }
    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPrivateDecryptInline(out, idx, &res, &key);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5510, exit_rsa);
    }
    if (ret != (int)inLen) {
        ERROR_OUT(-5511, exit_rsa);
    }
    if (XMEMCMP(res, in, inLen)) {
        ERROR_OUT(-5512, exit_rsa);
    }

    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaSSL_Sign(in, inLen, out, outSz, &key, &rng);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5513, exit_rsa);
    }

    idx = (word32)ret;
    XMEMSET(plain, 0, plainSz);
    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaSSL_Verify(out, idx, plain, plainSz, &key);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5514, exit_rsa);
    }

    if (XMEMCMP(plain, in, (size_t)ret)) {
        ERROR_OUT(-5515, exit_rsa);
    }

    #ifndef WC_NO_RSA_OAEP
    /* OAEP padding testing */
    #if !defined(HAVE_FAST_RSA) && !defined(HAVE_USER_RSA) && \
        !defined(HAVE_FIPS)
    #ifndef NO_SHA
    XMEMSET(plain, 0, plainSz);

    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPublicEncrypt_ex(in, inLen, out, outSz, &key, &rng,
                       WC_RSA_OAEP_PAD, WC_HASH_TYPE_SHA, WC_MGF1SHA1, NULL, 0);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5516, exit_rsa);
    }

    idx = (word32)ret;
    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPrivateDecrypt_ex(out, idx, plain, plainSz, &key,
                       WC_RSA_OAEP_PAD, WC_HASH_TYPE_SHA, WC_MGF1SHA1, NULL, 0);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5517, exit_rsa);
    }

    if (XMEMCMP(plain, in, inLen)) {
        ERROR_OUT(-5518, exit_rsa);
    }
    #endif /* NO_SHA */

    #ifndef NO_SHA256
    XMEMSET(plain, 0, plainSz);
    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPublicEncrypt_ex(in, inLen, out, outSz, &key, &rng,
                  WC_RSA_OAEP_PAD, WC_HASH_TYPE_SHA256, WC_MGF1SHA256, NULL, 0);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5519, exit_rsa);
    }

    idx = (word32)ret;
    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPrivateDecrypt_ex(out, idx, plain, plainSz, &key,
                  WC_RSA_OAEP_PAD, WC_HASH_TYPE_SHA256, WC_MGF1SHA256, NULL, 0);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5520, exit_rsa);
    }

    if (XMEMCMP(plain, in, inLen)) {
        ERROR_OUT(-5521, exit_rsa);
    }

    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPrivateDecryptInline_ex(out, idx, &res, &key,
                WC_RSA_OAEP_PAD, WC_HASH_TYPE_SHA256, WC_MGF1SHA256, NULL, 0);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5522, exit_rsa);
    }
    if (ret != (int)inLen) {
        ERROR_OUT(-5523, exit_rsa);
    }
    if (XMEMCMP(res, in, inLen)) {
        ERROR_OUT(-5524, exit_rsa);
    }

    /* check fails if not using the same optional label */
    XMEMSET(plain, 0, plainSz);
    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPublicEncrypt_ex(in, inLen, out, outSz, &key, &rng,
                  WC_RSA_OAEP_PAD, WC_HASH_TYPE_SHA256, WC_MGF1SHA256, NULL, 0);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5525, exit_rsa);
    }

/* TODO: investigate why Cavium Nitrox doesn't detect decrypt error here */
#ifndef HAVE_CAVIUM
    idx = (word32)ret;
    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPrivateDecrypt_ex(out, idx, plain, plainSz, &key,
               WC_RSA_OAEP_PAD, WC_HASH_TYPE_SHA256, WC_MGF1SHA256, in, inLen);
        }
    } while (ret == WC_PENDING_E);
    if (ret > 0) { /* in this case decrypt should fail */
        ERROR_OUT(-5526, exit_rsa);
    }
    ret = 0;
#endif /* !HAVE_CAVIUM */

    /* check using optional label with encrypt/decrypt */
    XMEMSET(plain, 0, plainSz);
    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPublicEncrypt_ex(in, inLen, out, outSz, &key, &rng,
               WC_RSA_OAEP_PAD, WC_HASH_TYPE_SHA256, WC_MGF1SHA256, in, inLen);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5527, exit_rsa);
    }

    idx = (word32)ret;
    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPrivateDecrypt_ex(out, idx, plain, plainSz, &key,
               WC_RSA_OAEP_PAD, WC_HASH_TYPE_SHA256, WC_MGF1SHA256, in, inLen);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5528, exit_rsa);
    }

    if (XMEMCMP(plain, in, inLen)) {
        ERROR_OUT(-5529, exit_rsa);
    }

    #ifndef NO_SHA
        /* check fail using mismatch hash algorithms */
        XMEMSET(plain, 0, plainSz);
        do {
    #if defined(WOLFSSL_ASYNC_CRYPT)
            ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
    #endif
            if (ret >= 0) {
                ret = wc_RsaPublicEncrypt_ex(in, inLen, out, outSz, &key, &rng,
                    WC_RSA_OAEP_PAD, WC_HASH_TYPE_SHA, WC_MGF1SHA1, in, inLen);
            }
        } while (ret == WC_PENDING_E);
        if (ret < 0) {
            ERROR_OUT(-5530, exit_rsa);
        }

/* TODO: investigate why Cavium Nitrox doesn't detect decrypt error here */
#ifndef HAVE_CAVIUM
        idx = (word32)ret;
        do {
    #if defined(WOLFSSL_ASYNC_CRYPT)
            ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
    #endif
            if (ret >= 0) {
                ret = wc_RsaPrivateDecrypt_ex(out, idx, plain, plainSz, &key,
                    WC_RSA_OAEP_PAD, WC_HASH_TYPE_SHA256, WC_MGF1SHA256,
                    in, inLen);
            }
        } while (ret == WC_PENDING_E);
        if (ret > 0) { /* should fail */
            ERROR_OUT(-5531, exit_rsa);
        }
        ret = 0;
#endif /* !HAVE_CAVIUM */
        #endif /* NO_SHA*/
    #endif /* NO_SHA256 */

    #ifdef WOLFSSL_SHA512
    /* Check valid RSA key size is used while using hash length of SHA512
       If key size is less than (hash length * 2) + 2 then is invalid use
       and test, since OAEP padding requires this.
       BAD_FUNC_ARG is returned when this case is not met */
    if (wc_RsaEncryptSize(&key) > ((int)WC_SHA512_DIGEST_SIZE * 2) + 2) {
        XMEMSET(plain, 0, plainSz);
        do {
    #if defined(WOLFSSL_ASYNC_CRYPT)
            ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
    #endif
            if (ret >= 0) {
                ret = wc_RsaPublicEncrypt_ex(in, inLen, out, outSz, &key, &rng,
                  WC_RSA_OAEP_PAD, WC_HASH_TYPE_SHA512, WC_MGF1SHA512, NULL, 0);
            }
        } while (ret == WC_PENDING_E);
        if (ret < 0) {
            ERROR_OUT(-5532, exit_rsa);
        }

        idx = ret;
        do {
    #if defined(WOLFSSL_ASYNC_CRYPT)
            ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
    #endif
            if (ret >= 0) {
                ret = wc_RsaPrivateDecrypt_ex(out, idx, plain, plainSz, &key,
                  WC_RSA_OAEP_PAD, WC_HASH_TYPE_SHA512, WC_MGF1SHA512, NULL, 0);
            }
        } while (ret == WC_PENDING_E);
        if (ret < 0) {
            ERROR_OUT(-5533, exit_rsa);
        }

        if (XMEMCMP(plain, in, inLen)) {
            ERROR_OUT(-5534, exit_rsa);
        }
    }
    #endif /* WOLFSSL_SHA512 */

    /* check using pkcsv15 padding with _ex API */
    XMEMSET(plain, 0, plainSz);
    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPublicEncrypt_ex(in, inLen, out, outSz, &key, &rng,
                  WC_RSA_PKCSV15_PAD, WC_HASH_TYPE_NONE, 0, NULL, 0);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5535, exit_rsa);
    }

    idx = (word32)ret;
    do {
#if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
        if (ret >= 0) {
            ret = wc_RsaPrivateDecrypt_ex(out, idx, plain, plainSz, &key,
                  WC_RSA_PKCSV15_PAD, WC_HASH_TYPE_NONE, 0, NULL, 0);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-5536, exit_rsa);
    }

    if (XMEMCMP(plain, in, inLen)) {
        ERROR_OUT(-5537, exit_rsa);
    }
    #endif /* !HAVE_FAST_RSA && !HAVE_FIPS */
    #endif /* WC_NO_RSA_OAEP */

    ret = rsa_flatten_test(&key);
    if (ret != 0)
        return ret;

#if defined(WOLFSSL_MDK_ARM)
    #define sizeof(s) XSTRLEN((char *)(s))
#endif

#ifdef USE_CERT_BUFFERS_1024
    XMEMCPY(tmp, client_cert_der_1024, (size_t)sizeof_client_cert_der_1024);
    bytes = (size_t)sizeof_client_cert_der_1024;
#elif defined(USE_CERT_BUFFERS_2048)
    XMEMCPY(tmp, client_cert_der_2048, (size_t)sizeof_client_cert_der_2048);
    bytes = (size_t)sizeof_client_cert_der_2048;
#elif !defined(NO_FILESYSTEM)
    file2 = fopen(clientCert, "rb");
    if (!file2) {
        ERROR_OUT(-5538, exit_rsa);
    }

    bytes = fread(tmp, 1, FOURK_BUF, file2);
    fclose(file2);
#else
    /* No certificate to use. */
    ERROR_OUT(-5539, exit_rsa);
#endif

#ifdef sizeof
        #undef sizeof
#endif

#ifdef WOLFSSL_TEST_CERT
    InitDecodedCert(&cert, tmp, (word32)bytes, 0);

    ret = ParseCert(&cert, CERT_TYPE, NO_VERIFY, 0);
    if (ret != 0) {
        FreeDecodedCert(&cert);
        ERROR_OUT(-5540, exit_rsa);
    }

    FreeDecodedCert(&cert);
#else
    (void)bytes;
#endif

#ifdef WOLFSSL_CERT_EXT

#ifdef USE_CERT_BUFFERS_1024
    XMEMCPY(tmp, client_keypub_der_1024, sizeof_client_keypub_der_1024);
    bytes = sizeof_client_keypub_der_1024;
#elif defined(USE_CERT_BUFFERS_2048)
    XMEMCPY(tmp, client_keypub_der_2048, sizeof_client_keypub_der_2048);
    bytes = sizeof_client_keypub_der_2048;
#else
    file = fopen(clientKeyPub, "rb");
    if (!file) {
        err_sys("can't open ./certs/client-keyPub.der, "
                "Please run from wolfSSL home dir", -40);
        ERROR_OUT(-5541, exit_rsa);
    }

    bytes = fread(tmp, 1, FOURK_BUF, file);
    fclose(file);
#endif /* USE_CERT_BUFFERS */

    ret = wc_InitRsaKey(&keypub, HEAP_HINT);
    if (ret != 0) {
        ERROR_OUT(-5542, exit_rsa);
    }
    idx = 0;

    ret = wc_RsaPublicKeyDecode(tmp, &idx, &keypub, (word32)bytes);
    if (ret != 0) {
        ERROR_OUT(-5543, exit_rsa);
    }
#endif /* WOLFSSL_CERT_EXT */

#ifdef WOLFSSL_KEY_GEN
    {
        int    derSz = 0;
        ret = wc_InitRsaKey(&genKey, HEAP_HINT);
        if (ret != 0) {
            ERROR_OUT(-5550, exit_rsa);
        }
        ret = wc_MakeRsaKey(&genKey, 1024, 65537, &rng);
        if (ret != 0) {
            ERROR_OUT(-5551, exit_rsa);
        }

        der = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (der == NULL) {
            ERROR_OUT(-5552, exit_rsa);
        }
        pem = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (pem == NULL) {
            ERROR_OUT(-5553, exit_rsa);
        }

        derSz = wc_RsaKeyToDer(&genKey, der, FOURK_BUF);
        if (derSz < 0) {
            ERROR_OUT(-5554, exit_rsa);
        }

        ret = SaveDerAndPem(der, derSz, pem, FOURK_BUF, keyDerFile, keyPemFile,
            PRIVATEKEY_TYPE, -5555);
        if (ret != 0) {
            goto exit_rsa;
        }

        wc_FreeRsaKey(&genKey);
        ret = wc_InitRsaKey(&genKey, HEAP_HINT);
        if (ret != 0) {
            ERROR_OUT(-5560, exit_rsa);
        }
        idx = 0;
        ret = wc_RsaPrivateKeyDecode(der, &idx, &genKey, derSz);
        if (ret != 0) {
            ERROR_OUT(-5561, exit_rsa);
        }

        wc_FreeRsaKey(&genKey);
        XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        pem = NULL;
        XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        der = NULL;
    }
#endif /* WOLFSSL_KEY_GEN */

#ifdef WOLFSSL_CERT_GEN
    /* self signed */
    {
        Cert        myCert;
        const byte  mySerial[8] = {1,2,3,4,5,6,7,8};
        int         certSz;
    #ifdef WOLFSSL_TEST_CERT
        DecodedCert decode;
    #endif

        der = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (der == NULL) {
            ERROR_OUT(-5570, exit_rsa);
        }
        pem = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT,DYNAMIC_TYPE_TMP_BUFFER);
        if (pem == NULL) {
            ERROR_OUT(-5571, exit_rsa);
        }

        if (wc_InitCert(&myCert)) {
            ERROR_OUT(-5572, exit_rsa);
        }

        XMEMCPY(&myCert.subject, &certDefaultName, sizeof(CertName));
        XMEMCPY(myCert.serial, mySerial, sizeof(mySerial));
        myCert.serialSz = (int)sizeof(mySerial);
        myCert.isCA    = 1;
    #ifndef NO_SHA256
        myCert.sigType = CTC_SHA256wRSA;
    #else
        myCert.sigType = CTC_SHAwRSA;
    #endif


    #ifdef WOLFSSL_CERT_EXT
        /* add Policies */
        XSTRNCPY(myCert.certPolicies[0], "2.16.840.1.101.3.4.1.42",
                CTC_MAX_CERTPOL_SZ);
        XSTRNCPY(myCert.certPolicies[1], "1.2.840.113549.1.9.16.6.5",
                CTC_MAX_CERTPOL_SZ);
        myCert.certPoliciesNb = 2;

        /* add SKID from the Public Key */
        if (wc_SetSubjectKeyIdFromPublicKey(&myCert, &keypub, NULL) != 0) {
            ERROR_OUT(-5573, exit_rsa);
        }

         /* add AKID from the Public Key */
         if (wc_SetAuthKeyIdFromPublicKey(&myCert, &keypub, NULL) != 0) {
            ERROR_OUT(-5574, exit_rsa);
        }

        /* add Key Usage */
        if (wc_SetKeyUsage(&myCert,"cRLSign,keyCertSign") != 0) {
            ERROR_OUT(-5575, exit_rsa);
        }
    #endif /* WOLFSSL_CERT_EXT */

        ret = 0;
        do {
    #if defined(WOLFSSL_ASYNC_CRYPT)
            ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
    #endif
            if (ret >= 0) {
                ret = wc_MakeSelfCert(&myCert, der, FOURK_BUF, &key, &rng);
            }
        } while (ret == WC_PENDING_E);
        if (ret < 0) {
            ERROR_OUT(-5576, exit_rsa);
        }
        certSz = ret;

    #ifdef WOLFSSL_TEST_CERT
        InitDecodedCert(&decode, der, certSz, HEAP_HINT);
        ret = ParseCert(&decode, CERT_TYPE, NO_VERIFY, 0);
        if (ret != 0) {
            FreeDecodedCert(&decode);
            ERROR_OUT(-5577, exit_rsa);
        }
        FreeDecodedCert(&decode);
    #endif

        ret = SaveDerAndPem(der, certSz, pem, FOURK_BUF, certDerFile,
            certPemFile, CERT_TYPE, -5578);
        if (ret != 0) {
            goto exit_rsa;
        }

        XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        pem = NULL;
        XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        der = NULL;
    }
    /* Make Cert / Sign example for RSA cert and RSA CA */
    {
        Cert        myCert;
        int         certSz;
        size_t      bytes3;
        word32      idx3 = 0;
    #if !defined(USE_CERT_BUFFERS_1024) && !defined(USE_CERT_BUFFERS_2048)
        FILE*       file3;
    #endif
    #ifdef WOLFSSL_TEST_CERT
        DecodedCert decode;
    #endif

        der = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (der == NULL) {
            ERROR_OUT(-5580, exit_rsa);
        }
        pem = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT,DYNAMIC_TYPE_TMP_BUFFER);
        if (pem == NULL) {
            ERROR_OUT(-5581, exit_rsa);
        }

        /* Get CA Key */
    #ifdef USE_CERT_BUFFERS_1024
        XMEMCPY(tmp, ca_key_der_1024, sizeof_ca_key_der_1024);
        bytes3 = sizeof_ca_key_der_1024;
    #elif defined(USE_CERT_BUFFERS_2048)
        XMEMCPY(tmp, ca_key_der_2048, sizeof_ca_key_der_2048);
        bytes3 = sizeof_ca_key_der_2048;
    #else
        file3 = fopen(rsaCaKeyFile, "rb");
        if (!file3) {
            ERROR_OUT(-5582, exit_rsa);
        }

        bytes3 = fread(tmp, 1, FOURK_BUF, file3);
        fclose(file3);
    #endif /* USE_CERT_BUFFERS */

        ret = wc_InitRsaKey(&caKey, HEAP_HINT);
        if (ret != 0) {
            ERROR_OUT(-5583, exit_rsa);
        }
        ret = wc_RsaPrivateKeyDecode(tmp, &idx3, &caKey, (word32)bytes3);
        if (ret != 0) {
            ERROR_OUT(-5584, exit_rsa);
        }

        /* Setup Certificate */
        if (wc_InitCert(&myCert)) {
            ERROR_OUT(-5585, exit_rsa);
        }

    #ifndef NO_SHA256
        myCert.sigType = CTC_SHA256wRSA;
    #else
        myCert.sigType = CTC_SHAwRSA;
    #endif

        XMEMCPY(&myCert.subject, &certDefaultName, sizeof(CertName));

    #ifdef WOLFSSL_CERT_EXT
        /* add Policies */
        XSTRNCPY(myCert.certPolicies[0], "2.16.840.1.101.3.4.1.42",
                CTC_MAX_CERTPOL_SZ);
        myCert.certPoliciesNb =1;

        /* add SKID from the Public Key */
        if (wc_SetSubjectKeyIdFromPublicKey(&myCert, &key, NULL) != 0) {
            ERROR_OUT(-5586, exit_rsa);
        }

        /* add AKID from the CA certificate */
    #if defined(USE_CERT_BUFFERS_2048)
        ret = wc_SetAuthKeyIdFromCert(&myCert, ca_cert_der_2048,
                                            sizeof_ca_cert_der_2048);
    #elif defined(USE_CERT_BUFFERS_1024)
        ret = wc_SetAuthKeyIdFromCert(&myCert, ca_cert_der_1024,
                                            sizeof_ca_cert_der_1024);
    #else
        ret = wc_SetAuthKeyId(&myCert, rsaCaCertFile);
    #endif
        if (ret != 0) {
            ERROR_OUT(-5587, exit_rsa);
        }

        /* add Key Usage */
        if (wc_SetKeyUsage(&myCert,"keyEncipherment,keyAgreement") != 0) {
            ERROR_OUT(-5588, exit_rsa);
        }
    #endif /* WOLFSSL_CERT_EXT */

    #if defined(USE_CERT_BUFFERS_2048)
        ret = wc_SetIssuerBuffer(&myCert, ca_cert_der_2048,
                                          sizeof_ca_cert_der_2048);
    #elif defined(USE_CERT_BUFFERS_1024)
        ret = wc_SetIssuerBuffer(&myCert, ca_cert_der_1024,
                                          sizeof_ca_cert_der_1024);
    #else
        ret = wc_SetIssuer(&myCert, rsaCaCertFile);
    #endif
        if (ret < 0) {
            ERROR_OUT(-5589, exit_rsa);
        }

        certSz = wc_MakeCert(&myCert, der, FOURK_BUF, &key, NULL, &rng);
        if (certSz < 0) {
            ERROR_OUT(-5590, exit_rsa);
        }

        ret = 0;
        do {
        #if defined(WOLFSSL_ASYNC_CRYPT)
            ret = wc_AsyncWait(ret, &caKey.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
        #endif
            if (ret >= 0) {
                ret = wc_SignCert(myCert.bodySz, myCert.sigType, der, FOURK_BUF,
                          &caKey, NULL, &rng);
            }
        } while (ret == WC_PENDING_E);
        if (ret < 0) {
            ERROR_OUT(-5591, exit_rsa);
        }
        certSz = ret;

    #ifdef WOLFSSL_TEST_CERT
        InitDecodedCert(&decode, der, certSz, HEAP_HINT);
        ret = ParseCert(&decode, CERT_TYPE, NO_VERIFY, 0);
        if (ret != 0) {
            FreeDecodedCert(&decode);
            ERROR_OUT(-5592, exit_rsa);
        }
        FreeDecodedCert(&decode);
    #endif

        ret = SaveDerAndPem(der, certSz, pem, FOURK_BUF, otherCertDerFile,
            otherCertPemFile, CERT_TYPE, -5593);
        if (ret != 0) {
            goto exit_rsa;
        }

        wc_FreeRsaKey(&caKey);

        XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        pem = NULL;
        XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        der = NULL;
    }
#if !defined(NO_RSA) && defined(HAVE_ECC)
    /* Make Cert / Sign example for ECC cert and RSA CA */
    {
        Cert        myCert;
        int         certSz;
        size_t      bytes3;
        word32      idx3 = 0;
    #if !defined(USE_CERT_BUFFERS_1024) && !defined(USE_CERT_BUFFERS_2048)
        FILE*       file3;
    #endif
    #ifdef WOLFSSL_TEST_CERT
        DecodedCert decode;
    #endif

        der = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (der == NULL) {
            ERROR_OUT(-5600, exit_rsa);
        }
        pem = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT,DYNAMIC_TYPE_TMP_BUFFER);
        if (pem == NULL) {
            ERROR_OUT(-5601, exit_rsa);
        }

        /* Get CA Key */
    #ifdef USE_CERT_BUFFERS_1024
        XMEMCPY(tmp, ca_key_der_1024, sizeof_ca_key_der_1024);
        bytes3 = sizeof_ca_key_der_1024;
    #elif defined(USE_CERT_BUFFERS_2048)
        XMEMCPY(tmp, ca_key_der_2048, sizeof_ca_key_der_2048);
        bytes3 = sizeof_ca_key_der_2048;
    #else
        file3 = fopen(rsaCaKeyFile, "rb");
        if (!file3) {
            ERROR_OUT(-5602, exit_rsa);
        }

        bytes3 = fread(tmp, 1, FOURK_BUF, file3);
        fclose(file3);
    #endif /* USE_CERT_BUFFERS */

        ret = wc_InitRsaKey(&caKey, HEAP_HINT);
        if (ret != 0) {
            ERROR_OUT(-5603, exit_rsa);
        }
        ret = wc_RsaPrivateKeyDecode(tmp, &idx3, &caKey, (word32)bytes3);
        if (ret != 0) {
            ERROR_OUT(-5604, exit_rsa);
        }

        /* Get Cert Key */
    #ifdef USE_CERT_BUFFERS_256
        XMEMCPY(tmp, ecc_key_pub_der_256, sizeof_ecc_key_pub_der_256);
        bytes3 = sizeof_ecc_key_pub_der_256;
    #else
        file3 = fopen(eccKeyPubFile, "rb");
        if (!file3) {
            ERROR_OUT(-5605, exit_rsa);
        }

        bytes3 = fread(tmp, 1, FOURK_BUF, file3);
        fclose(file3);
    #endif

        ret = wc_ecc_init_ex(&caEccKeyPub, HEAP_HINT, devId);
        if (ret != 0) {
            ERROR_OUT(-5606, exit_rsa);
        }

        idx3 = 0;
        ret = wc_EccPublicKeyDecode(tmp, &idx3, &caEccKeyPub, (word32)bytes3);
        if (ret != 0) {
            ERROR_OUT(-5607, exit_rsa);
        }

        /* Setup Certificate */
        if (wc_InitCert(&myCert)) {
            ERROR_OUT(-5608, exit_rsa);
        }

    #ifndef NO_SHA256
        myCert.sigType = CTC_SHA256wRSA;
    #else
        myCert.sigType = CTC_SHAwRSA;
    #endif

        XMEMCPY(&myCert.subject, &certDefaultName, sizeof(CertName));

#ifdef WOLFSSL_CERT_EXT
        /* add Policies */
        XSTRNCPY(myCert.certPolicies[0], "2.4.589440.587.101.2.1.9632587.1",
                CTC_MAX_CERTPOL_SZ);
        XSTRNCPY(myCert.certPolicies[1], "1.2.13025.489.1.113549",
                CTC_MAX_CERTPOL_SZ);
        myCert.certPoliciesNb = 2;

        /* add SKID from the Public Key */
        if (wc_SetSubjectKeyIdFromPublicKey(&myCert, NULL, &caEccKeyPub) != 0) {
            ERROR_OUT(-5609, exit_rsa);
        }

        /* add AKID from the CA certificate */
    #if defined(USE_CERT_BUFFERS_2048)
        ret = wc_SetAuthKeyIdFromCert(&myCert, ca_cert_der_2048,
                                               sizeof_ca_cert_der_2048);
    #elif defined(USE_CERT_BUFFERS_1024)
        ret = wc_SetAuthKeyIdFromCert(&myCert, ca_cert_der_1024,
                                               sizeof_ca_cert_der_1024);
    #else
        ret = wc_SetAuthKeyId(&myCert, rsaCaCertFile);
    #endif
        if (ret != 0) {
            ERROR_OUT(-5610, exit_rsa);
        }

        /* add Key Usage */
        if (wc_SetKeyUsage(&myCert, certKeyUsage) != 0) {
            ERROR_OUT(-5611, exit_rsa);
        }
#endif /* WOLFSSL_CERT_EXT */

    #if defined(USE_CERT_BUFFERS_2048)
        ret = wc_SetIssuerBuffer(&myCert, ca_cert_der_2048,
                                          sizeof_ca_cert_der_2048);
    #elif defined(USE_CERT_BUFFERS_1024)
        ret = wc_SetIssuerBuffer(&myCert, ca_cert_der_1024,
                                          sizeof_ca_cert_der_1024);
    #else
        ret = wc_SetIssuer(&myCert, rsaCaCertFile);
    #endif
        if (ret < 0) {
            ERROR_OUT(-5612, exit_rsa);
        }

        certSz = wc_MakeCert(&myCert, der, FOURK_BUF, NULL, &caEccKeyPub, &rng);
        if (certSz < 0) {
            ERROR_OUT(-5613, exit_rsa);
        }

        ret = 0;
        do {
        #if defined(WOLFSSL_ASYNC_CRYPT)
            ret = wc_AsyncWait(ret, &caEccKey.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
        #endif
            if (ret >= 0) {
                ret = wc_SignCert(myCert.bodySz, myCert.sigType, der,
                                  FOURK_BUF, &caKey, NULL, &rng);
            }
        } while (ret == WC_PENDING_E);
        if (ret < 0) {
            ERROR_OUT(-5614, exit_rsa);
        }
        certSz = ret;

    #ifdef WOLFSSL_TEST_CERT
        InitDecodedCert(&decode, der, certSz, 0);
        ret = ParseCert(&decode, CERT_TYPE, NO_VERIFY, 0);
        if (ret != 0) {
            FreeDecodedCert(&decode);
            ERROR_OUT(-5615, exit_rsa);

        }
        FreeDecodedCert(&decode);
    #endif

        ret = SaveDerAndPem(der, certSz, pem, FOURK_BUF, certEccRsaDerFile,
            certEccRsaPemFile, CERT_TYPE, -5616);
        if (ret != 0) {
            goto exit_rsa;
        }

        wc_ecc_free(&caEccKeyPub);
        wc_FreeRsaKey(&caKey);

        XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        pem = NULL;
        XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        der = NULL;
    }
#endif /* !NO_RSA && HAVE_ECC */
#ifdef HAVE_NTRU
    {
        Cert        myCert;
    #if !defined(USE_CERT_BUFFERS_1024) && !defined(USE_CERT_BUFFERS_2048)
        FILE*       caFile;
    #endif
        FILE*       ntruPrivFile;
        int         certSz;
        word32      idx3 = 0;
    #ifdef WOLFSSL_TEST_CERT
        DecodedCert decode;
    #endif
        byte   public_key[557];          /* sized for EES401EP2 */
        word16 public_key_len;           /* no. of octets in public key */
        byte   private_key[607];         /* sized for EES401EP2 */
        word16 private_key_len;          /* no. of octets in private key */
        DRBG_HANDLE drbg;
        static uint8_t const pers_str[] = {
                'C', 'y', 'a', 'S', 'S', 'L', ' ', 't', 'e', 's', 't'
        };
        word32 rc = ntru_crypto_drbg_instantiate(112, pers_str,
                          sizeof(pers_str), GetEntropy, &drbg);
        if (rc != DRBG_OK) {
            ERROR_OUT(-5620, exit_rsa);
        }

        rc = ntru_crypto_ntru_encrypt_keygen(drbg, NTRU_EES401EP2,
                                             &public_key_len, NULL,
                                             &private_key_len, NULL);
        if (rc != NTRU_OK) {
            ERROR_OUT(-5621, exit_rsa);
        }

        rc = ntru_crypto_ntru_encrypt_keygen(drbg, NTRU_EES401EP2,
                                             &public_key_len, public_key,
                                             &private_key_len, private_key);
        if (rc != NTRU_OK) {
            ERROR_OUT(-5622, exit_rsa);
        }

        rc = ntru_crypto_drbg_uninstantiate(drbg);
        if (rc != NTRU_OK) {
            ERROR_OUT(-5623, exit_rsa);
        }

    #ifdef USE_CERT_BUFFERS_1024
        XMEMCPY(tmp, ca_key_der_1024, sizeof_ca_key_der_1024);
        bytes = sizeof_ca_key_der_1024;
    #elif defined(USE_CERT_BUFFERS_2048)
        XMEMCPY(tmp, ca_key_der_2048, sizeof_ca_key_der_2048);
        bytes = sizeof_ca_key_der_2048;
    #else
        caFile = fopen(rsaCaKeyFile, "rb");
        if (!caFile) {
            ERROR_OUT(-5624, exit_rsa);
        }

        bytes = fread(tmp, 1, FOURK_BUF, caFile);
        fclose(caFile);
    #endif /* USE_CERT_BUFFERS */

        ret = wc_InitRsaKey(&caKey, HEAP_HINT);
        if (ret != 0) {
            ERROR_OUT(-5625, exit_rsa);
        }
        ret = wc_RsaPrivateKeyDecode(tmp, &idx3, &caKey, (word32)bytes);
        if (ret != 0) {
            ERROR_OUT(-5626, exit_rsa);
        }

        if (wc_InitCert(&myCert)) {
            ERROR_OUT(-5627, exit_rsa);
        }

        XMEMCPY(&myCert.subject, &certDefaultName, sizeof(CertName));
        myCert.daysValid = 1000;

    #ifdef WOLFSSL_CERT_EXT
        /* add SKID from the Public Key */
        if (wc_SetSubjectKeyIdFromNtruPublicKey(&myCert, public_key,
                                                public_key_len) != 0) {
            ERROR_OUT(-5628, exit_rsa);
        }

        /* add AKID from the CA certificate */
    #if defined(USE_CERT_BUFFERS_2048)
        ret = wc_SetAuthKeyIdFromCert(&myCert, ca_cert_der_2048,
                                            sizeof_ca_cert_der_2048);
    #elif defined(USE_CERT_BUFFERS_1024)
        ret = wc_SetAuthKeyIdFromCert(&myCert, ca_cert_der_1024,
                                            sizeof_ca_cert_der_1024);
    #else
        ret = wc_SetAuthKeyId(&myCert, rsaCaCertFile);
    #endif
        if (ret != 0) {
            ERROR_OUT(-5629, exit_rsa);
        }

        /* add Key Usage */
        if (wc_SetKeyUsage(&myCert, certKeyUsage2) != 0) {
            ERROR_OUT(-5630, exit_rsa);
        }
    #endif /* WOLFSSL_CERT_EXT */

    #if defined(USE_CERT_BUFFERS_2048)
        ret = wc_SetIssuerBuffer(&myCert, ca_cert_der_2048,
                                          sizeof_ca_cert_der_2048);
    #elif defined(USE_CERT_BUFFERS_1024)
        ret = wc_SetIssuerBuffer(&myCert, ca_cert_der_1024,
                                          sizeof_ca_cert_der_1024);
    #else
        ret = wc_SetIssuer(&myCert, rsaCaCertFile);
    #endif
        if (ret < 0) {
            ERROR_OUT(-5631, exit_rsa);
        }

        der = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (der == NULL) {
            ERROR_OUT(-5632, exit_rsa);
        }
        pem = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT,DYNAMIC_TYPE_TMP_BUFFER);
        if (pem == NULL) {
            ERROR_OUT(-5633, exit_rsa);
        }

        certSz = wc_MakeNtruCert(&myCert, der, FOURK_BUF, public_key,
                              public_key_len, &rng);
        if (certSz < 0) {
            ERROR_OUT(-5634, exit_rsa);
        }

        ret = 0;
        do {
        #if defined(WOLFSSL_ASYNC_CRYPT)
            ret = wc_AsyncWait(ret, &caKey.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
        #endif
            if (ret >= 0) {
                ret = wc_SignCert(myCert.bodySz, myCert.sigType, der, FOURK_BUF,
                          &caKey, NULL, &rng);
            }
        } while (ret == WC_PENDING_E);
        wc_FreeRsaKey(&caKey);
        if (ret < 0) {
            ERROR_OUT(-5635, exit_rsa);
        }
        certSz = ret;

    #ifdef WOLFSSL_TEST_CERT
        InitDecodedCert(&decode, der, certSz, HEAP_HINT);
        ret = ParseCert(&decode, CERT_TYPE, NO_VERIFY, 0);
        if (ret != 0) {
            FreeDecodedCert(&decode);
            ERROR_OUT(-5636, exit_rsa);
        }
        FreeDecodedCert(&decode);
    #endif

        ret = SaveDerAndPem(der, certSz, pem, FOURK_BUF, "./ntru-cert.der",
            "./ntru-cert.pem", CERT_TYPE, -5637);
        if (ret != 0) {
            goto exit_rsa;
        }

    #if !defined(NO_FILESYSTEM) && !defined(NO_WRITE_TEMP_FILES)
        ntruPrivFile = fopen("./ntru-key.raw", "wb");
        if (!ntruPrivFile) {
            ERROR_OUT(-5638, exit_rsa);
        }
        ret = (int)fwrite(private_key, 1, private_key_len, ntruPrivFile);
        fclose(ntruPrivFile);
        if (ret != private_key_len) {
            ERROR_OUT(-5639, exit_rsa);
        }
    #endif

        XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        pem = NULL;
        XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        der = NULL;
    }
#endif /* HAVE_NTRU */
#ifdef WOLFSSL_CERT_REQ
    {
        Cert        req;
        int         derSz;

        der = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT,DYNAMIC_TYPE_TMP_BUFFER);
        if (der == NULL) {
            ERROR_OUT(-5640, exit_rsa);
        }
        pem = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT,DYNAMIC_TYPE_TMP_BUFFER);
        if (pem == NULL) {
            ERROR_OUT(-5641, exit_rsa);
        }

        if (wc_InitCert(&req)) {
            ERROR_OUT(-5642, exit_rsa);
        }

        req.version = 0;
        req.isCA    = 1;
        XSTRNCPY(req.challengePw, "wolf123", CTC_NAME_SIZE);
        XMEMCPY(&req.subject, &certDefaultName, sizeof(CertName));

    #ifndef NO_SHA256
        req.sigType = CTC_SHA256wRSA;
    #else
        req.sigType = CTC_SHAwRSA;
    #endif

    #ifdef WOLFSSL_CERT_EXT
        /* add SKID from the Public Key */
        if (wc_SetSubjectKeyIdFromPublicKey(&req, &keypub, NULL) != 0) {
            ERROR_OUT(-5643, exit_rsa);
        }

        /* add Key Usage */
        if (wc_SetKeyUsage(&req, certKeyUsage2) != 0) {
            ERROR_OUT(-5644, exit_rsa);
        }

        /* add Extended Key Usage */
        if (wc_SetExtKeyUsage(&req, "serverAuth,clientAuth,codeSigning,"
                            "emailProtection,timeStamping,OCSPSigning") != 0) {
            ERROR_OUT(-5645, exit_rsa);
        }
    #endif /* WOLFSSL_CERT_EXT */

        derSz = wc_MakeCertReq(&req, der, FOURK_BUF, &key, NULL);
        if (derSz < 0) {
            ERROR_OUT(-5646, exit_rsa);
        }

    #ifdef WOLFSSL_CERT_EXT
        /* Try again with "any" flag set, will override all others */
        if (wc_SetExtKeyUsage(&req, "any") != 0) {
            ERROR_OUT(-5647, exit_rsa);
        }
        derSz = wc_MakeCertReq(&req, der, FOURK_BUF, &key, NULL);
        if (derSz < 0) {
            ERROR_OUT(-5648, exit_rsa);
        }
    #endif /* WOLFSSL_CERT_EXT */

        ret = 0;
        do {
        #if defined(WOLFSSL_ASYNC_CRYPT)
            ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
        #endif
            if (ret >= 0) {
                ret = wc_SignCert(req.bodySz, req.sigType, der, FOURK_BUF,
                          &key, NULL, &rng);
            }
        } while (ret == WC_PENDING_E);
        if (ret < 0) {
            ERROR_OUT(-5649, exit_rsa);
        }
        derSz = ret;

        ret = SaveDerAndPem(der, derSz, pem, FOURK_BUF, certReqDerFile,
            certReqPemFile, CERTREQ_TYPE, -5650);
        if (ret != 0) {
            goto exit_rsa;
        }

        XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        pem = NULL;
        XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        der = NULL;
    }
#endif /* WOLFSSL_CERT_REQ */
#endif /* WOLFSSL_CERT_GEN */

exit_rsa:
    wc_FreeRsaKey(&key);
#ifdef WOLFSSL_CERT_EXT
    wc_FreeRsaKey(&keypub);
#endif
#ifdef WOLFSSL_KEY_GEN
    wc_FreeRsaKey(&genKey);
#endif
#ifdef WOLFSSL_CERT_GEN
    wc_FreeRsaKey(&caKey);
    #ifdef HAVE_ECC
        wc_ecc_free(&caEccKey);
        #ifdef WOLFSSL_CERT_EXT
            wc_ecc_free(&caEccKeyPub);
        #endif
    #endif
#endif

    XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(tmp, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    wc_FreeRng(&rng);

    FREE_VAR(in, HEAP_HINT);
    FREE_VAR(out, HEAP_HINT);
    FREE_VAR(plain, HEAP_HINT);

    /* ret can be greater then 0 with certgen but all negative values should
     * be returned and treated as an error */
    if (ret >= 0) {
        return 0;
    }
    else {
        return ret;
    }
}

#endif


#ifndef NO_DH

static int dh_generate_test(WC_RNG *rng)
{
    int    ret = 0;
    DhKey  smallKey;
    byte   p[2] = { 0, 5 };
    byte   g[2] = { 0, 2 };
#ifdef WOLFSSL_DH_CONST
    /* the table for constant DH lookup will round to the lowest byte size 21 */
    byte   priv[21];
    byte   pub[21];
#else
    byte   priv[2];
    byte   pub[2];
#endif
    word32 privSz = sizeof(priv);
    word32 pubSz = sizeof(pub);

    ret = wc_InitDhKey_ex(&smallKey, HEAP_HINT, devId);
    if (ret != 0)
        return -5700;

    /* Parameter Validation testing. */
    ret = wc_DhSetKey(NULL, p, sizeof(p), g, sizeof(g));
    if (ret != BAD_FUNC_ARG) {
        ERROR_OUT(-5701, exit_gen_test);
    }
    ret = wc_DhSetKey(&smallKey, NULL, sizeof(p), g, sizeof(g));
    if (ret != BAD_FUNC_ARG) {
        ERROR_OUT(-5702, exit_gen_test);
    }
    ret = wc_DhSetKey(&smallKey, p, 0, g, sizeof(g));
    if (ret != BAD_FUNC_ARG) {
        ERROR_OUT(-5703, exit_gen_test);
    }
    ret = wc_DhSetKey(&smallKey, p, sizeof(p), NULL, sizeof(g));
    if (ret != BAD_FUNC_ARG) {
        ERROR_OUT(-5704, exit_gen_test);
    }
    ret = wc_DhSetKey(&smallKey, p, sizeof(p), g, 0);
    if (ret != BAD_FUNC_ARG) {
        ERROR_OUT(-5705, exit_gen_test);
    }
    ret = wc_DhSetKey(&smallKey, p, sizeof(p), g, sizeof(g));
    if (ret != 0) {
        ERROR_OUT(-5706, exit_gen_test);
    }

    /* Use API. */
    ret = wc_DhGenerateKeyPair(&smallKey, rng, priv, &privSz, pub, &pubSz);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &smallKey.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0) {
        ret = -5707;
    }

exit_gen_test:
    wc_FreeDhKey(&smallKey);

    return ret;
}

int dh_test(void)
{
    int    ret;
    word32 bytes;
    word32 idx = 0, privSz, pubSz, privSz2, pubSz2, agreeSz, agreeSz2;
    byte   tmp[1024];
    byte   priv[256];
    byte   pub[256];
    byte   priv2[256];
    byte   pub2[256];
    byte   agree[256];
    byte   agree2[256];
    DhKey  key;
    DhKey  key2;
    WC_RNG rng;

#ifdef USE_CERT_BUFFERS_1024
    XMEMCPY(tmp, dh_key_der_1024, (size_t)sizeof_dh_key_der_1024);
    bytes = (size_t)sizeof_dh_key_der_1024;
#elif defined(USE_CERT_BUFFERS_2048)
    XMEMCPY(tmp, dh_key_der_2048, (size_t)sizeof_dh_key_der_2048);
    bytes = (size_t)sizeof_dh_key_der_2048;
#elif defined(NO_ASN)
    /* don't use file, no DER parsing */
#elif !defined(NO_FILESYSTEM)
    FILE*  file = fopen(dhKey, "rb");
    if (!file)
        return -5710;

    bytes = (word32) fread(tmp, 1, sizeof(tmp), file);
    fclose(file);
#else
    /* No DH key to use. */
    return -5711;
#endif /* USE_CERT_BUFFERS */

    (void)idx;
    (void)tmp;
    (void)bytes;

    ret = wc_InitDhKey_ex(&key, HEAP_HINT, devId);
    if (ret != 0) {
        ERROR_OUT(-5712, done);
    }
    ret = wc_InitDhKey_ex(&key2, HEAP_HINT, devId);
    if (ret != 0) {
        ERROR_OUT(-5713, done);
    }

#ifdef NO_ASN
    ret = wc_DhSetKey(&key, dh_p, sizeof(dh_p), dh_g, sizeof(dh_g));
    if (ret != 0) {
        ERROR_OUT(-5714, done);
    }

    ret = wc_DhSetKey(&key2, dh_p, sizeof(dh_p), dh_g, sizeof(dh_g));
    if (ret != 0) {
        ERROR_OUT(-5715, done);
    }
#else
    ret = wc_DhKeyDecode(tmp, &idx, &key, bytes);
    if (ret != 0) {
        ERROR_OUT(-5716, done);
    }

    idx = 0;
    ret = wc_DhKeyDecode(tmp, &idx, &key2, bytes);
    if (ret != 0) {
        ERROR_OUT(-5717, done);
    }
#endif

#ifndef HAVE_FIPS
    ret = wc_InitRng_ex(&rng, HEAP_HINT, devId);
#else
    ret = wc_InitRng(&rng);
#endif
    if (ret != 0) {
        ERROR_OUT(-5718, done);
    }

    ret = wc_DhGenerateKeyPair(&key, &rng, priv, &privSz, pub, &pubSz);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0) {
        ERROR_OUT(-5719, done);
    }

    ret = wc_DhGenerateKeyPair(&key2, &rng, priv2, &privSz2, pub2, &pubSz2);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &key2.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0) {
        ERROR_OUT(-5720, done);
    }

    ret = wc_DhAgree(&key, agree, &agreeSz, priv, privSz, pub2, pubSz2);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &key.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0) {
        ERROR_OUT(-5721, done);
    }

    ret = wc_DhAgree(&key2, agree2, &agreeSz2, priv2, privSz2, pub, pubSz);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &key2.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0) {
        ERROR_OUT(-5722, done);
    }

    if (agreeSz != agreeSz2 || XMEMCMP(agree, agree2, agreeSz)) {
        ERROR_OUT(-5723, done);
    }

    ret = dh_generate_test(&rng);

done:

    wc_FreeDhKey(&key);
    wc_FreeDhKey(&key2);
    wc_FreeRng(&rng);

    return ret;
}

#endif /* NO_DH */


#ifndef NO_DSA

int dsa_test(void)
{
    int    ret, answer;
    word32 bytes;
    word32 idx = 0;
    byte   tmp[1024];
    DsaKey key;
    WC_RNG rng;
    wc_Sha sha;
    byte   hash[WC_SHA_DIGEST_SIZE];
    byte   signature[40];

#ifdef USE_CERT_BUFFERS_1024
    XMEMCPY(tmp, dsa_key_der_1024, sizeof_dsa_key_der_1024);
    bytes = sizeof_dsa_key_der_1024;
#elif defined(USE_CERT_BUFFERS_2048)
    XMEMCPY(tmp, dsa_key_der_2048, sizeof_dsa_key_der_2048);
    bytes = sizeof_dsa_key_der_2048;
#else
    FILE*  file = fopen(dsaKey, "rb");
    if (!file)
        return -5800;

    bytes = (word32) fread(tmp, 1, sizeof(tmp), file);
    fclose(file);
#endif /* USE_CERT_BUFFERS */

    ret = wc_InitSha_ex(&sha, HEAP_HINT, devId);
    if (ret != 0)
        return -5801;
    wc_ShaUpdate(&sha, tmp, bytes);
    wc_ShaFinal(&sha, hash);
    wc_ShaFree(&sha);

    ret = wc_InitDsaKey(&key);
    if (ret != 0) return -5802;

    ret = wc_DsaPrivateKeyDecode(tmp, &idx, &key, bytes);
    if (ret != 0) return -5803;

#ifndef HAVE_FIPS
    ret = wc_InitRng_ex(&rng, HEAP_HINT, devId);
#else
    ret = wc_InitRng(&rng);
#endif
    if (ret != 0) return -5804;

    ret = wc_DsaSign(hash, signature, &key, &rng);
    if (ret != 0) return -5805;

    ret = wc_DsaVerify(hash, signature, &key, &answer);
    if (ret != 0) return -5806;
    if (answer != 1) return -5807;

    wc_FreeDsaKey(&key);

#ifdef WOLFSSL_KEY_GEN
    {
    byte*  der;
    byte*  pem;
    int    derSz = 0;
    DsaKey derIn;
    DsaKey genKey;

    ret = wc_InitDsaKey(&genKey);
    if (ret != 0) return -5808;

    ret = wc_MakeDsaParameters(&rng, 1024, &genKey);
    if (ret != 0) {
        wc_FreeDsaKey(&genKey);
        return -5809;
    }

    ret = wc_MakeDsaKey(&rng, &genKey);
    if (ret != 0) {
        wc_FreeDsaKey(&genKey);
        return -5810;
    }

    der = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (der == NULL) {
        wc_FreeDsaKey(&genKey);
        return -5811;
    }
    pem = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (pem == NULL) {
        XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        wc_FreeDsaKey(&genKey);
        return -5812;
    }

    derSz = wc_DsaKeyToDer(&genKey, der, FOURK_BUF);
    if (derSz < 0) {
        XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return -5813;
    }

    ret = SaveDerAndPem(der, derSz, pem, FOURK_BUF, keyDerFile,
            keyPemFile, DSA_PRIVATEKEY_TYPE, -5814);
    if (ret != 0) {
        XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        wc_FreeDsaKey(&genKey);
        return ret;
    }

    ret = wc_InitDsaKey(&derIn);
    if (ret != 0) {
        XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        wc_FreeDsaKey(&genKey);
        return -5819;
    }

    idx = 0;
    ret = wc_DsaPrivateKeyDecode(der, &idx, &derIn, derSz);
    if (ret != 0) {
        XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        wc_FreeDsaKey(&derIn);
        wc_FreeDsaKey(&genKey);
        return -5820;
    }

    wc_FreeDsaKey(&derIn);
    wc_FreeDsaKey(&genKey);
    XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    }
#endif /* WOLFSSL_KEY_GEN */

    wc_FreeRng(&rng);
    return 0;
}

#endif /* NO_DSA */

#ifdef WOLFCRYPT_HAVE_SRP

static int generate_random_salt(byte *buf, word32 size)
{
    int ret = -5821;
    WC_RNG rng;

    if(NULL == buf || !size)
        return -5822;

    if (buf && size && wc_InitRng_ex(&rng, HEAP_HINT, devId) == 0) {
        ret = wc_RNG_GenerateBlock(&rng, (byte *)buf, size);

        wc_FreeRng(&rng);
    }

    return ret;
}

int srp_test(void)
{
    Srp cli, srv;
    int r;

    byte clientPubKey[80]; /* A */
    byte serverPubKey[80]; /* B */
    word32 clientPubKeySz = 80;
    word32 serverPubKeySz = 80;
    byte clientProof[SRP_MAX_DIGEST_SIZE]; /* M1 */
    byte serverProof[SRP_MAX_DIGEST_SIZE]; /* M2 */
    word32 clientProofSz = SRP_MAX_DIGEST_SIZE;
    word32 serverProofSz = SRP_MAX_DIGEST_SIZE;

    byte username[] = "user";
    word32 usernameSz = 4;

    byte password[] = "password";
    word32 passwordSz = 8;

    byte N[] = {
        0xC9, 0x4D, 0x67, 0xEB, 0x5B, 0x1A, 0x23, 0x46, 0xE8, 0xAB, 0x42, 0x2F,
        0xC6, 0xA0, 0xED, 0xAE, 0xDA, 0x8C, 0x7F, 0x89, 0x4C, 0x9E, 0xEE, 0xC4,
        0x2F, 0x9E, 0xD2, 0x50, 0xFD, 0x7F, 0x00, 0x46, 0xE5, 0xAF, 0x2C, 0xF7,
        0x3D, 0x6B, 0x2F, 0xA2, 0x6B, 0xB0, 0x80, 0x33, 0xDA, 0x4D, 0xE3, 0x22,
        0xE1, 0x44, 0xE7, 0xA8, 0xE9, 0xB1, 0x2A, 0x0E, 0x46, 0x37, 0xF6, 0x37,
        0x1F, 0x34, 0xA2, 0x07, 0x1C, 0x4B, 0x38, 0x36, 0xCB, 0xEE, 0xAB, 0x15,
        0x03, 0x44, 0x60, 0xFA, 0xA7, 0xAD, 0xF4, 0x83
    };

    byte g[] = {
        0x02
    };

    byte salt[10];

    byte verifier[80];
    word32 v_size = sizeof(verifier);

    /* set as 0's so if second init on srv not called SrpTerm is not on
     * garbage values */
    XMEMSET(&srv, 0, sizeof(Srp));
    XMEMSET(&cli, 0, sizeof(Srp));

    /* generating random salt */

    r = generate_random_salt(salt, sizeof(salt));

    /* client knows username and password.   */
    /* server knows N, g, salt and verifier. */

    if (!r) r = wc_SrpInit(&cli, SRP_TYPE_SHA, SRP_CLIENT_SIDE);
    if (!r) r = wc_SrpSetUsername(&cli, username, usernameSz);

    /* loading N, g and salt in advance to generate the verifier. */

    if (!r) r = wc_SrpSetParams(&cli, N,    sizeof(N),
                                      g,    sizeof(g),
                                      salt, sizeof(salt));
    if (!r) r = wc_SrpSetPassword(&cli, password, passwordSz);
    if (!r) r = wc_SrpGetVerifier(&cli, verifier, &v_size);

    /* client sends username to server */

    if (!r) r = wc_SrpInit(&srv, SRP_TYPE_SHA, SRP_SERVER_SIDE);
    if (!r) r = wc_SrpSetUsername(&srv, username, usernameSz);
    if (!r) r = wc_SrpSetParams(&srv, N,    sizeof(N),
                                      g,    sizeof(g),
                                      salt, sizeof(salt));
    if (!r) r = wc_SrpSetVerifier(&srv, verifier, v_size);
    if (!r) r = wc_SrpGetPublic(&srv, serverPubKey, &serverPubKeySz);

    /* server sends N, g, salt and B to client */

    if (!r) r = wc_SrpGetPublic(&cli, clientPubKey, &clientPubKeySz);
    if (!r) r = wc_SrpComputeKey(&cli, clientPubKey, clientPubKeySz,
                                       serverPubKey, serverPubKeySz);
    if (!r) r = wc_SrpGetProof(&cli, clientProof, &clientProofSz);

    /* client sends A and M1 to server */

    if (!r) r = wc_SrpComputeKey(&srv, clientPubKey, clientPubKeySz,
                                       serverPubKey, serverPubKeySz);
    if (!r) r = wc_SrpVerifyPeersProof(&srv, clientProof, clientProofSz);
    if (!r) r = wc_SrpGetProof(&srv, serverProof, &serverProofSz);

    /* server sends M2 to client */

    if (!r) r = wc_SrpVerifyPeersProof(&cli, serverProof, serverProofSz);

    wc_SrpTerm(&cli);
    wc_SrpTerm(&srv);

    return r;
}

#endif /* WOLFCRYPT_HAVE_SRP */

#if defined(OPENSSL_EXTRA) && !defined(WOLFCRYPT_ONLY)

int openssl_test(void)
{
    EVP_MD_CTX md_ctx;
    testVector a, b, c, d, e, f;
    byte       hash[WC_SHA256_DIGEST_SIZE*2];  /* max size */

    (void)a;
    (void)b;
    (void)c;
    (void)e;
    (void)f;

    /* test malloc / free , 10 is an arbitrary amount of memory chosen */
    {
        byte* p;
        p = (byte*)CRYPTO_malloc(10, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (p == NULL) {
            return -5900;
        }
        XMEMSET(p, 0, 10);
        CRYPTO_free(p, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    }

#ifndef NO_MD5

    a.input  = "1234567890123456789012345678901234567890123456789012345678"
               "9012345678901234567890";
    a.output = "\x57\xed\xf4\xa2\x2b\xe3\xc9\x55\xac\x49\xda\x2e\x21\x07\xb6"
               "\x7a";
    a.inLen  = XSTRLEN(a.input);
    a.outLen = WC_MD5_DIGEST_SIZE;

    EVP_MD_CTX_init(&md_ctx);
    EVP_DigestInit(&md_ctx, EVP_md5());

    EVP_DigestUpdate(&md_ctx, a.input, (unsigned long)a.inLen);
    EVP_DigestFinal(&md_ctx, hash, 0);

    if (XMEMCMP(hash, a.output, WC_MD5_DIGEST_SIZE) != 0)
        return -5901;

#endif /* NO_MD5 */

#ifndef NO_SHA

    b.input  = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
               "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
               "aaaaaaaaaa";
    b.output = "\xAD\x5B\x3F\xDB\xCB\x52\x67\x78\xC2\x83\x9D\x2F\x15\x1E\xA7"
               "\x53\x99\x5E\x26\xA0";
    b.inLen  = XSTRLEN(b.input);
    b.outLen = WC_SHA_DIGEST_SIZE;

    EVP_MD_CTX_init(&md_ctx);
    EVP_DigestInit(&md_ctx, EVP_sha1());

    EVP_DigestUpdate(&md_ctx, b.input, (unsigned long)b.inLen);
    EVP_DigestFinal(&md_ctx, hash, 0);

    if (XMEMCMP(hash, b.output, WC_SHA_DIGEST_SIZE) != 0)
        return -5902;

#endif /* NO_SHA */

#ifdef WOLFSSL_SHA224

    e.input  = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhi"
               "jklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    e.output = "\xc9\x7c\xa9\xa5\x59\x85\x0c\xe9\x7a\x04\xa9\x6d\xef\x6d\x99"
               "\xa9\xe0\xe0\xe2\xab\x14\xe6\xb8\xdf\x26\x5f\xc0\xb3";
    e.inLen  = XSTRLEN(e.input);
    e.outLen = WC_SHA224_DIGEST_SIZE;

    EVP_MD_CTX_init(&md_ctx);
    EVP_DigestInit(&md_ctx, EVP_sha224());

    EVP_DigestUpdate(&md_ctx, e.input, (unsigned long)e.inLen);
    EVP_DigestFinal(&md_ctx, hash, 0);

    if (XMEMCMP(hash, e.output, WC_SHA224_DIGEST_SIZE) != 0)
        return -5903;

#endif /* WOLFSSL_SHA224 */


    d.input  = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    d.output = "\x24\x8D\x6A\x61\xD2\x06\x38\xB8\xE5\xC0\x26\x93\x0C\x3E\x60"
               "\x39\xA3\x3C\xE4\x59\x64\xFF\x21\x67\xF6\xEC\xED\xD4\x19\xDB"
               "\x06\xC1";
    d.inLen  = XSTRLEN(d.input);
    d.outLen = WC_SHA256_DIGEST_SIZE;

    EVP_MD_CTX_init(&md_ctx);
    EVP_DigestInit(&md_ctx, EVP_sha256());

    EVP_DigestUpdate(&md_ctx, d.input, (unsigned long)d.inLen);
    EVP_DigestFinal(&md_ctx, hash, 0);

    if (XMEMCMP(hash, d.output, WC_SHA256_DIGEST_SIZE) != 0)
        return -5904;

#ifdef WOLFSSL_SHA384

    e.input  = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhi"
               "jklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    e.output = "\x09\x33\x0c\x33\xf7\x11\x47\xe8\x3d\x19\x2f\xc7\x82\xcd\x1b"
               "\x47\x53\x11\x1b\x17\x3b\x3b\x05\xd2\x2f\xa0\x80\x86\xe3\xb0"
               "\xf7\x12\xfc\xc7\xc7\x1a\x55\x7e\x2d\xb9\x66\xc3\xe9\xfa\x91"
               "\x74\x60\x39";
    e.inLen  = XSTRLEN(e.input);
    e.outLen = WC_SHA384_DIGEST_SIZE;

    EVP_MD_CTX_init(&md_ctx);
    EVP_DigestInit(&md_ctx, EVP_sha384());

    EVP_DigestUpdate(&md_ctx, e.input, (unsigned long)e.inLen);
    EVP_DigestFinal(&md_ctx, hash, 0);

    if (XMEMCMP(hash, e.output, WC_SHA384_DIGEST_SIZE) != 0)
        return -5905;

#endif /* WOLFSSL_SHA384 */


#ifdef WOLFSSL_SHA512

    f.input  = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhi"
               "jklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    f.output = "\x8e\x95\x9b\x75\xda\xe3\x13\xda\x8c\xf4\xf7\x28\x14\xfc\x14"
               "\x3f\x8f\x77\x79\xc6\xeb\x9f\x7f\xa1\x72\x99\xae\xad\xb6\x88"
               "\x90\x18\x50\x1d\x28\x9e\x49\x00\xf7\xe4\x33\x1b\x99\xde\xc4"
               "\xb5\x43\x3a\xc7\xd3\x29\xee\xb6\xdd\x26\x54\x5e\x96\xe5\x5b"
               "\x87\x4b\xe9\x09";
    f.inLen  = XSTRLEN(f.input);
    f.outLen = WC_SHA512_DIGEST_SIZE;

    EVP_MD_CTX_init(&md_ctx);
    EVP_DigestInit(&md_ctx, EVP_sha512());

    EVP_DigestUpdate(&md_ctx, f.input, (unsigned long)f.inLen);
    EVP_DigestFinal(&md_ctx, hash, 0);

    if (XMEMCMP(hash, f.output, WC_SHA512_DIGEST_SIZE) != 0)
        return -5906;

#endif /* WOLFSSL_SHA512 */


#ifndef NO_MD5
    if (RAND_bytes(hash, sizeof(hash)) != 1)
        return -5907;

    c.input  = "what do ya want for nothing?";
    c.output = "\x55\x78\xe8\x48\x4b\xcc\x93\x80\x93\xec\x53\xaf\x22\xd6\x14"
               "\x76";
    c.inLen  = XSTRLEN(c.input);
    c.outLen = WC_MD5_DIGEST_SIZE;

    HMAC(EVP_md5(),
                 "JefeJefeJefeJefe", 16, (byte*)c.input, (int)c.inLen, hash, 0);

    if (XMEMCMP(hash, c.output, WC_MD5_DIGEST_SIZE) != 0)
        return -5908;

#endif /* NO_MD5 */

#ifndef NO_DES3
    { /* des test */
    const byte vector[] = { /* "now is the time for all " w/o trailing 0 */
        0x6e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,
        0x68,0x65,0x20,0x74,0x69,0x6d,0x65,0x20,
        0x66,0x6f,0x72,0x20,0x61,0x6c,0x6c,0x20
    };

    byte plain[24];
    byte cipher[24];

    const_DES_cblock key =
    {
        0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef
    };

    DES_cblock iv =
    {
        0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef
    };

    DES_key_schedule sched;

    const byte verify[] =
    {
        0x8b,0x7c,0x52,0xb0,0x01,0x2b,0x6c,0xb8,
        0x4f,0x0f,0xeb,0xf3,0xfb,0x5f,0x86,0x73,
        0x15,0x85,0xb3,0x22,0x4b,0x86,0x2b,0x4b
    };

    DES_key_sched(&key, &sched);

    DES_cbc_encrypt(vector, cipher, sizeof(vector), &sched, &iv, DES_ENCRYPT);
    DES_cbc_encrypt(cipher, plain, sizeof(vector), &sched, &iv, DES_DECRYPT);

    if (XMEMCMP(plain, vector, sizeof(vector)) != 0)
        return -5909;

    if (XMEMCMP(cipher, verify, sizeof(verify)) != 0)
        return -5910;

        /* test changing iv */
    DES_ncbc_encrypt(vector, cipher, 8, &sched, &iv, DES_ENCRYPT);
    DES_ncbc_encrypt(vector + 8, cipher + 8, 16, &sched, &iv, DES_ENCRYPT);

    if (XMEMCMP(cipher, verify, sizeof(verify)) != 0)
        return -5911;

    }  /* end des test */

#endif /* NO_DES3 */

#ifndef NO_AES

    {  /* evp_cipher test: EVP_aes_128_cbc */
        EVP_CIPHER_CTX ctx;
        int idx, cipherSz, plainSz;

        const byte msg[] = { /* "Now is the time for all " w/o trailing 0 */
            0x6e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,
            0x68,0x65,0x20,0x74,0x69,0x6d,0x65,0x20,
            0x66,0x6f,0x72,0x20,0x61,0x6c,0x6c,0x20
        };

        const byte verify[] =
        {
            0x95,0x94,0x92,0x57,0x5f,0x42,0x81,0x53,
            0x2c,0xcc,0x9d,0x46,0x77,0xa2,0x33,0xcb,
            0x3b,0x5d,0x41,0x97,0x94,0x25,0xa4,0xb4,
            0xae,0x7b,0x34,0xd0,0x3f,0x0c,0xbc,0x06
        };

        const byte verify2[] =
        {
            0x95,0x94,0x92,0x57,0x5f,0x42,0x81,0x53,
            0x2c,0xcc,0x9d,0x46,0x77,0xa2,0x33,0xcb,
            0x7d,0x37,0x7b,0x0b,0x44,0xaa,0xb5,0xf0,
            0x5f,0x34,0xb4,0xde,0xb5,0xbd,0x2a,0xbb
        };

        byte key[] = "0123456789abcdef   ";  /* align */
        byte iv[]  = "1234567890abcdef   ";  /* align */

        byte cipher[AES_BLOCK_SIZE * 4];
        byte plain [AES_BLOCK_SIZE * 4];

        EVP_CIPHER_CTX_init(&ctx);
        if (EVP_CipherInit(&ctx, EVP_aes_128_cbc(), key, iv, 1) == 0)
            return -5912;

        if (EVP_CipherUpdate(&ctx, cipher, &idx, (byte*)msg, sizeof(msg)) == 0)
            return -5913;

        cipherSz = idx;
        if (EVP_CipherFinal(&ctx, cipher + cipherSz, &idx) == 0)
            return -8107;
        cipherSz += idx;

        if ((cipherSz != (int)sizeof(verify)) &&
                                             XMEMCMP(cipher, verify, cipherSz))
            return -5914;

        EVP_CIPHER_CTX_init(&ctx);
        if (EVP_CipherInit(&ctx, EVP_aes_128_cbc(), key, iv, 0) == 0)
            return -5915;

        if (EVP_CipherUpdate(&ctx, plain, &idx, cipher, cipherSz) == 0)
            return -5916;

        plainSz = idx;
        if (EVP_CipherFinal(&ctx, plain + plainSz, &idx) == 0)
            return -8108;
        plainSz += idx;

        if ((plainSz != sizeof(msg)) || XMEMCMP(plain, msg, sizeof(msg)))
            return -5917;

        EVP_CIPHER_CTX_init(&ctx);
        if (EVP_CipherInit(&ctx, EVP_aes_128_cbc(), key, iv, 1) == 0)
            return -8109;

        if (EVP_CipherUpdate(&ctx, cipher, &idx, msg, AES_BLOCK_SIZE) == 0)
            return -8110;

        cipherSz = idx;
        if (EVP_CipherFinal(&ctx, cipher + cipherSz, &idx) == 0)
            return -8111;
        cipherSz += idx;

        if ((cipherSz != (int)sizeof(verify2)) ||
                                            XMEMCMP(cipher, verify2, cipherSz))
            return -8112;

    }  /* end evp_cipher test: EVP_aes_128_cbc*/

#ifdef HAVE_AES_ECB
    {  /* evp_cipher test: EVP_aes_128_ecb*/
        EVP_CIPHER_CTX ctx;
        const byte msg[] =
        {
          0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
          0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a
        };

        const byte verify[] =
        {
            0xf3,0xee,0xd1,0xbd,0xb5,0xd2,0xa0,0x3c,
            0x06,0x4b,0x5a,0x7e,0x3d,0xb1,0x81,0xf8
        };

        const byte key[] =
        {
          0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,
          0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
          0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,
          0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4
        };


        byte cipher[AES_BLOCK_SIZE * 4];
        byte plain [AES_BLOCK_SIZE * 4];

        EVP_CIPHER_CTX_init(&ctx);
        if (EVP_CipherInit(&ctx, EVP_aes_256_ecb(), (unsigned char*)key,
                                                                NULL, 1) == 0)
            return -5918;

        if (EVP_Cipher(&ctx, cipher, (byte*)msg, 16) == 0)
            return -5919;

        if (XMEMCMP(cipher, verify, AES_BLOCK_SIZE))
            return -5920;

        EVP_CIPHER_CTX_init(&ctx);
        if (EVP_CipherInit(&ctx, EVP_aes_256_ecb(), (unsigned char*)key,
                                                                NULL, 0) == 0)
            return -5921;

        if (EVP_Cipher(&ctx, plain, cipher, 16) == 0)
            return -5922;

        if (XMEMCMP(plain, msg, AES_BLOCK_SIZE))
            return -5923;

    }  /* end evp_cipher test */
#endif

#endif /* NO_AES */

#define OPENSSL_TEST_ERROR (-10000)


#ifndef NO_AES
#ifdef WOLFSSL_AES_DIRECT
  /* enable HAVE_AES_DECRYPT for AES_encrypt/decrypt */
{

  /* Test: AES_encrypt/decrypt/set Key */
  AES_KEY enc;
#ifdef HAVE_AES_DECRYPT
  AES_KEY dec;
#endif

  const byte msg[] =
  {
      0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
      0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a
  };

  const byte verify[] =
  {
      0xf3,0xee,0xd1,0xbd,0xb5,0xd2,0xa0,0x3c,
      0x06,0x4b,0x5a,0x7e,0x3d,0xb1,0x81,0xf8
  };

  const byte key[] =
  {
      0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,
      0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
      0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,
      0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4
  };

  byte plain[sizeof(msg)];
  byte cipher[sizeof(msg)];

  printf("openSSL extra test\n") ;


  AES_set_encrypt_key(key, sizeof(key)*8, &enc);
  AES_set_decrypt_key(key,  sizeof(key)*8, &dec);

  AES_encrypt(msg, cipher, &enc);

#ifdef HAVE_AES_DECRYPT
  AES_decrypt(cipher, plain, &dec);
  if (XMEMCMP(plain, msg, AES_BLOCK_SIZE))
      return OPENSSL_TEST_ERROR-60;
#endif /* HAVE_AES_DECRYPT */

  if (XMEMCMP(cipher, verify, AES_BLOCK_SIZE))
      return OPENSSL_TEST_ERROR-61;
}

#endif

/* EVP_Cipher with EVP_aes_xxx_ctr() */
#ifdef WOLFSSL_AES_COUNTER
{
    const byte ctrKey[] =
    {
        0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
        0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c
    };

    const byte ctrIv[] =
    {
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
        0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
    };


    const byte ctrPlain[] =
    {
        0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
        0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a,
        0xae,0x2d,0x8a,0x57,0x1e,0x03,0xac,0x9c,
        0x9e,0xb7,0x6f,0xac,0x45,0xaf,0x8e,0x51,
        0x30,0xc8,0x1c,0x46,0xa3,0x5c,0xe4,0x11,
        0xe5,0xfb,0xc1,0x19,0x1a,0x0a,0x52,0xef,
        0xf6,0x9f,0x24,0x45,0xdf,0x4f,0x9b,0x17,
        0xad,0x2b,0x41,0x7b,0xe6,0x6c,0x37,0x10
    };

    const byte ctrCipher[] =
    {
        0x87,0x4d,0x61,0x91,0xb6,0x20,0xe3,0x26,
        0x1b,0xef,0x68,0x64,0x99,0x0d,0xb6,0xce,
        0x98,0x06,0xf6,0x6b,0x79,0x70,0xfd,0xff,
        0x86,0x17,0x18,0x7b,0xb9,0xff,0xfd,0xff,
        0x5a,0xe4,0xdf,0x3e,0xdb,0xd5,0xd3,0x5e,
        0x5b,0x4f,0x09,0x02,0x0d,0xb0,0x3e,0xab,
        0x1e,0x03,0x1d,0xda,0x2f,0xbe,0x03,0xd1,
        0x79,0x21,0x70,0xa0,0xf3,0x00,0x9c,0xee
    };

    byte plainBuff [64];
    byte cipherBuff[64];

    const byte oddCipher[] =
    {
        0xb9,0xd7,0xcb,0x08,0xb0,0xe1,0x7b,0xa0,
        0xc2
    };


    /* test vector from "Recommendation for Block Cipher Modes of Operation"
     * NIST Special Publication 800-38A */
    const byte ctr192Key[] =
    {
        0x8e,0x73,0xb0,0xf7,0xda,0x0e,0x64,0x52,
        0xc8,0x10,0xf3,0x2b,0x80,0x90,0x79,0xe5,
        0x62,0xf8,0xea,0xd2,0x52,0x2c,0x6b,0x7b
    };

    const byte ctr192Iv[] =
    {
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
        0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
    };


    const byte ctr192Plain[] =
    {
        0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
        0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a
    };

    const byte ctr192Cipher[] =
    {
        0x1a,0xbc,0x93,0x24,0x17,0x52,0x1c,0xa2,
        0x4f,0x2b,0x04,0x59,0xfe,0x7e,0x6e,0x0b
    };

    /* test vector from "Recommendation for Block Cipher Modes of Operation"
     * NIST Special Publication 800-38A */
    const byte ctr256Key[] =
    {
        0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,
        0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
        0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,
        0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4
    };

    const byte ctr256Iv[] =
    {
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
        0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
    };


    const byte ctr256Plain[] =
    {
        0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
        0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a
    };

    const byte ctr256Cipher[] =
    {
        0x60,0x1e,0xc3,0x13,0x77,0x57,0x89,0xa5,
        0xb7,0xa7,0xf5,0x04,0xbb,0xf3,0xd2,0x28
    };

    EVP_CIPHER_CTX en;
    EVP_CIPHER_CTX de;
    EVP_CIPHER_CTX *p_en;
    EVP_CIPHER_CTX *p_de;

    EVP_CIPHER_CTX_init(&en);
    if (EVP_CipherInit(&en, EVP_aes_128_ctr(),
            (unsigned char*)ctrKey, (unsigned char*)ctrIv, 0) == 0)
        return -5924;
    if (EVP_Cipher(&en, (byte*)cipherBuff, (byte*)ctrPlain,
                                                        AES_BLOCK_SIZE*4) == 0)
        return -5925;
    EVP_CIPHER_CTX_init(&de);
    if (EVP_CipherInit(&de, EVP_aes_128_ctr(),
            (unsigned char*)ctrKey, (unsigned char*)ctrIv, 0) == 0)
        return -5926;

    if (EVP_Cipher(&de, (byte*)plainBuff, (byte*)cipherBuff,
                                                        AES_BLOCK_SIZE*4) == 0)
        return -5927;

    if (XMEMCMP(cipherBuff, ctrCipher, AES_BLOCK_SIZE*4))
        return -5928;
    if (XMEMCMP(plainBuff, ctrPlain, AES_BLOCK_SIZE*4))
        return -5929;

    p_en = wolfSSL_EVP_CIPHER_CTX_new();
    if(p_en == NULL)return -5930;
    p_de = wolfSSL_EVP_CIPHER_CTX_new();
    if(p_de == NULL)return -5931;

    if (EVP_CipherInit(p_en, EVP_aes_128_ctr(),
            (unsigned char*)ctrKey, (unsigned char*)ctrIv, 0) == 0)
        return -5932;
    if (EVP_Cipher(p_en, (byte*)cipherBuff, (byte*)ctrPlain,
                                                        AES_BLOCK_SIZE*4) == 0)
        return -5933;
    if (EVP_CipherInit(p_de, EVP_aes_128_ctr(),
            (unsigned char*)ctrKey, (unsigned char*)ctrIv, 0) == 0)
        return -5934;

    if (EVP_Cipher(p_de, (byte*)plainBuff, (byte*)cipherBuff,
                                                        AES_BLOCK_SIZE*4) == 0)
        return -5935;

    wolfSSL_EVP_CIPHER_CTX_free(p_en);
    wolfSSL_EVP_CIPHER_CTX_free(p_de);

    if (XMEMCMP(cipherBuff, ctrCipher, AES_BLOCK_SIZE*4))
        return -5936;
    if (XMEMCMP(plainBuff, ctrPlain, AES_BLOCK_SIZE*4))
        return -5937;

    EVP_CIPHER_CTX_init(&en);
    if (EVP_CipherInit(&en, EVP_aes_128_ctr(),
        (unsigned char*)ctrKey, (unsigned char*)ctrIv, 0) == 0)
        return -5938;
    if (EVP_Cipher(&en, (byte*)cipherBuff, (byte*)ctrPlain, 9) == 0)
        return -5939;

    EVP_CIPHER_CTX_init(&de);
    if (EVP_CipherInit(&de, EVP_aes_128_ctr(),
        (unsigned char*)ctrKey, (unsigned char*)ctrIv, 0) == 0)
        return -5940;

    if (EVP_Cipher(&de, (byte*)plainBuff, (byte*)cipherBuff, 9) == 0)
        return -5941;

    if (XMEMCMP(plainBuff, ctrPlain, 9))
        return -5942;
    if (XMEMCMP(cipherBuff, ctrCipher, 9))
        return -5943;

    if (EVP_Cipher(&en, (byte*)cipherBuff, (byte*)ctrPlain, 9) == 0)
        return -5944;
    if (EVP_Cipher(&de, (byte*)plainBuff, (byte*)cipherBuff, 9) == 0)
        return -5945;

    if (XMEMCMP(plainBuff, ctrPlain, 9))
        return -5946;
    if (XMEMCMP(cipherBuff, oddCipher, 9))
        return -5947;

    EVP_CIPHER_CTX_init(&en);
    if (EVP_CipherInit(&en, EVP_aes_192_ctr(),
            (unsigned char*)ctr192Key, (unsigned char*)ctr192Iv, 0) == 0)
        return -5948;
    if (EVP_Cipher(&en, (byte*)cipherBuff, (byte*)ctr192Plain,
                                                        AES_BLOCK_SIZE) == 0)
        return -5949;
    EVP_CIPHER_CTX_init(&de);
    if (EVP_CipherInit(&de, EVP_aes_192_ctr(),
        (unsigned char*)ctr192Key, (unsigned char*)ctr192Iv, 0) == 0)
        return -5950;

    XMEMSET(plainBuff, 0, sizeof(plainBuff));
    if (EVP_Cipher(&de, (byte*)plainBuff, (byte*)cipherBuff,
                                                        AES_BLOCK_SIZE) == 0)
        return -5951;

    if (XMEMCMP(plainBuff, ctr192Plain, sizeof(ctr192Plain)))
        return -5952;
    if (XMEMCMP(ctr192Cipher, cipherBuff, sizeof(ctr192Cipher)))
        return -5953;

    EVP_CIPHER_CTX_init(&en);
    if (EVP_CipherInit(&en, EVP_aes_256_ctr(),
        (unsigned char*)ctr256Key, (unsigned char*)ctr256Iv, 0) == 0)
        return -5954;
    if (EVP_Cipher(&en, (byte*)cipherBuff, (byte*)ctr256Plain,
                                                        AES_BLOCK_SIZE) == 0)
        return -5955;
    EVP_CIPHER_CTX_init(&de);
    if (EVP_CipherInit(&de, EVP_aes_256_ctr(),
        (unsigned char*)ctr256Key, (unsigned char*)ctr256Iv, 0) == 0)
        return -5956;

    XMEMSET(plainBuff, 0, sizeof(plainBuff));
    if (EVP_Cipher(&de, (byte*)plainBuff, (byte*)cipherBuff,
                                                        AES_BLOCK_SIZE) == 0)
        return -5957;

    if (XMEMCMP(plainBuff, ctr256Plain, sizeof(ctr256Plain)))
        return -5958;
    if (XMEMCMP(ctr256Cipher, cipherBuff, sizeof(ctr256Cipher)))
        return -5959;
}
#endif /* HAVE_AES_COUNTER */

{
      /* EVP_CipherUpdate test */


        const byte cbcPlain[] =
        {
            0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
            0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a,
            0xae,0x2d,0x8a,0x57,0x1e,0x03,0xac,0x9c,
            0x9e,0xb7,0x6f,0xac,0x45,0xaf,0x8e,0x51,
            0x30,0xc8,0x1c,0x46,0xa3,0x5c,0xe4,0x11,
            0xe5,0xfb,0xc1,0x19,0x1a,0x0a,0x52,0xef,
            0xf6,0x9f,0x24,0x45,0xdf,0x4f,0x9b,0x17,
            0xad,0x2b,0x41,0x7b,0xe6,0x6c,0x37,0x10
        };

        byte key[] = "0123456789abcdef   ";  /* align */
        byte iv[]  = "1234567890abcdef   ";  /* align */

        byte cipher[AES_BLOCK_SIZE * 4];
        byte plain [AES_BLOCK_SIZE * 4];
        EVP_CIPHER_CTX en;
        EVP_CIPHER_CTX de;
        int outlen ;
        int total = 0;

        EVP_CIPHER_CTX_init(&en);
        if (EVP_CipherInit(&en, EVP_aes_128_cbc(),
            (unsigned char*)key, (unsigned char*)iv, 1) == 0)
            return -5960;
        if (EVP_CipherUpdate(&en, (byte*)cipher, &outlen,
                                                    (byte*)cbcPlain, 9) == 0)
            return -5961;
        if(outlen != 0)
            return -5962;
        total += outlen;

        if (EVP_CipherUpdate(&en, (byte*)&cipher[total], &outlen,
                                                (byte*)&cbcPlain[9]  , 9) == 0)
            return -5963;
        if(outlen != 16)
            return -5964;
        total += outlen;

        if (EVP_CipherFinal(&en, (byte*)&cipher[total], &outlen) == 0)
            return -5965;
        if(outlen != 16)
            return -5966;
        total += outlen;
        if(total != 32)
            return -5967;

        total = 0;
        EVP_CIPHER_CTX_init(&de);
        if (EVP_CipherInit(&de, EVP_aes_128_cbc(),
            (unsigned char*)key, (unsigned char*)iv, 0) == 0)
            return -5968;

        if (EVP_CipherUpdate(&de, (byte*)plain, &outlen, (byte*)cipher, 6) == 0)
            return -5969;
        if(outlen != 0)
            return -5970;
        total += outlen;

        if (EVP_CipherUpdate(&de, (byte*)&plain[total], &outlen,
                                                    (byte*)&cipher[6], 12) == 0)
            return -5971;
        if(outlen != 0)
        total += outlen;

        if (EVP_CipherUpdate(&de, (byte*)&plain[total], &outlen,
                                                (byte*)&cipher[6+12], 14) == 0)
            return -5972;
        if(outlen != 16)
            return -5973;
        total += outlen;

        if (EVP_CipherFinal(&de, (byte*)&plain[total], &outlen) == 0)
            return -5974;
        if(outlen != 2)
            return -5975;
        total += outlen;

        if(total != 18)
            return -5976;

        if (XMEMCMP(plain, cbcPlain, 18))
            return -5977;

    }
#endif /* ifndef NO_AES */
    return 0;
}


#endif /* OPENSSL_EXTRA && !WOLFCRYPT_ONLY */


#ifndef NO_PWDBASED
#ifdef HAVE_SCRYPT
/* Test vectors taken from RFC 7914: scrypt PBKDF - Section 12. */
int scrypt_test(void)
{
    int   ret;
    byte  derived[64];

    const byte verify1[] = {
        0x77, 0xd6, 0x57, 0x62, 0x38, 0x65, 0x7b, 0x20,
        0x3b, 0x19, 0xca, 0x42, 0xc1, 0x8a, 0x04, 0x97,
        0xf1, 0x6b, 0x48, 0x44, 0xe3, 0x07, 0x4a, 0xe8,
        0xdf, 0xdf, 0xfa, 0x3f, 0xed, 0xe2, 0x14, 0x42,
        0xfc, 0xd0, 0x06, 0x9d, 0xed, 0x09, 0x48, 0xf8,
        0x32, 0x6a, 0x75, 0x3a, 0x0f, 0xc8, 0x1f, 0x17,
        0xe8, 0xd3, 0xe0, 0xfb, 0x2e, 0x0d, 0x36, 0x28,
        0xcf, 0x35, 0xe2, 0x0c, 0x38, 0xd1, 0x89, 0x06
    };
    const byte verify2[] = {
        0xfd, 0xba, 0xbe, 0x1c, 0x9d, 0x34, 0x72, 0x00,
        0x78, 0x56, 0xe7, 0x19, 0x0d, 0x01, 0xe9, 0xfe,
        0x7c, 0x6a, 0xd7, 0xcb, 0xc8, 0x23, 0x78, 0x30,
        0xe7, 0x73, 0x76, 0x63, 0x4b, 0x37, 0x31, 0x62,
        0x2e, 0xaf, 0x30, 0xd9, 0x2e, 0x22, 0xa3, 0x88,
        0x6f, 0xf1, 0x09, 0x27, 0x9d, 0x98, 0x30, 0xda,
        0xc7, 0x27, 0xaf, 0xb9, 0x4a, 0x83, 0xee, 0x6d,
        0x83, 0x60, 0xcb, 0xdf, 0xa2, 0xcc, 0x06, 0x40
    };
#if !defined(BENCH_EMBEDDED) && !defined(HAVE_INTEL_QA)
    const byte verify3[] = {
        0x70, 0x23, 0xbd, 0xcb, 0x3a, 0xfd, 0x73, 0x48,
        0x46, 0x1c, 0x06, 0xcd, 0x81, 0xfd, 0x38, 0xeb,
        0xfd, 0xa8, 0xfb, 0xba, 0x90, 0x4f, 0x8e, 0x3e,
        0xa9, 0xb5, 0x43, 0xf6, 0x54, 0x5d, 0xa1, 0xf2,
        0xd5, 0x43, 0x29, 0x55, 0x61, 0x3f, 0x0f, 0xcf,
        0x62, 0xd4, 0x97, 0x05, 0x24, 0x2a, 0x9a, 0xf9,
        0xe6, 0x1e, 0x85, 0xdc, 0x0d, 0x65, 0x1e, 0x40,
        0xdf, 0xcf, 0x01, 0x7b, 0x45, 0x57, 0x58, 0x87
    };
#endif
#ifdef SCRYPT_TEST_ALL
    /* Test case is very slow.
     * Use for confirmation after code change or new platform.
     */
    const byte verify4[] = {
        0x21, 0x01, 0xcb, 0x9b, 0x6a, 0x51, 0x1a, 0xae,
        0xad, 0xdb, 0xbe, 0x09, 0xcf, 0x70, 0xf8, 0x81,
        0xec, 0x56, 0x8d, 0x57, 0x4a, 0x2f, 0xfd, 0x4d,
        0xab, 0xe5, 0xee, 0x98, 0x20, 0xad, 0xaa, 0x47,
        0x8e, 0x56, 0xfd, 0x8f, 0x4b, 0xa5, 0xd0, 0x9f,
        0xfa, 0x1c, 0x6d, 0x92, 0x7c, 0x40, 0xf4, 0xc3,
        0x37, 0x30, 0x40, 0x49, 0xe8, 0xa9, 0x52, 0xfb,
        0xcb, 0xf4, 0x5c, 0x6f, 0xa7, 0x7a, 0x41, 0xa4
    };
#endif

    ret = wc_scrypt(derived, NULL, 0, NULL, 0, 4, 1, 1, sizeof(verify1));
    if (ret != 0)
        return -6000;
    if (XMEMCMP(derived, verify1, sizeof(verify1)) != 0)
        return -6001;

    ret = wc_scrypt(derived, (byte*)"password", 8, (byte*)"NaCl", 4, 10, 8, 16,
                    sizeof(verify2));
    if (ret != 0)
        return -6002;
    if (XMEMCMP(derived, verify2, sizeof(verify2)) != 0)
        return -6003;

    /* Don't run these test on embedded, since they use large mallocs */
#if !defined(BENCH_EMBEDDED) && !defined(HAVE_INTEL_QA)
    ret = wc_scrypt(derived, (byte*)"pleaseletmein", 13,
                    (byte*)"SodiumChloride", 14, 14, 8, 1, sizeof(verify3));
    if (ret != 0)
        return -6004;
    if (XMEMCMP(derived, verify3, sizeof(verify3)) != 0)
        return -6005;

#ifdef SCRYPT_TEST_ALL
    ret = wc_scrypt(derived, (byte*)"pleaseletmein", 13,
                    (byte*)"SodiumChloride", 14, 20, 8, 1, sizeof(verify4));
    if (ret != 0)
        return -6006;
    if (XMEMCMP(derived, verify4, sizeof(verify4)) != 0)
        return -6007;
#endif
#endif /* !BENCH_EMBEDDED && !HAVE_INTEL_QA */

    return 0;
}
#endif

int pkcs12_test(void)
{
    const byte passwd[] = { 0x00, 0x73, 0x00, 0x6d, 0x00, 0x65, 0x00, 0x67,
                            0x00, 0x00 };
    const byte salt[] =   { 0x0a, 0x58, 0xCF, 0x64, 0x53, 0x0d, 0x82, 0x3f };

    const byte passwd2[] = { 0x00, 0x71, 0x00, 0x75, 0x00, 0x65, 0x00, 0x65,
                             0x00, 0x67, 0x00, 0x00 };
    const byte salt2[] =   { 0x16, 0x82, 0xC0, 0xfC, 0x5b, 0x3f, 0x7e, 0xc5 };
    byte  derived[64];

    const byte verify[] = {
        0x27, 0xE9, 0x0D, 0x7E, 0xD5, 0xA1, 0xC4, 0x11,
        0xBA, 0x87, 0x8B, 0xC0, 0x90, 0xF5, 0xCE, 0xBE,
        0x5E, 0x9D, 0x5F, 0xE3, 0xD6, 0x2B, 0x73, 0xAA
    };

    const byte verify2[] = {
        0x90, 0x1B, 0x49, 0x70, 0xF0, 0x94, 0xF0, 0xF8,
        0x45, 0xC0, 0xF3, 0xF3, 0x13, 0x59, 0x18, 0x6A,
        0x35, 0xE3, 0x67, 0xFE, 0xD3, 0x21, 0xFD, 0x7C
    };

    int id         =  1;
    int kLen       = 24;
    int iterations =  1;
    int ret = wc_PKCS12_PBKDF(derived, passwd, sizeof(passwd), salt, 8,
                                                  iterations, kLen, WC_SHA256, id);

    if (ret < 0)
        return -6100;

    if ( (ret = XMEMCMP(derived, verify, kLen)) != 0)
        return -6101;

    iterations = 1000;
    ret = wc_PKCS12_PBKDF(derived, passwd2, sizeof(passwd2), salt2, 8,
                                                  iterations, kLen, WC_SHA256, id);
    if (ret < 0)
        return -6102;

    ret = wc_PKCS12_PBKDF_ex(derived, passwd2, sizeof(passwd2), salt2, 8,
                                       iterations, kLen, WC_SHA256, id, HEAP_HINT);
    if (ret < 0)
        return -6103;

    if ( (ret = XMEMCMP(derived, verify2, 24)) != 0)
        return -6104;

    return 0;
}


int pbkdf2_test(void)
{
    char passwd[] = "passwordpassword";
    const byte salt[] = { 0x78, 0x57, 0x8E, 0x5a, 0x5d, 0x63, 0xcb, 0x06 };
    int   iterations = 2048;
    int   kLen = 24;
    byte  derived[64];

    const byte verify[] = {
        0x43, 0x6d, 0xb5, 0xe8, 0xd0, 0xfb, 0x3f, 0x35, 0x42, 0x48, 0x39, 0xbc,
        0x2d, 0xd4, 0xf9, 0x37, 0xd4, 0x95, 0x16, 0xa7, 0x2a, 0x9a, 0x21, 0xd1
    };

    int ret = wc_PBKDF2(derived, (byte*)passwd, (int)XSTRLEN(passwd), salt, 8,
                                                      iterations, kLen, WC_SHA256);
    if (ret != 0)
        return ret;

    if (XMEMCMP(derived, verify, sizeof(verify)) != 0)
        return -6200;

    return 0;

}


#ifndef NO_SHA
int pbkdf1_test(void)
{
    char passwd[] = "password";
    const byte salt[] = { 0x78, 0x57, 0x8E, 0x5a, 0x5d, 0x63, 0xcb, 0x06 };
    int   iterations = 1000;
    int   kLen = 16;
    byte  derived[16];

    const byte verify[] = {
        0xDC, 0x19, 0x84, 0x7E, 0x05, 0xC6, 0x4D, 0x2F, 0xAF, 0x10, 0xEB, 0xFB,
        0x4A, 0x3D, 0x2A, 0x20
    };

    wc_PBKDF1(derived, (byte*)passwd, (int)XSTRLEN(passwd), salt, 8, iterations,
           kLen, WC_SHA);

    if (XMEMCMP(derived, verify, sizeof(verify)) != 0)
        return -6300;

    return 0;
}
#endif


int pwdbased_test(void)
{
   int ret = 0;
#ifndef NO_SHA
   ret += pbkdf1_test();
#endif
   ret += pbkdf2_test();
   ret += pkcs12_test();
#ifdef HAVE_SCRYPT
   ret += scrypt_test();
#endif

   return ret;
}

#endif /* NO_PWDBASED */

#if defined(HAVE_HKDF) && (!defined(NO_SHA) || !defined(NO_SHA256))

int hkdf_test(void)
{
    int ret;
    int L = 42;
    byte okm1[42];
    byte ikm1[22] = { 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
                      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
                      0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b };
    byte salt1[13] ={ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                      0x08, 0x09, 0x0a, 0x0b, 0x0c };
    byte info1[10] ={ 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
                      0xf8, 0xf9 };
    byte res1[42] = { 0x0a, 0xc1, 0xaf, 0x70, 0x02, 0xb3, 0xd7, 0x61,
                      0xd1, 0xe5, 0x52, 0x98, 0xda, 0x9d, 0x05, 0x06,
                      0xb9, 0xae, 0x52, 0x05, 0x72, 0x20, 0xa3, 0x06,
                      0xe0, 0x7b, 0x6b, 0x87, 0xe8, 0xdf, 0x21, 0xd0,
                      0xea, 0x00, 0x03, 0x3d, 0xe0, 0x39, 0x84, 0xd3,
                      0x49, 0x18 };
    byte res2[42] = { 0x08, 0x5a, 0x01, 0xea, 0x1b, 0x10, 0xf3, 0x69,
                      0x33, 0x06, 0x8b, 0x56, 0xef, 0xa5, 0xad, 0x81,
                      0xa4, 0xf1, 0x4b, 0x82, 0x2f, 0x5b, 0x09, 0x15,
                      0x68, 0xa9, 0xcd, 0xd4, 0xf1, 0x55, 0xfd, 0xa2,
                      0xc2, 0x2e, 0x42, 0x24, 0x78, 0xd3, 0x05, 0xf3,
                      0xf8, 0x96 };
    byte res3[42] = { 0x8d, 0xa4, 0xe7, 0x75, 0xa5, 0x63, 0xc1, 0x8f,
                      0x71, 0x5f, 0x80, 0x2a, 0x06, 0x3c, 0x5a, 0x31,
                      0xb8, 0xa1, 0x1f, 0x5c, 0x5e, 0xe1, 0x87, 0x9e,
                      0xc3, 0x45, 0x4e, 0x5f, 0x3c, 0x73, 0x8d, 0x2d,
                      0x9d, 0x20, 0x13, 0x95, 0xfa, 0xa4, 0xb6, 0x1a,
                      0x96, 0xc8 };
    byte res4[42] = { 0x3c, 0xb2, 0x5f, 0x25, 0xfa, 0xac, 0xd5, 0x7a,
                      0x90, 0x43, 0x4f, 0x64, 0xd0, 0x36, 0x2f, 0x2a,
                      0x2d, 0x2d, 0x0a, 0x90, 0xcf, 0x1a, 0x5a, 0x4c,
                      0x5d, 0xb0, 0x2d, 0x56, 0xec, 0xc4, 0xc5, 0xbf,
                      0x34, 0x00, 0x72, 0x08, 0xd5, 0xb8, 0x87, 0x18,
                      0x58, 0x65 };

    (void)res1;
    (void)res2;
    (void)res3;
    (void)res4;
    (void)salt1;
    (void)info1;

#ifndef NO_SHA
    ret = wc_HKDF(WC_SHA, ikm1, 22, NULL, 0, NULL, 0, okm1, L);
    if (ret != 0)
        return -6400;

    if (XMEMCMP(okm1, res1, L) != 0)
        return -6401;

#ifndef HAVE_FIPS
    /* fips can't have key size under 14 bytes, salt is key too */
    ret = wc_HKDF(WC_SHA, ikm1, 11, salt1, 13, info1, 10, okm1, L);
    if (ret != 0)
        return -6402;

    if (XMEMCMP(okm1, res2, L) != 0)
        return -6403;
#endif /* HAVE_FIPS */
#endif /* NO_SHA */

#ifndef NO_SHA256
    ret = wc_HKDF(WC_SHA256, ikm1, 22, NULL, 0, NULL, 0, okm1, L);
    if (ret != 0)
        return -6404;

    if (XMEMCMP(okm1, res3, L) != 0)
        return -6405;

#ifndef HAVE_FIPS
    /* fips can't have key size under 14 bytes, salt is key too */
    ret = wc_HKDF(WC_SHA256, ikm1, 22, salt1, 13, info1, 10, okm1, L);
    if (ret != 0)
        return -6406;

    if (XMEMCMP(okm1, res4, L) != 0)
        return -6407;
#endif /* HAVE_FIPS */
#endif /* NO_SHA256 */

    return 0;
}

#endif /* HAVE_HKDF */


#if defined(HAVE_ECC) && defined(HAVE_X963_KDF)

int x963kdf_test(void)
{
    int ret;
    byte kek[128];

#ifndef NO_SHA
    /* SHA-1, COUNT = 0
     * shared secret length: 192
     * SharedInfo length: 0
     * key data length: 128
     */
    const byte Z[] = {
        0x1c, 0x7d, 0x7b, 0x5f, 0x05, 0x97, 0xb0, 0x3d,
        0x06, 0xa0, 0x18, 0x46, 0x6e, 0xd1, 0xa9, 0x3e,
        0x30, 0xed, 0x4b, 0x04, 0xdc, 0x64, 0xcc, 0xdd
    };

    const byte verify[] = {
        0xbf, 0x71, 0xdf, 0xfd, 0x8f, 0x4d, 0x99, 0x22,
        0x39, 0x36, 0xbe, 0xb4, 0x6f, 0xee, 0x8c, 0xcc
    };
#endif

#ifndef NO_SHA256
    /* SHA-256, COUNT = 3
     * shared secret length: 192
     * SharedInfo length: 0
     * key data length: 128
     */
    const byte Z2[] = {
        0xd3, 0x8b, 0xdb, 0xe5, 0xc4, 0xfc, 0x16, 0x4c,
        0xdd, 0x96, 0x7f, 0x63, 0xc0, 0x4f, 0xe0, 0x7b,
        0x60, 0xcd, 0xe8, 0x81, 0xc2, 0x46, 0x43, 0x8c
    };

    const byte verify2[] = {
        0x5e, 0x67, 0x4d, 0xb9, 0x71, 0xba, 0xc2, 0x0a,
        0x80, 0xba, 0xd0, 0xd4, 0x51, 0x4d, 0xc4, 0x84
    };
#endif

#ifdef WOLFSSL_SHA512
    /* SHA-512, COUNT = 0
     * shared secret length: 192
     * SharedInfo length: 0
     * key data length: 128
     */
    const byte Z3[] = {
        0x87, 0xfc, 0x0d, 0x8c, 0x44, 0x77, 0x48, 0x5b,
        0xb5, 0x74, 0xf5, 0xfc, 0xea, 0x26, 0x4b, 0x30,
        0x88, 0x5d, 0xc8, 0xd9, 0x0a, 0xd8, 0x27, 0x82
    };

    const byte verify3[] = {
        0x94, 0x76, 0x65, 0xfb, 0xb9, 0x15, 0x21, 0x53,
        0xef, 0x46, 0x02, 0x38, 0x50, 0x6a, 0x02, 0x45
    };

    /* SHA-512, COUNT = 0
     * shared secret length: 521
     * SharedInfo length: 128
     * key data length: 1024
     */
    const byte Z4[] = {
        0x00, 0xaa, 0x5b, 0xb7, 0x9b, 0x33, 0xe3, 0x89,
        0xfa, 0x58, 0xce, 0xad, 0xc0, 0x47, 0x19, 0x7f,
        0x14, 0xe7, 0x37, 0x12, 0xf4, 0x52, 0xca, 0xa9,
        0xfc, 0x4c, 0x9a, 0xdb, 0x36, 0x93, 0x48, 0xb8,
        0x15, 0x07, 0x39, 0x2f, 0x1a, 0x86, 0xdd, 0xfd,
        0xb7, 0xc4, 0xff, 0x82, 0x31, 0xc4, 0xbd, 0x0f,
        0x44, 0xe4, 0x4a, 0x1b, 0x55, 0xb1, 0x40, 0x47,
        0x47, 0xa9, 0xe2, 0xe7, 0x53, 0xf5, 0x5e, 0xf0,
        0x5a, 0x2d
    };

    const byte info4[] = {
        0xe3, 0xb5, 0xb4, 0xc1, 0xb0, 0xd5, 0xcf, 0x1d,
        0x2b, 0x3a, 0x2f, 0x99, 0x37, 0x89, 0x5d, 0x31
    };

    const byte verify4[] = {
        0x44, 0x63, 0xf8, 0x69, 0xf3, 0xcc, 0x18, 0x76,
        0x9b, 0x52, 0x26, 0x4b, 0x01, 0x12, 0xb5, 0x85,
        0x8f, 0x7a, 0xd3, 0x2a, 0x5a, 0x2d, 0x96, 0xd8,
        0xcf, 0xfa, 0xbf, 0x7f, 0xa7, 0x33, 0x63, 0x3d,
        0x6e, 0x4d, 0xd2, 0xa5, 0x99, 0xac, 0xce, 0xb3,
        0xea, 0x54, 0xa6, 0x21, 0x7c, 0xe0, 0xb5, 0x0e,
        0xef, 0x4f, 0x6b, 0x40, 0xa5, 0xc3, 0x02, 0x50,
        0xa5, 0xa8, 0xee, 0xee, 0x20, 0x80, 0x02, 0x26,
        0x70, 0x89, 0xdb, 0xf3, 0x51, 0xf3, 0xf5, 0x02,
        0x2a, 0xa9, 0x63, 0x8b, 0xf1, 0xee, 0x41, 0x9d,
        0xea, 0x9c, 0x4f, 0xf7, 0x45, 0xa2, 0x5a, 0xc2,
        0x7b, 0xda, 0x33, 0xca, 0x08, 0xbd, 0x56, 0xdd,
        0x1a, 0x59, 0xb4, 0x10, 0x6c, 0xf2, 0xdb, 0xbc,
        0x0a, 0xb2, 0xaa, 0x8e, 0x2e, 0xfa, 0x7b, 0x17,
        0x90, 0x2d, 0x34, 0x27, 0x69, 0x51, 0xce, 0xcc,
        0xab, 0x87, 0xf9, 0x66, 0x1c, 0x3e, 0x88, 0x16
    };
#endif

#ifndef NO_SHA
    ret = wc_X963_KDF(WC_HASH_TYPE_SHA, Z, sizeof(Z), NULL, 0,
                      kek, sizeof(verify));
    if (ret != 0)
        return -6500;

    if (XMEMCMP(verify, kek, sizeof(verify)) != 0)
        return -6501;
#endif

#ifndef NO_SHA256
    ret = wc_X963_KDF(WC_HASH_TYPE_SHA256, Z2, sizeof(Z2), NULL, 0,
                      kek, sizeof(verify2));
    if (ret != 0)
        return -6502;

    if (XMEMCMP(verify2, kek, sizeof(verify2)) != 0)
        return -6503;
#endif

#ifdef WOLFSSL_SHA512
    ret = wc_X963_KDF(WC_HASH_TYPE_SHA512, Z3, sizeof(Z3), NULL, 0,
                      kek, sizeof(verify3));
    if (ret != 0)
        return -6504;

    if (XMEMCMP(verify3, kek, sizeof(verify3)) != 0)
        return -6505;

    ret = wc_X963_KDF(WC_HASH_TYPE_SHA512, Z4, sizeof(Z4), info4,
                      sizeof(info4), kek, sizeof(verify4));
    if (ret != 0)
        return -6506;

    if (XMEMCMP(verify4, kek, sizeof(verify4)) != 0)
        return -6507;
#endif

    return 0;
}

#endif /* HAVE_X963_KDF */


#ifdef HAVE_ECC

#ifdef BENCH_EMBEDDED
    #define ECC_SHARED_SIZE 128
#else
    #define ECC_SHARED_SIZE 1024
#endif
#define ECC_DIGEST_SIZE     MAX_ECC_BYTES
#define ECC_SIG_SIZE        ECC_MAX_SIG_SIZE

#ifndef NO_ECC_VECTOR_TEST
    #if (defined(HAVE_ECC192) || defined(HAVE_ECC224) ||\
         !defined(NO_ECC256) || defined(HAVE_ECC384) ||\
         defined(HAVE_ECC521) || defined(HAVE_ALL_CURVES))
        #define HAVE_ECC_VECTOR_TEST
    #endif
#endif

#ifdef HAVE_ECC_VECTOR_TEST
typedef struct eccVector {
    const char* msg; /* SHA-1 Encoded Message */
    const char* Qx;
    const char* Qy;
    const char* d; /* Private Key */
    const char* R;
    const char* S;
    const char* curveName;
    word32 msgLen;
    word32 keySize;
} eccVector;

static int ecc_test_vector_item(const eccVector* vector)
{
    int ret = 0, verify;
    word32  x;
    ecc_key userA;
    DECLARE_VAR(sig, byte, ECC_SIG_SIZE, HEAP_HINT);

    wc_ecc_init_ex(&userA, HEAP_HINT, devId);

    XMEMSET(sig, 0, ECC_SIG_SIZE);
    x = ECC_SIG_SIZE;

    ret = wc_ecc_import_raw(&userA, vector->Qx, vector->Qy,
                                             vector->d, vector->curveName);
    if (ret != 0)
        goto done;

    ret = wc_ecc_rs_to_sig(vector->R, vector->S, sig, &x);
    if (ret != 0)
        goto done;

    do {
    #if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &userA.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
    #endif
        if (ret >= 0) {
            ret = wc_ecc_verify_hash(sig, x, (byte*)vector->msg, vector->msgLen,
                                                               &verify, &userA);
        }
    } while (ret == WC_PENDING_E);

    if (ret != 0)
        goto done;

    if (verify != 1)
        ret = -6508;

done:
    wc_ecc_free(&userA);

    FREE_VAR(sig, HEAP_HINT);

    return ret;
}

static int ecc_test_vector(int keySize)
{
    int     ret;
    eccVector vec;

    XMEMSET(&vec, 0, sizeof(vec));
    vec.keySize = (word32)keySize;

    switch(keySize) {

#if defined(HAVE_ECC112) || defined(HAVE_ALL_CURVES)
    case 14:
        return 0;
#endif /* HAVE_ECC112 */
#if defined(HAVE_ECC128) || defined(HAVE_ALL_CURVES)
    case 16:
        return 0;
#endif /* HAVE_ECC128 */
#if defined(HAVE_ECC160) || defined(HAVE_ALL_CURVES)
    case 20:
        return 0;
#endif /* HAVE_ECC160 */

#if defined(HAVE_ECC192) || defined(HAVE_ALL_CURVES)
    case 24:
        /* first [P-192,SHA-1] vector from FIPS 186-3 NIST vectors */
        #if 1
            vec.msg = "\x60\x80\x79\x42\x3f\x12\x42\x1d\xe6\x16\xb7\x49\x3e\xbe\x55\x1c\xf4\xd6\x5b\x92";
            vec.msgLen = 20;
        #else
            /* This is the raw message prior to SHA-1 */
            vec.msg =
                "\xeb\xf7\x48\xd7\x48\xeb\xbc\xa7\xd2\x9f\xb4\x73\x69\x8a\x6e\x6b"
                "\x4f\xb1\x0c\x86\x5d\x4a\xf0\x24\xcc\x39\xae\x3d\xf3\x46\x4b\xa4"
                "\xf1\xd6\xd4\x0f\x32\xbf\x96\x18\xa9\x1b\xb5\x98\x6f\xa1\xa2\xaf"
                "\x04\x8a\x0e\x14\xdc\x51\xe5\x26\x7e\xb0\x5e\x12\x7d\x68\x9d\x0a"
                "\xc6\xf1\xa7\xf1\x56\xce\x06\x63\x16\xb9\x71\xcc\x7a\x11\xd0\xfd"
                "\x7a\x20\x93\xe2\x7c\xf2\xd0\x87\x27\xa4\xe6\x74\x8c\xc3\x2f\xd5"
                "\x9c\x78\x10\xc5\xb9\x01\x9d\xf2\x1c\xdc\xc0\xbc\xa4\x32\xc0\xa3"
                "\xee\xd0\x78\x53\x87\x50\x88\x77\x11\x43\x59\xce\xe4\xa0\x71\xcf";
            vec.msgLen = 128;
        #endif
        vec.Qx = "07008ea40b08dbe76432096e80a2494c94982d2d5bcf98e6";
        vec.Qy = "76fab681d00b414ea636ba215de26d98c41bd7f2e4d65477";
        vec.d  = "e14f37b3d1374ff8b03f41b9b3fdd2f0ebccf275d660d7f3";
        vec.R  = "6994d962bdd0d793ffddf855ec5bf2f91a9698b46258a63e";
        vec.S  = "02ba6465a234903744ab02bc8521405b73cf5fc00e1a9f41";
        vec.curveName = "SECP192R1";
        break;
#endif /* HAVE_ECC192 */

#if defined(HAVE_ECC224) || defined(HAVE_ALL_CURVES)
    case 28:
        /* first [P-224,SHA-1] vector from FIPS 186-3 NIST vectors */
        #if 1
            vec.msg = "\xb9\xa3\xb8\x6d\xb0\xba\x99\xfd\xc6\xd2\x94\x6b\xfe\xbe\x9c\xe8\x3f\x10\x74\xfc";
            vec.msgLen = 20;
        #else
            /* This is the raw message prior to SHA-1 */
            vec.msg =
                "\x36\xc8\xb2\x29\x86\x48\x7f\x67\x7c\x18\xd0\x97\x2a\x9e\x20\x47"
                "\xb3\xaf\xa5\x9e\xc1\x62\x76\x4e\xc3\x0b\x5b\x69\xe0\x63\x0f\x99"
                "\x0d\x4e\x05\xc2\x73\xb0\xe5\xa9\xd4\x28\x27\xb6\x95\xfc\x2d\x64"
                "\xd9\x13\x8b\x1c\xf4\xc1\x21\x55\x89\x4c\x42\x13\x21\xa7\xbb\x97"
                "\x0b\xdc\xe0\xfb\xf0\xd2\xae\x85\x61\xaa\xd8\x71\x7f\x2e\x46\xdf"
                "\xe3\xff\x8d\xea\xb4\xd7\x93\x23\x56\x03\x2c\x15\x13\x0d\x59\x9e"
                "\x26\xc1\x0f\x2f\xec\x96\x30\x31\xac\x69\x38\xa1\x8d\x66\x45\x38"
                "\xb9\x4d\xac\x55\x34\xef\x7b\x59\x94\x24\xd6\x9b\xe1\xf7\x1c\x20";
            vec.msgLen = 128;
        #endif
        vec.Qx = "8a4dca35136c4b70e588e23554637ae251077d1365a6ba5db9585de7";
        vec.Qy = "ad3dee06de0be8279d4af435d7245f14f3b4f82eb578e519ee0057b1";
        vec.d  = "97c4b796e1639dd1035b708fc00dc7ba1682cec44a1002a1a820619f";
        vec.R  = "147b33758321e722a0360a4719738af848449e2c1d08defebc1671a7";
        vec.S  = "24fc7ed7f1352ca3872aa0916191289e2e04d454935d50fe6af3ad5b";
        vec.curveName = "SECP224R1";
        break;
#endif /* HAVE_ECC224 */

#if defined(HAVE_ECC239) || defined(HAVE_ALL_CURVES)
    case 30:
        return 0;
#endif /* HAVE_ECC239 */

#if !defined(NO_ECC256) || defined(HAVE_ALL_CURVES)
    case 32:
        /* first [P-256,SHA-1] vector from FIPS 186-3 NIST vectors */
        #if 1
            vec.msg = "\xa3\xf9\x1a\xe2\x1b\xa6\xb3\x03\x98\x64\x47\x2f\x18\x41\x44\xc6\xaf\x62\xcd\x0e";
            vec.msgLen = 20;
        #else
            /* This is the raw message prior to SHA-1 */
            vec.msg =
                "\xa2\x4b\x21\x76\x2e\x6e\xdb\x15\x3c\xc1\x14\x38\xdb\x0e\x92\xcd"
                "\xf5\x2b\x86\xb0\x6c\xa9\x70\x16\x06\x27\x59\xc7\x0d\x36\xd1\x56"
                "\x2c\xc9\x63\x0d\x7f\xc7\xc7\x74\xb2\x8b\x54\xe3\x1e\xf5\x58\x72"
                "\xb2\xa6\x5d\xf1\xd7\xec\x26\xde\xbb\x33\xe7\xd9\x27\xef\xcc\xf4"
                "\x6b\x63\xde\x52\xa4\xf4\x31\xea\xca\x59\xb0\x5d\x2e\xde\xc4\x84"
                "\x5f\xff\xc0\xee\x15\x03\x94\xd6\x1f\x3d\xfe\xcb\xcd\xbf\x6f\x5a"
                "\x73\x38\xd0\xbe\x3f\x2a\x77\x34\x51\x98\x3e\xba\xeb\x48\xf6\x73"
                "\x8f\xc8\x95\xdf\x35\x7e\x1a\x48\xa6\x53\xbb\x35\x5a\x31\xa1\xb4"
            vec.msgLen = 128;
        #endif
        vec.Qx = "fa2737fb93488d19caef11ae7faf6b7f4bcd67b286e3fc54e8a65c2b74aeccb0";
        vec.Qy = "d4ccd6dae698208aa8c3a6f39e45510d03be09b2f124bfc067856c324f9b4d09";
        vec.d  = "be34baa8d040a3b991f9075b56ba292f755b90e4b6dc10dad36715c33cfdac25";
        vec.R  = "2b826f5d44e2d0b6de531ad96b51e8f0c56fdfead3c236892e4d84eacfc3b75c";
        vec.S  = "a2248b62c03db35a7cd63e8a120a3521a89d3d2f61ff99035a2148ae32e3a248";
        vec.curveName = "SECP256R1";
        break;
#endif /* !NO_ECC256 */

#if defined(HAVE_ECC320) || defined(HAVE_ALL_CURVES)
    case 40:
        return 0;
#endif /* HAVE_ECC320 */

#if defined(HAVE_ECC384) || defined(HAVE_ALL_CURVES)
    case 48:
        /* first [P-384,SHA-1] vector from FIPS 186-3 NIST vectors */
        #if 1
            vec.msg = "\x9b\x9f\x8c\x95\x35\xa5\xca\x26\x60\x5d\xb7\xf2\xfa\x57\x3b\xdf\xc3\x2e\xab\x8b";
            vec.msgLen = 20;
        #else
            /* This is the raw message prior to SHA-1 */
            vec.msg =
                "\xab\xe1\x0a\xce\x13\xe7\xe1\xd9\x18\x6c\x48\xf7\x88\x9d\x51\x47"
                "\x3d\x3a\x09\x61\x98\x4b\xc8\x72\xdf\x70\x8e\xcc\x3e\xd3\xb8\x16"
                "\x9d\x01\xe3\xd9\x6f\xc4\xf1\xd5\xea\x00\xa0\x36\x92\xbc\xc5\xcf"
                "\xfd\x53\x78\x7c\x88\xb9\x34\xaf\x40\x4c\x03\x9d\x32\x89\xb5\xba"
                "\xc5\xae\x7d\xb1\x49\x68\x75\xb5\xdc\x73\xc3\x09\xf9\x25\xc1\x3d"
                "\x1c\x01\xab\xda\xaf\xeb\xcd\xac\x2c\xee\x43\x39\x39\xce\x8d\x4a"
                "\x0a\x5d\x57\xbb\x70\x5f\x3b\xf6\xec\x08\x47\x95\x11\xd4\xb4\xa3"
                "\x21\x1f\x61\x64\x9a\xd6\x27\x43\x14\xbf\x0d\x43\x8a\x81\xe0\x60"
            vec.msgLen = 128;
        #endif
        vec.Qx = "e55fee6c49d8d523f5ce7bf9c0425ce4ff650708b7de5cfb095901523979a7f042602db30854735369813b5c3f5ef868";
        vec.Qy = "28f59cc5dc509892a988d38a8e2519de3d0c4fd0fbdb0993e38f18506c17606c5e24249246f1ce94983a5361c5be983e";
        vec.d  = "a492ce8fa90084c227e1a32f7974d39e9ff67a7e8705ec3419b35fb607582bebd461e0b1520ac76ec2dd4e9b63ebae71";
        vec.R  = "6820b8585204648aed63bdff47f6d9acebdea62944774a7d14f0e14aa0b9a5b99545b2daee6b3c74ebf606667a3f39b7";
        vec.S  = "491af1d0cccd56ddd520b233775d0bc6b40a6255cc55207d8e9356741f23c96c14714221078dbd5c17f4fdd89b32a907";
        vec.curveName = "SECP384R1";
        break;
#endif /* HAVE_ECC384 */

#if defined(HAVE_ECC512) || defined(HAVE_ALL_CURVES)
    case 64:
        return 0;
#endif /* HAVE_ECC512 */

#if defined(HAVE_ECC521) || defined(HAVE_ALL_CURVES)
    case 66:
        /* first [P-521,SHA-1] vector from FIPS 186-3 NIST vectors */
        #if 1
            vec.msg = "\x1b\xf7\x03\x9c\xca\x23\x94\x27\x3f\x11\xa1\xd4\x8d\xcc\xb4\x46\x6f\x31\x61\xdf";
            vec.msgLen = 20;
        #else
            /* This is the raw message prior to SHA-1 */
            vec.msg =
                "\x50\x3f\x79\x39\x34\x0a\xc7\x23\xcd\x4a\x2f\x4e\x6c\xcc\x27\x33"
                "\x38\x3a\xca\x2f\xba\x90\x02\x19\x9d\x9e\x1f\x94\x8b\xe0\x41\x21"
                "\x07\xa3\xfd\xd5\x14\xd9\x0c\xd4\xf3\x7c\xc3\xac\x62\xef\x00\x3a"
                "\x2d\xb1\xd9\x65\x7a\xb7\x7f\xe7\x55\xbf\x71\xfa\x59\xe4\xd9\x6e"
                "\xa7\x2a\xe7\xbf\x9d\xe8\x7d\x79\x34\x3b\xc1\xa4\xbb\x14\x4d\x16"
                "\x28\xd1\xe9\xe9\xc8\xed\x80\x8b\x96\x2c\x54\xe5\xf9\x6d\x53\xda"
                "\x14\x7a\x96\x38\xf9\x4a\x91\x75\xd8\xed\x61\x05\x5f\x0b\xa5\x73"
                "\xa8\x2b\xb7\xe0\x18\xee\xda\xc4\xea\x7b\x36\x2e\xc8\x9c\x38\x2b"
            vec.msgLen = 128;
        #endif
        vec.Qx = "12fbcaeffa6a51f3ee4d3d2b51c5dec6d7c726ca353fc014ea2bf7cfbb9b910d32cbfa6a00fe39b6cdb8946f22775398b2e233c0cf144d78c8a7742b5c7a3bb5d23";
        vec.Qy = "09cdef823dd7bf9a79e8cceacd2e4527c231d0ae5967af0958e931d7ddccf2805a3e618dc3039fec9febbd33052fe4c0fee98f033106064982d88f4e03549d4a64d";
        vec.d  = "1bd56bd106118eda246155bd43b42b8e13f0a6e25dd3bb376026fab4dc92b6157bc6dfec2d15dd3d0cf2a39aa68494042af48ba9601118da82c6f2108a3a203ad74";
        vec.R  = "0bd117b4807710898f9dd7778056485777668f0e78e6ddf5b000356121eb7a220e9493c7f9a57c077947f89ac45d5acb6661bbcd17abb3faea149ba0aa3bb1521be";
        vec.S  = "019cd2c5c3f9870ecdeb9b323abdf3a98cd5e231d85c6ddc5b71ab190739f7f226e6b134ba1d5889ddeb2751dabd97911dff90c34684cdbe7bb669b6c3d22f2480c";
        vec.curveName = "SECP521R1";
        break;
#endif /* HAVE_ECC521 */
    default:
        return NOT_COMPILED_IN; /* Invalid key size / Not supported */
    }; /* Switch */

    ret = ecc_test_vector_item(&vec);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

#ifdef HAVE_ECC_CDH
static int ecc_test_cdh_vectors(void)
{
    int ret;
    ecc_key pub_key, priv_key;
    byte    sharedA[32] = {0}, sharedB[32] = {0};
    word32  x, z;

    const char* QCAVSx = "700c48f77f56584c5cc632ca65640db91b6bacce3a4df6b42ce7cc838833d287";
    const char* QCAVSy = "db71e509e3fd9b060ddb20ba5c51dcc5948d46fbf640dfe0441782cab85fa4ac";
    const char* dIUT =   "7d7dc5f71eb29ddaf80d6214632eeae03d9058af1fb6d22ed80badb62bc1a534";
    const char* QIUTx =  "ead218590119e8876b29146ff89ca61770c4edbbf97d38ce385ed281d8a6b230";
    const char* QIUTy =  "28af61281fd35e2fa7002523acc85a429cb06ee6648325389f59edfce1405141";
    const char* ZIUT =   "46fc62106420ff012e54a434fbdd2d25ccc5852060561e68040dd7778997bd7b";

    /* setup private and public keys */
    ret = wc_ecc_init(&pub_key);
    if (ret != 0)
        return ret;
    ret = wc_ecc_init(&priv_key);
    if (ret != 0) {
        wc_ecc_free(&pub_key);
        goto done;
    }
    wc_ecc_set_flags(&pub_key, WC_ECC_FLAG_COFACTOR);
    wc_ecc_set_flags(&priv_key, WC_ECC_FLAG_COFACTOR);
    ret = wc_ecc_import_raw(&pub_key, QCAVSx, QCAVSy, NULL, "SECP256R1");
    if (ret != 0)
        goto done;
    ret = wc_ecc_import_raw(&priv_key, QIUTx, QIUTy, dIUT, "SECP256R1");
    if (ret != 0)
        goto done;

    /* compute ECC Cofactor shared secret */
    x = sizeof(sharedA);
    ret = wc_ecc_shared_secret(&priv_key, &pub_key, sharedA, &x);
    if (ret != 0) {
        goto done;
    }

    /* read in expected Z */
    z = sizeof(sharedB);
    ret = Base16_Decode((const byte*)ZIUT, (word32)XSTRLEN(ZIUT), sharedB, &z);
    if (ret != 0)
        goto done;

    /* compare results */
    if (x != z || XMEMCMP(sharedA, sharedB, x)) {
        ERROR_OUT(-6509, done);
    }

done:
    wc_ecc_free(&priv_key);
    wc_ecc_free(&pub_key);
    return ret;
}
#endif /* HAVE_ECC_CDH */
#endif /* HAVE_ECC_VECTOR_TEST */

#ifdef HAVE_ECC_KEY_IMPORT
/* returns 0 on success */
static int ecc_test_make_pub(WC_RNG* rng)
{
    ecc_key key;
    unsigned char* exportBuf;
    unsigned char* tmp;
    unsigned char msg[] = "test wolfSSL ECC public gen";
    word32 x, tmpSz;
    int ret = 0;
    ecc_point* pubPoint = NULL;
#if defined(HAVE_ECC_DHE) && defined(HAVE_ECC_KEY_EXPORT)
    ecc_key pub;
#endif
#ifdef HAVE_ECC_VERIFY
    int verify = 0;
#endif

    tmp = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (tmp == NULL) {
        return -6810;
    }
    exportBuf = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (exportBuf == NULL) {
        XFREE(tmp, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return -6811;
    }

#ifdef USE_CERT_BUFFERS_256
    XMEMCPY(tmp, ecc_key_der_256, (size_t)sizeof_ecc_key_der_256);
    tmpSz = (size_t)sizeof_ecc_key_der_256;
#else
    FILE* file;
    file = fopen(eccKeyDerFile, "rb");
    if (!file) {
        ERROR_OUT(-6812, done);
    }

    tmpSz = (word32)fread(tmp, 1, FOURK_BUF, file);
    fclose(file);
#endif /* USE_CERT_BUFFERS_256 */

    wc_ecc_init(&key);

    /* import private only then test with */
    ret = wc_ecc_import_private_key(tmp, tmpSz, NULL, 0, NULL);
    if (ret == 0) {
        ERROR_OUT(-6813, done);
    }

    ret = wc_ecc_import_private_key(NULL, tmpSz, NULL, 0, &key);
    if (ret == 0) {
        ERROR_OUT(-6814, done);
    }

    x = 0;
    ret = wc_EccPrivateKeyDecode(tmp, &x, &key, tmpSz);
    if (ret != 0) {
        ERROR_OUT(-6815, done);
    }

#ifdef HAVE_ECC_KEY_EXPORT
    x = FOURK_BUF;
    ret = wc_ecc_export_private_only(&key, exportBuf, &x);
    if (ret != 0) {
        ERROR_OUT(-6816, done);
    }

    /* make private only key */
    wc_ecc_free(&key);
    wc_ecc_init(&key);
    ret = wc_ecc_import_private_key(exportBuf, x, NULL, 0, &key);
    if (ret != 0) {
        ERROR_OUT(-6817, done);
    }

    x = FOURK_BUF;
    ret = wc_ecc_export_x963_ex(&key, exportBuf, &x, 0);
    if (ret == 0) {
        ERROR_OUT(-6818, done);
    }

#endif /* HAVE_ECC_KEY_EXPORT */

    ret = wc_ecc_make_pub(NULL, NULL);
    if (ret == 0) {
        ERROR_OUT(-6819, done);
    }

    pubPoint = wc_ecc_new_point_h(HEAP_HINT);
    if (pubPoint == NULL) {
        ERROR_OUT(-6820, done);
    }

    ret = wc_ecc_make_pub(&key, pubPoint);
    if (ret != 0) {
        ERROR_OUT(-6821, done);
    }

#ifdef HAVE_ECC_KEY_EXPORT
    /* export should still fail, is private only key */
    x = FOURK_BUF;
    ret = wc_ecc_export_x963_ex(&key, exportBuf, &x, 0);
    if (ret == 0) {
        ERROR_OUT(-6822, done);
    }
#endif /* HAVE_ECC_KEY_EXPORT */

#ifdef HAVE_ECC_SIGN
    tmpSz = FOURK_BUF;
    ret = wc_ecc_sign_hash(msg, sizeof(msg), tmp, &tmpSz, rng, &key);
    if (ret != 0) {
        ERROR_OUT(-6823, done);
    }

#ifdef HAVE_ECC_VERIFY
    /* try verify with private only key */
    ret = wc_ecc_verify_hash(tmp, tmpSz, msg, sizeof(msg), &verify, &key);
    if (ret != 0) {
        ERROR_OUT(-6824, done);
    }

    if (verify != 1) {
        ERROR_OUT(-6825, done);
    }
#ifdef HAVE_ECC_KEY_EXPORT
    /* exporting the public part should now work */
    x = FOURK_BUF;
    ret = wc_ecc_export_x963_ex(&key, exportBuf, &x, 0);
    if (ret != 0) {
        ERROR_OUT(-6826, done);
    }
#endif /* HAVE_ECC_KEY_EXPORT */
#endif /* HAVE_ECC_VERIFY */

#endif /* HAVE_ECC_SIGN */

#if defined(HAVE_ECC_DHE) && defined(HAVE_ECC_KEY_EXPORT)
    /* now test private only key with creating a shared secret */
    x = FOURK_BUF;
    ret = wc_ecc_export_private_only(&key, exportBuf, &x);
    if (ret != 0) {
        ERROR_OUT(-6827, done);
    }

    /* make private only key */
    wc_ecc_free(&key);
    wc_ecc_init(&key);
    ret = wc_ecc_import_private_key(exportBuf, x, NULL, 0, &key);
    if (ret != 0) {
        ERROR_OUT(-6828, done);
    }

    /* check that public export fails with private only key */
    x = FOURK_BUF;
    ret = wc_ecc_export_x963_ex(&key, exportBuf, &x, 0);
    if (ret == 0) {
        ERROR_OUT(-6829, done);
    }

    /* make public key for shared secret */
    wc_ecc_init(&pub);
    ret = wc_ecc_make_key(rng, 32, &pub);
    if (ret != 0) {
        ERROR_OUT(-6830, done);
    }

    x = FOURK_BUF;
    ret = wc_ecc_shared_secret(&key, &pub, exportBuf, &x);
    wc_ecc_free(&pub);
    if (ret != 0) {
        ERROR_OUT(-6831, done);
    }
#endif /* HAVE_ECC_DHE && HAVE_ECC_KEY_EXPORT */

    ret = 0;

done:

    XFREE(tmp, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(exportBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

    wc_ecc_del_point_h(pubPoint, HEAP_HINT);
    wc_ecc_free(&key);

    return ret;
}
#endif /* HAVE_ECC_KEY_IMPORT */


#ifdef WOLFSSL_KEY_GEN
static int ecc_test_key_gen(WC_RNG* rng, int keySize)
{
    int   ret = 0;
    int   derSz;
    byte* der;
    byte* pem;
    ecc_key userA;

    der = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (der == NULL) {
        return -6840;
    }
    pem = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (pem == NULL) {
        XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return -6840;
    }

    ret = wc_ecc_init_ex(&userA, HEAP_HINT, devId);
    if (ret != 0)
        goto done;

    ret = wc_ecc_make_key(rng, keySize, &userA);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &userA.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
    if (ret != 0)
        goto done;

    ret = wc_ecc_check_key(&userA);
    if (ret != 0)
        goto done;

    derSz = wc_EccKeyToDer(&userA, der, FOURK_BUF);
    if (derSz < 0) {
        ERROR_OUT(derSz, done);
    }

    ret = SaveDerAndPem(der, derSz, pem, FOURK_BUF, eccCaKeyTempFile,
            eccCaKeyPemFile, ECC_PRIVATEKEY_TYPE, -6510);
    if (ret != 0) {
        goto done;
    }

    /* test export of public key */
    derSz = wc_EccPublicKeyToDer(&userA, der, FOURK_BUF, 1);
    if (derSz < 0) {
        ERROR_OUT(derSz, done);
    }
    if (derSz == 0) {
        ERROR_OUT(-6514, done);
    }

    ret = SaveDerAndPem(der, derSz, NULL, 0, eccPubKeyDerFile,
        NULL, 0, -6515);
    if (ret != 0) {
        goto done;
    }

done:

    XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    wc_ecc_free(&userA);

    return ret;
}
#endif /* WOLFSSL_KEY_GEN */

static int ecc_test_curve_size(WC_RNG* rng, int keySize, int testVerifyCount,
    int curve_id, const ecc_set_type* dp)
{
    DECLARE_VAR(sharedA, byte, ECC_SHARED_SIZE, HEAP_HINT);
    DECLARE_VAR(sharedB, byte, ECC_SHARED_SIZE, HEAP_HINT);
#ifdef HAVE_ECC_KEY_EXPORT
    byte    exportBuf[1024];
#endif
    word32  x, y;
#ifdef HAVE_ECC_SIGN
    DECLARE_VAR(sig, byte, ECC_SIG_SIZE, HEAP_HINT);
    DECLARE_VAR(digest, byte, ECC_DIGEST_SIZE, HEAP_HINT);
    int     i;
#ifdef HAVE_ECC_VERIFY
    int     verify;
#endif /* HAVE_ECC_VERIFY */
#endif /* HAVE_ECC_SIGN */
    int     ret;
    ecc_key userA, userB, pubKey;

    (void)testVerifyCount;
    (void)dp;

    XMEMSET(&userA, 0, sizeof(ecc_key));
    XMEMSET(&userB, 0, sizeof(ecc_key));
    XMEMSET(&pubKey, 0, sizeof(ecc_key));

    ret = wc_ecc_init_ex(&userA, HEAP_HINT, devId);
    if (ret != 0)
        goto done;
    ret = wc_ecc_init_ex(&userB, HEAP_HINT, devId);
    if (ret != 0)
        goto done;
    ret = wc_ecc_init_ex(&pubKey, HEAP_HINT, devId);
    if (ret != 0)
        goto done;

#ifdef WOLFSSL_CUSTOM_CURVES
    if (dp != NULL) {
        ret = wc_ecc_set_custom_curve(&userA, dp);
        if (ret != 0)
            goto done;
        ret = wc_ecc_set_custom_curve(&userB, dp);
        if (ret != 0)
            goto done;
    }
#endif

    ret = wc_ecc_make_key_ex(rng, keySize, &userA, curve_id);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &userA.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
    if (ret != 0)
        goto done;

    ret = wc_ecc_check_key(&userA);
    if (ret != 0)
        goto done;

    ret = wc_ecc_make_key_ex(rng, keySize, &userB, curve_id);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &userB.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
#endif
    if (ret != 0)
        goto done;

#ifdef HAVE_ECC_DHE
    x = ECC_SHARED_SIZE;
    do {
    #if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &userA.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
    #endif
        if (ret >= 0)
            ret = wc_ecc_shared_secret(&userA, &userB, sharedA, &x);
    } while (ret == WC_PENDING_E);
    if (ret != 0) {
        goto done;
    }

    y = ECC_SHARED_SIZE;
    do {
    #if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &userB.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
    #endif
        if (ret >= 0)
            ret = wc_ecc_shared_secret(&userB, &userA, sharedB, &y);
    } while (ret == WC_PENDING_E);
    if (ret != 0)
        goto done;

    if (y != x)
        ERROR_OUT(-6517, done);

    if (XMEMCMP(sharedA, sharedB, x))
        ERROR_OUT(-6518, done);
#endif /* HAVE_ECC_DHE */

#ifdef HAVE_ECC_CDH
    /* add cofactor flag */
    wc_ecc_set_flags(&userA, WC_ECC_FLAG_COFACTOR);
    wc_ecc_set_flags(&userB, WC_ECC_FLAG_COFACTOR);

    x = sizeof(sharedA);
    ret = wc_ecc_shared_secret(&userA, &userB, sharedA, &x);
    if (ret != 0) {
        goto done;
    }

    y = sizeof(sharedB);
    ret = wc_ecc_shared_secret(&userB, &userA, sharedB, &y);
    if (ret != 0)
        goto done;

    if (y != x)
        ERROR_OUT(-6519, done);

    if (XMEMCMP(sharedA, sharedB, x))
        ERROR_OUT(-6520, done);

    /* remove cofactor flag */
    wc_ecc_set_flags(&userA, 0);
    wc_ecc_set_flags(&userB, 0);
#endif /* HAVE_ECC_CDH */

#ifdef HAVE_ECC_KEY_EXPORT
    x = sizeof(exportBuf);
    ret = wc_ecc_export_x963_ex(&userA, exportBuf, &x, 0);
    if (ret != 0)
        goto done;

#ifdef HAVE_ECC_KEY_IMPORT
    #ifdef WOLFSSL_CUSTOM_CURVES
        if (dp != NULL) {
            ret = wc_ecc_set_custom_curve(&pubKey, dp);
            if (ret != 0) goto done;
        }
    #endif
    ret = wc_ecc_import_x963_ex(exportBuf, x, &pubKey, curve_id);
    if (ret != 0)
        goto done;

#ifdef HAVE_ECC_DHE
    y = ECC_SHARED_SIZE;
    do {
    #if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &userB.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
    #endif
        if (ret >= 0)
            ret = wc_ecc_shared_secret(&userB, &pubKey, sharedB, &y);
    } while (ret == WC_PENDING_E);
    if (ret != 0)
        goto done;

    if (XMEMCMP(sharedA, sharedB, y))
        ERROR_OUT(-6521, done);
#endif /* HAVE_ECC_DHE */

    #ifdef HAVE_COMP_KEY
        /* try compressed export / import too */
        x = sizeof(exportBuf);
        ret = wc_ecc_export_x963_ex(&userA, exportBuf, &x, 1);
        if (ret != 0)
            goto done;
        wc_ecc_free(&pubKey);

        ret = wc_ecc_init_ex(&pubKey, HEAP_HINT, devId);
        if (ret != 0)
            goto done;
    #ifdef WOLFSSL_CUSTOM_CURVES
        if (dp != NULL) {
            ret = wc_ecc_set_custom_curve(&pubKey, dp);
            if (ret != 0) goto done;
        }
    #endif
        ret = wc_ecc_import_x963_ex(exportBuf, x, &pubKey, curve_id);
        if (ret != 0)
            goto done;

    #ifdef HAVE_ECC_DHE
        y = ECC_SHARED_SIZE;
        do {
        #if defined(WOLFSSL_ASYNC_CRYPT)
            ret = wc_AsyncWait(ret, &userB.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
        #endif
            if (ret >= 0)
                ret = wc_ecc_shared_secret(&userB, &pubKey, sharedB, &y);
        } while (ret == WC_PENDING_E);
        if (ret != 0)
            goto done;

        if (XMEMCMP(sharedA, sharedB, y))
            ERROR_OUT(-6522, done);
    #endif /* HAVE_ECC_DHE */
    #endif /* HAVE_COMP_KEY */

#endif /* HAVE_ECC_KEY_IMPORT */
#endif /* HAVE_ECC_KEY_EXPORT */

#ifdef HAVE_ECC_SIGN
    /* ECC w/out Shamir has issue with all 0 digest */
    /* WC_BIGINT doesn't have 0 len well on hardware */
#if defined(ECC_SHAMIR) && !defined(WOLFSSL_ASYNC_CRYPT)
    /* test DSA sign hash with zeros */
    for (i = 0; i < (int)ECC_DIGEST_SIZE; i++) {
        digest[i] = 0;
    }

    x = ECC_SIG_SIZE;
    do {
    #if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &userA.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
    #endif
        if (ret >= 0)
            ret = wc_ecc_sign_hash(digest, ECC_DIGEST_SIZE, sig, &x, rng,
                                                                        &userA);
    } while (ret == WC_PENDING_E);
    if (ret != 0)
        goto done;

#ifdef HAVE_ECC_VERIFY
    for (i=0; i<testVerifyCount; i++) {
        verify = 0;
        do {
        #if defined(WOLFSSL_ASYNC_CRYPT)
            ret = wc_AsyncWait(ret, &userA.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
        #endif
            if (ret >= 0)
                ret = wc_ecc_verify_hash(sig, x, digest, ECC_DIGEST_SIZE,
                                                               &verify, &userA);
        } while (ret == WC_PENDING_E);
        if (ret != 0)
            goto done;
        if (verify != 1)
            ERROR_OUT(-6523, done);
    }
#endif /* HAVE_ECC_VERIFY */
#endif /* ECC_SHAMIR && !WOLFSSL_ASYNC_CRYPT */

    /* test DSA sign hash with sequence (0,1,2,3,4,...) */
    for (i = 0; i < (int)ECC_DIGEST_SIZE; i++) {
        digest[i] = (byte)i;
    }

    x = ECC_SIG_SIZE;
    do {
    #if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &userA.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
    #endif
        if (ret >= 0)
            ret = wc_ecc_sign_hash(digest, ECC_DIGEST_SIZE, sig, &x, rng,
                                                                        &userA);
    } while (ret == WC_PENDING_E);
    if (ret != 0)
        ERROR_OUT(-6524, done);

#ifdef HAVE_ECC_VERIFY
    for (i=0; i<testVerifyCount; i++) {
        verify = 0;
        do {
        #if defined(WOLFSSL_ASYNC_CRYPT)
            ret = wc_AsyncWait(ret, &userA.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
        #endif
            if (ret >= 0)
                ret = wc_ecc_verify_hash(sig, x, digest, ECC_DIGEST_SIZE,
                                                               &verify, &userA);
        } while (ret == WC_PENDING_E);
        if (ret != 0)
            goto done;
        if (verify != 1)
            ERROR_OUT(-6525, done);
    }
#endif /* HAVE_ECC_VERIFY */
#endif /* HAVE_ECC_SIGN */

#ifdef HAVE_ECC_KEY_EXPORT
    x = sizeof(exportBuf);
    ret = wc_ecc_export_private_only(&userA, exportBuf, &x);
    if (ret != 0)
        goto done;
#endif /* HAVE_ECC_KEY_EXPORT */

done:
    wc_ecc_free(&pubKey);
    wc_ecc_free(&userB);
    wc_ecc_free(&userA);

    FREE_VAR(sharedA, HEAP_HINT);
    FREE_VAR(sharedB, HEAP_HINT);
#ifdef HAVE_ECC_SIGN
    FREE_VAR(sig, HEAP_HINT);
    FREE_VAR(digest, HEAP_HINT);
#endif

    return ret;
}

#undef  ECC_TEST_VERIFY_COUNT
#define ECC_TEST_VERIFY_COUNT 2
static int ecc_test_curve(WC_RNG* rng, int keySize)
{
    int ret;

    ret = ecc_test_curve_size(rng, keySize, ECC_TEST_VERIFY_COUNT,
        ECC_CURVE_DEF, NULL);
    if (ret < 0) {
        if (ret == ECC_CURVE_OID_E) {
            /* ignore error for curves not found */
            /* some curve sizes are only available with:
                HAVE_ECC_SECPR2, HAVE_ECC_SECPR3, HAVE_ECC_BRAINPOOL
                and HAVE_ECC_KOBLITZ */
        }
        else {
            printf("ecc_test_curve_size %d failed!: %d\n", keySize, ret);
            return ret;
        }
    }

#ifdef HAVE_ECC_VECTOR_TEST
    ret = ecc_test_vector(keySize);
    if (ret < 0) {
        printf("ecc_test_vector %d failed!: %d\n", keySize, ret);
        return ret;
    }
#endif

#ifdef WOLFSSL_KEY_GEN
    ret = ecc_test_key_gen(rng, keySize);
    if (ret < 0) {
        if (ret == ECC_CURVE_OID_E) {
            /* ignore error for curves not found */
        }
        else {
            printf("ecc_test_key_gen %d failed!: %d\n", keySize, ret);
            return ret;
        }
    }
#endif

    return 0;
}

#if !defined(WOLFSSL_ATECC508A) && defined(HAVE_ECC_KEY_IMPORT) && \
     defined(HAVE_ECC_KEY_EXPORT)
static int ecc_point_test(void)
{
    int        ret;
    ecc_point* point;
    ecc_point* point2;
#ifdef HAVE_COMP_KEY
    ecc_point* point3;
    ecc_point* point4;
#endif
    word32     outLen;
    byte       out[65];
    byte       der[] = { 0x04, /* = Uncompressed */
                         0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
#ifdef HAVE_COMP_KEY
    byte       derComp0[] = { 0x02, /* = Compressed, y even */
                              0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    byte       derComp1[] = { 0x03, /* = Compressed, y odd */
                              0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
#endif
    byte       altDer[] = { 0x04, /* = Uncompressed */
                            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
    int curve_idx = wc_ecc_get_curve_idx(ECC_SECP256R1);

    /* if curve P256 is not enabled then test should not fail */
    if (curve_idx == ECC_CURVE_INVALID)
        return 0;

    outLen = sizeof(out);
    point = wc_ecc_new_point();
    if (point == NULL)
        return -6600;
    point2 = wc_ecc_new_point();
    if (point2 == NULL) {
        wc_ecc_del_point(point);
        return -6601;
    }
#ifdef HAVE_COMP_KEY
    point3 = wc_ecc_new_point();
    if (point3 == NULL) {
        wc_ecc_del_point(point2);
        wc_ecc_del_point(point);
        return -6602;
    }
    point4 = wc_ecc_new_point();
    if (point4 == NULL) {
        wc_ecc_del_point(point3);
        wc_ecc_del_point(point2);
        wc_ecc_del_point(point);
        return -6603;
    }
#endif

    /* Parameter Validation testing. */
    wc_ecc_del_point(NULL);
    ret = wc_ecc_import_point_der(NULL, sizeof(der), curve_idx, point);
    if (ret != ECC_BAD_ARG_E) {
        ret = -6604;
        goto done;
    }
    ret = wc_ecc_import_point_der(der, sizeof(der), ECC_CURVE_INVALID, point);
    if (ret != ECC_BAD_ARG_E) {
        ret = -6605;
        goto done;
    }
    ret = wc_ecc_import_point_der(der, sizeof(der), curve_idx, NULL);
    if (ret != ECC_BAD_ARG_E) {
        ret = -6606;
        goto done;
    }
    ret = wc_ecc_export_point_der(-1, point, out, &outLen);
    if (ret != ECC_BAD_ARG_E) {
        ret = -6607;
        goto done;
    }
    ret = wc_ecc_export_point_der(curve_idx, NULL, out, &outLen);
    if (ret != ECC_BAD_ARG_E) {
        ret = -6608;
        goto done;
    }
    ret = wc_ecc_export_point_der(curve_idx, point, NULL, &outLen);
    if (ret != LENGTH_ONLY_E || outLen != sizeof(out)) {
        ret = -6609;
        goto done;
    }
    ret = wc_ecc_export_point_der(curve_idx, point, out, NULL);
    if (ret != ECC_BAD_ARG_E) {
        ret = -6610;
        goto done;
    }
    outLen = 0;
    ret = wc_ecc_export_point_der(curve_idx, point, out, &outLen);
    if (ret != BUFFER_E) {
        ret = -6611;
        goto done;
    }
    ret = wc_ecc_copy_point(NULL, NULL);
    if (ret != ECC_BAD_ARG_E) {
        ret = -6612;
        goto done;
    }
    ret = wc_ecc_copy_point(NULL, point2);
    if (ret != ECC_BAD_ARG_E) {
        ret = -6613;
        goto done;
    }
    ret = wc_ecc_copy_point(point, NULL);
    if (ret != ECC_BAD_ARG_E) {
        ret = -6614;
        goto done;
    }
    ret = wc_ecc_cmp_point(NULL, NULL);
    if (ret != BAD_FUNC_ARG) {
        ret = -6615;
        goto done;
    }
    ret = wc_ecc_cmp_point(NULL, point2);
    if (ret != BAD_FUNC_ARG) {
        ret = -6616;
        goto done;
    }
    ret = wc_ecc_cmp_point(point, NULL);
    if (ret != BAD_FUNC_ARG) {
        ret = -6617;
        goto done;
    }

    /* Use API. */
    ret = wc_ecc_import_point_der(der, sizeof(der), curve_idx, point);
    if (ret != 0) {
        ret = -6618;
        goto done;
    }

    outLen = sizeof(out);
    ret = wc_ecc_export_point_der(curve_idx, point, out, &outLen);
    if (ret != 0) {
        ret = -6619;
        goto done;
    }
    if (outLen != sizeof(der)) {
        ret = -6620;
        goto done;
    }
    if (XMEMCMP(out, der, outLen) != 0) {
        ret = -6621;
        goto done;
    }

    ret = wc_ecc_copy_point(point2, point);
    if (ret != MP_OKAY) {
        ret = -6622;
        goto done;
    }
    ret = wc_ecc_cmp_point(point2, point);
    if (ret != MP_EQ) {
        ret = -6623;
        goto done;
    }

    ret = wc_ecc_import_point_der(altDer, sizeof(altDer), curve_idx, point2);
    if (ret != 0) {
        ret = -6624;
        goto done;
    }
    ret = wc_ecc_cmp_point(point2, point);
    if (ret != MP_GT) {
        ret = -6625;
        goto done;
    }

#ifdef HAVE_COMP_KEY
    ret = wc_ecc_import_point_der(derComp0, sizeof(der), curve_idx, point3);
    if (ret != 0) {
        ret = -6626;
        goto done;
    }

    ret = wc_ecc_import_point_der(derComp1, sizeof(der), curve_idx, point4);
    if (ret != 0) {
        ret = -6627;
        goto done;
    }
#endif

done:
#ifdef HAVE_COMP_KEY
    wc_ecc_del_point(point4);
    wc_ecc_del_point(point3);
#endif
    wc_ecc_del_point(point2);
    wc_ecc_del_point(point);

    return ret;
}
#endif /* !WOLFSSL_ATECC508A && HAVE_ECC_KEY_IMPORT && HAVE_ECC_KEY_EXPORT */

#ifndef NO_SIG_WRAPPER
static int ecc_sig_test(WC_RNG* rng, ecc_key* key)
{
    int     ret;
    word32  sigSz;
    int     size;
    byte    out[ECC_MAX_SIG_SIZE];
    byte   in[] = "Everyone gets Friday off.";
    word32 inLen = (word32)XSTRLEN((char*)in);

    size = wc_ecc_sig_size(key);

    ret = wc_SignatureGetSize(WC_SIGNATURE_TYPE_ECC, key, sizeof(*key));
    if (ret != size)
        return -6628;

    sigSz = (word32)ret;
    ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_ECC, in,
                               inLen, out, &sigSz, key, sizeof(*key), rng);
    if (ret != 0)
        return -6629;

    ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_ECC, in,
                             inLen, out, sigSz, key, sizeof(*key));
    if (ret != 0)
        return -6630;

    return 0;
}
#endif

#if defined(HAVE_ECC_KEY_IMPORT) && defined(HAVE_ECC_KEY_EXPORT)
static int ecc_exp_imp_test(ecc_key* key)
{
    int        ret;
    int        curve_id;
    ecc_key    keyImp;
    byte       priv[32];
    word32     privLen;
    byte       pub[65];
    word32     pubLen;
    const char qx[] = "7a4e287890a1a47ad3457e52f2f76a83"
                      "ce46cbc947616d0cbaa82323818a793d";
    const char qy[] = "eec4084f5b29ebf29c44cce3b3059610"
                      "922f8b30ea6e8811742ac7238fe87308";
    const char d[]  = "8c14b793cb19137e323a6d2e2a870bca"
                      "2e7a493ec1153b3a95feb8a4873f8d08";

    wc_ecc_init(&keyImp);

    privLen = sizeof(priv);
    ret = wc_ecc_export_private_only(key, priv, &privLen);
    if (ret != 0) {
        ret = -6631;
        goto done;
    }
    pubLen = sizeof(pub);
    ret = wc_ecc_export_point_der(key->idx, &key->pubkey, pub, &pubLen);
    if (ret != 0) {
        ret = -6632;
        goto done;
    }

    ret = wc_ecc_import_private_key(priv, privLen, pub, pubLen, &keyImp);
    if (ret != 0) {
        ret = -6633;
        goto done;
    }

    wc_ecc_free(&keyImp);
    wc_ecc_init(&keyImp);

    ret = wc_ecc_import_raw_ex(&keyImp, qx, qy, d, ECC_SECP256R1);
    if (ret != 0) {
        ret = -6634;
        goto done;
    }

    wc_ecc_free(&keyImp);
    wc_ecc_init(&keyImp);

    curve_id = wc_ecc_get_curve_id(key->idx);
    if (curve_id < 0)
        return -6635;

    /* test import private only */
    ret = wc_ecc_import_private_key_ex(priv, privLen, NULL, 0, &keyImp,
                                       curve_id);
    if (ret != 0)
        return -6636;

done:
    wc_ecc_free(&keyImp);
    return ret;
}
#endif /* HAVE_ECC_KEY_IMPORT && HAVE_ECC_KEY_EXPORT */

#ifndef WOLFSSL_ATECC508A
#if defined(HAVE_ECC_KEY_IMPORT) && !defined(WOLFSSL_VALIDATE_ECC_IMPORT)
static int ecc_mulmod_test(ecc_key* key1)
{
    int ret;
    ecc_key    key2;
    ecc_key    key3;

    wc_ecc_init(&key2);
    wc_ecc_init(&key3);

    /* TODO: Use test data, test with WOLFSSL_VALIDATE_ECC_IMPORT. */
    /* Need base point (Gx,Gy) and parameter A - load them as the public and
     * private key in key2.
     */
    ret = wc_ecc_import_raw_ex(&key2, key1->dp->Gx, key1->dp->Gy, key1->dp->Af,
                               ECC_SECP256R1);
    if (ret != 0)
        goto done;

    /* Need a point (Gx,Gy) and prime - load them as the public and private key
     * in key3.
     */
    ret = wc_ecc_import_raw_ex(&key3, key1->dp->Gx, key1->dp->Gy,
                               key1->dp->prime, ECC_SECP256R1);
    if (ret != 0)
        goto done;

    ret = wc_ecc_mulmod(&key1->k, &key2.pubkey, &key3.pubkey, &key2.k, &key3.k,
                        1);
    if (ret != 0) {
        ret = -6637;
        goto done;
    }

done:
    wc_ecc_free(&key3);
    wc_ecc_free(&key2);
    return ret;
}
#endif

static int ecc_ssh_test(ecc_key* key)
{
    int    ret;
    byte   out[128];
    word32 outLen = sizeof(out);

    /* Parameter Validation testing. */
    ret = wc_ecc_shared_secret_ssh(NULL, &key->pubkey, out, &outLen);
    if (ret != BAD_FUNC_ARG)
        return -6638;
    ret = wc_ecc_shared_secret_ssh(key, NULL, out, &outLen);
    if (ret != BAD_FUNC_ARG)
        return -6639;
    ret = wc_ecc_shared_secret_ssh(key, &key->pubkey, NULL, &outLen);
    if (ret != BAD_FUNC_ARG)
        return -6640;
    ret = wc_ecc_shared_secret_ssh(key, &key->pubkey, out, NULL);
    if (ret != BAD_FUNC_ARG)
        return -6641;

    /* Use API. */
    ret = wc_ecc_shared_secret_ssh(key, &key->pubkey, out, &outLen);
    if (ret != 0)
        return -6642;
    return 0;
}
#endif

static int ecc_def_curve_test(WC_RNG *rng)
{
    int     ret;
    ecc_key key;

    wc_ecc_init(&key);

    ret = wc_ecc_make_key(rng, 32, &key);
    if (ret != 0) {
        ret = -6643;
        goto done;
    }

#ifndef NO_SIG_WRAPPER
    ret = ecc_sig_test(rng, &key);
    if (ret < 0)
        goto done;
#endif
#if defined(HAVE_ECC_KEY_IMPORT) && defined(HAVE_ECC_KEY_EXPORT)
    ret = ecc_exp_imp_test(&key);
    if (ret < 0)
        goto done;
#endif
#ifndef WOLFSSL_ATECC508A
#if defined(HAVE_ECC_KEY_IMPORT) && !defined(WOLFSSL_VALIDATE_ECC_IMPORT)
    ret = ecc_mulmod_test(&key);
    if (ret < 0)
        goto done;
#endif
    ret = ecc_ssh_test(&key);
    if (ret < 0)
        goto done;
#endif /* WOLFSSL_ATECC508A */
done:
    wc_ecc_free(&key);
    return ret;
}

#ifdef WOLFSSL_CERT_EXT
static int ecc_decode_test(void)
{
    int        ret;
    word32     inSz;
    word32     inOutIdx;
    ecc_key    key;
    const byte good[] = { 0x30, 0x0d, 0x30, 0x0b, 0x06, 0x00, 0x06, 0x01, 0x01,
            0x03, 0x04, 0x00, 0x04, 0x01, 0x01 };
    const byte badNoObjId[] = { 0x30, 0x08, 0x30, 0x06, 0x03, 0x04,
            0x00, 0x04, 0x01, 0x01 };
    const byte badOneObjId[] = { 0x30, 0x0a, 0x30, 0x08, 0x06, 0x00, 0x03, 0x04,
            0x00, 0x04, 0x01, 0x01 };
    const byte badObjId1Len[] = { 0x30, 0x0c, 0x30, 0x0a, 0x06, 0x09,
            0x06, 0x00, 0x03, 0x04, 0x00, 0x04, 0x01, 0x01 };
    const byte badObj2d1Len[] = { 0x30, 0x0c, 0x30, 0x0a, 0x06, 0x00,
            0x06, 0x07, 0x03, 0x04, 0x00, 0x04, 0x01, 0x01 };
    const byte badNotBitStr[] = { 0x30, 0x0d, 0x30, 0x0b, 0x06, 0x00,
            0x06, 0x01, 0x01, 0x04, 0x04, 0x00, 0x04, 0x01, 0x01 };
    const byte badBitStrLen[] = { 0x30, 0x0d, 0x30, 0x0b, 0x06, 0x00,
            0x06, 0x01, 0x01, 0x03, 0x05, 0x00, 0x04, 0x01, 0x01 };
    const byte badNoBitStrZero[] = { 0x30, 0x0c, 0x30, 0x0a, 0x06, 0x00,
            0x06, 0x01, 0x01, 0x03, 0x03, 0x04, 0x01, 0x01 };
    const byte badPoint[] = { 0x30, 0x0b, 0x30, 0x09, 0x06, 0x00, 0x06, 0x01,
            0x01, 0x03, 0x03, 0x00, 0x04, 0x01 };

    XMEMSET(&key, 0, sizeof(key));
    wc_ecc_init(&key);

    inSz = sizeof(good);
    ret = wc_EccPublicKeyDecode(NULL, &inOutIdx, &key, inSz);
    if (ret != BAD_FUNC_ARG) {
        ret = -6700;
        goto done;
    }
    ret = wc_EccPublicKeyDecode(good, NULL, &key, inSz);
    if (ret != BAD_FUNC_ARG) {
        ret = -6701;
        goto done;
    }
    ret = wc_EccPublicKeyDecode(good, &inOutIdx, NULL, inSz);
    if (ret != BAD_FUNC_ARG) {
        ret = -6702;
        goto done;
    }
    ret = wc_EccPublicKeyDecode(good, &inOutIdx, &key, 0);
    if (ret != BAD_FUNC_ARG) {
        ret = -6703;
        goto done;
    }

    /* Change offset to produce bad input data. */
    inOutIdx = 2;
    inSz = sizeof(good) - inOutIdx;
    ret = wc_EccPublicKeyDecode(good, &inOutIdx, &key, inSz);
    if (ret != ASN_PARSE_E) {
        ret = -6704;
        goto done;
    }
    inOutIdx = 4;
    inSz = sizeof(good) - inOutIdx;
    ret = wc_EccPublicKeyDecode(good, &inOutIdx, &key, inSz);
    if (ret != ASN_PARSE_E) {
        ret = -6705;
        goto done;
    }
    /* Bad data. */
    inSz = sizeof(badNoObjId);
    inOutIdx = 0;
    ret = wc_EccPublicKeyDecode(badNoObjId, &inOutIdx, &key, inSz);
    if (ret != ASN_OBJECT_ID_E) {
        ret = -6706;
        goto done;
    }
    inSz = sizeof(badOneObjId);
    inOutIdx = 0;
    ret = wc_EccPublicKeyDecode(badOneObjId, &inOutIdx, &key, inSz);
    if (ret != ASN_OBJECT_ID_E) {
        ret = -6707;
        goto done;
    }
    inSz = sizeof(badObjId1Len);
    inOutIdx = 0;
    ret = wc_EccPublicKeyDecode(badObjId1Len, &inOutIdx, &key, inSz);
    if (ret != ASN_PARSE_E) {
        ret = -6708;
        goto done;
    }
    inSz = sizeof(badObj2d1Len);
    inOutIdx = 0;
    ret = wc_EccPublicKeyDecode(badObj2d1Len, &inOutIdx, &key, inSz);
    if (ret != ASN_PARSE_E) {
        ret = -6709;
        goto done;
    }
    inSz = sizeof(badNotBitStr);
    inOutIdx = 0;
    ret = wc_EccPublicKeyDecode(badNotBitStr, &inOutIdx, &key, inSz);
    if (ret != ASN_BITSTR_E) {
        ret = -6710;
        goto done;
    }
    inSz = sizeof(badBitStrLen);
    inOutIdx = 0;
    ret = wc_EccPublicKeyDecode(badBitStrLen, &inOutIdx, &key, inSz);
    if (ret != ASN_PARSE_E) {
        ret = -6711;
        goto done;
    }
    inSz = sizeof(badNoBitStrZero);
    inOutIdx = 0;
    ret = wc_EccPublicKeyDecode(badNoBitStrZero, &inOutIdx, &key, inSz);
    if (ret != ASN_EXPECT_0_E) {
        ret = -6712;
        goto done;
    }
    inSz = sizeof(badPoint);
    inOutIdx = 0;
    ret = wc_EccPublicKeyDecode(badPoint, &inOutIdx, &key, inSz);
    if (ret != ASN_ECC_KEY_E) {
        ret = -6713;
        goto done;
    }

    inSz = sizeof(good);
    inOutIdx = 0;
    ret = wc_EccPublicKeyDecode(good, &inOutIdx, &key, inSz);
    if (ret != 0) {
        ret = -6714;
        goto done;
    }

done:
    wc_ecc_free(&key);
    return ret;
}
#endif /* WOLFSSL_CERT_EXT */

#ifdef WOLFSSL_CUSTOM_CURVES
static int ecc_test_custom_curves(WC_RNG* rng)
{
    int ret;

    /* test use of custom curve - using BRAINPOOLP256R1 for test */
    const word32 ecc_oid_brainpoolp256r1_sum = 104;
    const ecc_oid_t ecc_oid_brainpoolp256r1[] = {
        0x2B,0x24,0x03,0x03,0x02,0x08,0x01,0x01,0x07
    };
    const ecc_set_type ecc_dp_brainpool256r1 = {
        32,                                                                 /* size/bytes */
        ECC_CURVE_CUSTOM,                                                   /* ID         */
        "BRAINPOOLP256R1",                                                  /* curve name */
        "A9FB57DBA1EEA9BC3E660A909D838D726E3BF623D52620282013481D1F6E5377", /* prime      */
        "7D5A0975FC2C3057EEF67530417AFFE7FB8055C126DC5C6CE94A4B44F330B5D9", /* A          */
        "26DC5C6CE94A4B44F330B5D9BBD77CBF958416295CF7E1CE6BCCDC18FF8C07B6", /* B          */
        "A9FB57DBA1EEA9BC3E660A909D838D718C397AA3B561A6F7901E0E82974856A7", /* order      */
        "8BD2AEB9CB7E57CB2C4B482FFC81B7AFB9DE27E1E3BD23C23A4453BD9ACE3262", /* Gx         */
        "547EF835C3DAC4FD97F8461A14611DC9C27745132DED8E545C1D54C72F046997", /* Gy         */
        ecc_oid_brainpoolp256r1,                                            /* oid/oidSz  */
        sizeof(ecc_oid_brainpoolp256r1) / sizeof(ecc_oid_t),
        ecc_oid_brainpoolp256r1_sum,                                        /* oid sum    */
        1,                                                                  /* cofactor   */
    };

    ret = ecc_test_curve_size(rng, 0, ECC_TEST_VERIFY_COUNT, ECC_CURVE_DEF,
        &ecc_dp_brainpool256r1);
    if (ret != 0) {
        printf("ECC test for custom curve failed! %d\n", ret);
        return ret;
    }

    #if defined(HAVE_ECC_BRAINPOOL) || defined(HAVE_ECC_KOBLITZ)
    {
        int curve_id;
        #ifdef HAVE_ECC_BRAINPOOL
            curve_id = ECC_BRAINPOOLP256R1;
        #else
            curve_id = ECC_SECP256K1;
        #endif
        /* Test and demonstrate use of non-SECP curve */
        ret = ecc_test_curve_size(rng, 0, ECC_TEST_VERIFY_COUNT, curve_id, NULL);
        if (ret < 0) {
            printf("ECC test for curve_id %d failed! %d\n", curve_id, ret);
            return ret;
        }
    }
    #endif

    return ret;
}
#endif /* WOLFSSL_CUSTOM_CURVES */

#ifdef WOLFSSL_CERT_GEN

/* Make Cert / Sign example for ECC cert and ECC CA */
static int ecc_test_cert_gen(WC_RNG* rng)
{
    int ret;
    Cert        myCert;
    int         certSz;
    size_t      bytes;
    word32      idx = 0;
#ifndef USE_CERT_BUFFERS_256
    FILE*       file;
#endif
#ifdef WOLFSSL_TEST_CERT
    DecodedCert decode;
#endif
    byte*  der = NULL;
    byte*  pem = NULL;
    ecc_key caEccKey;
    ecc_key certPubKey;

    XMEMSET(&caEccKey, 0, sizeof(caEccKey));
    XMEMSET(&certPubKey, 0, sizeof(certPubKey));

    der = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (der == NULL) {
        ERROR_OUT(-6720, exit);
    }
    pem = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (pem == NULL) {
        ERROR_OUT(-6721, exit);
    }

    /* Get cert private key */
#ifdef ENABLE_ECC384_CERT_GEN_TEST
    /* Get Cert Key 384 */
#ifdef USE_CERT_BUFFERS_256
    XMEMCPY(der, ca_ecc_key_der_384, sizeof_ca_ecc_key_der_384);
    bytes = sizeof_ca_ecc_key_der_384;
#else
    file = fopen(eccCaKey384File, "rb");
    if (!file) {
        ERROR_OUT(-6722, exit);
    }

    bytes = fread(der, 1, FOURK_BUF, file);
    fclose(file);
    (void)eccCaKeyFile;
#endif /* USE_CERT_BUFFERS_256 */
#else
#ifdef USE_CERT_BUFFERS_256
    XMEMCPY(der, ca_ecc_key_der_256, sizeof_ca_ecc_key_der_256);
    bytes = sizeof_ca_ecc_key_der_256;
#else
    file = fopen(eccCaKeyFile, "rb");
    if (!file) {
        ERROR_OUT(-6722, exit);
    }
    bytes = fread(der, 1, FOURK_BUF, file);
    fclose(file);
    (void)eccCaKey384File;
#endif /* USE_CERT_BUFFERS_256 */
#endif /* ENABLE_ECC384_CERT_GEN_TEST */

    /* Get CA Key */
    ret = wc_ecc_init_ex(&caEccKey, HEAP_HINT, devId);
    if (ret != 0) {
        ERROR_OUT(-6723, exit);
    }
    ret = wc_EccPrivateKeyDecode(der, &idx, &caEccKey, (word32)bytes);
    if (ret != 0) {
        ERROR_OUT(-6724, exit);
    }

    /* Make a public key */
    ret = wc_ecc_init_ex(&certPubKey, HEAP_HINT, devId);
    if (ret != 0) {
        ERROR_OUT(-6725, exit);
    }

    ret = wc_ecc_make_key(rng, 32, &certPubKey);
    if (ret != 0) {
        ERROR_OUT(-6726, exit);
    }

    /* Setup Certificate */
    if (wc_InitCert(&myCert)) {
        ERROR_OUT(-6727, exit);
    }

#ifndef NO_SHA256
    myCert.sigType = CTC_SHA256wECDSA;
#else
    myCert.sigType = CTC_SHAwECDSA;
#endif
    XMEMCPY(&myCert.subject, &certDefaultName, sizeof(CertName));

#ifdef WOLFSSL_CERT_EXT
    /* add Policies */
    XSTRNCPY(myCert.certPolicies[0], "2.4.589440.587.101.2.1.9632587.1",
            CTC_MAX_CERTPOL_SZ);
    XSTRNCPY(myCert.certPolicies[1], "1.2.13025.489.1.113549",
            CTC_MAX_CERTPOL_SZ);
    myCert.certPoliciesNb = 2;

    /* add SKID from the Public Key */
    if (wc_SetSubjectKeyIdFromPublicKey(&myCert, NULL, &certPubKey) != 0) {
        ERROR_OUT(-6728, exit);
    }

    /* add AKID from the Public Key */
    if (wc_SetAuthKeyIdFromPublicKey(&myCert, NULL, &caEccKey) != 0) {
        ERROR_OUT(-6729, exit);
    }

    /* add Key Usage */
    if (wc_SetKeyUsage(&myCert, certKeyUsage) != 0) {
        ERROR_OUT(-6730, exit);
    }
#endif /* WOLFSSL_CERT_EXT */

#ifdef ENABLE_ECC384_CERT_GEN_TEST
    #if defined(USE_CERT_BUFFERS_256)
    ret = wc_SetIssuerBuffer(&myCert, ca_ecc_cert_der_384,
                                      sizeof_ca_ecc_cert_der_384);
#else
    ret = wc_SetIssuer(&myCert, eccCaCert384File);
    (void)eccCaCertFile;
#endif
#else
#if defined(USE_CERT_BUFFERS_256)
    ret = wc_SetIssuerBuffer(&myCert, ca_ecc_cert_der_256,
                                      sizeof_ca_ecc_cert_der_256);
#else
    ret = wc_SetIssuer(&myCert, eccCaCertFile);
    (void)eccCaCert384File;
#endif
#endif /* ENABLE_ECC384_CERT_GEN_TEST */
    if (ret < 0) {
        ERROR_OUT(-6731, exit);
    }

    certSz = wc_MakeCert(&myCert, der, FOURK_BUF, NULL, &certPubKey, rng);
    if (certSz < 0) {
        ERROR_OUT(-6732, exit);
    }

    ret = 0;
    do {
    #if defined(WOLFSSL_ASYNC_CRYPT)
        ret = wc_AsyncWait(ret, &caEccKey.asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
    #endif
        if (ret >= 0) {
            ret = wc_SignCert(myCert.bodySz, myCert.sigType, der,
                              FOURK_BUF, NULL, &caEccKey, rng);
        }
    } while (ret == WC_PENDING_E);
    if (ret < 0) {
        ERROR_OUT(-6733, exit);
    }
    certSz = ret;

#ifdef WOLFSSL_TEST_CERT
    InitDecodedCert(&decode, der, certSz, 0);
    ret = ParseCert(&decode, CERT_TYPE, NO_VERIFY, 0);
    if (ret != 0) {
        FreeDecodedCert(&decode);
        ERROR_OUT(-6734, exit);

    }
    FreeDecodedCert(&decode);
#endif

    ret = SaveDerAndPem(der, certSz, pem, FOURK_BUF, certEccDerFile,
        certEccPemFile, CERT_TYPE, -6735);
    if (ret != 0) {
        goto exit;
    }

exit:
    wc_ecc_free(&certPubKey);
    wc_ecc_free(&caEccKey);

    XFREE(pem, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

    return ret;
}
#endif /* WOLFSSL_CERT_GEN */

int ecc_test(void)
{
    int ret;
    WC_RNG  rng;

#ifdef WOLFSSL_CERT_EXT
    ret = ecc_decode_test();
    if (ret < 0)
        return ret;
#endif

#ifndef HAVE_FIPS
    ret = wc_InitRng_ex(&rng, HEAP_HINT, devId);
#else
    ret = wc_InitRng(&rng);
#endif
    if (ret != 0)
        return -6800;

#if defined(HAVE_ECC112) || defined(HAVE_ALL_CURVES)
    ret = ecc_test_curve(&rng, 14);
    if (ret < 0) {
        goto done;
    }
#endif /* HAVE_ECC112 */
#if defined(HAVE_ECC128) || defined(HAVE_ALL_CURVES)
    ret = ecc_test_curve(&rng, 16);
    if (ret < 0) {
        goto done;
    }
#endif /* HAVE_ECC128 */
#if defined(HAVE_ECC160) || defined(HAVE_ALL_CURVES)
    ret = ecc_test_curve(&rng, 20);
    if (ret < 0) {
        goto done;
    }
#endif /* HAVE_ECC160 */
#if defined(HAVE_ECC192) || defined(HAVE_ALL_CURVES)
    ret = ecc_test_curve(&rng, 24);
    if (ret < 0) {
        goto done;
    }
#endif /* HAVE_ECC192 */
#if defined(HAVE_ECC224) || defined(HAVE_ALL_CURVES)
    ret = ecc_test_curve(&rng, 28);
    if (ret < 0) {
        goto done;
    }
#endif /* HAVE_ECC224 */
#if defined(HAVE_ECC239) || defined(HAVE_ALL_CURVES)
    ret = ecc_test_curve(&rng, 30);
    if (ret < 0) {
        goto done;
    }
#endif /* HAVE_ECC239 */
#if !defined(NO_ECC256) || defined(HAVE_ALL_CURVES)
    ret = ecc_test_curve(&rng, 32);
    if (ret < 0) {
        goto done;
    }
#if !defined(WOLFSSL_ATECC508A) && defined(HAVE_ECC_KEY_IMPORT) && \
     defined(HAVE_ECC_KEY_EXPORT)
    ret = ecc_point_test();
    if (ret < 0) {
        goto done;
    }
#endif
    ret = ecc_def_curve_test(&rng);
    if (ret < 0) {
        goto done;
    }
#endif /* !NO_ECC256 */
#if defined(HAVE_ECC320) || defined(HAVE_ALL_CURVES)
    ret = ecc_test_curve(&rng, 40);
    if (ret < 0) {
        goto done;
    }
#endif /* HAVE_ECC320 */
#if defined(HAVE_ECC384) || defined(HAVE_ALL_CURVES)
    ret = ecc_test_curve(&rng, 48);
    if (ret < 0) {
        goto done;
    }
#endif /* HAVE_ECC384 */
#if defined(HAVE_ECC512) || defined(HAVE_ALL_CURVES)
    ret = ecc_test_curve(&rng, 64);
    if (ret < 0) {
        goto done;
    }
#endif /* HAVE_ECC512 */
#if defined(HAVE_ECC521) || defined(HAVE_ALL_CURVES)
    ret = ecc_test_curve(&rng, 66);
    if (ret < 0) {
        goto done;
    }
#endif /* HAVE_ECC521 */

#if defined(WOLFSSL_CUSTOM_CURVES)
    ret = ecc_test_custom_curves(&rng);
    if (ret != 0) {
        goto done;
    }
#endif

#ifdef HAVE_ECC_CDH
    ret = ecc_test_cdh_vectors();
    if (ret != 0) {
        printf("ecc_test_cdh_vectors failed! %d\n", ret);
        goto done;
    }
#endif

    ret = ecc_test_make_pub(&rng);
    if (ret != 0) {
        printf("ecc_test_make_pub failed!: %d\n", ret);
        goto done;
    }

#ifdef WOLFSSL_CERT_GEN
    ret = ecc_test_cert_gen(&rng);
    if (ret != 0) {
        printf("ecc_test_cert_gen failed!: %d\n", ret);
        goto done;
    }
#endif

done:
    wc_FreeRng(&rng);

    return ret;
}

#ifdef HAVE_ECC_ENCRYPT

int ecc_encrypt_test(void)
{
    WC_RNG  rng;
    int     ret = 0;
    ecc_key userA, userB;
    byte    msg[48];
    byte    plain[48];
    byte    out[80];
    word32  outSz   = sizeof(out);
    word32  plainSz = sizeof(plain);
    int     i;
    ecEncCtx* cliCtx = NULL;
    ecEncCtx* srvCtx = NULL;
    byte cliSalt[EXCHANGE_SALT_SZ];
    byte srvSalt[EXCHANGE_SALT_SZ];
    const byte* tmpSalt;
    byte    msg2[48];
    byte    plain2[48];
    byte    out2[80];
    word32  outSz2   = sizeof(out2);
    word32  plainSz2 = sizeof(plain2);

#ifndef HAVE_FIPS
    ret = wc_InitRng_ex(&rng, HEAP_HINT, devId);
#else
    ret = wc_InitRng(&rng);
#endif
    if (ret != 0)
        return -6900;

    XMEMSET(&userA, 0, sizeof(userA));
    XMEMSET(&userB, 0, sizeof(userB));

    ret = wc_ecc_init_ex(&userA, HEAP_HINT, devId);
    if (ret != 0)
        goto done;
    ret = wc_ecc_init_ex(&userB, HEAP_HINT, devId);
    if (ret != 0)
        goto done;

    ret  = wc_ecc_make_key(&rng, 32, &userA);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &userA.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0){
        ret = -6901; goto done;
    }

    ret = wc_ecc_make_key(&rng, 32, &userB);
#if defined(WOLFSSL_ASYNC_CRYPT)
    ret = wc_AsyncWait(ret, &userB.asyncDev, WC_ASYNC_FLAG_NONE);
#endif
    if (ret != 0){
        ret = -6902; goto done;
    }

    /* set message to incrementing 0,1,2,etc... */
    for (i = 0; i < (int)sizeof(msg); i++)
        msg[i] = i;

    /* encrypt msg to B */
    ret = wc_ecc_encrypt(&userA, &userB, msg, sizeof(msg), out, &outSz, NULL);
    if (ret != 0) {
        ret = -6903; goto done;
    }

    /* decrypt msg from A */
    ret = wc_ecc_decrypt(&userB, &userA, out, outSz, plain, &plainSz, NULL);
    if (ret != 0) {
        ret = -6904; goto done;
    }

    if (XMEMCMP(plain, msg, sizeof(msg)) != 0) {
        ret = -6905; goto done;
    }

    /* let's verify message exchange works, A is client, B is server */
    cliCtx = wc_ecc_ctx_new(REQ_RESP_CLIENT, &rng);
    srvCtx = wc_ecc_ctx_new(REQ_RESP_SERVER, &rng);
    if (cliCtx == NULL || srvCtx == NULL) {
        ret = -6906; goto done;
    }

    /* get salt to send to peer */
    tmpSalt = wc_ecc_ctx_get_own_salt(cliCtx);
    if (tmpSalt == NULL) {
        ret = -6907; goto done;
    }
    XMEMCPY(cliSalt, tmpSalt, EXCHANGE_SALT_SZ);

    tmpSalt = wc_ecc_ctx_get_own_salt(srvCtx);
    if (tmpSalt == NULL) {
        ret = -6908; goto done;
    }
    XMEMCPY(srvSalt, tmpSalt, EXCHANGE_SALT_SZ);

    /* in actual use, we'd get the peer's salt over the transport */
    ret = wc_ecc_ctx_set_peer_salt(cliCtx, srvSalt);
    if (ret != 0)
        goto done;
    ret = wc_ecc_ctx_set_peer_salt(srvCtx, cliSalt);
    if (ret != 0)
        goto done;

    ret = wc_ecc_ctx_set_info(cliCtx, (byte*)"wolfSSL MSGE", 11);
    if (ret != 0)
        goto done;
    ret = wc_ecc_ctx_set_info(srvCtx, (byte*)"wolfSSL MSGE", 11);
    if (ret != 0)
        goto done;

    /* get encrypted msg (request) to send to B */
    outSz = sizeof(out);
    ret = wc_ecc_encrypt(&userA, &userB, msg, sizeof(msg), out, &outSz,cliCtx);
    if (ret != 0)
        goto done;

    /* B decrypts msg (request) from A */
    plainSz = sizeof(plain);
    ret = wc_ecc_decrypt(&userB, &userA, out, outSz, plain, &plainSz, srvCtx);
    if (ret != 0)
        goto done;

    if (XMEMCMP(plain, msg, sizeof(msg)) != 0) {
        ret = -6909; goto done;
    }

    /* msg2 (response) from B to A */
    for (i = 0; i < (int)sizeof(msg2); i++)
        msg2[i] = i + sizeof(msg2);

    /* get encrypted msg (response) to send to B */
    ret = wc_ecc_encrypt(&userB, &userA, msg2, sizeof(msg2), out2,
                      &outSz2, srvCtx);
    if (ret != 0)
        goto done;

    /* A decrypts msg (response) from B */
    ret = wc_ecc_decrypt(&userA, &userB, out2, outSz2, plain2, &plainSz2,
                     cliCtx);
    if (ret != 0)
        goto done;

    if (XMEMCMP(plain2, msg2, sizeof(msg2)) != 0) {
        ret = -6910; goto done;
    }

done:

    /* cleanup */
    wc_ecc_ctx_free(srvCtx);
    wc_ecc_ctx_free(cliCtx);

    wc_ecc_free(&userB);
    wc_ecc_free(&userA);
    wc_FreeRng(&rng);

    return ret;
}

#endif /* HAVE_ECC_ENCRYPT */

#ifdef USE_CERT_BUFFERS_256
int ecc_test_buffers(void) {
    size_t bytes;
    ecc_key cliKey;
    ecc_key servKey;
    WC_RNG rng;
    word32 idx = 0;
    int    ret;
    /* pad our test message to 32 bytes so evenly divisible by AES_BLOCK_SZ */
    byte   in[] = "Everyone gets Friday off. ecc p";
    word32 inLen = (word32)XSTRLEN((char*)in);
    byte   out[256];
    byte   plain[256];
    int verify = 0;
    word32 x;

    bytes = (size_t)sizeof_ecc_clikey_der_256;
    /* place client key into ecc_key struct cliKey */
    ret = wc_EccPrivateKeyDecode(ecc_clikey_der_256, &idx, &cliKey,
                                                                (word32)bytes);
    if (ret != 0)
        return -6915;

    idx = 0;
    bytes = (size_t)sizeof_ecc_key_der_256;

    /* place server key into ecc_key struct servKey */
    ret = wc_EccPrivateKeyDecode(ecc_key_der_256, &idx, &servKey,
                                                                (word32)bytes);
    if (ret != 0)
        return -6916;

#ifndef HAVE_FIPS
    ret = wc_InitRng_ex(&rng, HEAP_HINT, devId);
#else
    ret = wc_InitRng(&rng);
#endif
    if (ret != 0)
        return -6917;

#if defined(HAVE_ECC_ENCRYPT) && defined(HAVE_HKDF)
    {
        word32 y;
        /* test encrypt and decrypt if they're available */
        x = sizeof(out);
        ret = wc_ecc_encrypt(&cliKey, &servKey, in, sizeof(in), out, &x, NULL);
        if (ret < 0)
            return -6918;

        y = sizeof(plain);
        ret = wc_ecc_decrypt(&cliKey, &servKey, out, x, plain, &y, NULL);
        if (ret < 0)
            return -6919;

        if (XMEMCMP(plain, in, inLen))
            return -6920;
    }
#endif


    x = sizeof(out);
    ret = wc_ecc_sign_hash(in, inLen, out, &x, &rng, &cliKey);
    if (ret < 0)
        return -6921;

    XMEMSET(plain, 0, sizeof(plain));

    ret = wc_ecc_verify_hash(out, x, plain, sizeof(plain), &verify, &cliKey);
    if (ret < 0)
        return -6922;

    if (XMEMCMP(plain, in, (word32)ret))
        return -6923;

#ifdef WOLFSSL_CERT_EXT
    idx = 0;

    bytes = sizeof_ecc_clikeypub_der_256;

    ret = wc_EccPublicKeyDecode(ecc_clikeypub_der_256, &idx, &cliKey,
                                                               (word32) bytes);
    if (ret != 0)
        return -6924;
#endif

    wc_ecc_free(&cliKey);
    wc_ecc_free(&servKey);
    wc_FreeRng(&rng);

    return 0;
}
#endif /* USE_CERT_BUFFERS_256 */
#endif /* HAVE_ECC */


#ifdef HAVE_CURVE25519

int curve25519_test(void)
{
    WC_RNG  rng;
    int ret;
#ifdef HAVE_CURVE25519_SHARED_SECRET
    byte    sharedA[32];
    byte    sharedB[32];
    word32  y;
#endif
#ifdef HAVE_CURVE25519_KEY_EXPORT
    byte    exportBuf[32];
#endif
    word32  x;
    curve25519_key userA, userB, pubKey;

#if defined(HAVE_CURVE25519_SHARED_SECRET) && defined(HAVE_CURVE25519_KEY_IMPORT)
    /* test vectors from
       https://tools.ietf.org/html/draft-josefsson-tls-curve25519-03
     */

    /* secret key for party a */
    byte sa[] = {
        0x5A,0xC9,0x9F,0x33,0x63,0x2E,0x5A,0x76,
        0x8D,0xE7,0xE8,0x1B,0xF8,0x54,0xC2,0x7C,
        0x46,0xE3,0xFB,0xF2,0xAB,0xBA,0xCD,0x29,
        0xEC,0x4A,0xFF,0x51,0x73,0x69,0xC6,0x60
    };

    /* public key for party a */
    byte pa[] = {
        0x05,0x7E,0x23,0xEA,0x9F,0x1C,0xBE,0x8A,
        0x27,0x16,0x8F,0x6E,0x69,0x6A,0x79,0x1D,
        0xE6,0x1D,0xD3,0xAF,0x7A,0xCD,0x4E,0xEA,
        0xCC,0x6E,0x7B,0xA5,0x14,0xFD,0xA8,0x63
    };

    /* secret key for party b */
    byte sb[] = {
        0x47,0xDC,0x3D,0x21,0x41,0x74,0x82,0x0E,
        0x11,0x54,0xB4,0x9B,0xC6,0xCD,0xB2,0xAB,
        0xD4,0x5E,0xE9,0x58,0x17,0x05,0x5D,0x25,
        0x5A,0xA3,0x58,0x31,0xB7,0x0D,0x32,0x60
    };

    /* public key for party b */
    byte pb[] = {
        0x6E,0xB8,0x9D,0xA9,0x19,0x89,0xAE,0x37,
        0xC7,0xEA,0xC7,0x61,0x8D,0x9E,0x5C,0x49,
        0x51,0xDB,0xA1,0xD7,0x3C,0x28,0x5A,0xE1,
        0xCD,0x26,0xA8,0x55,0x02,0x0E,0xEF,0x04
    };

    /* expected shared key */
    byte ss[] = {
        0x61,0x45,0x0C,0xD9,0x8E,0x36,0x01,0x6B,
        0x58,0x77,0x6A,0x89,0x7A,0x9F,0x0A,0xEF,
        0x73,0x8B,0x99,0xF0,0x94,0x68,0xB8,0xD6,
        0xB8,0x51,0x11,0x84,0xD5,0x34,0x94,0xAB
    };
#endif /* HAVE_CURVE25519_SHARED_SECRET */

#ifndef HAVE_FIPS
    ret = wc_InitRng_ex(&rng, HEAP_HINT, devId);
#else
    ret = wc_InitRng(&rng);
#endif
    if (ret != 0)
        return -7000;

    wc_curve25519_init(&userA);
    wc_curve25519_init(&userB);
    wc_curve25519_init(&pubKey);

    /* make curve25519 keys */
    if (wc_curve25519_make_key(&rng, 32, &userA) != 0)
        return -7001;

    if (wc_curve25519_make_key(&rng, 32, &userB) != 0)
        return -7002;

#ifdef HAVE_CURVE25519_SHARED_SECRET
    /* find shared secret key */
    x = sizeof(sharedA);
    if (wc_curve25519_shared_secret(&userA, &userB, sharedA, &x) != 0)
        return -7003;

    y = sizeof(sharedB);
    if (wc_curve25519_shared_secret(&userB, &userA, sharedB, &y) != 0)
        return -7004;

    /* compare shared secret keys to test they are the same */
    if (y != x)
        return -7005;

    if (XMEMCMP(sharedA, sharedB, x))
        return -7006;
#endif

#ifdef HAVE_CURVE25519_KEY_EXPORT
    /* export a public key and import it for another user */
    x = sizeof(exportBuf);
    if (wc_curve25519_export_public(&userA, exportBuf, &x) != 0)
        return -7007;

#ifdef HAVE_CURVE25519_KEY_IMPORT
    if (wc_curve25519_import_public(exportBuf, x, &pubKey) != 0)
        return -7008;
#endif
#endif

#if defined(HAVE_CURVE25519_SHARED_SECRET) && defined(HAVE_CURVE25519_KEY_IMPORT)
    /* test shared key after importing a public key */
    XMEMSET(sharedB, 0, sizeof(sharedB));
    y = sizeof(sharedB);
    if (wc_curve25519_shared_secret(&userB, &pubKey, sharedB, &y) != 0)
        return -7009;

    if (XMEMCMP(sharedA, sharedB, y))
        return -7010;

    /* import RFC test vectors and compare shared key */
    if (wc_curve25519_import_private_raw(sa, sizeof(sa), pa, sizeof(pa), &userA)
            != 0)
        return -7011;

    if (wc_curve25519_import_private_raw(sb, sizeof(sb), pb, sizeof(pb), &userB)
            != 0)
        return -7012;

    /* test against known test vector */
    XMEMSET(sharedB, 0, sizeof(sharedB));
    y = sizeof(sharedB);
    if (wc_curve25519_shared_secret(&userA, &userB, sharedB, &y) != 0)
        return -7013;

    if (XMEMCMP(ss, sharedB, y))
        return -7014;

    /* test swaping roles of keys and generating same shared key */
    XMEMSET(sharedB, 0, sizeof(sharedB));
    y = sizeof(sharedB);
    if (wc_curve25519_shared_secret(&userB, &userA, sharedB, &y) != 0)
        return -7015;

    if (XMEMCMP(ss, sharedB, y))
        return -7016;

    /* test with 1 generated key and 1 from known test vector */
    if (wc_curve25519_import_private_raw(sa, sizeof(sa), pa, sizeof(pa), &userA)
        != 0)
        return -7017;

    if (wc_curve25519_make_key(&rng, 32, &userB) != 0)
        return -7018;

    x = sizeof(sharedA);
    if (wc_curve25519_shared_secret(&userA, &userB, sharedA, &x) != 0)
        return -7019;

    y = sizeof(sharedB);
    if (wc_curve25519_shared_secret(&userB, &userA, sharedB, &y) != 0)
        return -7020;

    /* compare shared secret keys to test they are the same */
    if (y != x)
        return -7021;

    if (XMEMCMP(sharedA, sharedB, x))
        return -7022;
#endif /* HAVE_CURVE25519_SHARED_SECRET */

    /* clean up keys when done */
    wc_curve25519_free(&pubKey);
    wc_curve25519_free(&userB);
    wc_curve25519_free(&userA);

    wc_FreeRng(&rng);

    return 0;
}
#endif /* HAVE_CURVE25519 */


#ifdef HAVE_ED25519
#ifdef WOLFSSL_TEST_CERT
static int ed25519_test_cert(void)
{
    DecodedCert  cert[2];
    DecodedCert* serverCert = NULL;
    DecodedCert* caCert = NULL;
#ifdef HAVE_ED25519_VERIFY
    ed25519_key  key;
    ed25519_key* pubKey = NULL;
    int          verify;
#endif /* HAVE_ED25519_VERIFY */
    int          ret;
    byte*        tmp;
    size_t       bytes;
    FILE*        file;

    tmp = XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (tmp == NULL) {
        ERROR_OUT(-7200, done);
    }

#ifdef USE_CERT_BUFFERS_256
    XMEMCPY(tmp, ca_ed25519_cert, sizeof_ca_ed25519_cert);
    bytes = sizeof_ca_ed25519_cert;
#elif !defined(NO_FILESYSTEM)
    file = fopen(caEd25519Cert, "rb");
    if (file == NULL) {
        ERROR_OUT(-7201, done);
    }
    bytes = fread(tmp, 1, FOURK_BUF, file);
    fclose(file);
#else
    /* No certificate to use. */
    ERROR_OUT(-7202, done);
#endif

    InitDecodedCert(&cert[0], tmp, (word32)bytes, 0);
    caCert = &cert[0];
    ret = ParseCert(caCert, CERT_TYPE, NO_VERIFY, NULL);
    if (ret != 0) {
        ERROR_OUT(-7203, done);
    }

#ifdef USE_CERT_BUFFERS_256
    XMEMCPY(tmp, server_ed25519_cert, sizeof_server_ed25519_cert);
    bytes = sizeof_server_ed25519_cert;
#elif !defined(NO_FILESYSTEM)
    file = fopen(serverEd25519Cert, "rb");
    if (file == NULL) {
        ERROR_OUT(-7204, done);
    }
    bytes = fread(tmp, 1, FOURK_BUF, file);
    fclose(file);
#else
    /* No certificate to use. */
    ERROR_OUT(-7205, done);
#endif

    InitDecodedCert(&cert[1], tmp, (word32)bytes, 0);
    serverCert = &cert[1];
    ret = ParseCert(serverCert, CERT_TYPE, NO_VERIFY, NULL);
    if (ret != 0) {
        ERROR_OUT(-7206, done);
    }

#ifdef HAVE_ED25519_VERIFY
    ret = wc_ed25519_init(&key);
    if (ret < 0) {
        ERROR_OUT(-7207, done);
    }
    pubKey = &key;
    ret = wc_ed25519_import_public(caCert->publicKey, caCert->pubKeySize,
                                                                        pubKey);
    if (ret < 0) {
        ERROR_OUT(-7208, done);
    }

    if (wc_ed25519_verify_msg(serverCert->signature, serverCert->sigLength,
                              serverCert->source + serverCert->certBegin,
                              serverCert->sigIndex - serverCert->certBegin,
                              &verify, pubKey) < 0 || verify != 1) {
        ERROR_OUT(-7209, done);
    }
#endif /* HAVE_ED25519_VERIFY */

done:
    if (tmp != NULL)
        XFREE(tmp, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
#ifdef HAVE_ED25519_VERIFY
    wc_ed25519_free(pubKey);
#endif /* HAVE_ED25519_VERIFY */
    if (caCert != NULL)
        FreeDecodedCert(caCert);
    if (serverCert != NULL)
        FreeDecodedCert(serverCert);

    return ret;
}

static int ed25519_test_make_cert(void)
{
    WC_RNG       rng;
    Cert         cert;
    DecodedCert  decode;
    ed25519_key  key;
    ed25519_key* privKey = NULL;
    int          ret = 0;
    byte*        tmp = NULL;

    wc_InitCert(&cert);

#ifndef HAVE_FIPS
    ret = wc_InitRng_ex(&rng, HEAP_HINT, devId);
#else
    ret = wc_InitRng(&rng);
#endif
    if (ret != 0)
        return -7220;

    wc_ed25519_init(&key);
    privKey = &key;
    wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, privKey);

    cert.daysValid = 365 * 2;
    cert.selfSigned = 1;
    XMEMCPY(&cert.issuer, &certDefaultName, sizeof(CertName));
    XMEMCPY(&cert.subject, &certDefaultName, sizeof(CertName));
    cert.isCA = 0;
#ifdef WOLFSSL_CERT_EXT
    ret = wc_SetKeyUsage(&cert, certKeyUsage);
    if (ret < 0) {
        ERROR_OUT(-7221, done);
    }
    ret = wc_SetSubjectKeyIdFromPublicKey_ex(&cert, ED25519_TYPE, privKey);
    if (ret < 0) {
        ERROR_OUT(-7222, done);
    }
    ret = wc_SetAuthKeyIdFromPublicKey_ex(&cert, ED25519_TYPE, privKey);
    if (ret < 0) {
        ERROR_OUT(-7223, done);
    }
#endif
    tmp = XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (tmp == NULL) {
        ERROR_OUT(-7224, done);
    }

    cert.sigType = CTC_ED25519;
    ret = wc_MakeCert_ex(&cert, tmp, FOURK_BUF, ED25519_TYPE, privKey, &rng);
    if (ret < 0) {
        ERROR_OUT(-7225, done);
    }
    ret = wc_SignCert_ex(cert.bodySz, cert.sigType, tmp, FOURK_BUF,
                                                   ED25519_TYPE, privKey, &rng);
    if (ret < 0) {
        ERROR_OUT(-7226, done);
    }

    InitDecodedCert(&decode, tmp, ret, HEAP_HINT);
    ret = ParseCert(&decode, CERT_TYPE, NO_VERIFY, 0);
    FreeDecodedCert(&decode);
    if (ret != 0) {
        ERROR_OUT(-7227, done);
    }

done:
    if (tmp != NULL)
        XFREE(tmp, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    wc_ed25519_free(privKey);
    wc_FreeRng(&rng);
    return ret;
}
#endif /* WOLFSSL_TEST_CERT */

int ed25519_test(void)
{
    int ret;
    WC_RNG rng;
#if defined(HAVE_ED25519_SIGN) && defined(HAVE_ED25519_KEY_EXPORT) &&\
    defined(HAVE_ED25519_KEY_IMPORT)
    byte   out[ED25519_SIG_SIZE];
    byte   exportPKey[ED25519_KEY_SIZE];
    byte   exportSKey[ED25519_KEY_SIZE];
    word32 exportPSz;
    word32 exportSSz;
    int    i;
    word32 outlen;
#ifdef HAVE_ED25519_VERIFY
    int    verify;
#endif /* HAVE_ED25519_VERIFY */
#endif /* HAVE_ED25519_SIGN && HAVE_ED25519_KEY_EXPORT && HAVE_ED25519_KEY_IMPORT */
    word32 keySz, sigSz;
    ed25519_key key;
    ed25519_key key2;

#if defined(HAVE_ED25519_SIGN) && defined(HAVE_ED25519_KEY_EXPORT) && \
    defined(HAVE_ED25519_KEY_IMPORT)
    /* test vectors from
       https://tools.ietf.org/html/draft-josefsson-eddsa-ed25519-02
     */

    static const byte sKey1[] = {
        0x9d,0x61,0xb1,0x9d,0xef,0xfd,0x5a,0x60,
        0xba,0x84,0x4a,0xf4,0x92,0xec,0x2c,0xc4,
        0x44,0x49,0xc5,0x69,0x7b,0x32,0x69,0x19,
        0x70,0x3b,0xac,0x03,0x1c,0xae,0x7f,0x60
    };

    static const byte sKey2[] = {
        0x4c,0xcd,0x08,0x9b,0x28,0xff,0x96,0xda,
        0x9d,0xb6,0xc3,0x46,0xec,0x11,0x4e,0x0f,
        0x5b,0x8a,0x31,0x9f,0x35,0xab,0xa6,0x24,
        0xda,0x8c,0xf6,0xed,0x4f,0xb8,0xa6,0xfb
    };

    static const byte sKey3[] = {
        0xc5,0xaa,0x8d,0xf4,0x3f,0x9f,0x83,0x7b,
        0xed,0xb7,0x44,0x2f,0x31,0xdc,0xb7,0xb1,
        0x66,0xd3,0x85,0x35,0x07,0x6f,0x09,0x4b,
        0x85,0xce,0x3a,0x2e,0x0b,0x44,0x58,0xf7
    };

    /* uncompressed test */
    static const byte sKey4[] = {
        0x9d,0x61,0xb1,0x9d,0xef,0xfd,0x5a,0x60,
        0xba,0x84,0x4a,0xf4,0x92,0xec,0x2c,0xc4,
        0x44,0x49,0xc5,0x69,0x7b,0x32,0x69,0x19,
        0x70,0x3b,0xac,0x03,0x1c,0xae,0x7f,0x60
    };

    /* compressed prefix test */
    static const byte sKey5[] = {
        0x9d,0x61,0xb1,0x9d,0xef,0xfd,0x5a,0x60,
        0xba,0x84,0x4a,0xf4,0x92,0xec,0x2c,0xc4,
        0x44,0x49,0xc5,0x69,0x7b,0x32,0x69,0x19,
        0x70,0x3b,0xac,0x03,0x1c,0xae,0x7f,0x60
    };

    static const byte sKey6[] = {
        0xf5,0xe5,0x76,0x7c,0xf1,0x53,0x31,0x95,
        0x17,0x63,0x0f,0x22,0x68,0x76,0xb8,0x6c,
        0x81,0x60,0xcc,0x58,0x3b,0xc0,0x13,0x74,
        0x4c,0x6b,0xf2,0x55,0xf5,0xcc,0x0e,0xe5
    };

    static const byte* sKeys[] = {sKey1, sKey2, sKey3, sKey4, sKey5, sKey6};

    static const byte pKey1[] = {
        0xd7,0x5a,0x98,0x01,0x82,0xb1,0x0a,0xb7,
        0xd5,0x4b,0xfe,0xd3,0xc9,0x64,0x07,0x3a,
        0x0e,0xe1,0x72,0xf3,0xda,0xa6,0x23,0x25,
        0xaf,0x02,0x1a,0x68,0xf7,0x07,0x51,0x1a
    };

    static const byte pKey2[] = {
        0x3d,0x40,0x17,0xc3,0xe8,0x43,0x89,0x5a,
        0x92,0xb7,0x0a,0xa7,0x4d,0x1b,0x7e,0xbc,
        0x9c,0x98,0x2c,0xcf,0x2e,0xc4,0x96,0x8c,
        0xc0,0xcd,0x55,0xf1,0x2a,0xf4,0x66,0x0c
    };

    static const byte pKey3[] = {
        0xfc,0x51,0xcd,0x8e,0x62,0x18,0xa1,0xa3,
        0x8d,0xa4,0x7e,0xd0,0x02,0x30,0xf0,0x58,
        0x08,0x16,0xed,0x13,0xba,0x33,0x03,0xac,
        0x5d,0xeb,0x91,0x15,0x48,0x90,0x80,0x25
    };

    /* uncompressed test */
    static const byte pKey4[] = {
        0x04,0x55,0xd0,0xe0,0x9a,0x2b,0x9d,0x34,
        0x29,0x22,0x97,0xe0,0x8d,0x60,0xd0,0xf6,
        0x20,0xc5,0x13,0xd4,0x72,0x53,0x18,0x7c,
        0x24,0xb1,0x27,0x86,0xbd,0x77,0x76,0x45,
        0xce,0x1a,0x51,0x07,0xf7,0x68,0x1a,0x02,
        0xaf,0x25,0x23,0xa6,0xda,0xf3,0x72,0xe1,
        0x0e,0x3a,0x07,0x64,0xc9,0xd3,0xfe,0x4b,
        0xd5,0xb7,0x0a,0xb1,0x82,0x01,0x98,0x5a,
        0xd7
    };

    /* compressed prefix */
    static const byte pKey5[] = {
        0x40,0xd7,0x5a,0x98,0x01,0x82,0xb1,0x0a,0xb7,
        0xd5,0x4b,0xfe,0xd3,0xc9,0x64,0x07,0x3a,
        0x0e,0xe1,0x72,0xf3,0xda,0xa6,0x23,0x25,
        0xaf,0x02,0x1a,0x68,0xf7,0x07,0x51,0x1a
    };

    static const byte pKey6[] = {
        0x27,0x81,0x17,0xfc,0x14,0x4c,0x72,0x34,
        0x0f,0x67,0xd0,0xf2,0x31,0x6e,0x83,0x86,
        0xce,0xff,0xbf,0x2b,0x24,0x28,0xc9,0xc5,
        0x1f,0xef,0x7c,0x59,0x7f,0x1d,0x42,0x6e
    };

    static const byte* pKeys[] = {pKey1, pKey2, pKey3, pKey4, pKey5, pKey6};
    static const byte  pKeySz[] = {sizeof(pKey1), sizeof(pKey2), sizeof(pKey3),
                            sizeof(pKey4), sizeof(pKey5), sizeof(pKey6)};

    static const byte sig1[] = {
        0xe5,0x56,0x43,0x00,0xc3,0x60,0xac,0x72,
        0x90,0x86,0xe2,0xcc,0x80,0x6e,0x82,0x8a,
        0x84,0x87,0x7f,0x1e,0xb8,0xe5,0xd9,0x74,
        0xd8,0x73,0xe0,0x65,0x22,0x49,0x01,0x55,
        0x5f,0xb8,0x82,0x15,0x90,0xa3,0x3b,0xac,
        0xc6,0x1e,0x39,0x70,0x1c,0xf9,0xb4,0x6b,
        0xd2,0x5b,0xf5,0xf0,0x59,0x5b,0xbe,0x24,
        0x65,0x51,0x41,0x43,0x8e,0x7a,0x10,0x0b
    };

    static const byte sig2[] = {
        0x92,0xa0,0x09,0xa9,0xf0,0xd4,0xca,0xb8,
        0x72,0x0e,0x82,0x0b,0x5f,0x64,0x25,0x40,
        0xa2,0xb2,0x7b,0x54,0x16,0x50,0x3f,0x8f,
        0xb3,0x76,0x22,0x23,0xeb,0xdb,0x69,0xda,
        0x08,0x5a,0xc1,0xe4,0x3e,0x15,0x99,0x6e,
        0x45,0x8f,0x36,0x13,0xd0,0xf1,0x1d,0x8c,
        0x38,0x7b,0x2e,0xae,0xb4,0x30,0x2a,0xee,
        0xb0,0x0d,0x29,0x16,0x12,0xbb,0x0c,0x00
    };

    static const byte sig3[] = {
        0x62,0x91,0xd6,0x57,0xde,0xec,0x24,0x02,
        0x48,0x27,0xe6,0x9c,0x3a,0xbe,0x01,0xa3,
        0x0c,0xe5,0x48,0xa2,0x84,0x74,0x3a,0x44,
        0x5e,0x36,0x80,0xd7,0xdb,0x5a,0xc3,0xac,
        0x18,0xff,0x9b,0x53,0x8d,0x16,0xf2,0x90,
        0xae,0x67,0xf7,0x60,0x98,0x4d,0xc6,0x59,
        0x4a,0x7c,0x15,0xe9,0x71,0x6e,0xd2,0x8d,
        0xc0,0x27,0xbe,0xce,0xea,0x1e,0xc4,0x0a
    };

    /* uncompressed test */
    static const byte sig4[] = {
        0xe5,0x56,0x43,0x00,0xc3,0x60,0xac,0x72,
        0x90,0x86,0xe2,0xcc,0x80,0x6e,0x82,0x8a,
        0x84,0x87,0x7f,0x1e,0xb8,0xe5,0xd9,0x74,
        0xd8,0x73,0xe0,0x65,0x22,0x49,0x01,0x55,
        0x5f,0xb8,0x82,0x15,0x90,0xa3,0x3b,0xac,
        0xc6,0x1e,0x39,0x70,0x1c,0xf9,0xb4,0x6b,
        0xd2,0x5b,0xf5,0xf0,0x59,0x5b,0xbe,0x24,
        0x65,0x51,0x41,0x43,0x8e,0x7a,0x10,0x0b
    };

    /* compressed prefix */
    static const byte sig5[] = {
        0xe5,0x56,0x43,0x00,0xc3,0x60,0xac,0x72,
        0x90,0x86,0xe2,0xcc,0x80,0x6e,0x82,0x8a,
        0x84,0x87,0x7f,0x1e,0xb8,0xe5,0xd9,0x74,
        0xd8,0x73,0xe0,0x65,0x22,0x49,0x01,0x55,
        0x5f,0xb8,0x82,0x15,0x90,0xa3,0x3b,0xac,
        0xc6,0x1e,0x39,0x70,0x1c,0xf9,0xb4,0x6b,
        0xd2,0x5b,0xf5,0xf0,0x59,0x5b,0xbe,0x24,
        0x65,0x51,0x41,0x43,0x8e,0x7a,0x10,0x0b
    };

    static const byte sig6[] = {
        0x0a,0xab,0x4c,0x90,0x05,0x01,0xb3,0xe2,
        0x4d,0x7c,0xdf,0x46,0x63,0x32,0x6a,0x3a,
        0x87,0xdf,0x5e,0x48,0x43,0xb2,0xcb,0xdb,
        0x67,0xcb,0xf6,0xe4,0x60,0xfe,0xc3,0x50,
        0xaa,0x53,0x71,0xb1,0x50,0x8f,0x9f,0x45,
        0x28,0xec,0xea,0x23,0xc4,0x36,0xd9,0x4b,
        0x5e,0x8f,0xcd,0x4f,0x68,0x1e,0x30,0xa6,
        0xac,0x00,0xa9,0x70,0x4a,0x18,0x8a,0x03
    };

    static const byte* sigs[] = {sig1, sig2, sig3, sig4, sig5, sig6};

    static const byte msg1[]  = {0x0 };
    static const byte msg2[]  = {0x72};
    static const byte msg3[]  = {0xAF,0x82};

    /* test of a 1024 byte long message */
    static const byte msg4[]  = {
        0x08,0xb8,0xb2,0xb7,0x33,0x42,0x42,0x43,
        0x76,0x0f,0xe4,0x26,0xa4,0xb5,0x49,0x08,
        0x63,0x21,0x10,0xa6,0x6c,0x2f,0x65,0x91,
        0xea,0xbd,0x33,0x45,0xe3,0xe4,0xeb,0x98,
        0xfa,0x6e,0x26,0x4b,0xf0,0x9e,0xfe,0x12,
        0xee,0x50,0xf8,0xf5,0x4e,0x9f,0x77,0xb1,
        0xe3,0x55,0xf6,0xc5,0x05,0x44,0xe2,0x3f,
        0xb1,0x43,0x3d,0xdf,0x73,0xbe,0x84,0xd8,
        0x79,0xde,0x7c,0x00,0x46,0xdc,0x49,0x96,
        0xd9,0xe7,0x73,0xf4,0xbc,0x9e,0xfe,0x57,
        0x38,0x82,0x9a,0xdb,0x26,0xc8,0x1b,0x37,
        0xc9,0x3a,0x1b,0x27,0x0b,0x20,0x32,0x9d,
        0x65,0x86,0x75,0xfc,0x6e,0xa5,0x34,0xe0,
        0x81,0x0a,0x44,0x32,0x82,0x6b,0xf5,0x8c,
        0x94,0x1e,0xfb,0x65,0xd5,0x7a,0x33,0x8b,
        0xbd,0x2e,0x26,0x64,0x0f,0x89,0xff,0xbc,
        0x1a,0x85,0x8e,0xfc,0xb8,0x55,0x0e,0xe3,
        0xa5,0xe1,0x99,0x8b,0xd1,0x77,0xe9,0x3a,
        0x73,0x63,0xc3,0x44,0xfe,0x6b,0x19,0x9e,
        0xe5,0xd0,0x2e,0x82,0xd5,0x22,0xc4,0xfe,
        0xba,0x15,0x45,0x2f,0x80,0x28,0x8a,0x82,
        0x1a,0x57,0x91,0x16,0xec,0x6d,0xad,0x2b,
        0x3b,0x31,0x0d,0xa9,0x03,0x40,0x1a,0xa6,
        0x21,0x00,0xab,0x5d,0x1a,0x36,0x55,0x3e,
        0x06,0x20,0x3b,0x33,0x89,0x0c,0xc9,0xb8,
        0x32,0xf7,0x9e,0xf8,0x05,0x60,0xcc,0xb9,
        0xa3,0x9c,0xe7,0x67,0x96,0x7e,0xd6,0x28,
        0xc6,0xad,0x57,0x3c,0xb1,0x16,0xdb,0xef,
        0xef,0xd7,0x54,0x99,0xda,0x96,0xbd,0x68,
        0xa8,0xa9,0x7b,0x92,0x8a,0x8b,0xbc,0x10,
        0x3b,0x66,0x21,0xfc,0xde,0x2b,0xec,0xa1,
        0x23,0x1d,0x20,0x6b,0xe6,0xcd,0x9e,0xc7,
        0xaf,0xf6,0xf6,0xc9,0x4f,0xcd,0x72,0x04,
        0xed,0x34,0x55,0xc6,0x8c,0x83,0xf4,0xa4,
        0x1d,0xa4,0xaf,0x2b,0x74,0xef,0x5c,0x53,
        0xf1,0xd8,0xac,0x70,0xbd,0xcb,0x7e,0xd1,
        0x85,0xce,0x81,0xbd,0x84,0x35,0x9d,0x44,
        0x25,0x4d,0x95,0x62,0x9e,0x98,0x55,0xa9,
        0x4a,0x7c,0x19,0x58,0xd1,0xf8,0xad,0xa5,
        0xd0,0x53,0x2e,0xd8,0xa5,0xaa,0x3f,0xb2,
        0xd1,0x7b,0xa7,0x0e,0xb6,0x24,0x8e,0x59,
        0x4e,0x1a,0x22,0x97,0xac,0xbb,0xb3,0x9d,
        0x50,0x2f,0x1a,0x8c,0x6e,0xb6,0xf1,0xce,
        0x22,0xb3,0xde,0x1a,0x1f,0x40,0xcc,0x24,
        0x55,0x41,0x19,0xa8,0x31,0xa9,0xaa,0xd6,
        0x07,0x9c,0xad,0x88,0x42,0x5d,0xe6,0xbd,
        0xe1,0xa9,0x18,0x7e,0xbb,0x60,0x92,0xcf,
        0x67,0xbf,0x2b,0x13,0xfd,0x65,0xf2,0x70,
        0x88,0xd7,0x8b,0x7e,0x88,0x3c,0x87,0x59,
        0xd2,0xc4,0xf5,0xc6,0x5a,0xdb,0x75,0x53,
        0x87,0x8a,0xd5,0x75,0xf9,0xfa,0xd8,0x78,
        0xe8,0x0a,0x0c,0x9b,0xa6,0x3b,0xcb,0xcc,
        0x27,0x32,0xe6,0x94,0x85,0xbb,0xc9,0xc9,
        0x0b,0xfb,0xd6,0x24,0x81,0xd9,0x08,0x9b,
        0xec,0xcf,0x80,0xcf,0xe2,0xdf,0x16,0xa2,
        0xcf,0x65,0xbd,0x92,0xdd,0x59,0x7b,0x07,
        0x07,0xe0,0x91,0x7a,0xf4,0x8b,0xbb,0x75,
        0xfe,0xd4,0x13,0xd2,0x38,0xf5,0x55,0x5a,
        0x7a,0x56,0x9d,0x80,0xc3,0x41,0x4a,0x8d,
        0x08,0x59,0xdc,0x65,0xa4,0x61,0x28,0xba,
        0xb2,0x7a,0xf8,0x7a,0x71,0x31,0x4f,0x31,
        0x8c,0x78,0x2b,0x23,0xeb,0xfe,0x80,0x8b,
        0x82,0xb0,0xce,0x26,0x40,0x1d,0x2e,0x22,
        0xf0,0x4d,0x83,0xd1,0x25,0x5d,0xc5,0x1a,
        0xdd,0xd3,0xb7,0x5a,0x2b,0x1a,0xe0,0x78,
        0x45,0x04,0xdf,0x54,0x3a,0xf8,0x96,0x9b,
        0xe3,0xea,0x70,0x82,0xff,0x7f,0xc9,0x88,
        0x8c,0x14,0x4d,0xa2,0xaf,0x58,0x42,0x9e,
        0xc9,0x60,0x31,0xdb,0xca,0xd3,0xda,0xd9,
        0xaf,0x0d,0xcb,0xaa,0xaf,0x26,0x8c,0xb8,
        0xfc,0xff,0xea,0xd9,0x4f,0x3c,0x7c,0xa4,
        0x95,0xe0,0x56,0xa9,0xb4,0x7a,0xcd,0xb7,
        0x51,0xfb,0x73,0xe6,0x66,0xc6,0xc6,0x55,
        0xad,0xe8,0x29,0x72,0x97,0xd0,0x7a,0xd1,
        0xba,0x5e,0x43,0xf1,0xbc,0xa3,0x23,0x01,
        0x65,0x13,0x39,0xe2,0x29,0x04,0xcc,0x8c,
        0x42,0xf5,0x8c,0x30,0xc0,0x4a,0xaf,0xdb,
        0x03,0x8d,0xda,0x08,0x47,0xdd,0x98,0x8d,
        0xcd,0xa6,0xf3,0xbf,0xd1,0x5c,0x4b,0x4c,
        0x45,0x25,0x00,0x4a,0xa0,0x6e,0xef,0xf8,
        0xca,0x61,0x78,0x3a,0xac,0xec,0x57,0xfb,
        0x3d,0x1f,0x92,0xb0,0xfe,0x2f,0xd1,0xa8,
        0x5f,0x67,0x24,0x51,0x7b,0x65,0xe6,0x14,
        0xad,0x68,0x08,0xd6,0xf6,0xee,0x34,0xdf,
        0xf7,0x31,0x0f,0xdc,0x82,0xae,0xbf,0xd9,
        0x04,0xb0,0x1e,0x1d,0xc5,0x4b,0x29,0x27,
        0x09,0x4b,0x2d,0xb6,0x8d,0x6f,0x90,0x3b,
        0x68,0x40,0x1a,0xde,0xbf,0x5a,0x7e,0x08,
        0xd7,0x8f,0xf4,0xef,0x5d,0x63,0x65,0x3a,
        0x65,0x04,0x0c,0xf9,0xbf,0xd4,0xac,0xa7,
        0x98,0x4a,0x74,0xd3,0x71,0x45,0x98,0x67,
        0x80,0xfc,0x0b,0x16,0xac,0x45,0x16,0x49,
        0xde,0x61,0x88,0xa7,0xdb,0xdf,0x19,0x1f,
        0x64,0xb5,0xfc,0x5e,0x2a,0xb4,0x7b,0x57,
        0xf7,0xf7,0x27,0x6c,0xd4,0x19,0xc1,0x7a,
        0x3c,0xa8,0xe1,0xb9,0x39,0xae,0x49,0xe4,
        0x88,0xac,0xba,0x6b,0x96,0x56,0x10,0xb5,
        0x48,0x01,0x09,0xc8,0xb1,0x7b,0x80,0xe1,
        0xb7,0xb7,0x50,0xdf,0xc7,0x59,0x8d,0x5d,
        0x50,0x11,0xfd,0x2d,0xcc,0x56,0x00,0xa3,
        0x2e,0xf5,0xb5,0x2a,0x1e,0xcc,0x82,0x0e,
        0x30,0x8a,0xa3,0x42,0x72,0x1a,0xac,0x09,
        0x43,0xbf,0x66,0x86,0xb6,0x4b,0x25,0x79,
        0x37,0x65,0x04,0xcc,0xc4,0x93,0xd9,0x7e,
        0x6a,0xed,0x3f,0xb0,0xf9,0xcd,0x71,0xa4,
        0x3d,0xd4,0x97,0xf0,0x1f,0x17,0xc0,0xe2,
        0xcb,0x37,0x97,0xaa,0x2a,0x2f,0x25,0x66,
        0x56,0x16,0x8e,0x6c,0x49,0x6a,0xfc,0x5f,
        0xb9,0x32,0x46,0xf6,0xb1,0x11,0x63,0x98,
        0xa3,0x46,0xf1,0xa6,0x41,0xf3,0xb0,0x41,
        0xe9,0x89,0xf7,0x91,0x4f,0x90,0xcc,0x2c,
        0x7f,0xff,0x35,0x78,0x76,0xe5,0x06,0xb5,
        0x0d,0x33,0x4b,0xa7,0x7c,0x22,0x5b,0xc3,
        0x07,0xba,0x53,0x71,0x52,0xf3,0xf1,0x61,
        0x0e,0x4e,0xaf,0xe5,0x95,0xf6,0xd9,0xd9,
        0x0d,0x11,0xfa,0xa9,0x33,0xa1,0x5e,0xf1,
        0x36,0x95,0x46,0x86,0x8a,0x7f,0x3a,0x45,
        0xa9,0x67,0x68,0xd4,0x0f,0xd9,0xd0,0x34,
        0x12,0xc0,0x91,0xc6,0x31,0x5c,0xf4,0xfd,
        0xe7,0xcb,0x68,0x60,0x69,0x37,0x38,0x0d,
        0xb2,0xea,0xaa,0x70,0x7b,0x4c,0x41,0x85,
        0xc3,0x2e,0xdd,0xcd,0xd3,0x06,0x70,0x5e,
        0x4d,0xc1,0xff,0xc8,0x72,0xee,0xee,0x47,
        0x5a,0x64,0xdf,0xac,0x86,0xab,0xa4,0x1c,
        0x06,0x18,0x98,0x3f,0x87,0x41,0xc5,0xef,
        0x68,0xd3,0xa1,0x01,0xe8,0xa3,0xb8,0xca,
        0xc6,0x0c,0x90,0x5c,0x15,0xfc,0x91,0x08,
        0x40,0xb9,0x4c,0x00,0xa0,0xb9,0xd0
    };

    static const byte* msgs[] =   {msg1, msg2, msg3, msg1, msg1, msg4};
    static const word16 msgSz[] = {0 /*sizeof(msg1)*/,
                                   sizeof(msg2),
                                   sizeof(msg3),
                                   0 /*sizeof(msg1)*/,
                                   0 /*sizeof(msg1)*/,
                                   sizeof(msg4)
    };
#endif /* HAVE_ED25519_SIGN && HAVE_ED25519_KEY_EXPORT && HAVE_ED25519_KEY_IMPORT */

    /* create ed25519 keys */
#ifndef HAVE_FIPS
    ret = wc_InitRng_ex(&rng, HEAP_HINT, devId);
#else
    ret = wc_InitRng(&rng);
#endif
    if (ret != 0)
        return -7100;

    wc_ed25519_init(&key);
    wc_ed25519_init(&key2);
    wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &key);
    wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &key2);

    /* helper functions for signature and key size */
    keySz = wc_ed25519_size(&key);
    sigSz = wc_ed25519_sig_size(&key);

#if defined(HAVE_ED25519_SIGN) && defined(HAVE_ED25519_KEY_EXPORT) &&\
        defined(HAVE_ED25519_KEY_IMPORT)
    for (i = 0; i < 6; i++) {
        outlen = sizeof(out);
        XMEMSET(out, 0, sizeof(out));

        if (wc_ed25519_import_private_key(sKeys[i], ED25519_KEY_SIZE, pKeys[i],
                pKeySz[i], &key) != 0)
            return -7101 - i;

        if (wc_ed25519_sign_msg(msgs[i], msgSz[i], out, &outlen, &key)
                != 0)
            return -7111 - i;

        if (XMEMCMP(out, sigs[i], 64))
            return -7121 - i;

#if defined(HAVE_ED25519_VERIFY)
        /* test verify on good msg */
        if (wc_ed25519_verify_msg(out, outlen, msgs[i], msgSz[i], &verify,
                    &key) != 0 || verify != 1)
            return -7131 - i;

        /* test verify on bad msg */
        out[outlen-1] = out[outlen-1] + 1;
        if (wc_ed25519_verify_msg(out, outlen, msgs[i], msgSz[i], &verify,
                    &key) == 0 || verify == 1)
            return -7141 - i;
#endif /* HAVE_ED25519_VERIFY */

        /* test api for import/exporting keys */
        exportPSz = sizeof(exportPKey);
        exportSSz = sizeof(exportSKey);
        if (wc_ed25519_export_public(&key, exportPKey, &exportPSz) != 0)
            return -7151 - i;

        if (wc_ed25519_import_public(exportPKey, exportPSz, &key2) != 0)
            return -7161 - i;

        if (wc_ed25519_export_private_only(&key, exportSKey, &exportSSz) != 0)
            return -7171 - i;

        if (wc_ed25519_import_private_key(exportSKey, exportSSz,
                                          exportPKey, exportPSz, &key2) != 0)
            return -7181 - i;

        /* clear "out" buffer and test sign with imported keys */
        outlen = sizeof(out);
        XMEMSET(out, 0, sizeof(out));
        if (wc_ed25519_sign_msg(msgs[i], msgSz[i], out, &outlen, &key2) != 0)
            return -7191 - i;

#if defined(HAVE_ED25519_VERIFY)
        if (wc_ed25519_verify_msg(out, outlen, msgs[i], msgSz[i], &verify,
                                  &key2) != 0 || verify != 1)
            return -7201 - i;

        if (XMEMCMP(out, sigs[i], 64))
            return -7211 - i;
#endif /* HAVE_ED25519_VERIFY */
    }
#endif /* HAVE_ED25519_SIGN && HAVE_ED25519_KEY_EXPORT && HAVE_ED25519_KEY_IMPORT */

    /* clean up keys when done */
    wc_ed25519_free(&key);
    wc_ed25519_free(&key2);

#if defined(HAVE_HASHDRBG) || defined(NO_RC4)
    wc_FreeRng(&rng);
#endif

    /* hush warnings of unused keySz and sigSz */
    (void)keySz;
    (void)sigSz;

#ifdef WOLFSSL_TEST_CERT
    ret = ed25519_test_cert();
    if (ret < 0)
        return ret;
#ifdef WOLFSSL_CERT_GEN
    ret = ed25519_test_make_cert();
    if (ret < 0)
        return ret;
#endif /* WOLFSSL_CERT_GEN */
#endif /* WOLFSSL_TEST_CERT */

    return 0;
}
#endif /* HAVE_ED25519 */


#if defined(WOLFSSL_CMAC) && !defined(NO_AES)

typedef struct CMAC_Test_Case {
    int type;
    int partial;
    const byte* m;
    word32 mSz;
    const byte* k;
    word32 kSz;
    const byte* t;
    word32 tSz;
} CMAC_Test_Case;

int cmac_test(void)
{
    const byte k128[] =
    {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    const byte k192[] =
    {
        0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52,
        0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5,
        0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b
    };
    const byte k256[] =
    {
        0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
        0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
        0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
        0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
    };
    #define KLEN_128 (sizeof(k128))
    #define KLEN_192 (sizeof(k192))
    #define KLEN_256 (sizeof(k256))

    const byte m[] =
    {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
        0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
        0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
        0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    };
    #define MLEN_0 (0)
    #define MLEN_128 (128/8)
    #define MLEN_320 (320/8)
    #define MLEN_319 (MLEN_320 - 1)
    #define MLEN_512 (512/8)

    const byte t128_0[] =
    {
        0xbb, 0x1d, 0x69, 0x29, 0xe9, 0x59, 0x37, 0x28,
        0x7f, 0xa3, 0x7d, 0x12, 0x9b, 0x75, 0x67, 0x46
    };
    const byte t128_128[] =
    {
        0x07, 0x0a, 0x16, 0xb4, 0x6b, 0x4d, 0x41, 0x44,
        0xf7, 0x9b, 0xdd, 0x9d, 0xd0, 0x4a, 0x28, 0x7c
    };
    const byte t128_319[] =
    {
        0x2c, 0x17, 0x84, 0x4c, 0x93, 0x1c, 0x07, 0x95,
        0x15, 0x92, 0x73, 0x0a, 0x34, 0xd0, 0xd9, 0xd2
    };
    const byte t128_320[] =
    {
        0xdf, 0xa6, 0x67, 0x47, 0xde, 0x9a, 0xe6, 0x30,
        0x30, 0xca, 0x32, 0x61, 0x14, 0x97, 0xc8, 0x27
    };
    const byte t128_512[] =
    {
        0x51, 0xf0, 0xbe, 0xbf, 0x7e, 0x3b, 0x9d, 0x92,
        0xfc, 0x49, 0x74, 0x17, 0x79, 0x36, 0x3c, 0xfe
    };

    const byte t192_0[] =
    {
        0xd1, 0x7d, 0xdf, 0x46, 0xad, 0xaa, 0xcd, 0xe5,
        0x31, 0xca, 0xc4, 0x83, 0xde, 0x7a, 0x93, 0x67
    };
    const byte t192_128[] =
    {
        0x9e, 0x99, 0xa7, 0xbf, 0x31, 0xe7, 0x10, 0x90,
        0x06, 0x62, 0xf6, 0x5e, 0x61, 0x7c, 0x51, 0x84
    };
    const byte t192_320[] =
    {
        0x8a, 0x1d, 0xe5, 0xbe, 0x2e, 0xb3, 0x1a, 0xad,
        0x08, 0x9a, 0x82, 0xe6, 0xee, 0x90, 0x8b, 0x0e
    };
    const byte t192_512[] =
    {
        0xa1, 0xd5, 0xdf, 0x0e, 0xed, 0x79, 0x0f, 0x79,
        0x4d, 0x77, 0x58, 0x96, 0x59, 0xf3, 0x9a, 0x11
    };

    const byte t256_0[] =
    {
        0x02, 0x89, 0x62, 0xf6, 0x1b, 0x7b, 0xf8, 0x9e,
        0xfc, 0x6b, 0x55, 0x1f, 0x46, 0x67, 0xd9, 0x83
    };
    const byte t256_128[] =
    {
        0x28, 0xa7, 0x02, 0x3f, 0x45, 0x2e, 0x8f, 0x82,
        0xbd, 0x4b, 0xf2, 0x8d, 0x8c, 0x37, 0xc3, 0x5c
    };
    const byte t256_320[] =
    {
        0xaa, 0xf3, 0xd8, 0xf1, 0xde, 0x56, 0x40, 0xc2,
        0x32, 0xf5, 0xb1, 0x69, 0xb9, 0xc9, 0x11, 0xe6
    };
    const byte t256_512[] =
    {
        0xe1, 0x99, 0x21, 0x90, 0x54, 0x9f, 0x6e, 0xd5,
        0x69, 0x6a, 0x2c, 0x05, 0x6c, 0x31, 0x54, 0x10
    };

    const CMAC_Test_Case testCases[] =
    {
        {WC_CMAC_AES, 0, m, MLEN_0, k128, KLEN_128, t128_0, AES_BLOCK_SIZE},
        {WC_CMAC_AES, 0, m, MLEN_128, k128, KLEN_128, t128_128, AES_BLOCK_SIZE},
        {WC_CMAC_AES, 0, m, MLEN_320, k128, KLEN_128, t128_320, AES_BLOCK_SIZE},
        {WC_CMAC_AES, 0, m, MLEN_512, k128, KLEN_128, t128_512, AES_BLOCK_SIZE},
        {WC_CMAC_AES, 5, m, MLEN_512, k128, KLEN_128, t128_512, AES_BLOCK_SIZE},
        {WC_CMAC_AES, 0, m, MLEN_0, k192, KLEN_192, t192_0, AES_BLOCK_SIZE},
        {WC_CMAC_AES, 0, m, MLEN_128, k192, KLEN_192, t192_128, AES_BLOCK_SIZE},
        {WC_CMAC_AES, 0, m, MLEN_320, k192, KLEN_192, t192_320, AES_BLOCK_SIZE},
        {WC_CMAC_AES, 0, m, MLEN_512, k192, KLEN_192, t192_512, AES_BLOCK_SIZE},
        {WC_CMAC_AES, 0, m, MLEN_0, k256, KLEN_256, t256_0, AES_BLOCK_SIZE},
        {WC_CMAC_AES, 0, m, MLEN_128, k256, KLEN_256, t256_128, AES_BLOCK_SIZE},
        {WC_CMAC_AES, 0, m, MLEN_320, k256, KLEN_256, t256_320, AES_BLOCK_SIZE},
        {WC_CMAC_AES, 0, m, MLEN_512, k256, KLEN_256, t256_512, AES_BLOCK_SIZE},
        {WC_CMAC_AES, 0, m, MLEN_319, k128, KLEN_128, t128_319, AES_BLOCK_SIZE}
    };

    Cmac cmac;
    byte tag[AES_BLOCK_SIZE];
    const CMAC_Test_Case* tc;
    word32 i, tagSz;

    for (i = 0, tc = testCases;
         i < sizeof(testCases)/sizeof(CMAC_Test_Case);
         i++, tc++) {

        XMEMSET(tag, 0, sizeof(tag));
        tagSz = AES_BLOCK_SIZE;
        if (wc_InitCmac(&cmac, tc->k, tc->kSz, tc->type, NULL) != 0)
            return -7300;
        if (tc->partial) {
            if (wc_CmacUpdate(&cmac, tc->m,
                                 tc->mSz/2 - tc->partial) != 0)
                return -7301;
            if (wc_CmacUpdate(&cmac, tc->m + tc->mSz/2 - tc->partial,
                                 tc->mSz/2 + tc->partial) != 0)
                return -7302;
        }
        else {
            if (wc_CmacUpdate(&cmac, tc->m, tc->mSz) != 0)
                return -7303;
        }
        if (wc_CmacFinal(&cmac, tag, &tagSz) != 0)
            return -7304;
        if (XMEMCMP(tag, tc->t, AES_BLOCK_SIZE) != 0)
            return -7305;

        XMEMSET(tag, 0, sizeof(tag));
        tagSz = sizeof(tag);
        if (wc_AesCmacGenerate(tag, &tagSz, tc->m, tc->mSz,
                               tc->k, tc->kSz) != 0)
            return -7306;
        if (XMEMCMP(tag, tc->t, AES_BLOCK_SIZE) != 0)
            return -7307;
        if (wc_AesCmacVerify(tc->t, tc->tSz, tc->m, tc->mSz,
                             tc->k, tc->kSz) != 0)
            return -7308;
    }

    return 0;
}

#endif /* NO_AES && WOLFSSL_CMAC */

#ifdef HAVE_LIBZ

const byte sample_text[] =
    "Biodiesel cupidatat marfa, cliche aute put a bird on it incididunt elit\n"
    "polaroid. Sunt tattooed bespoke reprehenderit. Sint twee organic id\n"
    "marfa. Commodo veniam ad esse gastropub. 3 wolf moon sartorial vero,\n"
    "plaid delectus biodiesel squid +1 vice. Post-ironic keffiyeh leggings\n"
    "selfies cray fap hoodie, forage anim. Carles cupidatat shoreditch, VHS\n"
    "small batch meggings kogi dolore food truck bespoke gastropub.\n"
    "\n"
    "Terry richardson adipisicing actually typewriter tumblr, twee whatever\n"
    "four loko you probably haven't heard of them high life. Messenger bag\n"
    "whatever tattooed deep v mlkshk. Brooklyn pinterest assumenda chillwave\n"
    "et, banksy ullamco messenger bag umami pariatur direct trade forage.\n"
    "Typewriter culpa try-hard, pariatur sint brooklyn meggings. Gentrify\n"
    "food truck next level, tousled irony non semiotics PBR ethical anim cred\n"
    "readymade. Mumblecore brunch lomo odd future, portland organic terry\n"
    "richardson elit leggings adipisicing ennui raw denim banjo hella. Godard\n"
    "mixtape polaroid, pork belly readymade organic cray typewriter helvetica\n"
    "four loko whatever street art yr farm-to-table.\n"
    "\n"
    "Vinyl keytar vice tofu. Locavore you probably haven't heard of them pug\n"
    "pickled, hella tonx labore truffaut DIY mlkshk elit cosby sweater sint\n"
    "et mumblecore. Elit swag semiotics, reprehenderit DIY sartorial nisi ugh\n"
    "nesciunt pug pork belly wayfarers selfies delectus. Ethical hoodie\n"
    "seitan fingerstache kale chips. Terry richardson artisan williamsburg,\n"
    "eiusmod fanny pack irony tonx ennui lo-fi incididunt tofu YOLO\n"
    "readymade. 8-bit sed ethnic beard officia. Pour-over iphone DIY butcher,\n"
    "ethnic art party qui letterpress nisi proident jean shorts mlkshk\n"
    "locavore.\n"
    "\n"
    "Narwhal flexitarian letterpress, do gluten-free voluptate next level\n"
    "banh mi tonx incididunt carles DIY. Odd future nulla 8-bit beard ut\n"
    "cillum pickled velit, YOLO officia you probably haven't heard of them\n"
    "trust fund gastropub. Nisi adipisicing tattooed, Austin mlkshk 90's\n"
    "small batch american apparel. Put a bird on it cosby sweater before they\n"
    "sold out pork belly kogi hella. Street art mollit sustainable polaroid,\n"
    "DIY ethnic ea pug beard dreamcatcher cosby sweater magna scenester nisi.\n"
    "Sed pork belly skateboard mollit, labore proident eiusmod. Sriracha\n"
    "excepteur cosby sweater, anim deserunt laborum eu aliquip ethical et\n"
    "neutra PBR selvage.\n"
    "\n"
    "Raw denim pork belly truffaut, irony plaid sustainable put a bird on it\n"
    "next level jean shorts exercitation. Hashtag keytar whatever, nihil\n"
    "authentic aliquip disrupt laborum. Tattooed selfies deserunt trust fund\n"
    "wayfarers. 3 wolf moon synth church-key sartorial, gastropub leggings\n"
    "tattooed. Labore high life commodo, meggings raw denim fingerstache pug\n"
    "trust fund leggings seitan forage. Nostrud ullamco duis, reprehenderit\n"
    "incididunt flannel sustainable helvetica pork belly pug banksy you\n"
    "probably haven't heard of them nesciunt farm-to-table. Disrupt nostrud\n"
    "mollit magna, sriracha sartorial helvetica.\n"
    "\n"
    "Nulla kogi reprehenderit, skateboard sustainable duis adipisicing viral\n"
    "ad fanny pack salvia. Fanny pack trust fund you probably haven't heard\n"
    "of them YOLO vice nihil. Keffiyeh cray lo-fi pinterest cardigan aliqua,\n"
    "reprehenderit aute. Culpa tousled williamsburg, marfa lomo actually anim\n"
    "skateboard. Iphone aliqua ugh, semiotics pariatur vero readymade\n"
    "organic. Marfa squid nulla, in laborum disrupt laboris irure gastropub.\n"
    "Veniam sunt food truck leggings, sint vinyl fap.\n"
    "\n"
    "Hella dolore pork belly, truffaut carles you probably haven't heard of\n"
    "them PBR helvetica in sapiente. Fashion axe ugh bushwick american\n"
    "apparel. Fingerstache sed iphone, jean shorts blue bottle nisi bushwick\n"
    "flexitarian officia veniam plaid bespoke fap YOLO lo-fi. Blog\n"
    "letterpress mumblecore, food truck id cray brooklyn cillum ad sed.\n"
    "Assumenda chambray wayfarers vinyl mixtape sustainable. VHS vinyl\n"
    "delectus, culpa williamsburg polaroid cliche swag church-key synth kogi\n"
    "magna pop-up literally. Swag thundercats ennui shoreditch vegan\n"
    "pitchfork neutra truffaut etsy, sed single-origin coffee craft beer.\n"
    "\n"
    "Odio letterpress brooklyn elit. Nulla single-origin coffee in occaecat\n"
    "meggings. Irony meggings 8-bit, chillwave lo-fi adipisicing cred\n"
    "dreamcatcher veniam. Put a bird on it irony umami, trust fund bushwick\n"
    "locavore kale chips. Sriracha swag thundercats, chillwave disrupt\n"
    "tousled beard mollit mustache leggings portland next level. Nihil esse\n"
    "est, skateboard art party etsy thundercats sed dreamcatcher ut iphone\n"
    "swag consectetur et. Irure skateboard banjo, nulla deserunt messenger\n"
    "bag dolor terry richardson sapiente.\n";


int compress_test(void)
{
    int ret = 0;
    word32 dSz = sizeof(sample_text);
    word32 cSz = (dSz + (word32)(dSz * 0.001) + 12);
    byte *c = NULL;
    byte *d = NULL;

    c = XMALLOC(cSz * sizeof(byte), HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    d = XMALLOC(dSz * sizeof(byte), HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (c == NULL || d == NULL) {
        ERROR_OUT(-7400, exit);
    }

    /* follow calloc and initialize to 0 */
    XMEMSET(c, 0, cSz);
    XMEMSET(d, 0, dSz);

    if ((ret = wc_Compress(c, cSz, sample_text, dSz, 0)) < 0) {
        ERROR_OUT(-7401, exit);
    }
    cSz = (word32)ret;

    if ((ret = wc_DeCompress(d, dSz, c, cSz)) != (int)dSz) {
        ERROR_OUT(-7402, exit);
    }

    if (XMEMCMP(d, sample_text, dSz)) {
        ERROR_OUT(-7403, exit);
    }
    ret = 0;

exit:
    if (c) XFREE(c, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (d) XFREE(d, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

    return ret;
}

#endif /* HAVE_LIBZ */

#ifdef HAVE_PKCS7

/* External Debugging/Testing Note:
 *
 * PKCS#7 test functions can output generated PKCS#7/CMS bundles for
 * additional testing. To dump bundles to files DER encoded files, please
 * define:
 *
 * #define PKCS7_OUTPUT_TEST_BUNDLES
 */


/* Loads certs and keys for use with PKCS7 tests, from either files
 * or buffers.
 *
 * rsaCert      - output buffer for RSA cert
 * rsaCertSz    - IN/OUT size of output buffer, size of RSA cert
 * rsaPrivKey   - output buffer for RSA private key
 * rsaPrivKeySz - IN/OUT size of output buffer, size of RSA key
 * eccCert      - output buffer for ECC cert
 * eccCertSz    - IN/OUT size of output buffer, size of ECC cert
 * eccPrivKey   - output buffer for ECC private key
 * eccPrivKeySz - IN/OUT size of output buffer, size of ECC private key
 *
 * Returns 0 on success, negative on error
 */
static int pkcs7_load_certs_keys(byte* rsaCert, word32* rsaCertSz,
                                 byte* rsaPrivKey,  word32* rsaPrivKeySz,
                                 byte* eccCert, word32* eccCertSz,
                                 byte* eccPrivKey,  word32* eccPrivKeySz)
{
#ifndef NO_FILESYSTEM
    FILE*  certFile;
    FILE*  keyFile;
#endif

#ifndef NO_RSA
    if (rsaCert == NULL || rsaCertSz == NULL ||
        rsaPrivKey == NULL || rsaPrivKeySz == NULL)
        return BAD_FUNC_ARG;
#endif

#ifdef HAVE_ECC
    if (eccCert == NULL || eccCertSz == NULL ||
        eccPrivKey == NULL || eccPrivKeySz == NULL)
        return BAD_FUNC_ARG;
#endif

/* RSA */
#ifndef NO_RSA

#ifdef USE_CERT_BUFFERS_1024
    if (*rsaCertSz < (word32)sizeof_client_cert_der_1024)
        return -7410;

    XMEMCPY(rsaCert, client_cert_der_1024, sizeof_client_cert_der_1024);
    *rsaCertSz = sizeof_client_cert_der_1024;
#elif defined(USE_CERT_BUFFERS_2048)
    if (*rsaCertSz < (word32)sizeof_client_cert_der_2048)
        return -7411;

    XMEMCPY(rsaCert, client_cert_der_2048, sizeof_client_cert_der_2048);
    *rsaCertSz = sizeof_client_cert_der_2048;
#else
    certFile = fopen(clientCert, "rb");
    if (!certFile)
        return -7412;

    *rsaCertSz = (word32)fread(rsaCert, 1, *rsaCertSz, certFile);
    fclose(certFile);
#endif

#ifdef USE_CERT_BUFFERS_1024
    if (*rsaPrivKeySz < (word32)sizeof_client_key_der_1024)
        return -7413;

    XMEMCPY(rsaPrivKey, client_key_der_1024, sizeof_client_key_der_1024);
    *rsaPrivKeySz = sizeof_client_key_der_1024;
#elif defined(USE_CERT_BUFFERS_2048)
    if (*rsaPrivKeySz < (word32)sizeof_client_key_der_2048)
        return -7414;

    XMEMCPY(rsaPrivKey, client_key_der_2048, sizeof_client_key_der_2048);
    *rsaPrivKeySz = sizeof_client_key_der_2048;
#else
    keyFile = fopen(clientKey, "rb");
    if (!keyFile)
        return -7415;

    *rsaPrivKeySz = (word32)fread(rsaPrivKey, 1, *rsaPrivKeySz, keyFile);
    fclose(keyFile);
#endif /* USE_CERT_BUFFERS */

#endif /* NO_RSA */

/* ECC */
#ifdef HAVE_ECC

#ifdef USE_CERT_BUFFERS_256
    if (*eccCertSz < (word32)sizeof_cliecc_cert_der_256)
        return -7416;

    XMEMCPY(eccCert, cliecc_cert_der_256, sizeof_cliecc_cert_der_256);
    *eccCertSz = sizeof_cliecc_cert_der_256;
#else
    certFile = fopen(eccClientCert, "rb");
    if (!certFile)
        return -7417;

    *eccCertSz = (word32)fread(eccCert, 1, *eccCertSz, certFile);
    fclose(certFile);
#endif /* USE_CERT_BUFFERS_256 */

#ifdef USE_CERT_BUFFERS_256
    if (*eccPrivKeySz < (word32)sizeof_ecc_clikey_der_256)
        return -7418;

    XMEMCPY(eccPrivKey, ecc_clikey_der_256, sizeof_ecc_clikey_der_256);
    *eccPrivKeySz = sizeof_ecc_clikey_der_256;
#else
    keyFile = fopen(eccClientKey, "rb");
    if (!keyFile)
        return -7419;

    *eccPrivKeySz = (word32)fread(eccPrivKey, 1, *eccPrivKeySz, keyFile);
    fclose(keyFile);
#endif /* USE_CERT_BUFFERS_256 */
#endif /* HAVE_ECC */

#ifdef NO_RSA
    (void)rsaCert;
    (void)rsaCertSz;
    (void)rsaPrivKey;
    (void)rsaPrivKeySz;
#endif
#ifndef HAVE_ECC
    (void)eccCert;
    (void)eccCertSz;
    (void)eccPrivKey;
    (void)eccPrivKeySz;
#endif
#ifndef NO_FILESYSTEM
    (void)certFile;
    (void)keyFile;
#endif
    return 0;
}


typedef struct {
    const byte*  content;
    word32       contentSz;
    int          contentOID;
    int          encryptOID;
    int          keyWrapOID;
    int          keyAgreeOID;
    byte*        cert;
    size_t       certSz;
    byte*        privateKey;
    word32       privateKeySz;
    byte*        optionalUkm;
    word32       optionalUkmSz;
    const char*  outFileName;
} pkcs7EnvelopedVector;


static int pkcs7enveloped_run_vectors(byte* rsaCert, word32 rsaCertSz,
                                      byte* rsaPrivKey,  word32 rsaPrivKeySz,
                                      byte* eccCert, word32 eccCertSz,
                                      byte* eccPrivKey,  word32 eccPrivKeySz)
{
    int ret, testSz, i;
    int envelopedSz, decodedSz;

    byte   enveloped[2048];
    byte   decoded[2048];
    PKCS7  pkcs7;
#ifdef PKCS7_OUTPUT_TEST_BUNDLES
    FILE*  pkcs7File;
#endif

    const byte data[] = { /* Hello World */
        0x48,0x65,0x6c,0x6c,0x6f,0x20,0x57,0x6f,
        0x72,0x6c,0x64
    };

#if !defined(NO_AES) && defined(HAVE_ECC)
    byte optionalUkm[] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07
    };
#endif /* NO_AES */

    const pkcs7EnvelopedVector testVectors[] =
    {
        /* key transport key encryption technique */
#ifndef NO_RSA
        {data, (word32)sizeof(data), DATA, DES3b, 0, 0, rsaCert, rsaCertSz,
         rsaPrivKey, rsaPrivKeySz, NULL, 0, "pkcs7envelopedDataDES3.der"},

    #ifndef NO_AES
        {data, (word32)sizeof(data), DATA, AES128CBCb, 0, 0, rsaCert, rsaCertSz,
         rsaPrivKey, rsaPrivKeySz, NULL, 0, "pkcs7envelopedDataAES128CBC.der"},

        {data, (word32)sizeof(data), DATA, AES192CBCb, 0, 0, rsaCert, rsaCertSz,
         rsaPrivKey, rsaPrivKeySz, NULL, 0, "pkcs7envelopedDataAES192CBC.der"},

        {data, (word32)sizeof(data), DATA, AES256CBCb, 0, 0, rsaCert, rsaCertSz,
         rsaPrivKey, rsaPrivKeySz, NULL, 0, "pkcs7envelopedDataAES256CBC.der"},
    #endif /* NO_AES */
#endif

        /* key agreement key encryption technique*/
#ifdef HAVE_ECC
    #ifndef NO_AES
        #ifndef NO_SHA
        {data, (word32)sizeof(data), DATA, AES128CBCb, AES128_WRAP,
         dhSinglePass_stdDH_sha1kdf_scheme, eccCert, eccCertSz, eccPrivKey,
         eccPrivKeySz, NULL, 0,
         "pkcs7envelopedDataAES128CBC_ECDH_SHA1KDF.der"},
        #endif

        #ifndef NO_SHA256
        {data, (word32)sizeof(data), DATA, AES256CBCb, AES256_WRAP,
         dhSinglePass_stdDH_sha256kdf_scheme, eccCert, eccCertSz, eccPrivKey,
         eccPrivKeySz, NULL, 0,
         "pkcs7envelopedDataAES256CBC_ECDH_SHA256KDF.der"},
        #endif /* NO_SHA256 */

        #ifdef WOLFSSL_SHA512
        {data, (word32)sizeof(data), DATA, AES256CBCb, AES256_WRAP,
         dhSinglePass_stdDH_sha512kdf_scheme, eccCert, eccCertSz, eccPrivKey,
         eccPrivKeySz, NULL, 0,
         "pkcs7envelopedDataAES256CBC_ECDH_SHA512KDF.der"},

        /* with optional user keying material (ukm) */
        {data, (word32)sizeof(data), DATA, AES256CBCb, AES256_WRAP,
         dhSinglePass_stdDH_sha512kdf_scheme, eccCert, eccCertSz, eccPrivKey,
         eccPrivKeySz, optionalUkm, sizeof(optionalUkm),
         "pkcs7envelopedDataAES256CBC_ECDH_SHA512KDF_ukm.der"},
        #endif /* WOLFSSL_SHA512 */
    #endif /* NO_AES */
#endif
    };

    testSz = sizeof(testVectors) / sizeof(pkcs7EnvelopedVector);

    for (i = 0; i < testSz; i++) {

        ret = wc_PKCS7_Init(&pkcs7, HEAP_HINT, devId);
        if (ret != 0)
            return -7419;

        ret = wc_PKCS7_InitWithCert(&pkcs7, testVectors[i].cert,
                                    (word32)testVectors[i].certSz);
        if (ret != 0)
            return -7420;

        pkcs7.content      = (byte*)testVectors[i].content;
        pkcs7.contentSz    = testVectors[i].contentSz;
        pkcs7.contentOID   = testVectors[i].contentOID;
        pkcs7.encryptOID   = testVectors[i].encryptOID;
        pkcs7.keyWrapOID   = testVectors[i].keyWrapOID;
        pkcs7.keyAgreeOID  = testVectors[i].keyAgreeOID;
        pkcs7.privateKey   = testVectors[i].privateKey;
        pkcs7.privateKeySz = testVectors[i].privateKeySz;
        pkcs7.ukm          = testVectors[i].optionalUkm;
        pkcs7.ukmSz        = testVectors[i].optionalUkmSz;

        /* encode envelopedData */
        envelopedSz = wc_PKCS7_EncodeEnvelopedData(&pkcs7, enveloped,
                                                   sizeof(enveloped));
        if (envelopedSz <= 0) {
            printf("DEBUG: i = %d, envelopedSz = %d\n", i, envelopedSz);
            return -7421;
        }

        /* decode envelopedData */
        decodedSz = wc_PKCS7_DecodeEnvelopedData(&pkcs7, enveloped, envelopedSz,
                                                 decoded, sizeof(decoded));
        if (decodedSz <= 0)
            return -7422;

        /* test decode result */
        if (XMEMCMP(decoded, data, sizeof(data)) != 0)
            return -7423;

#ifdef PKCS7_OUTPUT_TEST_BUNDLES
        /* output pkcs7 envelopedData for external testing */
        pkcs7File = fopen(testVectors[i].outFileName, "wb");
        if (!pkcs7File)
            return -7424;

        ret = (int)fwrite(enveloped, 1, envelopedSz, pkcs7File);
        fclose(pkcs7File);
        if (ret != envelopedSz) {
            return -7425;
        }
#endif /* PKCS7_OUTPUT_TEST_BUNDLES */

        wc_PKCS7_Free(&pkcs7);
    }

#ifndef HAVE_ECC
    (void)eccCert;
    (void)eccCertSz;
    (void)eccPrivKey;
    (void)eccPrivKeySz;
#endif
#ifdef NO_RSA
    (void)rsaCert;
    (void)rsaCertSz;
    (void)rsaPrivKey;
    (void)rsaPrivKeySz;
#endif

    return 0;
}


int pkcs7enveloped_test(void)
{
    int ret = 0;

    byte* rsaCert    = NULL;
    byte* rsaPrivKey = NULL;
    word32 rsaCertSz    = 0;
    word32 rsaPrivKeySz = 0;

    byte* eccCert    = NULL;
    byte* eccPrivKey = NULL;
    word32 eccCertSz    = 0;
    word32 eccPrivKeySz = 0;

#ifndef NO_RSA
    /* read client RSA cert and key in DER format */
    rsaCert = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (rsaCert == NULL)
        return -7500;

    rsaPrivKey = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (rsaPrivKey == NULL) {
        XFREE(rsaCert, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return -7501;
    }

    rsaCertSz = FOURK_BUF;
    rsaPrivKeySz = FOURK_BUF;
#endif /* NO_RSA */

#ifdef HAVE_ECC
    /* read client ECC cert and key in DER format */
    eccCert = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (eccCert == NULL) {
    #ifndef NO_RSA
        XFREE(rsaCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(rsaPrivKey, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    #endif
        return -7504;
    }

    eccPrivKey =(byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (eccPrivKey == NULL) {
    #ifndef NO_RSA
        XFREE(rsaCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(rsaPrivKey, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    #endif
        XFREE(eccCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return -7505;
    }

    eccCertSz = FOURK_BUF;
    eccPrivKeySz = FOURK_BUF;
#endif /* HAVE_ECC */

    ret = pkcs7_load_certs_keys(rsaCert, &rsaCertSz, rsaPrivKey,
                                &rsaPrivKeySz, eccCert, &eccCertSz,
                                eccPrivKey, &eccPrivKeySz);
    if (ret < 0) {
    #ifndef NO_RSA
        XFREE(rsaCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(rsaPrivKey, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    #endif
    #ifdef HAVE_ECC
        XFREE(eccCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(eccPrivKey, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    #endif
        return ret;
    }

    ret = pkcs7enveloped_run_vectors(rsaCert, (word32)rsaCertSz,
                                     rsaPrivKey, (word32)rsaPrivKeySz,
                                     eccCert, (word32)eccCertSz,
                                     eccPrivKey, (word32)eccPrivKeySz);

#ifndef NO_RSA
    XFREE(rsaCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(rsaPrivKey, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
#endif
#ifdef HAVE_ECC
    XFREE(eccCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(eccPrivKey, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
#endif

    return ret;
}


typedef struct {
    const byte*  content;
    word32       contentSz;
    int          contentOID;
    int          encryptOID;
    byte*        encryptionKey;
    word32       encryptionKeySz;
    PKCS7Attrib* attribs;
    word32       attribsSz;
    const char*  outFileName;
} pkcs7EncryptedVector;


int pkcs7encrypted_test(void)
{
    int ret = 0;
    int i, testSz;
    int encryptedSz, decodedSz, attribIdx;
    PKCS7 pkcs7;
    byte  encrypted[2048];
    byte  decoded[2048];
#ifdef PKCS7_OUTPUT_TEST_BUNDLES
    FILE* pkcs7File;
#endif

    PKCS7Attrib* expectedAttrib;
    PKCS7DecodedAttrib* decodedAttrib;

    const byte data[] = { /* Hello World */
        0x48,0x65,0x6c,0x6c,0x6f,0x20,0x57,0x6f,
        0x72,0x6c,0x64
    };

#ifndef NO_DES3
    byte desKey[] = {
        0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef
    };
    byte des3Key[] = {
        0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
        0xfe,0xde,0xba,0x98,0x76,0x54,0x32,0x10,
        0x89,0xab,0xcd,0xef,0x01,0x23,0x45,0x67
    };
#endif

#ifndef NO_AES
    byte aes128Key[] = {
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08
    };
    byte aes192Key[] = {
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08
    };
    byte aes256Key[] = {
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08
    };

    /* Attribute example from RFC 4134, Section 7.2
     * OID = 1.2.5555
     * OCTET STRING = 'This is a test General ASN Attribute, number 1.' */
    static byte genAttrOid[] = { 0x06, 0x03, 0x2a, 0xab, 0x33 };
    static byte genAttr[] = { 0x04, 47,
                              0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
                              0x61, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x47,
                              0x65, 0x6e, 0x65, 0x72, 0x61, 0x6c, 0x20, 0x41,
                              0x53, 0x4e, 0x20, 0x41, 0x74, 0x74, 0x72, 0x69,
                              0x62, 0x75, 0x74, 0x65, 0x2c, 0x20, 0x6e, 0x75,
                              0x6d, 0x62, 0x65, 0x72, 0x20, 0x31, 0x2e };

    static byte genAttrOid2[] = { 0x06, 0x03, 0x2a, 0xab, 0x34 };
    static byte genAttr2[] = { 0x04, 47,
                              0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
                              0x61, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x47,
                              0x65, 0x6e, 0x65, 0x72, 0x61, 0x6c, 0x20, 0x41,
                              0x53, 0x4e, 0x20, 0x41, 0x74, 0x74, 0x72, 0x69,
                              0x62, 0x75, 0x74, 0x65, 0x2c, 0x20, 0x6e, 0x75,
                              0x6d, 0x62, 0x65, 0x72, 0x20, 0x32, 0x2e };

    PKCS7Attrib attribs[] =
    {
        { genAttrOid, sizeof(genAttrOid), genAttr, sizeof(genAttr) }
    };

    PKCS7Attrib multiAttribs[] =
    {
        { genAttrOid, sizeof(genAttrOid), genAttr, sizeof(genAttr) },
        { genAttrOid2, sizeof(genAttrOid2), genAttr2, sizeof(genAttr2) }
    };
#endif /* NO_AES */

    const pkcs7EncryptedVector testVectors[] =
    {
#ifndef NO_DES3
        {data, (word32)sizeof(data), DATA, DES3b, des3Key, sizeof(des3Key),
         NULL, 0, "pkcs7encryptedDataDES3.der"},

        {data, (word32)sizeof(data), DATA, DESb, desKey, sizeof(desKey),
         NULL, 0, "pkcs7encryptedDataDES.der"},
#endif /* NO_DES3 */

#ifndef NO_AES
        {data, (word32)sizeof(data), DATA, AES128CBCb, aes128Key,
         sizeof(aes128Key), NULL, 0, "pkcs7encryptedDataAES128CBC.der"},

        {data, (word32)sizeof(data), DATA, AES192CBCb, aes192Key,
         sizeof(aes192Key), NULL, 0, "pkcs7encryptedDataAES192CBC.der"},

        {data, (word32)sizeof(data), DATA, AES256CBCb, aes256Key,
         sizeof(aes256Key), NULL, 0, "pkcs7encryptedDataAES256CBC.der"},

        /* test with optional unprotected attributes */
        {data, (word32)sizeof(data), DATA, AES256CBCb, aes256Key,
         sizeof(aes256Key), attribs, (sizeof(attribs)/sizeof(PKCS7Attrib)),
         "pkcs7encryptedDataAES256CBC_attribs.der"},

        /* test with multiple optional unprotected attributes */
        {data, (word32)sizeof(data), DATA, AES256CBCb, aes256Key,
         sizeof(aes256Key), multiAttribs,
         (sizeof(multiAttribs)/sizeof(PKCS7Attrib)),
         "pkcs7encryptedDataAES256CBC_multi_attribs.der"},
#endif /* NO_AES */
    };

    testSz = sizeof(testVectors) / sizeof(pkcs7EncryptedVector);

    for (i = 0; i < testSz; i++) {
        pkcs7.content              = (byte*)testVectors[i].content;
        pkcs7.contentSz            = testVectors[i].contentSz;
        pkcs7.contentOID           = testVectors[i].contentOID;
        pkcs7.encryptOID           = testVectors[i].encryptOID;
        pkcs7.encryptionKey        = testVectors[i].encryptionKey;
        pkcs7.encryptionKeySz      = testVectors[i].encryptionKeySz;
        pkcs7.unprotectedAttribs   = testVectors[i].attribs;
        pkcs7.unprotectedAttribsSz = testVectors[i].attribsSz;
        pkcs7.heap                 = HEAP_HINT;

        /* encode encryptedData */
        encryptedSz = wc_PKCS7_EncodeEncryptedData(&pkcs7, encrypted,
                                                   sizeof(encrypted));
        if (encryptedSz <= 0)
            return -7600;

        /* decode encryptedData */
        decodedSz = wc_PKCS7_DecodeEncryptedData(&pkcs7, encrypted, encryptedSz,
                                                 decoded, sizeof(decoded));
        if (decodedSz <= 0)
            return -7601;

        /* test decode result */
        if (XMEMCMP(decoded, data, sizeof(data)) != 0)
            return -7602;

        /* verify decoded unprotected attributes */
        if (pkcs7.decodedAttrib != NULL) {
            decodedAttrib = pkcs7.decodedAttrib;
            attribIdx = 1;

            while (decodedAttrib != NULL) {

                /* expected attribute, stored list is reversed */
                expectedAttrib = &(pkcs7.unprotectedAttribs
                        [pkcs7.unprotectedAttribsSz - attribIdx]);

                /* verify oid */
                if (XMEMCMP(decodedAttrib->oid, expectedAttrib->oid,
                            decodedAttrib->oidSz) != 0)
                    return -7603;

                /* verify value */
                if (XMEMCMP(decodedAttrib->value, expectedAttrib->value,
                            decodedAttrib->valueSz) != 0)
                    return -7604;

                decodedAttrib = decodedAttrib->next;
                attribIdx++;
            }
        }

#ifdef PKCS7_OUTPUT_TEST_BUNDLES
        /* output pkcs7 envelopedData for external testing */
        pkcs7File = fopen(testVectors[i].outFileName, "wb");
        if (!pkcs7File)
            return -7605;

        ret = (int)fwrite(encrypted, encryptedSz, 1, pkcs7File);
        fclose(pkcs7File);

        if (ret > 0)
            ret = 0;
#endif

        wc_PKCS7_Free(&pkcs7);
    }

    return ret;
}


typedef struct {
    const byte*  content;
    word32       contentSz;
    int          hashOID;
    int          encryptOID;
    byte*        privateKey;
    word32       privateKeySz;
    byte*        cert;
    size_t       certSz;
    PKCS7Attrib* signedAttribs;
    word32       signedAttribsSz;
    const char*  outFileName;
} pkcs7SignedVector;


static int pkcs7signed_run_vectors(byte* rsaCert, word32 rsaCertSz,
                                   byte* rsaPrivKey, word32 rsaPrivKeySz,
                                   byte* eccCert, word32 eccCertSz,
                                   byte* eccPrivKey, word32 eccPrivKeySz)
{
    int ret, testSz, i;
    int encodedSz;
    byte*  out;
    word32 outSz;
    WC_RNG rng;
    PKCS7  pkcs7;
#ifdef PKCS7_OUTPUT_TEST_BUNDLES
    FILE*  file;
#endif

    const byte data[] = { /* Hello World */
        0x48,0x65,0x6c,0x6c,0x6f,0x20,0x57,0x6f,
        0x72,0x6c,0x64
    };

    static byte transIdOid[] =
               { 0x06, 0x0a, 0x60, 0x86, 0x48, 0x01, 0x86, 0xF8, 0x45, 0x01,
                 0x09, 0x07 };
    static byte messageTypeOid[] =
               { 0x06, 0x0a, 0x60, 0x86, 0x48, 0x01, 0x86, 0xF8, 0x45, 0x01,
                 0x09, 0x02 };
    static byte senderNonceOid[] =
               { 0x06, 0x0a, 0x60, 0x86, 0x48, 0x01, 0x86, 0xF8, 0x45, 0x01,
                 0x09, 0x05 };
    static byte transId[(WC_SHA_DIGEST_SIZE + 1) * 2 + 1];
    static byte messageType[] = { 0x13, 2, '1', '9' };
    static byte senderNonce[PKCS7_NONCE_SZ + 2];

    PKCS7Attrib attribs[] =
    {
        { transIdOid, sizeof(transIdOid),
                     transId, sizeof(transId) - 1 }, /* take off the null */
        { messageTypeOid, sizeof(messageTypeOid),
                     messageType, sizeof(messageType) },
        { senderNonceOid, sizeof(senderNonceOid),
                     senderNonce, sizeof(senderNonce) }
    };

    const pkcs7SignedVector testVectors[] =
    {
#ifndef NO_RSA
    #ifndef NO_SHA
        /* RSA with SHA */
        {data, (word32)sizeof(data), SHAh, RSAk, rsaPrivKey, rsaPrivKeySz,
         rsaCert, rsaCertSz, attribs, (sizeof(attribs)/sizeof(PKCS7Attrib)),
         "pkcs7signedData_RSA_SHA.der"},

        /* RSA with SHA, no signed attributes */
        {data, (word32)sizeof(data), SHAh, RSAk, rsaPrivKey, rsaPrivKeySz,
         rsaCert, rsaCertSz, NULL, 0,
         "pkcs7signedData_RSA_SHA_noattr.der"},
    #endif
    #ifdef WOLFSSL_SHA224
        /* RSA with SHA224 */
        {data, (word32)sizeof(data), SHA224h, RSAk, rsaPrivKey, rsaPrivKeySz,
         rsaCert, rsaCertSz, attribs, (sizeof(attribs)/sizeof(PKCS7Attrib)),
         "pkcs7signedData_RSA_SHA224.der"},
    #endif
    #ifndef NO_SHA256
        /* RSA with SHA256 */
        {data, (word32)sizeof(data), SHA256h, RSAk, rsaPrivKey, rsaPrivKeySz,
         rsaCert, rsaCertSz, attribs, (sizeof(attribs)/sizeof(PKCS7Attrib)),
         "pkcs7signedData_RSA_SHA256.der"},
    #endif
    #if defined(WOLFSSL_SHA384)
        /* RSA with SHA384 */
        {data, (word32)sizeof(data), SHA384h, RSAk, rsaPrivKey, rsaPrivKeySz,
         rsaCert, rsaCertSz, attribs, (sizeof(attribs)/sizeof(PKCS7Attrib)),
         "pkcs7signedData_RSA_SHA384.der"},
    #endif
    #if defined(WOLFSSL_SHA512)
        /* RSA with SHA512 */
        {data, (word32)sizeof(data), SHA512h, RSAk, rsaPrivKey, rsaPrivKeySz,
         rsaCert, rsaCertSz, attribs, (sizeof(attribs)/sizeof(PKCS7Attrib)),
         "pkcs7signedData_RSA_SHA512.der"},
    #endif
#endif /* NO_RSA */

#ifdef HAVE_ECC
    #ifndef NO_SHA
        /* ECDSA with SHA */
        {data, (word32)sizeof(data), SHAh, ECDSAk, eccPrivKey, eccPrivKeySz,
         eccCert, eccCertSz, attribs, (sizeof(attribs)/sizeof(PKCS7Attrib)),
         "pkcs7signedData_ECDSA_SHA.der"},

        /* ECDSA with SHA, no signed attributes */
        {data, (word32)sizeof(data), SHAh, ECDSAk, eccPrivKey, eccPrivKeySz,
         eccCert, eccCertSz, NULL, 0,
         "pkcs7signedData_ECDSA_SHA_noattr.der"},
    #endif
    #ifdef WOLFSSL_SHA224
        /* ECDSA with SHA224 */
        {data, (word32)sizeof(data), SHA224h, ECDSAk, eccPrivKey, eccPrivKeySz,
         eccCert, eccCertSz, attribs, (sizeof(attribs)/sizeof(PKCS7Attrib)),
         "pkcs7signedData_ECDSA_SHA224.der"},
    #endif
    #ifndef NO_SHA256
        /* ECDSA with SHA256 */
        {data, (word32)sizeof(data), SHA256h, ECDSAk, eccPrivKey, eccPrivKeySz,
         eccCert, eccCertSz, attribs, (sizeof(attribs)/sizeof(PKCS7Attrib)),
         "pkcs7signedData_ECDSA_SHA256.der"},
    #endif
    #ifdef WOLFSSL_SHA384
        /* ECDSA with SHA384 */
        {data, (word32)sizeof(data), SHA384h, ECDSAk, eccPrivKey, eccPrivKeySz,
         eccCert, eccCertSz, attribs, (sizeof(attribs)/sizeof(PKCS7Attrib)),
         "pkcs7signedData_ECDSA_SHA384.der"},
    #endif
    #ifdef WOLFSSL_SHA512
        /* ECDSA with SHA512 */
        {data, (word32)sizeof(data), SHA512h, ECDSAk, eccPrivKey, eccPrivKeySz,
         eccCert, eccCertSz, attribs, (sizeof(attribs)/sizeof(PKCS7Attrib)),
         "pkcs7signedData_ECDSA_SHA512.der"},
    #endif
#endif /* HAVE_ECC */
    };

    testSz = sizeof(testVectors) / sizeof(pkcs7SignedVector);

    outSz = FOURK_BUF;
    out = (byte*)XMALLOC(outSz, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (out == NULL)
        return -7700;

#ifndef HAVE_FIPS
    ret = wc_InitRng_ex(&rng, HEAP_HINT, devId);
#else
    ret = wc_InitRng(&rng);
#endif
    if (ret != 0) {
        XFREE(out, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return -7701;
    }

    for (i = 0; i < testSz; i++) {

        pkcs7.heap = HEAP_HINT;
        ret = wc_PKCS7_InitWithCert(&pkcs7, testVectors[i].cert,
                                    (word32)testVectors[i].certSz);

        if (ret != 0) {
            XFREE(out, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            return -7702;
        }

        pkcs7.rng             = &rng;
        pkcs7.content         = (byte*)testVectors[i].content;
        pkcs7.contentSz       = testVectors[i].contentSz;
        pkcs7.hashOID         = testVectors[i].hashOID;
        pkcs7.encryptOID      = testVectors[i].encryptOID;
        pkcs7.privateKey      = testVectors[i].privateKey;
        pkcs7.privateKeySz    = testVectors[i].privateKeySz;
        pkcs7.signedAttribs   = testVectors[i].signedAttribs;
        pkcs7.signedAttribsSz = testVectors[i].signedAttribsSz;

        /* generate senderNonce */
        {
            senderNonce[0] = 0x04;
            senderNonce[1] = PKCS7_NONCE_SZ;

            ret = wc_RNG_GenerateBlock(&rng, &senderNonce[2], PKCS7_NONCE_SZ);
            if (ret != 0) {
                XFREE(out, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                wc_PKCS7_Free(&pkcs7);
                return -7703;
            }
        }

        /* generate trans ID */
        {
            wc_Sha sha;
            byte digest[WC_SHA_DIGEST_SIZE];
            int j,k;

            transId[0] = 0x13;
            transId[1] = WC_SHA_DIGEST_SIZE * 2;

            ret = wc_InitSha_ex(&sha, HEAP_HINT, devId);
            if (ret != 0) {
                XFREE(out, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
                wc_PKCS7_Free(&pkcs7);
                return -7704;
            }
            wc_ShaUpdate(&sha, pkcs7.publicKey, pkcs7.publicKeySz);
            wc_ShaFinal(&sha, digest);
            wc_ShaFree(&sha);

            for (j = 0, k = 2; j < WC_SHA_DIGEST_SIZE; j++, k += 2) {
                XSNPRINTF((char*)&transId[k], 3, "%02x", digest[j]);
            }
        }

        encodedSz = wc_PKCS7_EncodeSignedData(&pkcs7, out, outSz);
        if (encodedSz < 0) {
            XFREE(out, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            wc_PKCS7_Free(&pkcs7);
            return -7705;
        }

    #ifdef PKCS7_OUTPUT_TEST_BUNDLES
        /* write PKCS#7 to output file for more testing */
        file = fopen(testVectors[i].outFileName, "wb");
        if (!file) {
            XFREE(out, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            wc_PKCS7_Free(&pkcs7);
            return -7706;
        }
        ret = (int)fwrite(out, 1, encodedSz, file);
        fclose(file);
        if (ret != (int)encodedSz) {
            XFREE(out, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            wc_PKCS7_Free(&pkcs7);
            return -7707;
        }
    #endif /* PKCS7_OUTPUT_TEST_BUNDLES */

        wc_PKCS7_Free(&pkcs7);
        wc_PKCS7_InitWithCert(&pkcs7, NULL, 0);

        ret = wc_PKCS7_VerifySignedData(&pkcs7, out, outSz);
        if (ret < 0) {
            XFREE(out, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            wc_PKCS7_Free(&pkcs7);
            return -7708;
        }

        if (pkcs7.singleCert == NULL || pkcs7.singleCertSz == 0) {
            XFREE(out, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            wc_PKCS7_Free(&pkcs7);
            return -7709;
        }

    #ifdef PKCS7_OUTPUT_TEST_BUNDLES
        file = fopen("./pkcs7cert.der", "wb");
        if (!file) {
            XFREE(out, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            wc_PKCS7_Free(&pkcs7);
            return -7710;
        }
        ret = (int)fwrite(pkcs7.singleCert, 1, pkcs7.singleCertSz, file);
        fclose(file);
    #endif /* PKCS7_OUTPUT_TEST_BUNDLES */

        wc_PKCS7_Free(&pkcs7);
    }

    XFREE(out, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    wc_FreeRng(&rng);

    if (ret > 0)
        return 0;

#ifndef HAVE_ECC
    (void)eccCert;
    (void)eccCertSz;
    (void)eccPrivKey;
    (void)eccPrivKeySz;
#endif
#ifdef NO_RSA
    (void)rsaCert;
    (void)rsaCertSz;
    (void)rsaPrivKey;
    (void)rsaPrivKeySz;
#endif

    return ret;
}


int pkcs7signed_test(void)
{
    int ret = 0;

    byte* rsaCert    = NULL;
    byte* eccCert    = NULL;
    byte* rsaPrivKey = NULL;
    byte* eccPrivKey = NULL;

    word32 rsaCertSz    = 0;
    word32 eccCertSz    = 0;
    word32 rsaPrivKeySz = 0;
    word32 eccPrivKeySz = 0;

#ifndef NO_RSA
    /* read client RSA cert and key in DER format */
    rsaCert = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (rsaCert == NULL)
        return -7720;

    rsaPrivKey = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (rsaPrivKey == NULL) {
        XFREE(rsaCert, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return -7721;
    }

    rsaCertSz = FOURK_BUF;
    rsaPrivKeySz = FOURK_BUF;
#endif /* NO_RSA */

#ifdef HAVE_ECC
    /* read client ECC cert and key in DER format */
    eccCert = (byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (eccCert == NULL) {
        XFREE(rsaCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(rsaPrivKey, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return -7722;
    }

    eccPrivKey =(byte*)XMALLOC(FOURK_BUF, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (eccPrivKey == NULL) {
        XFREE(rsaCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(rsaPrivKey, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(eccCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return -7723;
    }

    eccCertSz = FOURK_BUF;
    eccPrivKeySz = FOURK_BUF;
#endif /* HAVE_ECC */

    ret = pkcs7_load_certs_keys(rsaCert, &rsaCertSz, rsaPrivKey,
                                &rsaPrivKeySz, eccCert, &eccCertSz,
                                eccPrivKey, &eccPrivKeySz);
    if (ret < 0) {
        XFREE(rsaCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(rsaPrivKey, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(eccCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        XFREE(eccPrivKey, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        return ret;
    }

    ret = pkcs7signed_run_vectors(rsaCert, (word32)rsaCertSz,
                                  rsaPrivKey, (word32)rsaPrivKeySz,
                                  eccCert, (word32)eccCertSz,
                                  eccPrivKey, (word32)eccPrivKeySz);

    XFREE(rsaCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(rsaPrivKey, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(eccCert,    HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(eccPrivKey, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

    return ret;
}

#endif /* HAVE_PKCS7 */

#ifdef HAVE_VALGRIND
/* Need a static build to have access to symbols. */

/* Maximum number of bytes in a number to test. */
#define MP_MAX_TEST_BYTE_LEN      16

#if defined(HAVE_ECC) || defined(WOLFSSL_KEY_GEN)
static int randNum(mp_int* n, int len, WC_RNG* rng, void* heap)
{
    byte d[MP_MAX_TEST_BYTE_LEN];
    int  ret;

    (void)heap;

    do {
        ret = wc_RNG_GenerateBlock(rng, d, len);
        if (ret != 0)
            return ret;
        ret = mp_read_unsigned_bin(n, d, len);
        if (ret != 0)
            return ret;
    } while (mp_iszero(n));

    return 0;
}
#endif

int mp_test(void)
{
    WC_RNG rng;
    int    ret;
#if defined(HAVE_ECC) || defined(WOLFSSL_KEY_GEN)
    int    i, j, k;
    mp_digit d;
#endif
    mp_int a, b, r1, r2, p;

    ret = mp_init_multi(&a, &b, &r1, &r2, NULL, NULL);
    if (ret != 0)
        return -7800;

    mp_init_copy(&p, &a);

#ifndef HAVE_FIPS
    ret = wc_InitRng_ex(&rng, HEAP_HINT, devId);
#else
    ret = wc_InitRng(&rng);
#endif
    if (ret != 0)
        goto done;

#if defined(HAVE_ECC) || defined(WOLFSSL_KEY_GEN)
    mp_set_int(&a, 0);
    if (a.used != 0 || a.dp[0] != 0)
        return -7801;

    for (j = 1; j <= MP_MAX_TEST_BYTE_LEN; j++) {
        for (i = 0; i < 4 * j; i++) {
            /* New values to use. */
            ret = randNum(&p, j, &rng, NULL);
            if (ret != 0)
                return -7802;
            ret = randNum(&a, j, &rng, NULL);
            if (ret != 0)
                return -7803;
            ret = randNum(&b, j, &rng, NULL);
            if (ret != 0)
                return -7804;
            ret = wc_RNG_GenerateBlock(&rng, (byte*)&d, sizeof(d));
            if (ret != 0)
                return -7805;
            d &= MP_MASK;

            /* Ensure sqrmod produce same result as mulmod. */
            ret = mp_sqrmod(&a, &p, &r1);
            if (ret != 0)
                return -7806;
            ret = mp_mulmod(&a, &a, &p, &r2);
            if (ret != 0)
                return -7807;
            if (mp_cmp(&r1, &r2) != 0)
                return -7808;

            /* Ensure add with mod produce same result as sub with mod. */
            ret = mp_addmod(&a, &b, &p, &r1);
            if (ret != 0)
                return -7809;
            b.sign ^= 1;
            ret = mp_submod(&a, &b, &p, &r2);
            if (ret != 0)
                return -7810;
            if (mp_cmp(&r1, &r2) != 0)
                return -7811;

            /* Ensure add digit produce same result as sub digit. */
            ret = mp_add_d(&a, d, &r1);
            if (ret != 0)
                return -7812;
            ret = mp_sub_d(&r1, d, &r2);
            if (ret != 0)
                return -7813;
            if (mp_cmp(&a, &r2) != 0)
                return -7814;

            /* Invert - if p is even it will use the slow impl.
             *        - if p and a are even it will fail.
             */
            ret = mp_invmod(&a, &p, &r1);
            if (ret != 0 && ret != MP_VAL)
                return -7815;
            ret = 0;

            /* Shift up and down number all bits in a digit. */
            for (k = 0; k < DIGIT_BIT; k++) {
                mp_mul_2d(&a, k, &r1);
                mp_div_2d(&r1, k, &r2, &p);
                if (mp_cmp(&a, &r2) != 0)
                    return -7816;
                if (!mp_iszero(&p))
                    return -7817;
                mp_rshb(&r1, k);
                if (mp_cmp(&a, &r1) != 0)
                    return -7818;
            }
        }
    }

    /* Check that setting a 32-bit digit works. */
    d &= 0xffffffff;
    mp_set_int(&a, d);
    if (a.used != 1 || a.dp[0] != d)
        return -7819;

    /* Check setting a bit and testing a bit works. */
    for (i = 0; i < MP_MAX_TEST_BYTE_LEN * 8; i++) {
        mp_zero(&a);
        mp_set_bit(&a, i);
        if (!mp_is_bit_set(&a, i))
            return -7820;
    }
#endif

done:
    mp_clear(&p);
    mp_clear(&r2);
    mp_clear(&r1);
    mp_clear(&b);
    mp_clear(&a);
    wc_FreeRng(&rng);
    return ret;
}
#endif

#ifdef HAVE_VALGRIND
/* Need a static build to have access to symbols. */

#ifndef WOLFSSL_SSL_H
/* APIs hiding in ssl.h */
extern int wolfSSL_Debugging_ON(void);
extern void wolfSSL_Debugging_OFF(void);
#endif

#ifdef DEBUG_WOLFSSL
static int log_cnt = 0;
static void my_Logging_cb(const int logLevel, const char *const logMessage)
{
    (void)logLevel;
    (void)logMessage;
    log_cnt++;
}
#endif

int logging_test(void)
{
#ifdef DEBUG_WOLFSSL
    const char* msg = "Testing, testing. 1, 2, 3, 4 ...";
    byte        a[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    byte        b[256];
    size_t      i;

    for (i = 0; i < sizeof(b); i++)
        b[i] = i;

    if (wolfSSL_Debugging_ON() != 0)
        return -7900;
    if (wolfSSL_SetLoggingCb(NULL) != BAD_FUNC_ARG)
        return -7901;

    WOLFSSL_MSG(msg);
    WOLFSSL_BUFFER(a, sizeof(a));
    WOLFSSL_BUFFER(b, sizeof(b));
    WOLFSSL_BUFFER(NULL, 0);

    wolfSSL_Debugging_OFF();

    WOLFSSL_MSG(msg);
    WOLFSSL_BUFFER(b, sizeof(b));

    if (wolfSSL_SetLoggingCb(my_Logging_cb) != 0)
        return -7902;

    wolfSSL_Debugging_OFF();

    WOLFSSL_MSG(msg);
    WOLFSSL_BUFFER(b, sizeof(b));

    if (log_cnt != 0)
        return -7903;
    if (wolfSSL_Debugging_ON() != 0)
        return -7904;

    WOLFSSL_MSG(msg);
    WOLFSSL_BUFFER(b, sizeof(b));

    /* One call for each line of output. */
    if (log_cnt != 17)
        return -7905;
#else
    if (wolfSSL_Debugging_ON() != NOT_COMPILED_IN)
        return -7906;
    wolfSSL_Debugging_OFF();
    if (wolfSSL_SetLoggingCb(NULL) != NOT_COMPILED_IN)
        return -7907;
#endif
    return 0;
}
#endif

int mutex_test(void)
{
#ifdef WOLFSSL_PTHREADS
    wolfSSL_Mutex m;
#endif
    wolfSSL_Mutex *mm = wc_InitAndAllocMutex();
    if (mm == NULL)
        return -8000;
    wc_FreeMutex(mm);
    XFREE(mm, NULL, DYNAMIC_TYPE_MUTEX);

#ifdef WOLFSSL_PTHREADS
    if (wc_InitMutex(&m) != 0)
        return -8001;
    if (wc_LockMutex(&m) != 0)
        return -8002;
    if (wc_FreeMutex(&m) != BAD_MUTEX_E)
        return -8003;
    if (wc_UnLockMutex(&m) != 0)
        return -8004;
    if (wc_FreeMutex(&m) != 0)
        return -8005;
    if (wc_LockMutex(&m) != BAD_MUTEX_E)
        return -8006;
    if (wc_UnLockMutex(&m) != BAD_MUTEX_E)
        return -8007;
#endif

    return 0;
}

#if defined(USE_WOLFSSL_MEMORY) && !defined(FREERTOS)
static int malloc_cnt = 0;
static int realloc_cnt = 0;
static int free_cnt = 0;

static void *my_Malloc_cb(size_t size)
{
    malloc_cnt++;
    return malloc(size);
}
static void my_Free_cb(void *ptr)
{
    free_cnt++;
    free(ptr);
}
static void *my_Realloc_cb(void *ptr, size_t size)
{
    realloc_cnt++;
    return realloc(ptr, size);
}

int memcb_test(void)
{
    int ret = 0;
    byte* b = NULL;
    wolfSSL_Malloc_cb  mc;
    wolfSSL_Free_cb    fc;
    wolfSSL_Realloc_cb rc;

    /* Save existing memory callbacks */
    if (wolfSSL_GetAllocators(&mc, &fc, &rc) != 0)
        return -8100;

    /* test realloc */
    b = (byte*)XREALLOC(b, 1024, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (b == NULL) {
        ERROR_OUT(-8101, exit_memcb);
    }
    XFREE(b, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    b = NULL;

    /* Parameter Validation testing. */
    if (wolfSSL_SetAllocators(NULL, (wolfSSL_Free_cb)&my_Free_cb,
            (wolfSSL_Realloc_cb)&my_Realloc_cb) != BAD_FUNC_ARG) {
        ERROR_OUT(-8102, exit_memcb);
    }
    if (wolfSSL_SetAllocators((wolfSSL_Malloc_cb)&my_Malloc_cb, NULL,
            (wolfSSL_Realloc_cb)&my_Realloc_cb) != BAD_FUNC_ARG) {
        ERROR_OUT(-8103, exit_memcb);
    }
    if (wolfSSL_SetAllocators((wolfSSL_Malloc_cb)&my_Malloc_cb,
            (wolfSSL_Free_cb)&my_Free_cb, NULL) != BAD_FUNC_ARG) {
        ERROR_OUT(-8104, exit_memcb);
    }

    /* Use API. */
    if (wolfSSL_SetAllocators((wolfSSL_Malloc_cb)&my_Malloc_cb,
        (wolfSSL_Free_cb)&my_Free_cb, (wolfSSL_Realloc_cb)my_Realloc_cb) != 0) {
        ERROR_OUT(-8105, exit_memcb);
    }

    b = (byte*)XMALLOC(1024, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    b = (byte*)XREALLOC(b, 1024, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(b, NULL, DYNAMIC_TYPE_TMP_BUFFER);

#ifndef WOLFSSL_STATIC_MEMORY
    if (malloc_cnt != 1 || free_cnt != 1 || realloc_cnt != 1)
#else
    if (malloc_cnt != 0 || free_cnt != 0 || realloc_cnt != 0)
#endif
        ret = -8106;

exit_memcb:

    /* restore memory callbacks */
    wolfSSL_SetAllocators(mc, fc, rc);

    return ret;
}
#endif

#undef ERROR_OUT

#else
    #ifndef NO_MAIN_DRIVER
        int main() { return 0; }
    #endif
#endif /* NO_CRYPT_TEST */
