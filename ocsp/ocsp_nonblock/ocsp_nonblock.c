
/* ocsp_nonblock.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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
 **/

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfio.h>
#include <wolfssl/error-ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

static const char* kCACertsDir = "ca_certs"; /* put CA certs into this directory */
static const char* kGoogleCom = "google.pem"; /* www.google.com */

#ifndef HTTP_SCRATCH_BUFFER_SIZE
    #define HTTP_SCRATCH_BUFFER_SIZE 512
#endif
#ifndef MAX_URL_ITEM_SIZE
    #define MAX_URL_ITEM_SIZE   80
#endif
#ifndef DEFAULT_TIMEOUT_SEC
    #define DEFAULT_TIMEOUT_SEC 0 /* no timeout */
#endif

static int io_timeout_sec = DEFAULT_TIMEOUT_SEC;


static SOCKET_T sfd = SOCKET_INVALID;
static word16   port;
static char     path[MAX_URL_ITEM_SIZE];
static char     domainName[MAX_URL_ITEM_SIZE];
static int      nonBlockCnt = 0;
static byte*    httpBuf;

/* Return size of the OCSP response or negative for error */
static int OcspLookupNonBlockCb(void* ctx, const char* url, int urlSz,
                        byte* ocspReqBuf, int ocspReqSz, byte** ocspRespBuf)
{
    int ret = WOLFSSL_CBIO_ERR_GENERAL;

    if (sfd != SOCKET_INVALID) {
        ret = wolfIO_HttpProcessResponseOcsp(sfd, ocspRespBuf,
                        httpBuf, HTTP_SCRATCH_BUFFER_SIZE, NULL);
        nonBlockCnt++;
        if (ret == OCSP_WANT_READ)
            return WOLFSSL_CBIO_ERR_WANT_READ;
        printf("OCSP Response: ret %d, nonblock count %d\n",
            ret, nonBlockCnt);
        XFREE(httpBuf, NULL, DYNAMIC_TYPE_OCSP);
        httpBuf = NULL;
        return ret;
    }

    if (ocspReqBuf == NULL || ocspReqSz == 0) {
        printf("OCSP request is required for lookup\n");
    }
    else if (ocspRespBuf == NULL) {
        printf("Cannot save OCSP response\n");
    }
    else if (wolfIO_DecodeUrl(url, urlSz, domainName, path, &port) < 0) {
        printf("Unable to decode OCSP URL\n");
    }
    else {
        /* Note: This is free'd in OcspRespFreeCb callback */
        int   httpBufSz = HTTP_SCRATCH_BUFFER_SIZE;
        httpBuf         = (byte*)XMALLOC(httpBufSz, NULL, DYNAMIC_TYPE_OCSP);

        printf("OCSP Lookup:\n");
        printf("\tURL: %s\n", url);
        printf("\tDomain: %s\n", domainName);
        printf("\tPath: %s\n", path);
        printf("\tPort: %d\n", port);

        if (httpBuf == NULL) {
            printf("Unable to create OCSP response buffer\n");
        }
        else {
            httpBufSz = wolfIO_HttpBuildRequestOcsp(domainName, path, ocspReqSz,
                                                            httpBuf, httpBufSz);

            ret = wolfIO_TcpConnect(&sfd, domainName, port, io_timeout_sec);
            if (ret == 0) {
            #if defined(WOLFSSL_NONBLOCK_OCSP) && defined(HAVE_IO_TIMEOUT)
                wolfIO_SetBlockingMode(sfd, 1); /* non-blocking */
            #endif

                if (wolfIO_Send(sfd, (char*)httpBuf, httpBufSz, 0) !=
                                                                    httpBufSz) {
                    printf("OCSP http request failed\n");
                }
                else if (wolfIO_Send(sfd, (char*)ocspReqBuf, ocspReqSz, 0) !=
                                                                    ocspReqSz) {
                    printf("OCSP ocsp request failed\n");
                }
                else {
                    do {
                        ret = wolfIO_HttpProcessResponseOcsp(sfd, ocspRespBuf, 
                                        httpBuf, HTTP_SCRATCH_BUFFER_SIZE, NULL);
                        nonBlockCnt++;
                        if (ret == OCSP_WANT_READ)
                            return WOLFSSL_CBIO_ERR_WANT_READ;
                    } while (ret == OCSP_WANT_READ);
                    printf("OCSP Response: ret %d, nonblock count %d\n", 
                        ret, nonBlockCnt);
                }
            }
            else {
                printf("OCSP Responder connection failed\n");
            }
            if (sfd != SOCKET_INVALID)
                CloseSocket(sfd);
            XFREE(httpBuf, NULL, DYNAMIC_TYPE_OCSP);
            httpBuf = NULL;
        }
    }
    (void)ctx;
    printf("Resp ret: %d\n", ret);
    return ret;
}

static void OcspRespFreeCb(void* ctx, byte *resp)
{
    if (resp)
        XFREE(resp, NULL, DYNAMIC_TYPE_OCSP);
    httpBuf = NULL;

    (void)ctx;
}

int main(int argc, char** argv)
{
    int ret;
    WOLFSSL_CERT_MANAGER* pCm;
    char pem[2048];
    int pemSz = 0;
    byte der[2000];
    int derSz = 0;
    FILE* file;
    const char* certFile = kGoogleCom;

#if 0
    wolfSSL_Debugging_ON();
#endif

    if (argc > 1) {
        certFile = argv[1];
    }

    /* Create certificate manager context */
    pCm = wolfSSL_CertManagerNew();
    if (pCm) {
    #ifdef HAVE_OCSP
        /* Enable OCSP */
        ret = wolfSSL_CertManagerEnableOCSP(pCm, 0);
        if (ret == WOLFSSL_SUCCESS) {
            /* Setup callbacks for OCSP */
            ret = wolfSSL_CertManagerSetOCSP_Cb(pCm, 
                OcspLookupNonBlockCb, 
                OcspRespFreeCb, 
                NULL /* optional context */
            );
        }
    #else
        ret = WOLFSSL_SUCCESS;
    #endif
        if (ret == WOLFSSL_SUCCESS) {
            /* Load root CAs into Certificate Manager */
            ret = wolfSSL_CertManagerLoadCA(pCm, NULL, kCACertsDir); 
            printf("Loaded Trusted CA dir %s (ret %d)\n", kCACertsDir, ret);
        }
        if (ret == WOLFSSL_SUCCESS) {
            /* Load PEM to buffer */
            file = fopen(certFile, "rb");
            if (file != NULL) {
                pemSz = fread(pem, 1, sizeof(pem), file);
                fclose(file);
            }

            /* Convert certificate to DER/ASN.1 */
            ret = wc_CertPemToDer(
                (byte*)pem, pemSz,
                der, sizeof(der), CERT_TYPE);
            if (ret >= 0) {
                derSz = ret;
                ret = WOLFSSL_SUCCESS;
            }
            printf("Convert Google.com PEM cert to DER (ret %d)\n", ret);
        }
        if (ret == WOLFSSL_SUCCESS) {
            /* Load and verify certificate */
            ret = wolfSSL_CertManagerVerifyBuffer(pCm, 
                der, derSz, WOLFSSL_FILETYPE_ASN1);
            printf("Verify Google.com cert: %d\n", ret);
        }
    #ifdef HAVE_OCSP
        if (ret == WOLFSSL_SUCCESS) {
            /* Check OCSP for certificate */
            do {
                ret = wolfSSL_CertManagerCheckOCSP(pCm,
                    der, derSz);
            } while (ret == OCSP_WANT_READ);
            printf("Check OCSP for Google.com (ret %d)\n", ret);
        }
    #endif

        wolfSSL_CertManagerFree(pCm);
    }
    else {
        ret = MEMORY_E;
    }

    printf("Ret = %d: %s\n", 
        ret, (ret == WOLFSSL_SUCCESS) ? 
            "success" : 
            wc_GetErrorString(ret));

    return ret;
}
