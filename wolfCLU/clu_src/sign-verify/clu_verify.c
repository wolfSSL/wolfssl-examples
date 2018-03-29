/* clu_verify.c
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "clu_include/sign-verify/clu_verify.h"
#include <wolfssl/wolfcrypt/types.h>
#include <wolfssl/ssl.h>
#include "clu_include/clu_header_main.h"

enum {
    RSA_SIGN,
    ECC_SIGN,
    ED25519_SIGN,
};

byte* wolfCLU_generate_public_key(char* privKey, byte* outBuf, int* outBufSz) {
#ifndef NO_RSA
    int ret;
    int privFileSz;
    word32 index = 0;
    FILE* privKeyFile;
    RsaKey key;
    WC_RNG rng;
    byte* keyBuf;
    
    XMEMSET(&rng, 0, sizeof(rng));
    XMEMSET(&key, 0, sizeof(key));
    
    ret = wc_InitRsaKey(&key, NULL);
    if (ret != 0) {
        printf("Failed to initialize RsaKey.\nRet: %d", ret);
        *outBufSz = ret;
        return outBuf;
    }
    

    /* read in and store private key */

    privKeyFile = fopen(privKey, "rb");
    fseek(privKeyFile, 0, SEEK_END);
    privFileSz = ftell(privKeyFile);
    keyBuf = malloc(privFileSz*sizeof(keyBuf));
    fseek(privKeyFile, 0, SEEK_SET);
    fread(keyBuf, 1, privFileSz, privKeyFile);
    fclose(privKeyFile);
    
    /* retrieving private key and storing in the RsaKey */
    ret = wc_RsaPrivateKeyDecode(keyBuf, &index, &key, privFileSz);
    if (ret < 0 ) {
        printf("Failed to decode private key.\nRET: %d\n", ret);
        *outBufSz = ret;
        return outBuf;
    }
    
    /* set output buffer to twice the private key size to ensure enough space */
    *outBufSz = 2*wc_RsaEncryptSize(&key);
    
    /* setting up output buffer based on privateKeyFile size */
    outBuf = malloc(*outBufSz*sizeof(outBuf));
    XMEMSET(outBuf, 0, *outBufSz);
    
    ret = wc_RsaKeyToPublicDer(&key, outBuf, *outBufSz);
    if (ret < 0) {
        printf("Failed to create RSA public key.\nBuf size: %d\nRET: %d\n", *outBufSz, ret);
        *outBufSz = ret;
        return outBuf;
    }
    *outBufSz = ret;
    return outBuf;
#else
    return NOT_COMPILED_IN;
#endif
}

int wolfCLU_verify_signature(char* sig, char* hash, 
                             char* out, char* keyPath, int keyType, int pubIn) {

    int hSz;
    FILE* h;
    byte* h_mssg;

    int ret = -1;
    int fSz;
    FILE* f = fopen(sig,"rb");
    
    fseek(f, 0, SEEK_END);
    fSz = ftell(f);

    byte data[fSz];

    fseek(f, 0, SEEK_SET);
    fread(data, 1, fSz, f);
    fclose(f);

    switch(keyType) {
        case RSA_SIGN:
            ret = wolfCLU_verify_signature_rsa(data, out, fSz, keyPath, pubIn);
            break;
            
        case ECC_SIGN:
            hSz;
            h = fopen(hash,"rb");
            
            fseek(h, 0, SEEK_END);
            hSz = ftell(h);

            h_mssg = malloc(hSz*sizeof(h_mssg));

            fseek(h, 0, SEEK_SET);
            fread(h_mssg, 1, hSz, h);
            fclose(h);
            ret = wolfCLU_verify_signature_ecc(data, fSz, h_mssg, hSz, keyPath);
            break;
            
        case ED25519_SIGN:
            ret = wolfCLU_verify_signature_ed25519(data, fSz, h_mssg, hSz, keyPath);
            break;
    }
    return ret;
}

int wolfCLU_verify_signature_rsa(byte* sig, char* out, int sigSz, char* keyPath, int pubIn) {

#ifndef NO_RSA    
    int ret;
    int keyFileSz;
    word32 index = 0;
    FILE* keyPathFile;
    RsaKey key;
    WC_RNG rng;
    byte* keyBuf;
    
    XMEMSET(&rng, 0, sizeof(rng));
    XMEMSET(&key, 0, sizeof(key));
    
    ret = wc_InitRsaKey(&key, NULL);
    if (ret != 0) {
        printf("Failed to initialize RsaKey.\nRet: %d", ret);
        return ret;
    }
    
    if (pubIn == 1) {
    
    /* read in and store rsa key */
        keyPathFile = fopen(keyPath, "rb");
        fseek(keyPathFile, 0, SEEK_END);
        keyFileSz = ftell(keyPathFile);
        keyBuf = malloc(keyFileSz*sizeof(keyBuf));
        fseek(keyPathFile, 0, SEEK_SET);
        fread(keyBuf, 1, keyFileSz, keyPathFile);
        fclose(keyPathFile);
    } else {
        keyBuf = wolfCLU_generate_public_key(keyPath, keyBuf, &keyFileSz);
        if (keyFileSz < 0) {
                printf("Failed to derive public key from private key.\n");
                return ret;
        }
    }
    
    /* retrieving public key and storing in the RsaKey */
    ret = wc_RsaPublicKeyDecode(keyBuf, &index, &key, keyFileSz);
    if (ret < 0 ) {
        printf("Failed to decode public key.\nRET: %d\n", ret);
        return ret;
    }
    
    /* setting up output buffer based on key size */
    byte outBuf[wc_RsaEncryptSize(&key)];
    XMEMSET(&outBuf, 0, sizeof(outBuf));
    
    ret = wc_RsaSSL_Verify(sig, sigSz, outBuf, sizeof(outBuf), &key);
    if (ret < 0) {
        printf("Failed to verify data with RSA public key.\nRET: %d\n", ret);
        return ret;
    }
    else {
        FILE* s = fopen(out, "wb");
        fwrite(outBuf, 1, sizeof(outBuf), s);
    }
    
    return ret;
#else
    return NOT_COMPILED_IN;
#endif
}

int wolfCLU_verify_signature_ecc(byte* sig, int sigSz, byte* hash, int hashSz, 
                                 char* keyPath) {

#ifdef HAVE_ECC
    int ret;
    int keyFileSz;
    int stat = 0;
    word32 index = 0;
    
    FILE* keyPathFile;
    ecc_key key;
    WC_RNG rng;
    byte* keyBuf;
    
    XMEMSET(&rng, 0, sizeof(rng));
    XMEMSET(&key, 0, sizeof(key));
    
    ret = wc_ecc_init(&key);
    if (ret != 0) {
        printf("Failed to initialize ecc key.\nRet: %d", ret);
        return ret;
    }
    
    /* read in and store ecc key */
    keyPathFile = fopen(keyPath, "rb");
    fseek(keyPathFile, 0, SEEK_END);
    keyFileSz = ftell(keyPathFile);
    keyBuf = malloc(keyFileSz*sizeof(keyBuf));
    fseek(keyPathFile, 0, SEEK_SET);
    fread(keyBuf, 1, keyFileSz, keyPathFile);
    fclose(keyPathFile);
    
    /* retrieving public key and storing in the ecc key */
    ret = wc_EccPublicKeyDecode(keyBuf, &index, &key, keyFileSz);
    if (ret < 0 ) {
        printf("Failed to decode public key.\nRET: %d\n", ret);
        return ret;
    }
    
    ret = wc_ecc_verify_hash(sig, sigSz, hash, hashSz, &stat, &key);
    if (ret < 0) {
        printf("Failed to verify data with Ecc public key.\nRET: %d\n", ret);
        return ret;
    } else if (stat == 1) {
        printf("Valid Signature.\n");
    } else {
        printf("Invalid Signature.\n");
    }
    
    return ret;
#else
    return NOT_COMPILED_IN;
#endif
}

int wolfCLU_verify_signature_ed25519(byte* data, int fSz, 
                              byte* mssg, int mSz, char* path) {
                              
    printf("ERROR: FEATURE COMING SOON! (not yet implemented)\n");
    return FEATURE_COMING_SOON;
}
