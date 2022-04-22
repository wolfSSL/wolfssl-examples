/* csr_cryptocb.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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

/* Example of generating a PEM-encoded certificate signing request (CSR) using
 * the Crypto Callbacks to show signing against HSM/TPM */

#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/cryptocb.h>

#define LARGE_TEMP_SZ 4096

#if defined(WOLF_CRYPTO_CB) && defined(WOLFSSL_CERT_REQ) && \
    defined(WOLFSSL_CERT_EXT) && defined(WOLFSSL_CERT_GEN) && \
    (!defined(NO_RSA) || defined(HAVE_ECC) || defined(HAVE_ED25519))

#define ENABLE_CSR_EXAMPLE
#endif

/* Private and public key files for signing */
#define ECC_KEY_FILE        "../certs/ecc-key.pem"
#define ECC_KEYPUB_FILE     "../certs/ecc-keyPub.pem"
#define RSA_KEY_FILE        "../certs/client-key.pem"
#define RSA_KEYPUB_FILE     "../certs/client-keyPub.pem"
#define ED25519_KEY_FILE    "../certs/ed25519-keyPriv.pem"
#define ED25519_KEYPUB_FILE "../certs/ed25519-keyPub.pem"


#ifdef ENABLE_CSR_EXAMPLE
static void usage(void)
{
    printf("Invalid input supplied try one of the below examples\n");
    printf("Examples:\n\n");
    printf("./csr_cryptocb rsa\n");
    printf("./csr_cryptocb ecc\n");
    printf("./csr_cryptocb ed25519\n");
}

/* Example custom context for crypto callback */
typedef struct {
    const char* keyFilePub;
    const char* keyFilePriv;
} myCryptoCbCtx;

/* Forward declarations */
static int load_key_file(const char* fname, byte* derBuf, word32* derLen,
    int isPubKey);

/* Example crypto dev callback function that calls software version */
/* This is where you would plug-in calls to your own hardware crypto */
static int myCryptoCb(int devIdArg, wc_CryptoInfo* info, void* ctx)
{
    int ret = CRYPTOCB_UNAVAILABLE; /* return this to bypass HW and use SW */
    myCryptoCbCtx* myCtx = (myCryptoCbCtx*)ctx;
    word32 idx = 0;

    if (info == NULL)
        return BAD_FUNC_ARG;

#ifdef DEBUG_CRYPTOCB
    wc_CryptoCb_InfoString(info);
#endif

    if (info->algo_type == WC_ALGO_TYPE_PK) {
        byte   der[LARGE_TEMP_SZ];
        word32 derSz;

        ret = load_key_file(myCtx->keyFilePriv, der, &derSz, 0);
        if (ret != 0) {
            printf("Error %d loading %s\n", ret, myCtx->keyFilePriv);
            return ret;
        }

    #ifndef NO_RSA
        if (info->pk.type == WC_PK_TYPE_RSA) {
            switch (info->pk.rsa.type) {
                case RSA_PUBLIC_ENCRYPT:
                case RSA_PUBLIC_DECRYPT:
                    /* set devId to invalid, so software is used */
                    info->pk.rsa.key->devId = INVALID_DEVID;
                    /* perform software based RSA public op */
                    ret = wc_RsaFunction(
                        info->pk.rsa.in, info->pk.rsa.inLen,
                        info->pk.rsa.out, info->pk.rsa.outLen,
                        info->pk.rsa.type, info->pk.rsa.key, info->pk.rsa.rng);
                    info->pk.rsa.key->devId = devIdArg; /* reset devId */
                    break;
                case RSA_PRIVATE_ENCRYPT:
                case RSA_PRIVATE_DECRYPT:
                {
                    RsaKey rsaPriv;

                    ret = wc_InitRsaKey_ex(&rsaPriv, NULL, INVALID_DEVID);
                    if (ret != 0) {
                        return ret;
                    }
                    ret = wc_RsaPrivateKeyDecode(der, &idx, &rsaPriv, derSz);
                    if (ret != 0) {
                        wc_FreeRsaKey(&rsaPriv);
                        return ret;
                    }
                
                    /* perform software based RSA private op */
                    ret = wc_RsaFunction(
                        info->pk.rsa.in, info->pk.rsa.inLen,
                        info->pk.rsa.out, info->pk.rsa.outLen,
                        info->pk.rsa.type, &rsaPriv, info->pk.rsa.rng);
                    wc_FreeRsaKey(&rsaPriv);
                    break;
                }
            }
        }
    #endif /* !NO_RSA */
    #ifdef HAVE_ECC
        if (info->pk.type == WC_PK_TYPE_ECDSA_SIGN) {
            ecc_key eccPriv;
            ret = wc_ecc_init_ex(&eccPriv, NULL, INVALID_DEVID);
            if (ret == 0) {
                ret = wc_EccPrivateKeyDecode(der, &idx, &eccPriv, derSz);
                if (ret == 0) {
                    ret = wc_ecc_sign_hash(
                        info->pk.eccsign.in, info->pk.eccsign.inlen,
                        info->pk.eccsign.out, info->pk.eccsign.outlen,
                        info->pk.eccsign.rng, &eccPriv);
                }
                wc_ecc_free(&eccPriv);
            }
        }
    #endif /* HAVE_ECC */
    #ifdef HAVE_ED25519
        if (info->pk.type == WC_PK_TYPE_ED25519_SIGN) {
            ed25519_key edPriv;
            ret = wc_ed25519_init_ex(&edPriv, NULL, INVALID_DEVID);
            if (ret == 0) {
                ret = wc_Ed25519PrivateKeyDecode(der, &idx, &edPriv, derSz);
                if (ret == 0) {
                    /* calculate public key */
                    ret = wc_ed25519_make_public(&edPriv, edPriv.p,
                        ED25519_PUB_KEY_SIZE);
                }
                if (ret == 0) {
                    edPriv.pubKeySet = 1;
                    ret = wc_ed25519_sign_msg_ex(
                        info->pk.ed25519sign.in, info->pk.ed25519sign.inLen,
                        info->pk.ed25519sign.out, info->pk.ed25519sign.outLen,
                        &edPriv, info->pk.ed25519sign.type,
                        info->pk.ed25519sign.context,
                        info->pk.ed25519sign.contextLen);
                }
                wc_ed25519_free(&edPriv);
            }
        }
    #endif /* HAVE_ED25519 */
    }
    else {
        ret = CRYPTOCB_UNAVAILABLE; /* return this to bypass HW and use SW */
    }

    (void)devIdArg;
    (void)myCtx;

    return ret;
}

/* reads file size, allocates buffer, reads into buffer, returns buffer */
static int load_file(const char* fname, byte** buf, size_t* bufLen)
{
    int ret;
    long int fileSz;
    XFILE lFile;

    if (fname == NULL || buf == NULL || bufLen == NULL)
        return BAD_FUNC_ARG;

    /* set defaults */
    *buf = NULL;
    *bufLen = 0;

    /* open file (read-only binary) */
    lFile = XFOPEN(fname, "rb");
    if (!lFile) {
        printf("Error loading %s\n", fname);
        return BAD_PATH_ERROR;
    }

    fseek(lFile, 0, SEEK_END);
    fileSz = (int)ftell(lFile);
    rewind(lFile);
    if (fileSz > 0) {
        *bufLen = (size_t)fileSz;
        *buf = (byte*)malloc(*bufLen);
        if (*buf == NULL) {
            ret = MEMORY_E;
            printf("Error allocating %lu bytes\n", (unsigned long)*bufLen);
        }
        else {
            size_t readLen = fread(*buf, *bufLen, 1, lFile);

            /* check response code */
            ret = (readLen > 0) ? 0 : -1;
        }
    }
    else {
        ret = BUFFER_E;
    }
    fclose(lFile);

    return ret;
}

static int load_key_file(const char* fname, byte* derBuf, word32* derLen,
    int isPubKey)
{
    int ret;
    byte* buf = NULL;
    size_t bufLen;

    ret = load_file(fname, &buf, &bufLen);
    if (ret != 0)
        return ret;

    if (isPubKey) {
        ret = wc_PubKeyPemToDer(buf, (word32)bufLen, derBuf, (word32)bufLen);
    } else {
        ret = wc_KeyPemToDer(buf, (word32)bufLen, derBuf, (word32)bufLen, NULL);
    }
    if (ret < 0) {
        free(buf);
        return ret;
    }
    *derLen = ret;
    free(buf);

    return 0;
}

#ifdef WOLFSSL_DER_TO_PEM
static int save_der_as_pem(const byte* der, word32 derSz, const char* arg1,
    int type)
{
    int ret;
    byte pem[LARGE_TEMP_SZ];
    int  pemSz;
    FILE* file = NULL;
    char outFile[255];

    memset(pem, 0, sizeof(pem));
    ret = wc_DerToPem(der, derSz, pem, sizeof(pem), type);
    if (ret <= 0) {
        printf("CSR DER to PEM failed: %d\n", ret);
        return ret;
    }
    pemSz = ret;
    printf("%s (%d)\n", pem, pemSz);

    snprintf(outFile, sizeof(outFile), "%s-csr.pem", arg1);
    printf("Saved CSR PEM to \"%s\"\n", outFile);
    file = fopen(outFile, "wb");
    if (file) {
        ret = (int)fwrite(pem, 1, pemSz, file);
        if (ret == pemSz) {
            ret = 0;
        }
        fclose(file);
    }
    else {
        ret = -1;
    }
    return ret;
}
#endif

static int gen_csr(const char* arg1)
{
    int ret;
    int type;
#ifdef HAVE_ECC
    ecc_key ecKeyPub;
#endif
#ifndef NO_RSA
    RsaKey rsaKeyPub;
#endif
#ifdef HAVE_ED25519
    ed25519_key edKeyPub;
#endif
    void* keyPtr = NULL;
    WC_RNG rng;
    Cert req;
    byte   der[LARGE_TEMP_SZ];
    word32 derSz;
    word32 idx = 0;
    myCryptoCbCtx myCtx;
    int devId = 1; /* any value besides INVALID_DEVID */

    if (XSTRNCMP(arg1, "rsa", 3) == 0)
        type = RSA_TYPE;
    else if (XSTRNCMP(arg1, "ecc", 3) == 0)
        type = ECC_TYPE;
    else if (XSTRNCMP(arg1, "ed25519", 7) == 0)
        type = ED25519_TYPE;
    else {
        printf("Invalid argument or not compiled in (expect: rsa, ecc or ed25519)\n");
        return NOT_COMPILED_IN;
    }

    wolfCrypt_Init();

    /* register a devID for crypto callbacks */
    ret = wc_CryptoCb_RegisterDevice(devId, myCryptoCb, &myCtx);
    if (ret != 0) {
        printf("Crypto callback register failed: %d\n", ret);
        goto exit;
    }

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("RNG initialization failed: %d\n", ret);
        goto exit;
    }

    /* setup test key */
#ifdef HAVE_ECC
    if (type == ECC_TYPE) {
        myCtx.keyFilePub =  ECC_KEYPUB_FILE;
        myCtx.keyFilePriv = ECC_KEY_FILE;
    }
#endif
#ifndef NO_RSA
    if (type == RSA_TYPE) {
        myCtx.keyFilePub =  RSA_KEYPUB_FILE;
        myCtx.keyFilePriv = RSA_KEY_FILE;
    }
#endif
#ifdef HAVE_ED25519
    if (type == ED25519_TYPE) {
        myCtx.keyFilePub =  ED25519_KEYPUB_FILE;
        myCtx.keyFilePriv = ED25519_KEY_FILE;
    }
#endif

    /* convert PEM to DER */
    derSz = sizeof(der);
    ret = load_key_file(myCtx.keyFilePub, der, &derSz, 1);
    if (ret != 0) {
        printf("Error %d loading the public key %s\n", ret, myCtx.keyFilePub);
        goto exit;
    }

    /* setup public key */
#ifdef HAVE_ECC
    if (type == ECC_TYPE) {
        keyPtr = &ecKeyPub;
        ret = wc_ecc_init_ex(&ecKeyPub, NULL, devId);
    }
#endif
#ifndef NO_RSA
    if (type == RSA_TYPE) {
        keyPtr = &rsaKeyPub;
        ret = wc_InitRsaKey_ex(&rsaKeyPub, NULL, devId);
    }
#endif
#ifdef HAVE_ED25519
    if (type == ED25519_TYPE) {
        keyPtr = &edKeyPub;
        ret = wc_ed25519_init_ex(&edKeyPub, NULL, devId);
    }
#endif
    if (ret != 0) {
        printf("Key initialization failed: %d\n", ret);
        goto exit;
    }

    /* decode public key */
#ifdef HAVE_ECC
    if (type == ECC_TYPE) {
        ret = wc_EccPublicKeyDecode(der, &idx, &ecKeyPub, derSz);
    }
#endif
#ifndef NO_RSA
    if (type == RSA_TYPE) {
        ret = wc_RsaPublicKeyDecode(der, &idx, &rsaKeyPub, derSz);
    }
#endif
#ifdef HAVE_ED25519
    if (type == ED25519_TYPE) {
        ret = wc_Ed25519PublicKeyDecode(der, &idx, &edKeyPub, derSz);
    }
#endif
    if (ret != 0) {
        printf("Key decode failed: %d\n", ret);
        goto exit;
    }

    /* setup the CSR data */
    ret = wc_InitCert(&req);
    if (ret != 0) {
        printf("Init Cert failed: %d\n", ret);
        goto exit;
    }
    strncpy(req.subject.country, "US", CTC_NAME_SIZE);
    strncpy(req.subject.state, "OR", CTC_NAME_SIZE);
    strncpy(req.subject.locality, "Portland", CTC_NAME_SIZE);
    strncpy(req.subject.org, "wolfSSL", CTC_NAME_SIZE);
    strncpy(req.subject.unit, "Development", CTC_NAME_SIZE);
    strncpy(req.subject.commonName, "www.wolfssl.com", CTC_NAME_SIZE);
    strncpy(req.subject.email, "info@wolfssl.com", CTC_NAME_SIZE);
    ret = wc_MakeCertReq_ex(&req, der, sizeof(der), type, keyPtr);
    if (ret <= 0) {
        printf("Make Cert Req failed: %d\n", ret);
        goto exit;
    }
    derSz = ret;

#ifdef HAVE_ECC
    if (type == ECC_TYPE)
        req.sigType = CTC_SHA256wECDSA;
#endif
#ifndef NO_RSA
    if (type == RSA_TYPE)
        req.sigType = CTC_SHA256wRSA;
#endif
#ifdef HAVE_ED25519
    if (type == ED25519_TYPE)
        req.sigType = CTC_ED25519;
#endif
    /* Because the key has devId set, it will call myCryptoCb for signing */
    ret = wc_SignCert_ex(req.bodySz, req.sigType, der, sizeof(der), type,
        keyPtr, &rng);
    if (ret <= 0) {
        printf("Sign Cert failed: %d\n", ret);
        goto exit;
    }
    derSz = ret;

#ifdef WOLFSSL_DER_TO_PEM
    ret = save_der_as_pem(der, derSz, arg1, CERTREQ_TYPE);
#endif

    ret = 0; /* success */

exit:
#ifdef HAVE_ECC
    if (type == ECC_TYPE)
        wc_ecc_free(&ecKeyPub);
#endif
#ifndef NO_RSA
    if (type == RSA_TYPE)
        wc_FreeRsaKey(&rsaKeyPub);
#endif
#ifdef HAVE_ED25519
    if (type == ED25519_TYPE)
        wc_ed25519_free(&edKeyPub);
#endif
    wc_FreeRng(&rng);

    wolfCrypt_Cleanup();

    return ret;
}

#endif

int main(int argc, char** argv)
{
#ifdef ENABLE_CSR_EXAMPLE
    if (argc != 2) {
        usage();
        return 1;
    }

    return gen_csr(argv[1]);
#else
    printf("Please compile wolfSSL with `--enable-certreq --enable-certgen "
           "--enable-certext --enable-cryptocb` and RSA, ECC or ED25519\n");
    return 0;
#endif
}
