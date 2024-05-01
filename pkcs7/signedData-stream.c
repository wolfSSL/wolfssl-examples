/* signedData-stream.c
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

#define certFile "../certs/client-cert.der"
#define keyFile  "../certs/client-key.der"
#define encodedFile "signedData_stream.der"


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
    #if 0
        word32 z;
        printf("Writing out %d bytes : ", outputSz);
        for ( z = 0; z < outputSz; z++)printf("%02X", output[z]);
        printf("\n");
    #endif

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


static int signedData(byte* cert, word32 certSz, byte* key, word32 keySz,
    int contentSz, byte* contentHash)
{
    int ret;
    PKCS7* pkcs7;
    WC_RNG rng;
    word32 outputSz;

    byte messageTypeOid[] =
               { 0x06, 0x0a, 0x60, 0x86, 0x48, 0x01, 0x86, 0xF8, 0x45, 0x01,
                 0x09, 0x02 };
    byte messageType[] = { 0x13, 2, '1', '9' };

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
    pkcs7->content         = NULL;
    pkcs7->contentSz       = contentSz;
    pkcs7->contentOID      = DATA;
    pkcs7->hashOID         = SHA256h;
    pkcs7->encryptOID      = RSAk;
    pkcs7->privateKey      = key;
    pkcs7->privateKeySz    = keySz;
    pkcs7->signedAttribs   = attribs;
    pkcs7->signedAttribsSz = sizeof(attribs)/sizeof(PKCS7Attrib);

    /* use streaming mode with IO callbacks */
    wc_PKCS7_SetStreamMode(pkcs7, 1, GetContentCB, StreamOutputCB,
        (void*)&testIO);

    /* encode signedData, returns size */
    ret = wc_PKCS7_EncodeSignedData_ex(pkcs7, contentHash, WC_SHA256_DIGEST_SIZE, NULL, &outputSz, NULL, NULL);
    if (ret <= 0) {
        printf("ERROR: wc_PKCS7_EncodeSignedData() failed, ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        wc_FreeRng(&rng);
        return -1;

    } else {
        printf("Successfully encoded SignedData bundle (%s)\n",
               encodedFile);
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

    if (ret < 0) {
        if (ret == PKCS7_SIGNEEDS_CHECK) {
            printf("ATTENTION: Parsed through bundle but no certificates found to"
                   " verify signature with\n");
        }
        else {
            printf("ERROR: Failed to verify SignedData bundle, ret = %d\n",
                ret);
        }
    } else {
        printf("Successfully verified SignedData bundle.\n");
    }

    wc_PKCS7_Free(pkcs7);

    return ret;
}

#ifdef HAVE_PKCS7
#define DEFAULT_EXAMPLE_BUFFER_SIZE 2048

int main(int argc, char** argv)
{
    int ret = 0;
    int encryptedSz = 0, decryptedSz = 0, contentSz = 0;
    word32 certSz, keySz;

    byte contentHash[WC_SHA256_DIGEST_SIZE];
    byte cert[DEFAULT_EXAMPLE_BUFFER_SIZE];
    byte key[DEFAULT_EXAMPLE_BUFFER_SIZE];
    byte *encrypted = NULL;
    byte *decrypted = NULL;

    if (argc != 2) {
        printf("Expecting content file as input\n");
        printf("%s <content file name>\n", argv[0]);
        return -1;
    }

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif
    contentRead = (byte*)XMALLOC(TEST_SIZE, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (contentRead == NULL) {
        printf("Unable to malloc content read buffer\n");
        ret = MEMORY_E;
    }

    if (ret == 0) {
        testIO.fileIn  = fopen(argv[1], "rb");
        if (testIO.fileIn == NULL) {
            printf("Issue opening file %s\n", argv[1]);
            XFREE(contentRead, NULL, DYNAMIC_TYPE_TMP_BUFFER);
            return -1;
        }
    }

    if (ret == 0) {
        testIO.fileOut = fopen(encodedFile, "wb");
        if (testIO.fileOut == NULL) {
            printf("Issue opening file %s\n", encodedFile);
            fclose(testIO.fileIn);
            XFREE(contentRead, NULL, DYNAMIC_TYPE_TMP_BUFFER);
            return -1;
        }
    }

    /* create hash of content, should be same hash type as pkcs7->hashOID */
    if (ret == 0) {
        int readSz;

        wc_Sha256 sha256;
        ret = wc_InitSha256(&sha256);
        if (ret != 0) {
            printf("Sha256 init failed, ret = %d\n", ret);
        }

        if (ret == 0) {
            do {
                readSz = fread(contentRead, 1, TEST_SIZE, testIO.fileIn);
                if (readSz > 0) {
                    ret = wc_Sha256Update(&sha256, contentRead, readSz);
                    if (ret != 0) {
                        printf("Sha256 update failed, ret = %d\n", ret);
                        break;
                    }
                }
            } while (readSz >= TEST_SIZE);
        }

        if (ret == 0) {
            ret = wc_Sha256Final(&sha256, contentHash);
            if (ret != 0) {
                printf("Sha256 final failed, ret = %d\n", ret);
            }
        }
        wc_Sha256Free(&sha256);
    }

    contentSz = ftell(testIO.fileIn);
    fseek(testIO.fileIn, 0, SEEK_SET);
    printf("contentSz = %d\n", contentSz);

    if (ret == 0) {
        certSz = sizeof(cert);
        keySz  = sizeof(key);
        ret = load_certs(cert, &certSz, key, &keySz);
    }

    /* default attributes + messageType attribute */
    if (ret == 0) {
        encryptedSz = signedData(cert, certSz, key, keySz, contentSz,
            contentHash);
    }

    fclose(testIO.fileIn);
    fclose(testIO.fileOut);
    if (encryptedSz < 0) {
        ret = encryptedSz;
        printf("Error %d with signing data\n", ret);
        goto out;
    }

    if (ret != 0)
        goto out;

#if 1
    decryptedSz = encryptedSz;
    encrypted = XMALLOC(encryptedSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    decrypted = XMALLOC(decryptedSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (encrypted == NULL || decrypted == NULL) {
        ret = MEMORY_E;
        goto out;
    }

    {
        FILE* f = fopen(encodedFile, "rb");
        encryptedSz = fread(encrypted, 1, encryptedSz, f);
        fclose(f);
    }
    if (encryptedSz <= 0) {
        printf("error reading file %s\n", encodedFile);
        goto out;
    }
    printf("read %d bytes from file\n", encryptedSz);

    decryptedSz = signedData_verify(encrypted, encryptedSz,
                                    cert, certSz, key, keySz,
                                    decrypted, decryptedSz);
    if (decryptedSz < 0)
        return -1;
#endif
out:

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
    printf("Must build wolfSSL using ./configure --enable-pkcs7\n");
    return 0;
}

#endif

