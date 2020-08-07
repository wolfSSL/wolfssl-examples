
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


/* GTS Root R1 */
static const char* kGlobalSignRootCA =
"-----BEGIN CERTIFICATE-----\n"
"MIIFYjCCBEqgAwIBAgIQd70NbNs2+RrqIQ/E8FjTDTANBgkqhkiG9w0BAQsFADBX\n"
"MQswCQYDVQQGEwJCRTEZMBcGA1UEChMQR2xvYmFsU2lnbiBudi1zYTEQMA4GA1UE\n"
"CxMHUm9vdCBDQTEbMBkGA1UEAxMSR2xvYmFsU2lnbiBSb290IENBMB4XDTIwMDYx\n"
"OTAwMDA0MloXDTI4MDEyODAwMDA0MlowRzELMAkGA1UEBhMCVVMxIjAgBgNVBAoT\n"
"GUdvb2dsZSBUcnVzdCBTZXJ2aWNlcyBMTEMxFDASBgNVBAMTC0dUUyBSb290IFIx\n"
"MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAthECix7joXebO9y/lD63\n"
"ladAPKH9gvl9MgaCcfb2jH/76Nu8ai6Xl6OMS/kr9rH5zoQdsfnFl97vufKj6bwS\n"
"iV6nqlKr+CMny6SxnGPb15l+8Ape62im9MZaRw1NEDPjTrETo8gYbEvs/AmQ351k\n"
"KSUjB6G00j0uYODP0gmHu81I8E3CwnqIiru6z1kZ1q+PsAewnjHxgsHA3y6mbWwZ\n"
"DrXYfiYaRQM9sHmklCitD38m5agI/pboPGiUU+6DOogrFZYJsuB6jC511pzrp1Zk\n"
"j5ZPaK49l8KEj8C8QMALXL32h7M1bKwYUH+E4EzNktMg6TO8UpmvMrUpsyUqtEj5\n"
"cuHKZPfmghCN6J3Cioj6OGaK/GP5Afl4/Xtcd/p2h/rs37EOeZVXtL0m79YB0esW\n"
"CruOC7XFxYpVq9Os6pFLKcwZpDIlTirxZUTQAs6qzkm06p98g7BAe+dDq6dso499\n"
"iYH6TKX/1Y7DzkvgtdizjkXPdsDtQCv9Uw+wp9U7DbGKogPeMa3Md+pvez7W35Ei\n"
"Eua++tgy/BBjFFFy3l3WFpO9KWgz7zpm7AeKJt8T11dleCfeXkkUAKIAf5qoIbap\n"
"sZWwpbkNFhHax2xIPEDgfg1azVY80ZcFuctL7TlLnMQ/0lUTbiSw1nH69MG6zO0b\n"
"9f6BQdgAmD06yK56mDcYBZUCAwEAAaOCATgwggE0MA4GA1UdDwEB/wQEAwIBhjAP\n"
"BgNVHRMBAf8EBTADAQH/MB0GA1UdDgQWBBTkrysmcRorSCeFL1JmLO/wiRNxPjAf\n"
"BgNVHSMEGDAWgBRge2YaRQ2XyolQL30EzTSo//z9SzBgBggrBgEFBQcBAQRUMFIw\n"
"JQYIKwYBBQUHMAGGGWh0dHA6Ly9vY3NwLnBraS5nb29nL2dzcjEwKQYIKwYBBQUH\n"
"MAKGHWh0dHA6Ly9wa2kuZ29vZy9nc3IxL2dzcjEuY3J0MDIGA1UdHwQrMCkwJ6Al\n"
"oCOGIWh0dHA6Ly9jcmwucGtpLmdvb2cvZ3NyMS9nc3IxLmNybDA7BgNVHSAENDAy\n"
"MAgGBmeBDAECATAIBgZngQwBAgIwDQYLKwYBBAHWeQIFAwIwDQYLKwYBBAHWeQIF\n"
"AwMwDQYJKoZIhvcNAQELBQADggEBADSkHrEoo9C0dhemMXoh6dFSPsjbdBZBiLg9\n"
"NR3t5P+T4Vxfq7vqfM/b5A3Ri1fyJm9bvhdGaJQ3b2t6yMAYN/olUazsaL+yyEn9\n"
"WprKASOshIArAoyZl+tJaox118fessmXn1hIVw41oeQa1v1vg4Fv74zPl6/AhSrw\n"
"9U5pCZEt4Wi4wStz6dTZ/CLANx8LZh1J7QJVj2fhMtfTJr9w4z30Z209fOU0iOMy\n"
"+qduBmpvvYuR7hZL6Dupszfnw0Skfths18dG9ZKb59UhvmaSGZRVbNQpsg3BZlvi\n"
"d0lIKO2d1xozclOzgjXPYovJJIultzkMu34qQb9Sz/yilrbCgj8=\n"
"-----END CERTIFICATE-----";

/* GTS CA 1C3 - Intermediate */
static const char* kGTSCA101 =
"-----BEGIN CERTIFICATE-----\n"
"MIIFljCCA36gAwIBAgINAgO8U1lrNMcY9QFQZjANBgkqhkiG9w0BAQsFADBHMQsw\n"
"CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\n"
"MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMjAwODEzMDAwMDQyWhcNMjcwOTMwMDAw\n"
"MDQyWjBGMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp\n"
"Y2VzIExMQzETMBEGA1UEAxMKR1RTIENBIDFDMzCCASIwDQYJKoZIhvcNAQEBBQAD\n"
"ggEPADCCAQoCggEBAPWI3+dijB43+DdCkH9sh9D7ZYIl/ejLa6T/belaI+KZ9hzp\n"
"kgOZE3wJCor6QtZeViSqejOEH9Hpabu5dOxXTGZok3c3VVP+ORBNtzS7XyV3NzsX\n"
"lOo85Z3VvMO0Q+sup0fvsEQRY9i0QYXdQTBIkxu/t/bgRQIh4JZCF8/ZK2VWNAcm\n"
"BA2o/X3KLu/qSHw3TT8An4Pf73WELnlXXPxXbhqW//yMmqaZviXZf5YsBvcRKgKA\n"
"gOtjGDxQSYflispfGStZloEAoPtR28p3CwvJlk/vcEnHXG0g/Zm0tOLKLnf9LdwL\n"
"tmsTDIwZKxeWmLnwi/agJ7u2441Rj72ux5uxiZ0CAwEAAaOCAYAwggF8MA4GA1Ud\n"
"DwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwEgYDVR0T\n"
"AQH/BAgwBgEB/wIBADAdBgNVHQ4EFgQUinR/r4XN7pXNPZzQ4kYU83E1HScwHwYD\n"
"VR0jBBgwFoAU5K8rJnEaK0gnhS9SZizv8IkTcT4waAYIKwYBBQUHAQEEXDBaMCYG\n"
"CCsGAQUFBzABhhpodHRwOi8vb2NzcC5wa2kuZ29vZy9ndHNyMTAwBggrBgEFBQcw\n"
"AoYkaHR0cDovL3BraS5nb29nL3JlcG8vY2VydHMvZ3RzcjEuZGVyMDQGA1UdHwQt\n"
"MCswKaAnoCWGI2h0dHA6Ly9jcmwucGtpLmdvb2cvZ3RzcjEvZ3RzcjEuY3JsMFcG\n"
"A1UdIARQME4wOAYKKwYBBAHWeQIFAzAqMCgGCCsGAQUFBwIBFhxodHRwczovL3Br\n"
"aS5nb29nL3JlcG9zaXRvcnkvMAgGBmeBDAECATAIBgZngQwBAgIwDQYJKoZIhvcN\n"
"AQELBQADggIBAIl9rCBcDDy+mqhXlRu0rvqrpXJxtDaV/d9AEQNMwkYUuxQkq/BQ\n"
"cSLbrcRuf8/xam/IgxvYzolfh2yHuKkMo5uhYpSTld9brmYZCwKWnvy15xBpPnrL\n"
"RklfRuFBsdeYTWU0AIAaP0+fbH9JAIFTQaSSIYKCGvGjRFsqUBITTcFTNvNCCK9U\n"
"+o53UxtkOCcXCb1YyRt8OS1b887U7ZfbFAO/CVMkH8IMBHmYJvJh8VNS/UKMG2Yr\n"
"PxWhu//2m+OBmgEGcYk1KCTd4b3rGS3hSMs9WYNRtHTGnXzGsYZbr8w0xNPM1IER\n"
"lQCh9BIiAfq0g3GvjLeMcySsN1PCAJA/Ef5c7TaUEDu9Ka7ixzpiO2xj2YC/WXGs\n"
"Yye5TBeg2vZzFb8q3o/zpWwygTMD0IZRcZk0upONXbVRWPeyk+gB9lm+cZv9TSjO\n"
"z23HFtz30dZGm6fKa+l3D/2gthsjgx0QGtkJAITgRNOidSOzNIb2ILCkXhAd4FJG\n"
"AJ2xDx8hcFH1mt0G/FX0Kw4zd8NLQsLxdxP8c4CU6x+7Nz/OAipmsHMdMqUybDKw\n"
"juDEI/9bfU1lcKwrmz3O2+BtjjKAvpafkmO8l7tdufThcV4q5O8DIrGKZTqPwJNl\n"
"1IXNDw9bg1kWRxYtnCQ6yICmJhSFm/Y3m6xv+cXDBlHz4n/FsRC6UfTd\n"
"-----END CERTIFICATE-----";

/* Google.com */
static const char* kGoogleCom =
"-----BEGIN CERTIFICATE-----\n"
"MIIEhjCCA26gAwIBAgIQWwvxxxXoxEkSWJsftFiO7jANBgkqhkiG9w0BAQsFADBG\n"
"MQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExM\n"
"QzETMBEGA1UEAxMKR1RTIENBIDFDMzAeFw0yMjA1MDQxNzQwMDVaFw0yMjA3Mjcx\n"
"NzQwMDRaMBkxFzAVBgNVBAMTDnd3dy5nb29nbGUuY29tMFkwEwYHKoZIzj0CAQYI\n"
"KoZIzj0DAQcDQgAEy3kqjk9F7+Ap8XWjvvDnAUfiJXV6bHblqegicb6Krq3zUw8T\n"
"KUQ8wxMtRoZXHv9DtZgC1ErW6qAPt0BWdzP7waOCAmYwggJiMA4GA1UdDwEB/wQE\n"
"AwIHgDATBgNVHSUEDDAKBggrBgEFBQcDATAMBgNVHRMBAf8EAjAAMB0GA1UdDgQW\n"
"BBSoMrJUWSIVHdDkqXgfi2VI5nQ2TjAfBgNVHSMEGDAWgBSKdH+vhc3ulc09nNDi\n"
"RhTzcTUdJzBqBggrBgEFBQcBAQReMFwwJwYIKwYBBQUHMAGGG2h0dHA6Ly9vY3Nw\n"
"LnBraS5nb29nL2d0czFjMzAxBggrBgEFBQcwAoYlaHR0cDovL3BraS5nb29nL3Jl\n"
"cG8vY2VydHMvZ3RzMWMzLmRlcjAZBgNVHREEEjAQgg53d3cuZ29vZ2xlLmNvbTAh\n"
"BgNVHSAEGjAYMAgGBmeBDAECATAMBgorBgEEAdZ5AgUDMDwGA1UdHwQ1MDMwMaAv\n"
"oC2GK2h0dHA6Ly9jcmxzLnBraS5nb29nL2d0czFjMy9RT3ZKME4xc1QyQS5jcmww\n"
"ggEDBgorBgEEAdZ5AgQCBIH0BIHxAO8AdQBByMqx3yJGShDGoToJQodeTjGLGwPr\n"
"60vHaPCQYpYG9gAAAYCQX05XAAAEAwBGMEQCIA/HX1T2lssgnL8weEBFzPsILM4q\n"
"/3iJ5FyXJgZZ9ZMQAiBi0HochB+UgZMpslJ72ei48hvzGErcXvUJUwXVx4x6ZwB2\n"
"ACl5vvCeOTkh8FZzn2Old+W+V32cYAr4+U1dJlwlXceEAAABgJBfTiYAAAQDAEcw\n"
"RQIhAIcwKuzq6j1VwM1F3P/3L0Un5LKUt4o52+KREIULHJ6yAiAIVxHlI0vTToyP\n"
"N96UQkuM0FvPus2vGZLfIimVHrqrQzANBgkqhkiG9w0BAQsFAAOCAQEAw/wVl+C1\n"
"0mjwVu3NCu9sbnX47TuPz2lwT/6aUOMmRQg5Z3I9qWwRs5TdwYS/RXjGbATG8STu\n"
"Qmq5h4GRil5523D2OKmJ2ZBc033tk/aDJzf3bRQrFnzYNDIo2zW7rrdg0yUE2ytq\n"
"30pP0so32wVtqAKZOdtgYyQs1WXEgOVouGkecgdKv2pMyWa6TVjMNnMxCwqq4MRG\n"
"R5thr5l5tg20zvpGM7bE/VuYegTSqQyaF6arUpjpOX7xclfERZ1RUOh1EHHnH4gf\n"
"l7eOUXh950nbb3bjp2bUF1CjsnveJI1UfqcUrp3Tuoh7ScT1gEiJ82qGsVtyq3AU\n"
"FvKz0TJH0ipymA==\n"
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
            printf("Load Trusted: GlobalSign CA (ret %d)\n", ret);
        }
        if (ret == WOLFSSL_SUCCESS) {
            /* Load intermediate CA into Certificate Manager */
            ret = wolfSSL_CertManagerLoadCABuffer(pCm, 
                (const unsigned char *)kGTSCA101, 
                XSTRLEN(kGTSCA101), WOLFSSL_FILETYPE_PEM);
            printf("Load Trusted: GTS CA 101 (ret %d)\n", ret);
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
            ret = wolfSSL_CertManagerCheckOCSP(pCm, 
                der, derSz);
            printf("Check OCSP for Google.com (ret %d)\n", ret);
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
