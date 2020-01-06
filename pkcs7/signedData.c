/* signedData.c
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
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/pkcs7.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

#define certFile "../certs/client-cert.der"
#define keyFile  "../certs/client-key.der"
#define encodedFileNoAttrs "signedData_noattrs.der"
#define encodedFileAttrs   "signedData_attrs.der"

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
                                   word32 keySz, byte* out, word32 outSz)
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
    pkcs7 = wc_PKCS7_New(NULL, INVALID_DEVID);
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
                                 word32 keySz, byte* out, word32 outSz)
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
    pkcs7 = wc_PKCS7_New(NULL, INVALID_DEVID);
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
                             byte* out, word32 outSz)
{
    int ret;
    PKCS7* pkcs7;

    pkcs7 = wc_PKCS7_New(NULL, INVALID_DEVID);
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

#ifdef HAVE_PKCS7

int main(int argc, char** argv)
{
    int ret;
    int encryptedSz, decryptedSz;
    word32 certSz, keySz;

    byte cert[2048];
    byte key[2048];
    byte encrypted[2048];
    byte decrypted[2048];
    
#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    certSz = sizeof(cert);
    keySz = sizeof(key);
    ret = load_certs(cert, &certSz, key, &keySz);
    if (ret != 0)
        return -1;

    /* no attributes */
    encryptedSz = signedData_sign_noattrs(cert, certSz, key, keySz,
                                          encrypted, sizeof(encrypted));
    if (encryptedSz < 0)
        return -1;

    decryptedSz = signedData_verify(encrypted, encryptedSz,
                                    cert, certSz, key, keySz,
                                    decrypted, sizeof(decrypted));
    if (decryptedSz < 0)
        return -1;

    /* default attributes + messageType attribute */
    encryptedSz = signedData_sign_attrs(cert, certSz, key, keySz,
                                        encrypted, sizeof(encrypted));
    if (encryptedSz < 0)
        return -1;

    decryptedSz = signedData_verify(encrypted, encryptedSz,
                                    cert, certSz, key, keySz,
                                    decrypted, sizeof(decrypted));
    if (decryptedSz < 0)
        return -1;

    (void)argc;
    (void)argv;

    return 0;
}

#else

int main(int argc, char** argv)
{
    printf("Must build wolfSSL using ./configure --enable-pkcs7\n");
    return 0;
}

#endif

