/* certloadverifybuffer.c
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
#include <stdlib.h>
#include <unistd.h>

#ifdef WOLFSSL_USER_SETTINGS
    #include <wolfssl/wolfcrypt/settings.h>
#else
    #include <wolfssl/options.h>
#endif
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/types.h>

/* root ca (certs/ca-ecc-cert.pem) */
static const byte authCert[] = "\
-----BEGIN CERTIFICATE-----\n\
MIICizCCAjCgAwIBAgIJAP0OKSFmy0ijMAoGCCqGSM49BAMCMIGXMQswCQYDVQQG\n\
EwJVUzETMBEGA1UECAwKV2FzaGluZ3RvbjEQMA4GA1UEBwwHU2VhdHRsZTEQMA4G\n\
A1UECgwHd29sZlNTTDEUMBIGA1UECwwLRGV2ZWxvcG1lbnQxGDAWBgNVBAMMD3d3\n\
dy53b2xmc3NsLmNvbTEfMB0GCSqGSIb3DQEJARYQaW5mb0B3b2xmc3NsLmNvbTAe\n\
Fw0xODA0MTMxNTIzMTBaFw0yMTAxMDcxNTIzMTBaMIGXMQswCQYDVQQGEwJVUzET\n\
MBEGA1UECAwKV2FzaGluZ3RvbjEQMA4GA1UEBwwHU2VhdHRsZTEQMA4GA1UECgwH\n\
d29sZlNTTDEUMBIGA1UECwwLRGV2ZWxvcG1lbnQxGDAWBgNVBAMMD3d3dy53b2xm\n\
c3NsLmNvbTEfMB0GCSqGSIb3DQEJARYQaW5mb0B3b2xmc3NsLmNvbTBZMBMGByqG\n\
SM49AgEGCCqGSM49AwEHA0IABALT2W7WAY5FyLmQMeXATOOerSk4mLoQ1ukJKoCp\n\
LhcquYq/M4NG45UL5HdAtTtDRTMPYVN8N0TBy/yAyuhD6qejYzBhMB0GA1UdDgQW\n\
BBRWjprD8ELeGLlFVW75k8/qw/OlITAfBgNVHSMEGDAWgBRWjprD8ELeGLlFVW75\n\
k8/qw/OlITAPBgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAKBggqhkjO\n\
PQQDAgNJADBGAiEA8HvMJHMZP2Fo7cgKVEq4rHnvEDKRUiw+v1CqXxjBl/UCIQDZ\n\
S2Nnb5spqddrY5uYnzKCNtrwqfdRtJeq+vrd7+9Krg==\n\
-----END CERTIFICATE-----\n";

/* chain cert, signed by authCert (above) (certs/server-ecc.pem) */
static const byte testCert1[] = "\
-----BEGIN CERTIFICATE-----\n\
MIIDUDCCAvWgAwIBAgICEAAwCgYIKoZIzj0EAwIwgZcxCzAJBgNVBAYTAlVTMRMw\n\
EQYDVQQIDApXYXNoaW5ndG9uMRAwDgYDVQQHDAdTZWF0dGxlMRAwDgYDVQQKDAd3\n\
b2xmU1NMMRQwEgYDVQQLDAtEZXZlbG9wbWVudDEYMBYGA1UEAwwPd3d3LndvbGZz\n\
c2wuY29tMR8wHQYJKoZIhvcNAQkBFhBpbmZvQHdvbGZzc2wuY29tMB4XDTE3MTAy\n\
MDE4MTkwNloXDTI3MTAxODE4MTkwNlowgY8xCzAJBgNVBAYTAlVTMRMwEQYDVQQI\n\
DApXYXNoaW5ndG9uMRAwDgYDVQQHDAdTZWF0dGxlMRAwDgYDVQQKDAdFbGlwdGlj\n\
MQwwCgYDVQQLDANFQ0MxGDAWBgNVBAMMD3d3dy53b2xmc3NsLmNvbTEfMB0GCSqG\n\
SIb3DQEJARYQaW5mb0B3b2xmc3NsLmNvbTBZMBMGByqGSM49AgEGCCqGSM49AwEH\n\
A0IABLszrEwnUErGSqUEwzzenzbbci3OlOor+ssgCTksFuhhAumvTdMCk5oxW5eS\n\
IX/wzxjakRECNIboIFgzC4A0idijggE1MIIBMTAJBgNVHRMEAjAAMBEGCWCGSAGG\n\
+EIBAQQEAwIGQDAdBgNVHQ4EFgQUXV0m76x+NvmbdhUrSiUCI++yiTAwgcwGA1Ud\n\
IwSBxDCBwYAUVo6aw/BC3hi5RVVu+ZPP6sPzpSGhgZ2kgZowgZcxCzAJBgNVBAYT\n\
AlVTMRMwEQYDVQQIDApXYXNoaW5ndG9uMRAwDgYDVQQHDAdTZWF0dGxlMRAwDgYD\n\
VQQKDAd3b2xmU1NMMRQwEgYDVQQLDAtEZXZlbG9wbWVudDEYMBYGA1UEAwwPd3d3\n\
LndvbGZzc2wuY29tMR8wHQYJKoZIhvcNAQkBFhBpbmZvQHdvbGZzc2wuY29tggkA\n\
l7S9Fnj4R/IwDgYDVR0PAQH/BAQDAgOoMBMGA1UdJQQMMAoGCCsGAQUFBwMBMAoG\n\
CCqGSM49BAMCA0kAMEYCIQC+uFjw5BUBH99wVHNKbEAfd6i061Iev/UNsTPKasR2\n\
uQIhAJcI3iwowUVxtixUh5hjdqghNJCo954//AKw59MJMSfk\n\
-----END CERTIFICATE-----\n";

/* This is a self-signed test cert so load in both as CA and entity cert
    (certs/client-ecc-cert.pem) */
static const byte testCert2[] = "\n\
-----BEGIN CERTIFICATE-----\n\
MIIDCDCCAq+gAwIBAgIJAJO/at6bQZ2tMAoGCCqGSM49BAMCMIGNMQswCQYDVQQG\n\
EwJVUzEPMA0GA1UECAwGT3JlZ29uMQ4wDAYDVQQHDAVTYWxlbTETMBEGA1UECgwK\n\
Q2xpZW50IEVDQzENMAsGA1UECwwERmFzdDEYMBYGA1UEAwwPd3d3LndvbGZzc2wu\n\
Y29tMR8wHQYJKoZIhvcNAQkBFhBpbmZvQHdvbGZzc2wuY29tMB4XDTE4MDQxMzE1\n\
MjMxMFoXDTIxMDEwNzE1MjMxMFowgY0xCzAJBgNVBAYTAlVTMQ8wDQYDVQQIDAZP\n\
cmVnb24xDjAMBgNVBAcMBVNhbGVtMRMwEQYDVQQKDApDbGllbnQgRUNDMQ0wCwYD\n\
VQQLDARGYXN0MRgwFgYDVQQDDA93d3cud29sZnNzbC5jb20xHzAdBgkqhkiG9w0B\n\
CQEWEGluZm9Ad29sZnNzbC5jb20wWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAARV\n\
v/QPRFCaPc6bt/DFTfVwe9TsJI4ZgOxaTKIkA2Ism9rvojUSQ4R2FsZWlQbMAam9\n\
9nUaQve9qbI2Il/HXX+0o4H1MIHyMB0GA1UdDgQWBBTr1EtZa5VhP1FXtgRNiUGI\n\
RFyr8jCBwgYDVR0jBIG6MIG3gBTr1EtZa5VhP1FXtgRNiUGIRFyr8qGBk6SBkDCB\n\
jTELMAkGA1UEBhMCVVMxDzANBgNVBAgMBk9yZWdvbjEOMAwGA1UEBwwFU2FsZW0x\n\
EzARBgNVBAoMCkNsaWVudCBFQ0MxDTALBgNVBAsMBEZhc3QxGDAWBgNVBAMMD3d3\n\
dy53b2xmc3NsLmNvbTEfMB0GCSqGSIb3DQEJARYQaW5mb0B3b2xmc3NsLmNvbYIJ\n\
AJO/at6bQZ2tMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDRwAwRAIgYbydTYhk\n\
hrhxqjVZaLjuLPMjtRq5ukFQqMbDWOtYvWACIGGq67VzDQHbaY9S9XJtN0K1/ZS2\n\
brHEJS6WlvM5sl3q\n\
-----END CERTIFICATE-----\n";

int main(void)
{
    int ret;
    WOLFSSL_CERT_MANAGER* cm = NULL;

    /* CA to be used for verification, load into certmanager */
    const byte* caCert = authCert;
    const byte* cert1 = testCert1;
    const byte* cert2 = testCert2;
    int caSz = sizeof(authCert);
    int cSz1 = sizeof(testCert1);
    int cSz2 = sizeof(testCert2);

    wolfSSL_Init();
#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    if ((cm = wolfSSL_CertManagerNew()) == NULL) {
        printf("cert manager new failed\n");
        return -1;
    }

    ret = wolfSSL_CertManagerLoadCABuffer(cm, caCert, caSz, SSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        printf("loading the ca chain failed\n");
        printf("Error: (%d): %s\n", ret, wolfSSL_ERR_reason_error_string(ret));
        wolfSSL_CertManagerFree(cm);
        return -1;
    }

    ret = wolfSSL_CertManagerLoadCABuffer(cm, testCert2, cSz2, SSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        printf("loading the ca chain failed\n");
        printf("Error: (%d): %s\n", ret, wolfSSL_ERR_reason_error_string(ret));
        wolfSSL_CertManagerFree(cm);
        return -1;
    }

    printf("------------------------------------------------------------\n\n");
    ret = wolfSSL_CertManagerVerifyBuffer(cm, cert1, cSz1, SSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        printf("could not verify certificate.\n");
        printf("Error: (%d): %s\n", ret, wolfSSL_ERR_reason_error_string(ret));
        wolfSSL_CertManagerFree(cm);
        return -2;
    }

    printf("Verification successful on cert1!\n");

    printf("------------------------------------------------------------\n\n");
    ret = wolfSSL_CertManagerVerifyBuffer(cm, cert2, cSz2, SSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        printf("could not verify certificate.\n");
        printf("Error: (%d): %s\n", ret, wolfSSL_ERR_reason_error_string(ret));
        wolfSSL_CertManagerFree(cm);
        return -2;
    }

    printf("Verification successful on cert2!\n");
    printf("------------------------------------------------------------\n\n");

    wolfSSL_CertManagerFree(cm);
    wolfSSL_Cleanup();

    return 0;
}
