/* envelopedData-ktri-stream.c
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

#define encodedFileKTRI "envelopedDataKTRI-stream.der"

typedef struct ExampleIO {
    FILE *fileOut;
    FILE *fileIn;
} ExampleIO;
static ExampleIO testIO;

#define TEST_SIZE 256
static byte* contentRead = NULL;

static int GetContentCB(PKCS7* pkcs7, byte** content, void* ctx)
{
    int ret;
    ExampleIO* io = (ExampleIO*)ctx;

    if (io == NULL) {
        printf("Issue getting user ctx in content CB\n");
        return -1;
    }

    ret = fread(contentRead, 1, TEST_SIZE, io->fileIn);
    *content = contentRead;

    return ret;
}


static int StreamOutputCB(PKCS7* pkcs7, const byte* output, word32 outputSz,
    void* ctx)
{
    ExampleIO* io = (ExampleIO*)ctx;

    if (io == NULL) {
        printf("Issue getting user ctx in stream output CB\n");
        return -1;
    }

    if (outputSz > 0) {
        if (fwrite(output, 1, outputSz, io->fileOut) != outputSz) {
            return -1;
        }
    }
    return 0;
}


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


static int envelopedData_encrypt(byte* cert, word32 certSz, byte* key,
                                 word32 keySz, byte* out, word32 outSz,
                                 word32 contentSz, byte useStreamMode)
{
    int ret;
    PKCS7* pkcs7;

    pkcs7 = wc_PKCS7_New(NULL, INVALID_DEVID);
    if (pkcs7 == NULL)
        return -1;

    pkcs7->content        = NULL;
    pkcs7->contentSz      = contentSz;
    pkcs7->contentOID     = DATA;
    pkcs7->encryptOID     = AES256CBCb;

    if (useStreamMode) {
        wc_PKCS7_SetStreamMode(pkcs7, 1, GetContentCB, StreamOutputCB,
            (void*)&testIO);
    }

    /* add recipient using RSA certificate (KTRI type) */
    ret = wc_PKCS7_AddRecipient_KTRI(pkcs7, cert, certSz, 0);
    if (ret < 0) {
        printf("wc_PKCS7_AddRecipient_KTRI() failed, ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        return -1;
    }

    /* encode envelopedData, returns size */
    ret = wc_PKCS7_EncodeEnvelopedData(pkcs7, out, outSz);
    if (ret <= 0) {
        printf("ERROR: wc_PKCS7_EncodeEnvelopedData() failed, ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        return -1;

    } else {
        printf("Successfully encoded EnvelopedData bundle (%s), stream mode"
               " %d\n", encodedFileKTRI, useStreamMode);
    }

    wc_PKCS7_Free(pkcs7);

    return ret;
}

static int envelopedData_decrypt(byte* in, word32 inSz, byte* cert,
                                word32 certSz, byte* key, word32 keySz,
                                byte* out, word32 outSz)
{
    int ret;
    PKCS7* pkcs7;

    pkcs7 = wc_PKCS7_New(NULL, INVALID_DEVID);
    if (pkcs7 == NULL)
        return -1;

    /* init with recipient cert */
    ret = wc_PKCS7_InitWithCert(pkcs7, cert, certSz);
    if (ret != 0) {
        wc_PKCS7_Free(pkcs7);
        return -1;
    }

    /* set recipient private key */
    ret = wc_PKCS7_SetKey(pkcs7, key, keySz);
    if (ret != 0) {
        wc_PKCS7_Free(pkcs7);
        return -1;
    }

    /* decode envelopedData, returns size */
    ret = wc_PKCS7_DecodeEnvelopedData(pkcs7, in, inSz, out, outSz);
    if (ret <= 0) {
        printf("Failed to decode EnvelopedData bundle (%s), error %d\n",
                encodedFileKTRI, ret);
        wc_PKCS7_Free(pkcs7);
        return -1;
    } else {
        printf("Successfully decoded EnvelopedData bundle (%s)\n",
                encodedFileKTRI);
    }

    wc_PKCS7_Free(pkcs7);

    return ret;
}

#if defined(HAVE_PKCS7) && defined(ASN_BER_TO_DER)

#define EXTRA_ASN1_SIZE 1024

int main(int argc, char** argv)
{
    int ret = 0;
    int encryptedSz = 0, decryptedSz;
    word32 certSz, keySz, contentSz;

    byte cert[2048];
    byte key[2048];
    byte* encrypted = NULL;
    byte* decrypted = NULL;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    if (argc != 2) {
        printf("USAGE: %s <content file name>\n", argv[0]);
        return -1;
    }

    if (wolfCrypt_Init() != 0) {
        printf("Issue with wolfcrypt init\n");
        return -1;
    }

    testIO.fileIn  = fopen(argv[1], "rb");
    if (testIO.fileIn == NULL) {
        printf("Issue opening file %s\n", argv[1]);
        return -1;
    }

    testIO.fileOut = fopen(encodedFileKTRI, "wb");
    if (testIO.fileOut == NULL) {
        printf("Issue opening file %s\n", encodedFileKTRI);
        fclose(testIO.fileIn);
        return -1;
    }

    contentRead = (byte*)XMALLOC(TEST_SIZE, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (contentRead == NULL) {
        printf("Unable to malloc content read buffer\n");
        ret = MEMORY_E;
    }

    if (ret == 0) {
        fseek(testIO.fileIn, 0, SEEK_END);
        contentSz = ftell(testIO.fileIn);
        fseek(testIO.fileIn, 0, SEEK_SET);
        printf("contentSz = %d\n", contentSz);

        certSz = sizeof(cert);
        keySz  = sizeof(key);
        ret    = load_certs(cert, &certSz, key, &keySz);
    }

    if (ret == 0) {
        encryptedSz = envelopedData_encrypt(cert, certSz, key, keySz,
                                    encrypted, encryptedSz, contentSz, 1);
        if (encryptedSz < 0) {
            ret = encryptedSz;
            printf("Issue %d with encrypt\n", ret);
        }
    }
    fclose(testIO.fileIn);
    fclose(testIO.fileOut);


#if 1
    decryptedSz = encryptedSz = contentSz + EXTRA_ASN1_SIZE;
    if (ret == 0) {
        encrypted = (byte*)XMALLOC(encryptedSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        decrypted = (byte*)XMALLOC(decryptedSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        if (encrypted == NULL || decrypted == NULL) {
            ret = MEMORY_E;
        }
    }

    if (ret == 0) {
        FILE* f = fopen(encodedFileKTRI, "rb");
        encryptedSz = fread(encrypted, 1, encryptedSz, f);
        fclose(f);
        if (encryptedSz <= 0) {
            printf("error reading file %s\n", encodedFileKTRI);
            ret = -1;
        }
        printf("Read %d bytes for encrypted file found\n", encryptedSz);
    }

    if (ret == 0) {
        decryptedSz = envelopedData_decrypt(encrypted, encryptedSz,
                                        cert, certSz, key, keySz,
                                        decrypted, decryptedSz);
        if (decryptedSz < 0) {
            ret = decryptedSz;
            printf("Issue %d with decrypt\n", ret);
        }
    }

#endif

    if (contentRead != NULL)
        XFREE(contentRead, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (encrypted != NULL)
        XFREE(encrypted, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (decrypted != NULL)
        XFREE(decrypted, NULL, DYNAMIC_TYPE_TMP_BUFFER);

    return ret;
}

#else

int main(int argc, char** argv)
{
    printf("Must build wolfSSL using ./configure --enable-pkcs7 --enable-indef\n");
    return 0;
}

#endif

