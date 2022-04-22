/* signedData-p7b.c
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
 *
 * DESCRIPTION:
 *
 * This file includes an example of parsing a .p7b certificate bundle using
 * wolfCrypt's PKCS#7 SignedData API, looping over each extracted certificate,
 * converting each certificate to PEM format (from DER), and printing
 * it to the terminal for info/reference.
 *
 * This is only provided as an example and may need modification if integrated
 * into a production application.
 */
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/pkcs7.h>
#include <wolfssl/wolfcrypt/asn_public.h>

/* Sample certificate .p7b file, to be converted to DER/PEM */
#define p7bFile  "../certs/test-degenerate.p7b"

/* Max PEM cert size, used to allocate memory below. Change as needed
 * for your expected PEM certificate sizes */
#define MAX_PEM_CERT_SIZE 4096

#if defined(HAVE_PKCS7) && defined(WOLFSSL_DER_TO_PEM)

int main(int argc, char** argv)
{
    int ret, i;
    PKCS7* pkcs7;
    word32 p7bBufSz;            /* size of p7b we read, bytes */
    byte   p7bBuf[4096];        /* array to hold input p7b file */
    byte*  singleCertDer;       /* tmp ptr to one DER cert in decoded PKCS7 */
    word32 singleCertDerSz;     /* tmp size of one DER cert in decoded PKCS7 */
    byte*  singleCertPem;
    word32 singleCertPemSz;
    FILE* file;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    /* read p7b file into buffer */
    p7bBufSz = sizeof(p7bBuf);
    file = fopen(p7bFile, "rb");
    if (file == NULL) {
        printf("Error opening %s\n", p7bFile);
        return -1;
    }
    p7bBufSz = (word32)fread(p7bBuf, 1, p7bBufSz, file);
    fclose(file);

    /* extract cert from PKCS#7 (p7b) format */
    pkcs7 = wc_PKCS7_New(NULL, INVALID_DEVID);
    if (pkcs7 == NULL) {
        printf("Error creating new PKCS7 structure\n");
        return -1;
    }

    ret = wc_PKCS7_VerifySignedData(pkcs7, p7bBuf, p7bBufSz);
    if (ret < 0) {
        printf("Error in wc_PKCS7_VerifySignedData(), ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        return -1;
    }
    printf("Successfully verified SignedData bundle.\n");

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
    }

    wc_PKCS7_Free(pkcs7);

    (void)argc;
    (void)argv;

    return 0;
}

#else

int main(int argc, char** argv)
{
    printf("Must build wolfSSL using ./configure --enable-pkcs7 "
           "CFLAGS=\"-DWOLFSSL_DER_TO_PEM\"\n");
    return 0;
}

#endif

