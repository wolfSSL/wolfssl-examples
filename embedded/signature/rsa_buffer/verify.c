/* rsa_pub_2048.h
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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

/* This file is an example of verifying an RSA signature.
 * The signature is PKCS#1.5 formatted.
 * Key and data are held in buffers.
 * "signature.h", used by this program, can be generated using "sign.c".
 */

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/asn_public.h>

#include "rsa_pub_2048.h"
#include "signature.h"
#include<wolfssl/test.h>

/* Maximum bound on digest algorithm encoding around digest */
#define MAX_ENC_ALG_SZ      32

/* Verifies the signature with the message and RSA public key.
 * Returns 0 on success and 1 otherwise.
 */
int verify()
{
    int            ret = 0;
    Sha256         sha256;
    Sha256*        pSha256 = NULL;
    RsaKey         rsaKey;
    RsaKey*        pRsaKey = NULL;
    word32         idx;
    unsigned char  digest[WC_SHA256_DIGEST_SIZE];
    unsigned char  encSig[WC_SHA256_DIGEST_SIZE + MAX_ENC_ALG_SZ];
    word32         encSigLen = 0;
    unsigned char  decSig[sizeof(rsa_sig_2048)];
    word32         decSigLen = 0;

/* Variables for benchmark */
    double start, total_time;
#ifndef BENCH_TIME_SEC
    #define BENCH_TIME_SEC 3
#endif
    int count;

#ifdef DEBUG_MEMORY
    wolfCrypt_Init();
    InitMemoryTracker();
#endif
    /* Calculate SHA-256 digest of message */
    if (ret == 0)
        ret = wc_InitSha256(&sha256);
    if (ret == 0) {
        pSha256 = &sha256;
        ret = wc_Sha256Update(&sha256, msg, sizeof(msg));
    }
    if (ret == 0)
        ret = wc_Sha256Final(&sha256, digest);

    /* Encode digest with algorithm information as per PKCS#1.5 */
    if (ret == 0) {
        encSigLen = wc_EncodeSignature(encSig, digest, sizeof(digest), SHA256h);
        if ((int)encSigLen < 0)
            ret = (int)encSigLen;
    }

    /* Initialize the RSA key and decode the DER encoded public key. */
    if (ret == 0)
        ret = wc_InitRsaKey(&rsaKey, NULL);
    if (ret == 0) {
        pRsaKey = &rsaKey;

        idx = 0;
        ret = wc_RsaPublicKeyDecode(public_key_2048, &idx, &rsaKey,
                                    sizeof(public_key_2048));
    }

#ifdef BENCHMARK 
    count = 0;
    printf("Running benchmark...\n");
    printf("Please Wait %.2f seconds\n", (double)BENCH_TIME_SEC);
    start = current_time(0);// 1 0
    while( (double)BENCH_TIME_SEC > (total_time = current_time(0) - start ) ){
#endif
        /* Verify the signature by decrypting the value. */
        if (ret == 0) {
            decSigLen = wc_RsaSSL_Verify(rsa_sig_2048, sizeof(rsa_sig_2048),
                                            decSig, sizeof(decSig), &rsaKey);
            if ((int)decSigLen < 0)
                ret = (int)decSigLen;
        }
        /* Check the decrypted result matches the encoded digest. */
        if (ret == 0 && encSigLen != decSigLen)
            ret = -1;
        if (ret == 0 && XMEMCMP(encSig, decSig, encSigLen) != 0)
            ret = -1;

        if(ret != 0){
            printf("Invalid Signature!\n");
            goto finish;
        }

#ifdef BENCHMARK
        count++;
    }
   
    printf("Takes %1.2f Sec for %d times,    %6.2f Cycles/sec\n", total_time, count, count/total_time);
    printf("Finished Benchmark \n");
#else 
    printf("Verified\n");
#endif

finish: 
    /* Free the data structures */
    if (pRsaKey != NULL)
        wc_FreeRsaKey(pRsaKey);
    if (pSha256 != NULL)
        wc_Sha256Free(pSha256);

#ifdef DEBUG_MEMORY
    ShowMemoryTracker();
    CleanupMemoryTracker();
    wolfCrypt_Cleanup();
#endif 
    return ret;
}

int main(){
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
    return StackSizeCheck(NULL, (thread_func)verify);
#else 

    return verify();
#endif
}
