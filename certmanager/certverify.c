/* standalone.c
 *
 * Copyright (C) 2006-2015 wolfSSL Inc.
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

#include <stdio.h>
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

int main(void)
{
    int ret;
    WOLFSSL_CERT_MANAGER* cm = 0;

    const char* caCert     = "../certs/ca-cert.pem";
    const char* verifyCert = "../certs/server-cert.pem";

    cm = wolfSSL_CertManagerNew();
    if (cm == NULL) {
        printf("wolfSSL_CertManagerNew() failed\n");
        return -1;
    }

    ret = wolfSSL_CertManagerLoadCA(cm, caCert, 0);
    if (ret != SSL_SUCCESS) {
        printf("wolfSSL_CertManagerLoadCA() failed (%d): %s\n",
                ret, wc_GetErrorString(ret));
        wolfSSL_CertManagerFree(cm);
        return -1;
    }

    ret = wolfSSL_CertManagerVerify(cm, verifyCert, SSL_FILETYPE_PEM);
    if (ret != SSL_SUCCESS) {
        printf("wolfSSL_CertManagerVerify() failed (%d): %s\n",
                ret, wc_GetErrorString(ret));
        wolfSSL_CertManagerFree(cm);
        return -1;
    }
    printf("Verification Successful!\n");
}

