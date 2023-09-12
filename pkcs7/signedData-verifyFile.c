/* signedData-verifyFile.c
 *
 * Copyright (C) 2006-2023 wolfSSL Inc.
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

#include <wolfssl/ssl.h>
#include <wolfssl/test.h>

/* Max PEM cert size, used to allocate memory below. Change as needed
 * for your expected PEM certificate sizes */
#define MAX_PEM_CERT_SIZE 4096

/* The index of the command line option */
int myoptind = 0;

/* The current command line option */
char* myoptarg = NULL;

/**
 * Verify PKCS#7/CMS bundle contained in bundleBytes, of size bundleSz.
 *
 * Return 0 on success, negative on error
 */
static int VerifySignedData(byte* bundleBytes, word32 bundleSz,
    byte* detachedContent, word32 detachedContentSz)
{
    int ret, i;
    PKCS7* pkcs7 = NULL;
    byte*  singleCertDer;       /* tmp ptr to one DER cert in decoded PKCS7 */
    word32 singleCertDerSz;     /* tmp size of one DER cert in decoded PKCS7 */
#ifdef WOLFSSL_DER_TO_PEM
    byte*  singleCertPem;
    word32 singleCertPemSz;
#endif
    (void)singleCertDer;

    if (bundleBytes == NULL || bundleSz == 0) {
        return BAD_FUNC_ARG;
    }

    pkcs7 = wc_PKCS7_New(NULL, INVALID_DEVID);
    if (pkcs7 == NULL) {
        printf("wc_PKCS7_New failed\n");
        return -1;
    }

    if (detachedContent != NULL) {
        pkcs7->content = detachedContent;
        pkcs7->contentSz = detachedContentSz;
    }

    /* Decode signedData, returns size */
    ret = wc_PKCS7_VerifySignedData(pkcs7, bundleBytes, bundleSz);
    if (ret < 0) {
        wc_PKCS7_Free(pkcs7);
        return ret;
    }

#ifdef DEBUG_WOLFSSL
    printf("Decoded content (%d bytes):\n", pkcs7->contentSz);
    WOLFSSL_BUFFER(pkcs7->content, pkcs7->contentSz);
#else
    printf("Decoded content size is %d bytes\n", pkcs7->contentSz);
#endif

    /* wc_PKCS7_VerifySignedData() decodes input PKCS#7 and stores
     * decoded DER certificates into pkcs7->cert[]. Sizes of each cert entry
     * is stored in the separate pkcs7->certSz[] array. Max size of each
     * of the arrays is MAX_PKCS7_CERTS. Array memory is owned by wolfCrypt
     * PKCS7 and freed when calling wc_PKCS7_Free(). */

    for (i = 0; i < MAX_PKCS7_CERTS; i++) {
        if (pkcs7->certSz[i] == 0) {
            /* reached end of valid certs in array */
            break;
        }

        singleCertDer = pkcs7->cert[i];
        singleCertDerSz = pkcs7->certSz[i];
        printf("CERT [%d] size = %d bytes\n", i, singleCertDerSz);

    #ifdef WOLFSSL_DER_TO_PEM
        /* allocate array for PEM */
        singleCertPem = (byte*)XMALLOC(MAX_PEM_CERT_SIZE, NULL,
                                       DYNAMIC_TYPE_TMP_BUFFER);
        if (singleCertPem == NULL) {
            printf("Error allocating memory for PEM\n");
            break;
        }
        singleCertPemSz = MAX_PEM_CERT_SIZE;
        XMEMSET(singleCertPem, 0, singleCertPemSz);

        /* convert DER to PEM */
        singleCertPemSz = wc_DerToPem(singleCertDer, singleCertDerSz,
                                      singleCertPem, singleCertPemSz,
                                      CERT_TYPE);
        if (singleCertPem < 0) {
            printf("Error converting DER to PEM, ret = %d\n", ret);
            XFREE(singleCertPem, NULL, DYNAMIC_TYPE_TMP_BUFFER);
            break;
        }
        printf("converted DER to PEM, pemSz = %d\n", singleCertPemSz);
        printf("CERT [%d] PEM:\n", i);

        /* print PEM to terminal, only if able to NULL terminate */
        if (singleCertPemSz < MAX_PEM_CERT_SIZE - 1) {
            singleCertPem[singleCertPemSz] = 0;
            printf("%s\n", singleCertPem);
        }

        /* PEM is now in singleCertPem, of size singleCertPemSz */
        XFREE(singleCertPem, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    #endif /* WOLFSSL_DER_TO_PEM */
    }

    wc_PKCS7_Free(pkcs7);

    return ret;
}

#ifdef HAVE_PKCS7

/**
 * Read file into newly-allocated buffer fileBytes, set size of allocated
 * buffer into fileSz.
 *
 * Return 0 on success, negative on error
 */
static int ReadFile(char* fileName, byte** fileBytes, word32* fileSz)
{
    int ret = 0;
    FILE* fp = NULL;
    word32 sz = 0;

    if (fileName == NULL || fileBytes == NULL || fileSz == NULL) {
        return -1;
    }

    fp = XFOPEN(fileName, "rb");
    if (fp == XBADFILE) {
        return -1;
    }

    if (XFSEEK(fp, 0, XSEEK_END) != 0) {
        ret = -1;
    }

    if (ret == 0) {
        sz = XFTELL(fp);
        if (sz <= 0) {
            ret = -1;
        }
    }

    if (ret == 0) {
        if (XFSEEK(fp, 0, XSEEK_SET) != 0) {
            ret = -1;
        }
    }

    if (ret == 0) {
        *fileBytes = (byte*)XMALLOC(sz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        if (*fileBytes == NULL) {
            ret = MEMORY_E;
        }
        else {
            XMEMSET(*fileBytes, 0, sz);
            *fileSz = sz;
        }
    }

    if (ret == 0) {
        if ((size_t)XFREAD(*fileBytes, 1, (size_t)sz, fp) != (size_t)sz) {
            ret = -1;
        }
    }

    if (fp != XBADFILE) {
        XFCLOSE(fp);
    }

    return ret;
}

static void Usage(void)
{
    printf("signedData-verifyFile " LIBWOLFSSL_VERSION_STRING
           " (NOTE: All files relative to current directory)\n");
    printf("-?            Help, print this usage\n");
    printf("-b <file>     PKCS#7/CMS bundle to verify (DER format)\n");
    printf("-c <content>  Detached content, if needed\n");
}

int main(int argc, char** argv)
{
    int ret = 0;
    int ch = 0;

    char* bundleFile = NULL;
    byte* bundleBytes = NULL;
    word32 bundleSz = 0;

    char* detachedContentFile = NULL;
    byte* detachedContentBytes = NULL;
    word32 detachedContentSz = 0;

    printf("wolfCrypt PKCS#7/CMS SignedData verification example\n\n");

    while ((ch = mygetopt(argc, argv, "?b:c:")) != -1) {
        switch (ch) {
            case '?':
                Usage();
                exit(EXIT_SUCCESS);

            /* File containing PKCS#7/CMS bundle */
            case 'b':
                bundleFile = myoptarg;
                break;

            case 'c':
                detachedContentFile = myoptarg;
                break;

            default:
                Usage();
                exit(MY_EX_USAGE);
        }
    }
    
#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    wolfSSL_Init();

    /* Read PKCS#7 bundle file into array */
    ret = ReadFile(bundleFile, &bundleBytes, &bundleSz);
    if (ret == 0) {
        printf("Read %d bytes from %s\n", bundleSz, bundleFile);
    }
    else {
        printf("Failed to read bundle file: %s\n", bundleFile);
    }

    /* Read detached content file into array, if given */
    if (ret == 0 && detachedContentFile != NULL) {
        ret = ReadFile(detachedContentFile, &detachedContentBytes,
                &detachedContentSz); 
        if (ret == 0) {
            printf("Read %d bytes from content file: %s\n",
                   detachedContentSz, detachedContentFile);
        }
        else {
            printf("Failed to read content file: %s\n", detachedContentFile);
        }
    }

    /* Verify PKCS#7/CMS SignedData bundle */
    if (ret == 0) {
        ret = VerifySignedData(bundleBytes, bundleSz,
                detachedContentBytes, detachedContentSz);
        if (ret == 0) {
            printf("Successfully verified SignedData bundle!\n");
        }
        else {
            printf("Failed to verify SignedData bundle, ret = %d\n", ret);
        }
    }

    /* Free detached content array, allocated by ReadFile() */
    if (detachedContentBytes != NULL) {
        XMEMSET(detachedContentBytes, 0, detachedContentSz);
        XFREE(detachedContentBytes, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }
    /* Free bundle array, allocated by ReadFile() */
    if (bundleBytes != NULL) {
        XMEMSET(bundleBytes, 0, bundleSz);
        XFREE(bundleBytes, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }

    wolfSSL_Cleanup();

    exit(EXIT_SUCCESS);
}

#else

int main(int argc, char** argv)
{
    printf("Must build wolfSSL using ./configure --enable-pkcs7\n");
    return 0;
}

#endif /* HAVE_PKCS7 */

