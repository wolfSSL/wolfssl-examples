/* clu_sign.c
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

#include "clu_include/sign-verify/clu_sign.h"
#include <wolfssl/wolfcrypt/types.h>
#include <wolfssl/ssl.h>
#include "clu_include/clu_header_main.h"

int wolfCLU_sign_data(char* in, char* out, char* privKey, int keyType) {
    int ret = -1;
    int fSz;
    FILE* f = fopen(in,"rb");
    
    fseek(f, 0, SEEK_END);
    fSz = ftell(f);

    byte data[fSz];

    fseek(f, 0, SEEK_SET);
    fread(data, 1, fSz, f);
    fclose(f);

    switch(keyType) {
    
    case RSA_SIGN:
        ret = wolfCLU_sign_data_rsa(data, out, fSz, privKey);
        break;
        
    case ECC_SIGN:
        ret = wolfCLU_sign_data_ecc(data, out, fSz, privKey);
        break;
        
    case ED25519_SIGN:
        ret = wolfCLU_sign_data_ed25519(data, out, fSz, privKey);
        break;
    
    }
    
    return ret;
}

int wolfCLU_sign_data_rsa(byte* data, char* out, word32 dataSz, char* privKey) {
#ifndef NO_RSA
        int ret;
        int privFileSz;
        size_t rsaKeySz;
        word32 index = 0;
        
        FILE* privKeyFile;
        
        RsaKey key;
        WC_RNG rng;
        
        XMEMSET(&rng, 0, sizeof(rng));
        XMEMSET(&key, 0, sizeof(key));
        
        /* init the RsaKey */
        ret = wc_InitRsaKey(&key, NULL);
        if (ret != 0) {
            printf("Failed to initialize RsaKey\nRET: %d\n", ret);
            return ret;
        }
        
        /* read in and store private key */
        privKeyFile = fopen(privKey, "rb");
        fseek(privKeyFile, 0, SEEK_END);
        privFileSz = ftell(privKeyFile);
        byte keyBuf[privFileSz];
        fseek(privKeyFile, 0, SEEK_SET);
        fread(keyBuf, 1, privFileSz, privKeyFile);
        fclose(privKeyFile);
        
        /* retrieving private key and storing in the RsaKey */
        ret = wc_RsaPrivateKeyDecode(keyBuf, &index, &key, privFileSz);
        if (ret != 0 ) {
            printf("Failed to decode private key.\nRET: %d\n", ret);
            return ret;
        }

        /* setting up output buffer based on key size */
        byte outBuf[wc_RsaEncryptSize(&key)];
        XMEMSET(&outBuf, 0, sizeof(outBuf));
        
        ret = wc_InitRng(&rng);
        #ifdef WC_RSA_BLINDING
        ret = wc_RsaSetRNG(&key, &rng);
        if (ret < 0) {
            printf("Failed to initialize rng.\nRET: %d\n", ret);
            return ret;
        }
        #else
        if (ret != 0) {
            printf("Failed to initialize rng.\nRET: %d\n", ret);
            return ret;
        }
        #endif
        
        ret = wc_RsaSSL_Sign(data, dataSz, outBuf, sizeof(outBuf), &key, &rng);
        if (ret < 0) {
            printf("Failed to sign data with RSA private key.\nRET: %d\n", ret);
            return ret;
        }
        else {
            FILE* s;
            s = fopen(out, "wb");
            fwrite(outBuf, 1, sizeof(outBuf), s);
            fclose(s);
        }
        
        return 0;
#else
        return NOT_COMPILED_IN;
#endif
}

int wolfCLU_sign_data_ecc(byte* data, char* out, word32 fSz, char* privKey) {
#ifdef HAVE_ECC
        int ret;
        int privFileSz;
        size_t eccKeySz;
        word32 index = 0;
        word32 outLen;
        
        FILE* privKeyFile;
        
        ecc_key key;
        WC_RNG rng;
        
        XMEMSET(&rng, 0, sizeof(rng));
        XMEMSET(&key, 0, sizeof(key));
        
        /* init the ecc key */
        ret = wc_ecc_init(&key);
        if (ret != 0) {
            printf("Failed to initialize ecc key\nRET: %d\n", ret);
            return ret;
        }
        
        ret = wc_InitRng(&rng);
        if (ret != 0) {
            printf("Failed to initialize rng.\nRET: %d\n", ret);
            return ret;
        }

        /* read in and store private key */
        privKeyFile = fopen(privKey, "rb");
        fseek(privKeyFile, 0, SEEK_END);
        privFileSz = ftell(privKeyFile);
        byte keyBuf[privFileSz];
        fseek(privKeyFile, 0, SEEK_SET);
        fread(keyBuf, 1, privFileSz, privKeyFile);
        fclose(privKeyFile);

        /* retrieving private key and storing in the Ecc Key */
        ret = wc_EccPrivateKeyDecode(keyBuf, &index, &key, privFileSz);
        if (ret != 0 ) {
            printf("Failed to decode private key.\nRET: %d\n", ret);
            return ret;
        }

        /* setting up output buffer based on key size */
        byte outBuf[wc_ecc_sig_size(&key)];
        XMEMSET(&outBuf, 0, sizeof(outBuf));
        outLen = sizeof(outBuf);

        /* signing input with ecc priv key to produce signature */
        ret = wc_ecc_sign_hash(data, fSz, outBuf, &outLen, &rng, &key);
        if (ret < 0) {
            printf("Failed to sign data with Ecc private key.\nRET: %d\n", ret);
            return ret;
        }
        else {
            FILE* s;
            s = fopen(out, "wb");
            fwrite(outBuf, 1, sizeof(outBuf), s);
            fclose(s);
        }
        
        return ret;
#else
    return NOT_COMPILED_IN;
#endif
}

int wolfCLU_sign_data_ed25519 (byte* data, char* out, 
                               word32 fSz, char* privKey) {

    printf("ERROR: FEATURE COMING SOON! (not yet implemented)\n");
    return FEATURE_COMING_SOON;
}
