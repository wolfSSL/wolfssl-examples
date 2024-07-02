/* pkcs7-verify.c
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

#ifdef HAVE_PKCS7

static const char* pkcs7SignedDer = "signed.p7b"; /* DER */
static const char* pkcs7SignedPem = "signed.p7s"; /* PEM */

int main(int argc, char** argv)
{
    int rc = 0;
    PKCS7 pkcs7;
    XFILE derFile;
    byte* fileBuf = NULL;
    word32 fileSz = 0;
    byte* derBuf = NULL;
    word32 derSz = 0;

    (void)argc;
    (void)argv;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    /* load PKCS7 */
    derFile = fopen(pkcs7SignedPem, "rb");
    if (derFile) {
        fseek(derFile, 0, SEEK_END);
        fileSz = (int)ftell(derFile);
        rewind(derFile);

        fileBuf = (byte*)XMALLOC(fileSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        derBuf = (byte*)XMALLOC(fileSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        if (fileBuf == NULL || derBuf == NULL) {
            rc = MEMORY_E; goto exit;
        }
        derSz = fileSz;

        rc = (int)fread(fileBuf, 1, fileSz, derFile);
        fclose(derFile);

        if (rc != fileSz) {
            printf("Failed to read der file!\n");
            return -1;
        }
    }

    /* PKCS_Init captures/saves this, so make sure
     * isDynamic = 0 since it is on the stack */
    pkcs7.isDynamic = 0;

    /* Test verify */
    rc = wc_PKCS7_Init(&pkcs7, NULL, INVALID_DEVID);
    if (rc != 0) goto exit;
    rc = wc_PKCS7_InitWithCert(&pkcs7, NULL, 0);
    if (rc != 0) goto exit;

    /* convert PEM to DER */
    rc = wc_CertPemToDer(fileBuf, fileSz, derBuf, derSz, PKCS7_TYPE);
    if (rc < 0) {
        goto exit;
    }
    derSz = rc;
    rc = 0;

    printf("Der %d\n", derSz);
    WOLFSSL_BUFFER(derBuf, derSz);

    rc = wc_PKCS7_VerifySignedData(&pkcs7, derBuf, derSz);
    if (rc != 0) goto exit;

    printf("PKCS7 Verify Success\n");

exit:

    if (rc != 0)
        printf("RC=%d\n", rc);

    wc_PKCS7_Free(&pkcs7);
    XFREE(derBuf, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    XFREE(fileBuf, NULL, DYNAMIC_TYPE_TMP_BUFFER);

    return rc;
}

#else

int main(int argc, char** argv)
{
    printf("Not compiled in: Must build wolfSSL using ./configure --enable-pkcs7\n");
    return 0;
}

#endif