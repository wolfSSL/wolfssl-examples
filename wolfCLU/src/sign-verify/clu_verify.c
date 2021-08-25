/* clu_verify.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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

#include <wolfclu/clu_header_main.h>
#include <wolfclu/sign-verify/clu_verify.h>
#include <wolfclu/sign-verify/clu_sign.h> /* for RSA_SIG_VER, ECC_SIG_VER,
                                             ED25519_SIG_VER */

byte* wolfCLU_generate_public_key_rsa(char* privKey, byte* outBuf,
                                      int* outBufSz) {
#ifndef NO_RSA
    int ret;
    int privFileSz;
    word32 index = 0;
    FILE* privKeyFile;
    RsaKey key;
    WC_RNG rng;
    byte* keyBuf;

    if (outBufSz == NULL) {
        printf("Unexpected null argument\n");
        return NULL;
    }

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
    if (privKeyFile == NULL) {
        printf("Unable to open file %s\n", privKey);
        return NULL;
    }
    fseek(privKeyFile, 0, SEEK_END);
    privFileSz = ftell(privKeyFile);
    keyBuf = malloc(privFileSz);
    if (keyBuf != NULL) {
        fseek(privKeyFile, 0, SEEK_SET);
        fread(keyBuf, 1, privFileSz, privKeyFile);
    }
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
    outBuf = malloc(*outBufSz);
    if (outBuf == NULL) {
        return NULL;
    }
    XMEMSET(outBuf, 0, *outBufSz);

    ret = wc_RsaKeyToPublicDer(&key, outBuf, *outBufSz);
    if (ret < 0) {
        printf("Failed to create RSA public key.\nBuf size: %d\nRET: %d\n",
               *outBufSz, ret);
        *outBufSz = ret;
        return outBuf;
    }
    *outBufSz = ret;
    return outBuf;
#else
    return NOT_COMPILED_IN;
#endif
}

int wolfCLU_generate_public_key_ed25519(char* privKey, byte* outBuf) {
#ifdef HAVE_ED25519
    int ret;
    word32 outLen = ED25519_KEY_SIZE;
    FILE* privKeyFile;
    ed25519_key key;
    byte privBuf[ED25519_SIG_SIZE];

    XMEMSET(&key, 0, sizeof(key));
    XMEMSET(privBuf, 0, ED25519_SIG_SIZE);

    ret = wc_ed25519_init(&key);
    if (ret != 0) {
        printf("Failed to initialize ED25519.\nRet: %d", ret);
        return ret;
    }


    /* read in and store private key */

    privKeyFile = fopen(privKey, "rb");
    fread(privBuf, 1, ED25519_SIG_SIZE, privKeyFile);
    fclose(privKeyFile);

    /* retrieving private key and storing in the ED25519 */
    ret = wc_ed25519_import_private_key(privBuf,
                                        ED25519_KEY_SIZE,
                                        privBuf + ED25519_KEY_SIZE,
                                        ED25519_KEY_SIZE,
                                        &key);
    if (ret < 0 ) {
        printf("Failed to decode private key.\nRET: %d\n", ret);
        return ret;
    }

    /* retrive public key from private */
    ret = wc_ed25519_export_public(&key, outBuf, &outLen);
    if (ret != 0) {
        printf("Failed to create ED25519 public key.\nRET: %d\n", ret);
        return ret;
    }
    return ret;
#else
    return NOT_COMPILED_IN;
#endif
}

int wolfCLU_verify_signature(char* sig, char* hash,
                             char* out, char* keyPath, int keyType, int pubIn) {

    int hSz;
    int fSz;
    int ret;

    FILE* h;
    byte* h_mssg;
    FILE* f = fopen(sig,"rb");

    if (f == NULL) {
        printf("unable to open file %s\n", sig);
        return BAD_FUNC_ARG;
    }

    fseek(f, 0, SEEK_END);
    fSz = ftell(f);

    byte data[fSz];
    fseek(f, 0, SEEK_SET);
    fread(data, 1, fSz, f);
    fclose(f);

    switch(keyType) {
        case RSA_SIG_VER:
            ret = wolfCLU_verify_signature_rsa(data, out, fSz, keyPath, pubIn);
            break;

        case ECC_SIG_VER:
            h = fopen(hash,"rb");
            if (h == NULL) {
                printf("unable to open file %s\n", hash);
                ret = BAD_FUNC_ARG;
                break;
            }

            fseek(h, 0, SEEK_END);
            hSz = ftell(h);

            h_mssg = malloc(hSz);

            fseek(h, 0, SEEK_SET);
            fread(h_mssg, 1, hSz, h);
            fclose(h);
            ret = wolfCLU_verify_signature_ecc(data, fSz, h_mssg, hSz, keyPath,
                                               pubIn);
            break;

        case ED25519_SIG_VER:
            h = fopen(hash,"rb");
            if (h == NULL) {
                printf("unable to open file %s\n", hash);
                ret = BAD_FUNC_ARG;
                break;
            }

            fseek(h, 0, SEEK_END);
            hSz = ftell(h);

            h_mssg = malloc(hSz);

            fseek(h, 0, SEEK_SET);
            fread(h_mssg, 1, hSz, h);
            fclose(h);
            ret = wolfCLU_verify_signature_ed25519(data, fSz, h_mssg, hSz,
                                                   keyPath, pubIn);
            break;

        default:
            printf("No valid verify algorithm selected.\n");
            ret = -1;
    }
    return ret;
}

int wolfCLU_verify_signature_rsa(byte* sig, char* out, int sigSz, char* keyPath,
                                 int pubIn) {

#ifndef NO_RSA
    int ret;
    int keyFileSz = 0;
    word32 index = 0;
    FILE* keyPathFile;
    RsaKey key;
    WC_RNG rng;
    byte* keyBuf = NULL;

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
        if (keyPathFile == NULL) {
            printf("unable to open file %s\n", keyPath);
            return BAD_FUNC_ARG;
        }

        fseek(keyPathFile, 0, SEEK_END);
        keyFileSz = ftell(keyPathFile);
        keyBuf = (byte*)malloc(keyFileSz*sizeof(keyBuf));
        if (keyBuf != NULL) {
            fseek(keyPathFile, 0, SEEK_SET);
            fread(keyBuf, 1, keyFileSz, keyPathFile);
        }
        fclose(keyPathFile);
    } else {
        keyBuf = wolfCLU_generate_public_key_rsa(keyPath, keyBuf, &keyFileSz);
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
        if (s == NULL) {
            printf("unable to open file %s\n", out);
            ret = BAD_FUNC_ARG;
        }
        else {
            fwrite(outBuf, 1, sizeof(outBuf), s);
            fclose(s);
        }
    }

    return ret;
#else
    return NOT_COMPILED_IN;
#endif
}

int wolfCLU_verify_signature_ecc(byte* sig, int sigSz, byte* hash, int hashSz,
                                 char* keyPath, int pubIn) {

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
    if (keyPathFile == NULL) {
        printf("unable to open file %s\n", keyPath);
        return BAD_FUNC_ARG;
    }

    fseek(keyPathFile, 0, SEEK_END);
    keyFileSz = ftell(keyPathFile);
    keyBuf = malloc(keyFileSz);
    if (keyBuf != NULL) {
        fseek(keyPathFile, 0, SEEK_SET);
        fread(keyBuf, 1, keyFileSz, keyPathFile);
    }
    fclose(keyPathFile);

    if (pubIn == 1) {
        /* retrieving public key and storing in the ecc key */
        ret = wc_EccPublicKeyDecode(keyBuf, &index, &key, keyFileSz);
        if (ret < 0 ) {
            printf("Failed to decode public key.\nRET: %d\n", ret);
            return ret;
        }
    } else {
        /* retrieving private key and storing in the Ecc Key */
        ret = wc_EccPrivateKeyDecode(keyBuf, &index, &key, keyFileSz);
        if (ret != 0 ) {
            printf("Failed to decode private key.\nRET: %d\n", ret);
            return ret;
        }
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

int wolfCLU_verify_signature_ed25519(byte* sig, int sigSz,
                              byte* hash, int hashSz, char* keyPath, int pubIn) {

#ifdef HAVE_ED25519
    int ret;
    int keyFileSz;
    int stat = 0;

    FILE* keyPathFile;
    ed25519_key key;
    byte* keyBuf = (byte*)malloc(ED25519_KEY_SIZE);
    if (keyBuf == NULL) {
        printf("malloc failed\n");
        return MEMORY_E;
    }

    XMEMSET(&key, 0, sizeof(key));
    XMEMSET(keyBuf, 0, ED25519_KEY_SIZE);

    ret = wc_ed25519_init(&key);
    if (ret != 0) {
        printf("Failed to initialize ED25519 key.\nRet: %d", ret);
        free(keyBuf);
        return ret;
    }

    /* retrieving public key and storing in the ED25519 key */
    if (pubIn == 1) {

    /* read in and store ED25519 key */
        keyPathFile = fopen(keyPath, "rb");
        fread(keyBuf, 1, ED25519_KEY_SIZE, keyPathFile);
        fclose(keyPathFile);

    } else {

        ret = wolfCLU_generate_public_key_ed25519(keyPath, keyBuf);
        if (ret != 0) {
            printf("Failed to derive public key from private key.\n");
            free(keyBuf);
            return ret;
        }
    }

    ret = wc_ed25519_import_public(keyBuf, ED25519_KEY_SIZE, &key);
    if (ret != 0 ) {
        printf("Failed to decode public key.\nRET: %d\n", ret);
        free(keyBuf);
        return ret;
    }
    free(keyBuf);

    ret = wc_ed25519_verify_msg(sig, sigSz, hash, hashSz, &stat, &key);
    if (ret != 0) {
        printf("Failed to verify data with ED25519 public key.\nRET: %d\n", ret);
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
