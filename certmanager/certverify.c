/* certverify.c
 *
 * Copyright (C) 2006-2018 wolfSSL Inc.
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

#include <stdio.h>
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/test.h>

int main(void)
{
    int ret;
    WOLFSSL_CERT_MANAGER* cm = NULL;

    const char* caCert     = "../certs/ca-cert.pem";
    const char* verifyCert = "../certs/server-cert.pem";

#ifdef HAVE_CRL
    const char* crlPem     = "../certs/crl/crl.pem";
    const char* caCertDer  = "../certs/ca-cert.der";
    FILE* file;
    byte buf[4096];
    int bufSz;
#endif

    wolfSSL_Init();
#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    cm = wolfSSL_CertManagerNew();
    if (cm == NULL) {
        printf("wolfSSL_CertManagerNew() failed\n");
        return -1;
    }

    wolfSSL_CertManagerSetVerify(cm, myVerify);

    ret = wolfSSL_CertManagerLoadCA(cm, caCert, NULL);
    if (ret != WOLFSSL_SUCCESS) {
        printf("wolfSSL_CertManagerLoadCA() failed (%d): %s\n",
                ret, wolfSSL_ERR_reason_error_string(ret));
        ret = -1; goto exit;
    }

    ret = wolfSSL_CertManagerVerify(cm, verifyCert, WOLFSSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        printf("wolfSSL_CertManagerVerify() failed (%d): %s\n",
                ret, wolfSSL_ERR_reason_error_string(ret));
        ret = -1; goto exit;
    }
    printf("Verification Successful!\n");

#ifdef HAVE_CRL
    file = fopen(crlPem, "rb");
    if (file == NULL) {
        ret = -1; goto exit;
    }

    bufSz = fread(buf, 1, sizeof(buf), file);
    fclose(file);

    ret = wolfSSL_CertManagerLoadCRLBuffer(cm, buf, bufSz, WOLFSSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        printf("wolfSSL_CertManagerLoadCRLBuffer() failed (%d): %s\n",
            ret, wolfSSL_ERR_reason_error_string(ret));
        ret = -1; goto exit;
    }


    file = fopen(caCertDer, "rb");
    if (file == NULL) {
        ret = -1; goto exit;
    }

    bufSz = fread(buf, 1, sizeof(buf), file);
    fclose(file);

    ret = wolfSSL_CertManagerCheckCRL(cm, buf, bufSz);
    if (ret != WOLFSSL_SUCCESS) {
        printf("wolfSSL_CertManagerCheckCRL() failed (%d): %s\n",
            ret, wolfSSL_ERR_reason_error_string(ret));
        ret = -1; goto exit;
    }

    printf("CRL Verification Successful!\n");
#endif

exit:
    wolfSSL_CertManagerFree(cm);
    wolfSSL_Cleanup();

    return ret;
}
