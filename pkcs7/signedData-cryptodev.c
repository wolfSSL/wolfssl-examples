/* signedData-cryptodev.c
 *
 * Copyright (C) 2006-2018 wolfSSL Inc.
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
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/pkcs7.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/cryptodev.h>

#define certFile "../certs/client-cert.der"
#define keyFile  "../certs/client-key.der"
#define encodedFileNoAttrs "signedData_cryptodev_noattrs.der"
#define encodedFileAttrs   "signedData_cryptodev_attrs.der"

#if defined(HAVE_PKCS7) && defined(WOLF_CRYPTO_DEV)

static const byte data[] = { /* Hello World */
    0x48,0x65,0x6c,0x6c,0x6f,0x20,0x57,0x6f,
    0x72,0x6c,0x64
};

static int load_certs(byte* cert, word32* certSz, byte* key, word32* keySz)
{
    FILE* file;

    /* certificate file */
    file = fopen(certFile, "rb");
    if (!file)
        return -1;

    *certSz = (word32)fread(cert, 1, *certSz, file);
    fclose(file);

    /* key file */
    file = fopen(keyFile, "rb");
    if (!file)
        return -1;

    *keySz = (word32)fread(key, 1, *keySz, file);
    fclose(file);

    return 0;
}

static int write_file_buffer(const char* fileName, byte* in, word32 inSz)
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

static int signedData_sign_noattrs(byte* cert, word32 certSz, byte* key,
                                   word32 keySz, byte* out, word32 outSz,
                                   int devId)
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
    pkcs7->hashOID         = SHA256h;
    pkcs7->encryptOID      = RSAk;
    pkcs7->privateKey      = key;
    pkcs7->privateKeySz    = keySz;
    pkcs7->signedAttribs   = NULL;
    pkcs7->signedAttribsSz = 0;

    /* encode signedData, returns size */
    ret = wc_PKCS7_EncodeSignedData(pkcs7, out, outSz);
    if (ret <= 0) {
        printf("ERROR: wc_PKCS7_EncodeSignedData() failed, ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        wc_FreeRng(&rng);
        return -1;

    } else {
        printf("Successfully encoded SignedData bundle (%s)\n",
               encodedFileNoAttrs);

#ifdef DEBUG_WOLFSSL
        printf("Encoded DER (%d bytes):\n", ret);
        WOLFSSL_BUFFER(out, ret);
#endif

        if (write_file_buffer(encodedFileNoAttrs, out, ret) != 0) {
            printf("ERROR: error writing encoded to output file\n");
            return -1;
        }
    }

    wc_PKCS7_Free(pkcs7);
    wc_FreeRng(&rng);

    return ret;
}

static int signedData_sign_attrs(byte* cert, word32 certSz, byte* key,
                                 word32 keySz, byte* out, word32 outSz,
                                 int devId)
{
    int ret;
    PKCS7* pkcs7;
    WC_RNG rng;

    static byte messageTypeOid[] =
               { 0x06, 0x0a, 0x60, 0x86, 0x48, 0x01, 0x86, 0xF8, 0x45, 0x01,
                 0x09, 0x02 };
    static byte messageType[] = { 0x13, 2, '1', '9' };

    PKCS7Attrib attribs[] =
    {
        { messageTypeOid, sizeof(messageTypeOid), messageType,
                                       sizeof(messageType) }
    };

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
    pkcs7->hashOID         = SHA256h;
    pkcs7->encryptOID      = RSAk;
    pkcs7->privateKey      = key;
    pkcs7->privateKeySz    = keySz;
    pkcs7->signedAttribs   = attribs;
    pkcs7->signedAttribsSz = sizeof(attribs)/sizeof(PKCS7Attrib);

    /* encode signedData, returns size */
    ret = wc_PKCS7_EncodeSignedData(pkcs7, out, outSz);
    if (ret <= 0) {
        printf("ERROR: wc_PKCS7_EncodeSignedData() failed, ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        wc_FreeRng(&rng);
        return -1;

    } else {
        printf("Successfully encoded SignedData bundle (%s)\n",
               encodedFileAttrs);

#ifdef DEBUG_WOLFSSL
        printf("Encoded DER (%d bytes):\n", ret);
        WOLFSSL_BUFFER(out, ret);
#endif

        if (write_file_buffer(encodedFileAttrs, out, ret) != 0) {
            printf("ERROR: error writing encoded to output file\n");
            return -1;
        }

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

typedef struct {
    int exampleVar; /* example, not used */
} myCryptoDevCtx;

/* Example crypto dev callback function that calls software versions, could
 * be set up to call down to hardware module for crypto operations if
 * desired by user. If an algorithm is not supported by hardware, or user
 * callback, the cryptodev callback can return NOT_COMPILED_IN to default
 * back to using software crypto implementation. */
static int myCryptoDevCb(int devIdArg, wc_CryptoInfo* info, void* ctx)
{
    int ret = NOT_COMPILED_IN; /* return this to bypass HW and use SW */
    myCryptoDevCtx* myCtx = (myCryptoDevCtx*)ctx;

    if (info == NULL)
        return BAD_FUNC_ARG;

    if (info->algo_type == WC_ALGO_TYPE_PK) {
    #ifdef DEBUG_WOLFSSL
        printf("CryptoDevCb: Pk Type %d\n", info->pk.type);
    #endif

    #ifndef NO_RSA
        if (info->pk.type == WC_PK_TYPE_RSA) {
            /* set devId to invalid, so software is used */
            info->pk.rsa.key->devId = INVALID_DEVID;

            switch (info->pk.rsa.type) {
                case RSA_PUBLIC_ENCRYPT:
                case RSA_PUBLIC_DECRYPT:
                    /* perform software based RSA public op */
                    ret = wc_RsaFunction(
                        info->pk.rsa.in, info->pk.rsa.inLen,
                        info->pk.rsa.out, info->pk.rsa.outLen,
                        info->pk.rsa.type, info->pk.rsa.key, info->pk.rsa.rng);
                    break;
                case RSA_PRIVATE_ENCRYPT:
                case RSA_PRIVATE_DECRYPT:
                    /* perform software based RSA private op */
                    ret = wc_RsaFunction(
                        info->pk.rsa.in, info->pk.rsa.inLen,
                        info->pk.rsa.out, info->pk.rsa.outLen,
                        info->pk.rsa.type, info->pk.rsa.key, info->pk.rsa.rng);
                    break;
            }

            /* reset devId */
            info->pk.rsa.key->devId = devIdArg;
        }
    #ifdef WOLFSSL_KEY_GEN
        else if (info->pk.type == WC_PK_TYPE_RSA_KEYGEN) {
            info->pk.rsakg.key->devId = INVALID_DEVID;

            ret = wc_MakeRsaKey(info->pk.rsakg.key, info->pk.rsakg.size,
                info->pk.rsakg.e, info->pk.rsakg.rng);

            /* reset devId */
            info->pk.rsakg.key->devId = devIdArg;
        }
    #endif
    #endif /* !NO_RSA */
    #ifdef HAVE_ECC
        if (info->pk.type == WC_PK_TYPE_EC_KEYGEN) {
            /* set devId to invalid, so software is used */
            info->pk.eckg.key->devId = INVALID_DEVID;

            ret = wc_ecc_make_key_ex(info->pk.eckg.rng, info->pk.eckg.size,
                info->pk.eckg.key, info->pk.eckg.curveId);

            /* reset devId */
            info->pk.eckg.key->devId = devIdArg;
        }
        else if (info->pk.type == WC_PK_TYPE_ECDSA_SIGN) {
            /* set devId to invalid, so software is used */
            info->pk.eccsign.key->devId = INVALID_DEVID;

            ret = wc_ecc_sign_hash(
                info->pk.eccsign.in, info->pk.eccsign.inlen,
                info->pk.eccsign.out, info->pk.eccsign.outlen,
                info->pk.eccsign.rng, info->pk.eccsign.key);

            /* reset devId */
            info->pk.eccsign.key->devId = devIdArg;
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
        else if (info->pk.type == WC_PK_TYPE_ECDH) {
            /* set devId to invalid, so software is used */
            info->pk.ecdh.private_key->devId = INVALID_DEVID;

            ret = wc_ecc_shared_secret(
                info->pk.ecdh.private_key, info->pk.ecdh.public_key,
                info->pk.ecdh.out, info->pk.ecdh.outlen);

            /* reset devId */
            info->pk.ecdh.private_key->devId = devIdArg;
        }
    #endif /* HAVE_ECC */
    }
    else if (info->algo_type == WC_ALGO_TYPE_CIPHER) {
    #if !defined(NO_AES) && defined(HAVE_AESGCM)
        if (info->cipher.type == WC_CIPHER_AES_GCM) {

            if (info->cipher.enc) {
                /* set devId to invalid, so software is used */
                info->cipher.aesgcm_enc.aes->devId = INVALID_DEVID;

                ret = wc_AesGcmEncrypt(
                    info->cipher.aesgcm_enc.aes,
                    info->cipher.aesgcm_enc.out,
                    info->cipher.aesgcm_enc.in,
                    info->cipher.aesgcm_enc.sz,
                    info->cipher.aesgcm_enc.iv,
                    info->cipher.aesgcm_enc.ivSz,
                    info->cipher.aesgcm_enc.authTag,
                    info->cipher.aesgcm_enc.authTagSz,
                    info->cipher.aesgcm_enc.authIn,
                    info->cipher.aesgcm_enc.authInSz);

                /* reset devId */
                info->cipher.aesgcm_enc.aes->devId = devIdArg;
            }
            else {
                /* set devId to invalid, so software is used */
                info->cipher.aesgcm_dec.aes->devId = INVALID_DEVID;

                ret = wc_AesGcmDecrypt(
                    info->cipher.aesgcm_dec.aes,
                    info->cipher.aesgcm_dec.out,
                    info->cipher.aesgcm_dec.in,
                    info->cipher.aesgcm_dec.sz,
                    info->cipher.aesgcm_dec.iv,
                    info->cipher.aesgcm_dec.ivSz,
                    info->cipher.aesgcm_dec.authTag,
                    info->cipher.aesgcm_dec.authTagSz,
                    info->cipher.aesgcm_dec.authIn,
                    info->cipher.aesgcm_dec.authInSz);

                /* reset devId */
                info->cipher.aesgcm_dec.aes->devId = devIdArg;
            }
        }
    #endif /* !NO_AES && HAVE_AESGCM */
    }

    (void)devIdArg;
    (void)myCtx;

    return ret;
}

int main(int argc, char** argv)
{
    int ret, devId;
    int encryptedSz, decryptedSz;
    word32 certSz, keySz;
    myCryptoDevCtx myCtx;

    byte cert[2048];
    byte key[2048];
    byte encrypted[2048];
    byte decrypted[2048];
    
#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    ret = wolfCrypt_Init();
    if (ret != 0) {
        printf("wolfCrypt initialization failed\n");
        return -1;
    }

    /* example data for callback */
    myCtx.exampleVar = 1;

    /* setting devId to something other than INVALID_DEVID, enables
       cryptodev callback to be used internally by wolfCrypt */
    devId = 1;
    ret = wc_CryptoDev_RegisterDevice(devId, myCryptoDevCb, &myCtx);
    if (ret != 0) {
        printf("Failed to register crypto dev device, ret = %d\n", ret);
        return -1;
    }

    certSz = sizeof(cert);
    keySz = sizeof(key);
    ret = load_certs(cert, &certSz, key, &keySz);
    if (ret != 0)
        return -1;

    /* no attributes */
    encryptedSz = signedData_sign_noattrs(cert, certSz, key, keySz,
                                          encrypted, sizeof(encrypted), devId);
    if (encryptedSz < 0)
        return -1;

    decryptedSz = signedData_verify(encrypted, encryptedSz,
                                    cert, certSz, key, keySz,
                                    decrypted, sizeof(decrypted), devId);
    if (decryptedSz < 0)
        return -1;

    /* default attributes + messageType attribute */
    encryptedSz = signedData_sign_attrs(cert, certSz, key, keySz,
                                        encrypted, sizeof(encrypted), devId);
    if (encryptedSz < 0)
        return -1;

    decryptedSz = signedData_verify(encrypted, encryptedSz,
                                    cert, certSz, key, keySz,
                                    decrypted, sizeof(decrypted), devId);
    if (decryptedSz < 0)
        return -1;

    (void)argc;
    (void)argv;

    return 0;
}

#else

int main(int argc, char** argv)
{
    printf("Must build wolfSSL using ./configure --enable-pkcs7 --enable-cryptodev\n");
    return 0;
}

#endif /* HAVE_PKCS7 & WOLF_CRYPTO_DEV */

