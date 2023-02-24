/* rsa_sign_verify.c
 *
 * Copyright (C) 2006-2023 wolfSSL Inc.
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

/* This file is an example of signing and verifying an RSA signature.
 * The signature can be PKCS#1.5 formatted and PSS formatted.
 * 
 * - PKCS#1.5
 *  1. hash -> encSig
 *  2. encSig -> signature 
 *  3. signature -> decSig
 * 
 * - PSS
 *  1. hash -> signature 
 *  2. signature -> decSig
 * 
 * PKCS#1.5 is used for the Signature by default.
 * To turning on PSS, define PSS_PADDING
 */

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include<wolfssl/test.h>

/* Maximum bound on digest algorithm encoding around digest */
#define MAX_ENC_ALG_SZ      32

/* RSA Key size bits */
#define RSA_KEY_SIZE 2048





#define CHECK_RET(a, b, eLabel, msg) { \
                                        if (a != b) {                    \
                                            printf("failed %s\n", msg);  \
                                            printf("ret = %d\n", a);   \
                                            goto eLabel;                 \
                                        }                                \
                                     }

/* Variables to be used in both sign() and verify() */
byte        msg[] = "This is a message.";
byte        hash[WC_SHA256_DIGEST_SIZE];
byte        signature[ RSA_KEY_SIZE / 8];
word32      sigLen;
byte        encSig[WC_SHA256_DIGEST_SIZE + MAX_ENC_ALG_SZ];
word32      encSigLen = 0;
byte        decSig[ RSA_KEY_SIZE / 8];
word32      decSigLen;

RsaKey      key;
RsaKey*     pKey = NULL;


int sign(){

#ifdef DEBUG_MEMORY
    wolfCrypt_Init();
    InitMemoryTracker();
#endif

    int ret = 0;
    wc_Sha256   sha256;
    wc_Sha256*  pSha256 = NULL;
    WC_RNG      rng;
    WC_RNG*     pRng;
    long        e = 65537; /* standard value to use for exponent */

    /* Calculate SHA-256 digest of message */
    ret = wc_InitSha256(&sha256);
    CHECK_RET(ret, 0, finish, "wc_InitSha256()");
    pSha256 = &sha256;
    ret = wc_Sha256Update(&sha256, msg, sizeof(msg));
    CHECK_RET(ret, 0, finish, "wc_Sha256Update()");
    ret = wc_Sha256Final(&sha256, hash);
    CHECK_RET(ret, 0, finish, "wc_Sha256Final()");


    /* Initialize the RSA key. */
    ret = wc_InitRsaKey(&key, NULL);
    CHECK_RET(ret, 0, finish, "wc_InitRng()");
    pKey = &key;

    ret = wc_InitRng(&rng);
    CHECK_RET(ret, 0, finish, "wc_InitRng()");
    pRng = &rng;
#if defined(WC_RSA_BLINDING) || defined(PSS_PADDING)
    ret = wc_RsaSetRNG(&key, &rng);
    CHECK_RET(ret, 0, finish, "wc_RsaSetRNG()");
#endif
    /* Generate 2048-bit RSA key*/
    ret = wc_MakeRsaKey(&key, RSA_KEY_SIZE, e, &rng);
    CHECK_RET(ret, 0, finish, "wc_MakeRsaKey()");

    /* Encode digest with algorithm information as per PKCS#1.5 */
    encSigLen = wc_EncodeSignature(encSig, hash, sizeof(hash), SHA256h);
    if ((int)encSigLen < 0)
        ret = (int)encSigLen;
    CHECK_RET(ret, 0, finish, "wc_EncodeSignature()");

#ifdef PSS_PADDING
    sigLen = wc_RsaPSS_Sign(hash, sizeof(hash), signature, sizeof(signature)\
                           , WC_HASH_TYPE_SHA256, WC_MGF1SHA256, &key, &rng);
    if ((int)sigLen < 0)
        ret = (int)sigLen;
    CHECK_RET(ret, 0, finish, "wc_RsaPSS_Sign()");

#else /* PKCS#1.5 */
    sigLen = wc_RsaSSL_Sign(encSig, encSigLen, signature, sizeof(signature),\
                             &key, NULL);
    if ((int)sigLen < 0)
        ret = (int)sigLen;
    CHECK_RET(ret, 0, finish, "wc_RsaSSL_Sign()");

#endif


/* Generated Rsakey must be released in verify()  */
finish:
    if (pSha256 != NULL)
        wc_Sha256Free(pSha256);
    if (pRng != NULL)
        wc_FreeRng(pRng);

#if defined(DEBUG_MEMORY)
    printf("Memory usage : sign() \n");
    printf("=================================\n");
    ShowMemoryTracker();
    CleanupMemoryTracker();
    wolfCrypt_Cleanup();
#endif

    return ret;
}

/* Verifies the signature with the message and RSA public key.
 * Returns 0 on success and 1 otherwise.
 */
int verify(){

#ifdef DEBUG_MEMORY
    wolfCrypt_Init();
    InitMemoryTracker();
#endif

    int ret = 0;
/* Variables for benchmark */
#ifdef BENCHMARK
    double start, total_time;
#ifndef BENCH_TIME_SEC
    #define BENCH_TIME_SEC 3
#endif
    int count;
#endif

/* Check the RSA Key */
    if (pKey == NULL){
        printf("RSA Key is NULL in verify()\n");
        return -1;
    }

#ifdef BENCHMARK 
    count = 0;
    printf("Running benchmark...\n");
    printf("Please Wait %.2f seconds\n", (double)BENCH_TIME_SEC);
    start = current_time(0);
    while( (double)BENCH_TIME_SEC > (total_time = current_time(0) - start ) ){
#endif

        /* Verify the signature by decrypting the value. */
        
        #ifdef PSS_PADDING
            decSigLen = wc_RsaPSS_VerifyCheck(signature, sizeof(signature),
                decSig, sizeof(decSig), hash, sizeof(hash), WC_HASH_TYPE_SHA256, WC_MGF1SHA256, &key);
            
            if ((int)decSigLen < 0)
                ret = (int)decSigLen;
            CHECK_RET(ret, 0, finish, "wc_RsaPSS_VerifyCheck()");
        
        #else /* PKCS#1.5 */
            decSigLen = wc_RsaSSL_Verify(signature, sizeof(signature),
                                            decSig, sizeof(decSig), &key);
            if ((int)decSigLen < 0)
                ret = (int)decSigLen;
            CHECK_RET(ret, 0, finish, "wc_RsaSSL_Verify()");

        /* Check the decrypted result matches the encoded digest. */
        if (ret == 0 && encSigLen != decSigLen)
            ret = -1;
        if (ret == 0 && XMEMCMP(encSig, decSig, encSigLen) != 0)
            ret = -1;

        if(ret != 0){
            printf("Invalid Signature!\n");
            goto finish;
        }

        #endif

#ifdef BENCHMARK 
        count++;
    }
   
    printf("Takes %1.2f Sec for %d times,    %6.2f Cycles/sec\n", total_time, count, count/total_time);
    printf("Finished Benchmark \n");
#elif defined(DEBUG_MEMORY)

#else
    printf("Verified!\n");
#endif


finish:
    if (pKey != NULL)
        wc_FreeRsaKey(pKey);

#ifdef DEBUG_MEMORY
    printf("\n");
    printf("Memory usage : verify() \n");
    printf("=================================\n");
    ShowMemoryTracker();
    CleanupMemoryTracker();
    wolfCrypt_Cleanup();
#endif
    return ret;
}



int main(){
    int ret = 0;
#ifdef BENCHMARK
    printf("---------------------------------------------------------------\n");
#if defined(SP_C64_FLAG)
    printf("Enabled 64-bit SP \n");
#elif defined(SP_C32_FLAG)
    printf("Enabled 32-bit SP \n");
#elif defined(SP_X86_64_FLAG)
    printf("Enabled SP for x86_64\n");
#elif defined(SP_ARM64_FLAG)
    printf("Enabled SP for Arm64\n");
#elif defined(TFM_FLAG)
    printf("Enabled TFM \n");
#endif
    printf("---------------------------------------------------------------\n");
#endif /* BENCHMARK */

#ifdef DEBUG_MEMORY
    ret = StackSizeCheck(NULL, (thread_func)sign);
#else 
    ret = sign();
#endif

/* Check the return value of sign() */
    if(ret != 0)
        return ret;

#ifdef DEBUG_MEMORY
    ret = StackSizeCheck(NULL, (thread_func)verify);
#else 
    ret = verify();
#endif
    return ret;
}
