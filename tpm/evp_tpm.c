/* evp_tpm.c
 *
 * Copyright (C) 2006-2021 wolfSSL Inc.
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

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
    #include <wolftpm/options.h>
#endif

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/cryptocb.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

#include <wolfssl/openssl/ssl.h>
#include <wolfssl/openssl/evp.h>

#include <wolftpm/tpm2_wrap.h>
#include <wolftpm/tpm2_tis.h>


/* Storage Key Parameters */
#define TPM2_DEMO_STORAGE_EC_KEY_HANDLE 0x81000201  /* Persistent Storage Key Handle (ECC) */
static const char gStorageKeyAuth[] = "ThisIsMyStorageKeyAuth";
static const char gKeyAuth[] =        "ThisIsMyKeyAuth";

typedef struct TpmCryptoCbCtx {
    WOLFTPM2_DEV* dev;
    WOLFTPM2_KEY* eccKey;
} TpmCryptoCbCtx;

static int CreateLoadExportTpmKey(WOLFTPM2_DEV* dev, WOLFTPM2_KEY* tpmKey, 
    WOLFTPM2_KEY* storageKey, byte* pubKeyDer, word32* pubKeyDerSz)
{
    int ret;
    TPMT_PUBLIC publicTemplate;
    ecc_key* wolfEccKey;

    wolfTPM2_GetKeyTemplate_ECC(&publicTemplate,
        TPMA_OBJECT_sensitiveDataOrigin | TPMA_OBJECT_userWithAuth |
        TPMA_OBJECT_sign | TPMA_OBJECT_noDA,
        TPM_ECC_NIST_P256, TPM_ALG_ECDSA);
    ret = wolfTPM2_CreateAndLoadKey(dev, tpmKey, &storageKey->handle,
        &publicTemplate, (byte*)gKeyAuth, sizeof(gKeyAuth)-1);
    if (ret == 0) {
        wolfEccKey = wc_ecc_key_new(NULL);
        if (wolfEccKey != NULL) {
            /* extract public key der */
            /* load public portion of key into wolf ECC Key */
            ret = wolfTPM2_EccKey_TpmToWolf(dev, tpmKey, wolfEccKey);
            if (ret == 0) {
                ret = wc_EccPublicKeyToDer(wolfEccKey,
                    pubKeyDer, *pubKeyDerSz, 1);
                if (ret >= 0) {
                    *pubKeyDerSz = ret;
                    ret = 0;
                }
            }
            wc_ecc_key_free(wolfEccKey);
        }
    }

    if (ret == 0) {
        printf("Create/Load ECC Key: Handle 0x%x (%d bytes)\n",
            (word32)tpmKey->handle.hndl, tpmKey->pub.size);
    }
    else {
        printf("Create/Load ECC Key: failed 0x%x: %s\n", ret,
            TPM2_GetRCString(ret));
    }
    return ret;
}

static int LoadPrimaryStoragekey(WOLFTPM2_DEV* pDev, WOLFTPM2_KEY* pStorageKey,
    TPM_ALG_ID alg)
{
    int ret;

    /* See if SRK already exists */
    ret = wolfTPM2_ReadPublicKey(pDev, pStorageKey,
        TPM2_DEMO_STORAGE_EC_KEY_HANDLE);
    if (ret != 0) {
        /* Create primary storage key */
        ret = wolfTPM2_CreateSRK(pDev, pStorageKey, alg,
            (byte*)gStorageKeyAuth, sizeof(gStorageKeyAuth)-1);
        if (ret == TPM_RC_SUCCESS) {
            /* Move storage key into persistent NV */
            ret = wolfTPM2_NVStoreKey(pDev, TPM_RH_OWNER, pStorageKey,
                TPM2_DEMO_STORAGE_EC_KEY_HANDLE);
        }
    }
    else {
        /* specify auth password for storage key */
        pStorageKey->handle.auth.size = sizeof(gStorageKeyAuth)-1;
        XMEMCPY(pStorageKey->handle.auth.buffer, gStorageKeyAuth,
                pStorageKey->handle.auth.size);
    }

    if (ret == 0) {
        printf("Loading SRK: Storage 0x%x (%d bytes)\n",
            (word32)pStorageKey->handle.hndl, pStorageKey->pub.size);
    }
    else {
        printf("Loading SRK: Storage failed 0x%x: %s\n", ret,
            TPM2_GetRCString(ret));
    }
    return ret;
}


int CryptoCbFunc(int devId, wc_CryptoInfo* info, void* ctx)
{
    int ret = CRYPTOCB_UNAVAILABLE;
    TpmCryptoCbCtx* tlsCtx = (TpmCryptoCbCtx*)ctx;

    if (info == NULL || ctx == NULL || tlsCtx->dev == NULL) {
        return BAD_FUNC_ARG;
    }

    (void)devId;

    if (info->algo_type == WC_ALGO_TYPE_PK) {
        printf("CryptoCbFunc Pk: Type %d\n", info->pk.type);

        if (info->pk.type == WC_PK_TYPE_ECDSA_SIGN) {
            byte sigRS[MAX_ECC_BYTES*2];
            byte *r = sigRS, *s;
            word32 rsLen = sizeof(sigRS), rLen, sLen;
            word32 inlen = info->pk.eccsign.inlen;

            /* truncate input to match key size */
            rLen = wc_ecc_size(info->pk.eccsign.key);
            if (inlen > rLen)
                inlen = rLen;

            ret = wolfTPM2_SignHash(tlsCtx->dev, tlsCtx->eccKey,
                info->pk.eccsign.in, inlen, sigRS, (int*)&rsLen);
            if (ret == 0) {
                /* Encode ECDSA Header */
                rLen = sLen = rsLen / 2;
                s = &sigRS[rLen];
                ret = wc_ecc_rs_raw_to_sig(r, rLen, s, sLen,
                    info->pk.eccsign.out, info->pk.eccsign.outlen);
            }
        }
    }

    /* need to return negative here for error */
    if (ret != TPM_RC_SUCCESS && ret != CRYPTOCB_UNAVAILABLE) {
        printf("CryptoCbFunc failed ret = %d\n", ret);
        ret = WC_HW_E;
    }

    return ret;
}

static int test_EVP_MD_ecc_signing(int devId, const byte* pubKeyBuf, word32 pubKeySz)
{
    int ret = 0;
#if defined(OPENSSL_EXTRA) && defined(HAVE_ECC)
    EVP_MD_CTX mdCtx;
    EVP_PKEY* privKey;
    EVP_PKEY* pubKey = NULL;
    const char   testData[] = "Hi There";
    unsigned int testDataSz = (unsigned int)XSTRLEN(testData);
    unsigned char check[ECC_MAX_SIG_SIZE];
    size_t        checkSz = ECC_MAX_SIG_SIZE;
    const unsigned char *p;

    printf("EVP_MD_ecc_signing()\n");

    privKey = wolfSSL_d2i_PrivateKey_id(EVP_PKEY_EC, NULL, NULL, devId);
    if (privKey == NULL) {
        ret = MEMORY_E;
    }
    if (ret == 0) {
        p = pubKeyBuf;
        pubKey = d2i_PUBKEY(NULL, &p, pubKeySz);
        if (pubKey == NULL) {
            ret = MEMORY_E;
        }
    }

    if (ret == 0) {
        EVP_MD_CTX_init(&mdCtx);
        ret = EVP_DigestSignInit(&mdCtx, NULL, EVP_sha256(), NULL, privKey);
        if (ret == WOLFSSL_SUCCESS)
            ret = EVP_DigestSignUpdate(&mdCtx, testData, testDataSz);
        if (ret == WOLFSSL_SUCCESS)
            ret = EVP_DigestSignFinal(&mdCtx, check, &checkSz);
        EVP_MD_CTX_cleanup(&mdCtx);

        if (ret == WOLFSSL_SUCCESS) {
            printf("EVP_DigestSignFinal sz %d\n", (int)checkSz);
            ret = 0;
        }
        else {
            ret = -1; /* failure */
        }
    }

    if (ret == 0) {
        EVP_MD_CTX_init(&mdCtx);
        ret = EVP_DigestVerifyInit(&mdCtx, NULL, EVP_sha256(), NULL, pubKey);
        if (ret == WOLFSSL_SUCCESS)
            ret = EVP_DigestVerifyUpdate(&mdCtx, testData, testDataSz);
        if (ret == WOLFSSL_SUCCESS)
            ret = EVP_DigestVerifyFinal(&mdCtx, check, checkSz);
        EVP_MD_CTX_cleanup(&mdCtx);
        
        if (ret == WOLFSSL_SUCCESS) {
            printf("EVP_DigestVerifyFinal success\n");
            ret = 0;
        }
        else {
            ret = -1; /* failure */
        }
    }

    EVP_PKEY_free(pubKey);
    EVP_PKEY_free(privKey);    
#else
    ret = NOT_COMPILED_IN;
#endif
    return ret;
}


int main(void)
{
    int ret;
    WOLFTPM2_DEV dev;
    WOLFTPM2_KEY tpmKey;
    WOLFTPM2_KEY storageKey;
    TpmCryptoCbCtx tpmCtx;
    int devId = INVALID_DEVID;
    byte pubKeyDer[1024];
    word32 pubKeyDerSz = 1024;

    XMEMSET(&tpmCtx, 0, sizeof(tpmCtx));
    XMEMSET(&tpmKey, 0, sizeof(tpmKey));
    XMEMSET(&storageKey, 0, sizeof(storageKey));
    XMEMSET(pubKeyDer, 0, sizeof(pubKeyDer));
    tpmCtx.eccKey = &tpmKey;

    ret = wolfTPM2_Init(&dev, NULL, NULL);
    if (ret == 0) {
        wolfSSL_Debugging_OFF();

        ret = wolfTPM2_GetTpmDevId(&dev);
        if (ret >= 0) {
            devId = ret;
            tpmCtx.dev = &dev;

            ret = wc_CryptoDev_RegisterDevice(devId, CryptoCbFunc, &tpmCtx);
        }
    }
    if (ret == 0) {
        ret = LoadPrimaryStoragekey(&dev, &storageKey, TPM_ALG_ECC);
    }
    if (ret == 0) {
        ret = CreateLoadExportTpmKey(&dev, &tpmKey, &storageKey,
            pubKeyDer, &pubKeyDerSz);
    }
    if (ret == 0) {
        ret = test_EVP_MD_ecc_signing(devId, pubKeyDer, pubKeyDerSz);
    }

    printf("Result: %s (%d)\n", ret == 0 ? "success" :
        wolfSSL_ERR_reason_error_string(ret), ret);

    wolfTPM2_UnloadHandle(&dev, &tpmKey.handle);
    wolfTPM2_Cleanup(&dev);

    return ret;
}
