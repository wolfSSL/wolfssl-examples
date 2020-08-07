
/* ocsp-nonblock.c
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
 **/

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfio.h>
#include <wolfssl/error-ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>


/* GlobalSign Root CA - R2 */
static const char* kGlobalSignRootCA =
"-----BEGIN CERTIFICATE-----\n"
"MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G\n"
"A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp\n"
"Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1\n"
"MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG\n"
"A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n"
"hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL\n"
"v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8\n"
"eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq\n"
"tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd\n"
"C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa\n"
"zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB\n"
"mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH\n"
"V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n\n"
"bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG\n"
"3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs\n"
"J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO\n"
"291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS\n"
"ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd\n"
"AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\n"
"TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\n"
"-----END CERTIFICATE-----";

/* GTS CA 1O1 - Intermediate */
static const char* kGTSCA101 =
"-----BEGIN CERTIFICATE-----\n"
"MIIESjCCAzKgAwIBAgINAeO0mqGNiqmBJWlQuDANBgkqhkiG9w0BAQsFADBMMSAw\n"
"HgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEGA1UEChMKR2xvYmFs\n"
"U2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjAeFw0xNzA2MTUwMDAwNDJaFw0yMTEy\n"
"MTUwMDAwNDJaMEIxCzAJBgNVBAYTAlVTMR4wHAYDVQQKExVHb29nbGUgVHJ1c3Qg\n"
"U2VydmljZXMxEzARBgNVBAMTCkdUUyBDQSAxTzEwggEiMA0GCSqGSIb3DQEBAQUA\n"
"A4IBDwAwggEKAoIBAQDQGM9F1IvN05zkQO9+tN1pIRvJzzyOTHW5DzEZhD2ePCnv\n"
"UA0Qk28FgICfKqC9EksC4T2fWBYk/jCfC3R3VZMdS/dN4ZKCEPZRrAzDsiKUDzRr\n"
"mBBJ5wudgzndIMYcLe/RGGFl5yODIKgjEv/SJH/UL+dEaltN11BmsK+eQmMF++Ac\n"
"xGNhr59qM/9il71I2dN8FGfcddwuaej4bXhp0LcQBbjxMcI7JP0aM3T4I+DsaxmK\n"
"FsbjzaTNC9uzpFlgOIg7rR25xoynUxv8vNmkq7zdPGHXkxWY7oG9j+JkRyBABk7X\n"
"rJfoucBZEqFJJSPk7XA0LKW0Y3z5oz2D0c1tJKwHAgMBAAGjggEzMIIBLzAOBgNV\n"
"HQ8BAf8EBAMCAYYwHQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMBIGA1Ud\n"
"EwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFJjR+G4Q68+b7GCfGJAboOt9Cf0rMB8G\n"
"A1UdIwQYMBaAFJviB1dnHB7AagbeWbSaLd/cGYYuMDUGCCsGAQUFBwEBBCkwJzAl\n"
"BggrBgEFBQcwAYYZaHR0cDovL29jc3AucGtpLmdvb2cvZ3NyMjAyBgNVHR8EKzAp\n"
"MCegJaAjhiFodHRwOi8vY3JsLnBraS5nb29nL2dzcjIvZ3NyMi5jcmwwPwYDVR0g\n"
"BDgwNjA0BgZngQwBAgIwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly9wa2kuZ29vZy9y\n"
"ZXBvc2l0b3J5LzANBgkqhkiG9w0BAQsFAAOCAQEAGoA+Nnn78y6pRjd9XlQWNa7H\n"
"TgiZ/r3RNGkmUmYHPQq6Scti9PEajvwRT2iWTHQr02fesqOqBY2ETUwgZQ+lltoN\n"
"FvhsO9tvBCOIazpswWC9aJ9xju4tWDQH8NVU6YZZ/XteDSGU9YzJqPjY8q3MDxrz\n"
"mqepBCf5o8mw/wJ4a2G6xzUr6Fb6T8McDO22PLRL6u3M4Tzs3A2M1j6bykJYi8wW\n"
"IRdAvKLWZu/axBVbzYmqmwkm5zLSDW5nIAJbELCQCZwMH56t2Dvqofxs6BBcCFIZ\n"
"USpxu6x6td0V7SvJCCosirSmIatj/9dSSVDQibet8q/7UK4v4ZUN80atnZz1yg==\n"
"-----END CERTIFICATE-----";

/* Google.com */
static const char* kGoogleCom =
"-----BEGIN CERTIFICATE-----\n"
"MIIEyDCCA7CgAwIBAgIQX3VDfXmI4h0CAAAAAHJtEDANBgkqhkiG9w0BAQsFADBC\n"
"MQswCQYDVQQGEwJVUzEeMBwGA1UEChMVR29vZ2xlIFRydXN0IFNlcnZpY2VzMRMw\n"
"EQYDVQQDEwpHVFMgQ0EgMU8xMB4XDTIwMDcwNzA4MTAyMVoXDTIwMDkyOTA4MTAy\n"
"MVowaDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCkNhbGlmb3JuaWExFjAUBgNVBAcT\n"
"DU1vdW50YWluIFZpZXcxEzARBgNVBAoTCkdvb2dsZSBMTEMxFzAVBgNVBAMTDnd3\n"
"dy5nb29nbGUuY29tMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEhLyhDhZ3asO1\n"
"TiVTNpBEWRIRRw8tV9AQoSoXYTal1flvU5s3l2bJ2drpaZdQjIrHogTKyi6trWWQ\n"
"tWjrCY3vtaOCAl0wggJZMA4GA1UdDwEB/wQEAwIHgDATBgNVHSUEDDAKBggrBgEF\n"
"BQcDATAMBgNVHRMBAf8EAjAAMB0GA1UdDgQWBBSGdDnZhScVWc1JpHYnYKFI60H9\n"
"ojAfBgNVHSMEGDAWgBSY0fhuEOvPm+xgnxiQG6DrfQn9KzBoBggrBgEFBQcBAQRc\n"
"MFowKwYIKwYBBQUHMAGGH2h0dHA6Ly9vY3NwLnBraS5nb29nL2d0czFvMWNvcmUw\n"
"KwYIKwYBBQUHMAKGH2h0dHA6Ly9wa2kuZ29vZy9nc3IyL0dUUzFPMS5jcnQwGQYD\n"
"VR0RBBIwEIIOd3d3Lmdvb2dsZS5jb20wIQYDVR0gBBowGDAIBgZngQwBAgIwDAYK\n"
"KwYBBAHWeQIFAzAzBgNVHR8ELDAqMCigJqAkhiJodHRwOi8vY3JsLnBraS5nb29n\n"
"L0dUUzFPMWNvcmUuY3JsMIIBBQYKKwYBBAHWeQIEAgSB9gSB8wDxAHcAsh4FzIui\n"
"zYogTodm+Su5iiUgZ2va+nDnsklTLe+LkF4AAAFzKIpYAAAABAMASDBGAiEAqlTh\n"
"jodl0bKqdgQC8uvzXSIE7JL3PopM76L/PBVULR4CIQDEsoM11TQLdFnGVn4CPkR1\n"
"hoAKaQuniY9MHhcEbZYWfwB2APCVpFnyANGCQBAtL5OIjq1L/h1H45nh0DSmsKiq\n"
"jrJzAAABcyiKV/cAAAQDAEcwRQIhAI9h3kdpe6IsRC0oSYmVb9N3bVZGeBlaqbS2\n"
"fZvKJmZaAiAr26toYboDtme/jfLXFpk2JFI7cuCCWQyOl8QY14Ns+TANBgkqhkiG\n"
"9w0BAQsFAAOCAQEAj78GoTWwwHyOywCAybfW1ieUHBH5wvN7KFs9aq5sYgcBs4dz\n"
"cde4Rf5n3NyLd1MA7fjmUwDFiocJot/8zwUyKmfo/s7Zh3iI+53f6aQPErD3+hnA\n"
"Aupl6HGf/oITlyVFl1N76swrA7ADpxcvU2im2veX/wWJleCCxx0nHtFexX5ROF8C\n"
"4nMcqjoeA1dE+5hMhevSrgsbMjh45/nFs1rNMDxGP9gslrDse8alq7NFHz7oFZVj\n"
"RJ8VT6CZOo7MKSr4BsnNFWsUUU5WaQKYx8cBYzbJT1OmhYku2KzNCECIlQv77v84\n"
"9od6cPzyFPzbUIeq7S0uO/tTYgIQXFt3fpRERA==\n"
"-----END CERTIFICATE-----";


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

/* Return size of the OCSP response or negative for error */
static int OcspLookupNonBlockCb(void* ctx, const char* url, int urlSz,
                        byte* ocspReqBuf, int ocspReqSz, byte** ocspRespBuf)
{
    SOCKET_T sfd = SOCKET_INVALID;
    word16   port;
    int      ret = -1;
    char     path[MAX_URL_ITEM_SIZE];
    char     domainName[MAX_URL_ITEM_SIZE];
    int      nonBlockCnt = 0;

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
        byte* httpBuf   = (byte*)XMALLOC(httpBufSz, NULL, DYNAMIC_TYPE_OCSP);

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
                                        httpBuf, HTTP_SCRATCH_BUFFER_SIZE, ctx);
                        nonBlockCnt++;
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
            XFREE(httpBuf, ctx, DYNAMIC_TYPE_OCSP);
        }
    }
    return ret;
}

static void OcspRespFreeCb(void* ctx, byte *resp)
{
    if (resp)
        XFREE(resp, NULL, DYNAMIC_TYPE_OCSP);

    (void)ctx;
}

int main(void)
{
    int ret = -1;
    WOLFSSL_CERT_MANAGER* pCm;
    byte der[2000];
    int derSz;

#if 0
    wolfSSL_Debugging_ON();
#endif

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
            /* Load root CA into Certificate Manager */
            ret = wolfSSL_CertManagerLoadCABuffer(pCm, 
                (const unsigned char *)kGlobalSignRootCA, 
                XSTRLEN(kGlobalSignRootCA), WOLFSSL_FILETYPE_PEM);
            printf("Load Trusted: GlobalSign CA %d\n", ret);
        }
        if (ret == WOLFSSL_SUCCESS) {
            /* Load intermediate CA into Certificate Manager */
            ret = wolfSSL_CertManagerLoadCABuffer(pCm, 
                (const unsigned char *)kGTSCA101, 
                XSTRLEN(kGTSCA101), WOLFSSL_FILETYPE_PEM);
            printf("Load Trusted: GTS CA 101 %d\n", ret);
        }
        if (ret == WOLFSSL_SUCCESS) {
            /* Convert certificate to DER/ASN.1 */
            ret = wc_CertPemToDer(
                (unsigned char*)kGoogleCom, XSTRLEN(kGoogleCom),
                der, sizeof(der), CERT_TYPE);
            if (ret >= 0) {
                derSz = ret;
                ret = WOLFSSL_SUCCESS;
            }
            printf("Convert Google.com PEM cert to DER: %d\n", ret);
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
            ret = wolfSSL_CertManagerCheckOCSP(pCm, 
                der, derSz);
            printf("Check OCSP for Google.com: %d\n", ret);
        }
    #endif

        wolfSSL_CertManagerFree(pCm);
    }

    printf("Ret = %d: %s\n", 
        ret, (ret == WOLFSSL_SUCCESS) ? 
            "success" : 
            wc_GetErrorString(ret));

    return ret;
}
