/* signedData-cryptocb.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
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

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/pkcs7.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/cryptocb.h>
#ifdef USE_PSA
#include <wolfssl/wolfcrypt/port/psa/psa.h>
#include <psa/crypto.h>
#endif

/* Default certificates and keys */
#define RSA_CERT_FILE   "../certs/client-cert.der"
#define RSA_KEY_FILE    "../certs/client-key.der"
#define RSA_KEYPUB_FILE "../certs/client-keyPub.der"

#define ECC_CERT_FILE   "../certs/client-ecc-cert.der"
#define ECC_KEY_FILE    "../certs/ecc-client-key.der"
#define ECC_KEYPUB_FILE "../certs/ecc-client-keyPub.der"

/* Default output file name */
#define OUTPUT_FILE     "signedData_cryptocb.der"

/* Maximum temporary buffer size */
#define LARGE_TEMP_SZ 2048


#if defined(HAVE_PKCS7) && defined(WOLF_CRYPTO_CB)

/* crypto callback context */
typedef struct {
    const char* keyFilePriv;
#ifdef USE_PSA
    psa_key_id_t key_id;
#endif
} myCryptoCbCtx;

/* test data to sign (could be CSR for example) */
static const byte data[] = { /* Hello World */
    0x48,0x65,0x6c,0x6c,0x6f,0x20,0x57,0x6f,0x72,0x6c,0x64
};


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

static int write_file(const char* fileName, byte* in, word32 inSz)
{
    int ret;
    FILE* file;

    file = fopen(fileName, "wb");
    if (file == NULL) {
        printf("ERROR: opening file for writing: %s\n", fileName);
        return -1;
    }

    ret = (int)fwrite(in, 1, inSz, file);
    if (ret == 0) {
        printf("ERROR: writing buffer to output file\n");
        return -1;
    }
    fclose(file);

    return 0;
}

#ifdef USE_PSA
static int psa_map_hash_alg(int hash_len)
{
    switch (hash_len) {
        case 20:
            return PSA_ALG_SHA_1;
        case 28:
            return PSA_ALG_SHA_224;
        case 32:
            return PSA_ALG_SHA_256;
        case 48:
            return PSA_ALG_SHA_384;
        case 64:
            return PSA_ALG_SHA_512;
        default:
            return PSA_ALG_NONE;
    }
}

/* import private key helper */
static int import_ecc_key(byte* keyBuf, word32 keySz, psa_key_id_t* id)
{
    int ret;
    psa_key_attributes_t key_attr = { 0 };
    psa_key_type_t key_type;
    psa_key_id_t key_id;
    psa_status_t status;
    ecc_key eccPriv;
    byte   d[MAX_ECC_BYTES]; /* private d */
    word32 dSz = (word32)sizeof(d);

    memset(d, 0, sizeof(d));

    ret = wc_ecc_init_ex(&eccPriv, NULL, INVALID_DEVID);
    if (ret == 0) {
        word32 idx = 0;
        ret = wc_EccPrivateKeyDecode(keyBuf, &idx, &eccPriv, keySz);
        if (ret == 0) {
            ret = wc_ecc_export_private_only(&eccPriv, d, &dSz);
        }
        wc_ecc_free(&eccPriv);
    }
    if (ret == 0) {
        psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_SIGN_HASH);
        psa_set_key_lifetime(&key_attr, PSA_KEY_LIFETIME_VOLATILE);
        psa_set_key_algorithm(&key_attr, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
        key_type = PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1);
        psa_set_key_type(&key_attr, key_type);
        psa_set_key_bits(&key_attr, (dSz * 8));

        status = psa_import_key(&key_attr, d, dSz, &key_id);
        if (status != PSA_SUCCESS) {
            fprintf(stderr,
                "ERROR: provisioning of private key failed: [%d] \n", status);
            ret = WC_HW_E;
        }
        else {
            ret = 0;
            *id = key_id;
        }
    }
    return ret;
}
#endif

/* Example crypto dev callback function that calls software versions, could
 * be set up to call down to hardware module for crypto operations if
 * desired by user. If an algorithm is not supported by hardware, or user
 * callback, the crypto callback can return CRYPTOCB_UNAVAILABLE to default
 * back to using software crypto implementation. */
static int myCryptoCb(int devIdArg, wc_CryptoInfo* info, void* ctx)
{
    int ret = CRYPTOCB_UNAVAILABLE; /* return this to bypass HW and use SW */
    myCryptoCbCtx* myCtx = (myCryptoCbCtx*)ctx;

    if (info == NULL)
        return BAD_FUNC_ARG;

#ifdef DEBUG_CRYPTOCB
    wc_CryptoCb_InfoString(info);
#endif

    if (info->algo_type == WC_ALGO_TYPE_PK) {
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
                    byte*  der = NULL;
                    size_t derSz = 0;
                    word32 idx = 0;

                    ret = load_file(myCtx->keyFilePriv, &der, &derSz);
                    if (ret != 0) {
                        printf("Error %d loading %s\n", ret, myCtx->keyFilePriv);
                        return ret;
                    }

                    ret = wc_InitRsaKey_ex(&rsaPriv, NULL, INVALID_DEVID);
                    if (ret != 0) {
                        return ret;
                    }
                    ret = wc_RsaPrivateKeyDecode(der, &idx, &rsaPriv, derSz);
                    if (ret == 0) {
                        /* perform software based RSA private op */
                        ret = wc_RsaFunction(
                            info->pk.rsa.in, info->pk.rsa.inLen,
                            info->pk.rsa.out, info->pk.rsa.outLen,
                            info->pk.rsa.type, &rsaPriv, info->pk.rsa.rng);
                    }
                    wc_FreeRsaKey(&rsaPriv);
                    if (der != NULL)
                        free(der);
                    break;
                }
            }
        }
    #endif /* !NO_RSA */
    #ifdef HAVE_ECC
        if (info->pk.type == WC_PK_TYPE_ECDSA_SIGN) {
        #ifdef USE_PSA
            uint8_t rs[MAX_ECC_BYTES * 2];
            size_t  rs_length;
            psa_status_t status;
            psa_algorithm_t hash_algo;

            /* get the desired key ID into your callback context */
            psa_key_id_t id = myCtx->key_id;

            /* Get correct hash algorithm that matches input hash length */
            hash_algo = psa_map_hash_alg(info->pk.eccsign.inlen);

            status = psa_sign_hash(
                id,
                PSA_ALG_ECDSA(hash_algo),
                info->pk.eccsign.in, info->pk.eccsign.inlen,
                rs, sizeof(rs),
                &rs_length);
            if (status == PSA_SUCCESS) {
                word32 point_len = (word32)(rs_length / 2);
                ret = wc_ecc_rs_raw_to_sig(
                    rs, point_len,
                    rs + point_len, point_len,
                    info->pk.eccsign.out, info->pk.eccsign.outlen);
            }
            else {
                ret = WC_HW_E;
            }
        #else
            ecc_key eccPriv;
            byte*  der = NULL;
            size_t derSz = 0;
            word32 idx = 0;

            ret = load_file(myCtx->keyFilePriv, &der, &derSz);
            if (ret != 0) {
                printf("Error %d loading %s\n", ret, myCtx->keyFilePriv);
                return ret;
            }

            ret = wc_ecc_init_ex(&eccPriv, NULL, INVALID_DEVID);
            if (ret != 0) {
                return ret;
            }
            ret = wc_EccPrivateKeyDecode(der, &idx, &eccPriv, derSz);
            if (ret == 0) {
                ret = wc_ecc_sign_hash(
                    info->pk.eccsign.in, info->pk.eccsign.inlen,
                    info->pk.eccsign.out, info->pk.eccsign.outlen,
                    info->pk.eccsign.rng, &eccPriv);
            }
            wc_ecc_free(&eccPriv);
            if (der != NULL)
                free(der);
        #endif
        }
        else if (info->pk.type == WC_PK_TYPE_ECDSA_VERIFY) {
            /* set devId to invalid, so software is used */
            info->pk.eccverify.key->devId = INVALID_DEVID;

            ret = wc_ecc_verify_hash(
                info->pk.eccverify.sig, info->pk.eccverify.siglen,
                info->pk.eccverify.hash, info->pk.eccverify.hashlen,
                info->pk.eccverify.res, info->pk.eccverify.key);

            /* reset devId */
            info->pk.eccverify.key->devId = devIdArg;
        }
    #endif /* HAVE_ECC */
    }

    (void)devIdArg;
    (void)myCtx;

    return ret;
}


static int signedData_sign(byte* cert, word32 certSz, byte* key,
                           word32 keySz, byte* out, word32 outSz,
                           int devId, int algOid, int hashOid)
{
    int ret;
    PKCS7* pkcs7;
    WC_RNG rng;

    /* init rng */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("ERROR: wc_InitRng() failed, ret = %d\n", ret);
        return -1;
    }

    /* init PKCS7 */
    pkcs7 = wc_PKCS7_New(NULL, devId);
    if (pkcs7 == NULL) {
        wc_FreeRng(&rng);
        return -1;
    }

    ret = wc_PKCS7_InitWithCert(pkcs7, cert, certSz);
    if (ret != 0) {
        printf("ERROR: wc_PKCS7_InitWithCert() failed, ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        wc_FreeRng(&rng);
        return -1;
    }

    pkcs7->rng             = &rng;
    pkcs7->content         = (byte*)data;
    pkcs7->contentSz       = sizeof(data);
    pkcs7->contentOID      = DATA;
    pkcs7->hashOID         = hashOid;
    pkcs7->encryptOID      = algOid;
    pkcs7->privateKey      = key;
    pkcs7->privateKeySz    = keySz;

    /* encode signedData, returns size */
    ret = wc_PKCS7_EncodeSignedData(pkcs7, out, outSz);
    if (ret <= 0) {
        printf("ERROR: wc_PKCS7_EncodeSignedData() failed, ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        wc_FreeRng(&rng);
        return -1;

    }
    else {
        printf("Successfully encoded SignedData bundle\n");
#ifdef DEBUG_WOLFSSL
        printf("Encoded DER (%d bytes):\n", ret);
        WOLFSSL_BUFFER(out, ret);
#endif
    }

    wc_PKCS7_Free(pkcs7);
    wc_FreeRng(&rng);

    return ret;
}

static int signedData_verify(byte* in, word32 inSz, byte* cert,
                             word32 certSz, byte* key, word32 keySz,
                             byte* out, word32 outSz, int devId)
{
    int ret;
    PKCS7* pkcs7;

    pkcs7 = wc_PKCS7_New(NULL, devId);
    if (pkcs7 == NULL)
        return -1;

    /* decode signedData, returns size */
    ret = wc_PKCS7_VerifySignedData(pkcs7, in, inSz);

    if (ret < 0 || (pkcs7->contentSz != sizeof(data)) ||
        (XMEMCMP(pkcs7->content, data, pkcs7->contentSz) != 0)) {
        printf("ERROR: Failed to verify SignedData bundle, ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        return -1;

    } else {
        printf("Successfully verified SignedData bundle.\n");

#ifdef DEBUG_WOLFSSL
        printf("Decoded content (%d bytes):\n", pkcs7->contentSz);
        WOLFSSL_BUFFER(pkcs7->content, pkcs7->contentSz);
#endif
    }

    wc_PKCS7_Free(pkcs7);

    return ret;
}


static void usage(void)
{
    printf("Expected usage:\n");
    printf("./signData-cryptocb [-ecc/-rsa] [-out=]\n");
    printf("* -ecc/-rsa: Use ECC or RSA key (default is ECC)\n");
    printf("* -key=file: DER formatted private key\n");
    printf("* -keyPub=file: DER formatted public key\n");
    printf("* -cert=file: Certificate for signing key\n");
    printf("* -out=file: Generated PKCS7 file containing signed data and certificate (default %s)\n", OUTPUT_FILE);
}

int main(int argc, char** argv)
{
    int ret, devId;
    int encryptedSz, decryptedSz;
    size_t certSz, keySz;
    myCryptoCbCtx myCtx;
    const char* certFile = NULL;
    const char* keyFile = NULL;
    const char* keyFilePub = NULL;
    const char* outFile = OUTPUT_FILE;
    int algOid = ECDSAk;
    int hashOid = SHA256h;
    byte* cert = NULL;
    byte* key = NULL;

    byte encrypted[LARGE_TEMP_SZ];
    byte decrypted[LARGE_TEMP_SZ];

    if (argc >= 2) {
        if (XSTRCMP(argv[1], "-?") == 0 ||
            XSTRCMP(argv[1], "-h") == 0 ||
            XSTRCMP(argv[1], "--help") == 0) {
            usage();
            return 0;
        }
    }
    while (argc > 1) {
        if (XSTRCMP(argv[argc-1], "-ecc") == 0) {
            algOid = ECDSAk;
        }
        else if (XSTRCMP(argv[argc-1], "-rsa") == 0) {
            algOid = RSAk;
        }
        else if (XSTRNCMP(argv[argc-1], "-cert=",
                XSTRLEN("-cert=")) == 0) {
            certFile = argv[argc-1] + XSTRLEN("-cert=");
        }
        else if (XSTRNCMP(argv[argc-1], "-key=",
                XSTRLEN("-key=")) == 0) {
            keyFile = argv[argc-1] + XSTRLEN("-key=");
        }
        else if (XSTRNCMP(argv[argc-1], "-keyPub=",
                XSTRLEN("-keyPub=")) == 0) {
            keyFilePub = argv[argc-1] + XSTRLEN("-keyPub=");
        }
        else if (XSTRNCMP(argv[argc-1], "-out=",
                XSTRLEN("-out=")) == 0) {
            outFile = argv[argc-1] + XSTRLEN("-out=");
        }
        else {
            printf("Warning: Unrecognized option: %s\n", argv[argc-1]);
        }
        argc--;
    }

    if (certFile == NULL) {
        if (algOid == RSAk) {
            certFile = RSA_CERT_FILE;
            keyFile = RSA_KEY_FILE;
            keyFilePub = RSA_KEYPUB_FILE;
        }
        else {
            certFile = ECC_CERT_FILE;
            keyFile = ECC_KEY_FILE;
            keyFilePub = ECC_KEYPUB_FILE;
        }
    }

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    ret = wolfCrypt_Init();
    if (ret != 0) {
        printf("wolfCrypt initialization failed\n");
        return -1;
    }

    /* provide private key to crypto callback */
    myCtx.keyFilePriv = keyFile;
#ifdef USE_PSA
    ret = load_file(keyFile, &key, &keySz);
    if (ret == 0) {
        ret = import_ecc_key(key, keySz, &myCtx.key_id);
    }
    free(key); key = NULL;
    if (ret != 0)
        goto exit;
#endif

    /* Load certificate and public key */
    ret = load_file(certFile, &cert, &certSz);
    if (ret == 0)
        ret = load_file(keyFilePub, &key, &keySz);
    if (ret != 0)
        goto exit;

    /* setting devId to something other than INVALID_DEVID, enables
     * crypto callback to be used internally by wolfCrypt */
    devId = 1;
    ret = wc_CryptoCb_RegisterDevice(devId, myCryptoCb, &myCtx);
    if (ret != 0) {
        printf("Failed to register crypto dev device, ret = %d\n", ret);
        goto exit;
    }

    /* Sign bundle */
    encryptedSz = signedData_sign(cert, certSz, key, keySz,
        encrypted, sizeof(encrypted), devId, algOid, hashOid);
    if (encryptedSz >= 0) {
        if (write_file(outFile, encrypted, encryptedSz) != 0) {
            printf("ERROR: error writing encoded to output file %s\n", outFile);
            ret =-1;
        }
        else {
            printf("Wrote encoded PKCS7 bundle to file %s\n", outFile);
            ret = 0;
        }
    }
    else {
        ret = -1; goto exit;
    }

    /* Verify bundle */
    decryptedSz = signedData_verify(encrypted, encryptedSz,
                                    cert, certSz, key, keySz,
                                    decrypted, sizeof(decrypted), devId);
    if (decryptedSz < 0) {
        ret = -1; goto exit;
    }

exit:
    if (cert != NULL) free(cert);
    if (key != NULL) free(key);
    wolfCrypt_Cleanup();

    return ret;
}

#else

int main(int argc, char** argv)
{
    printf("Must build wolfSSL using ./configure --enable-pkcs7 --enable-cryptocb\n");
    return 0;
}

#endif /* HAVE_PKCS7 & WOLF_CRYPTO_DEV */

