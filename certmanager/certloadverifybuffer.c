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
MIIClDCCAjugAwIBAgIUfr3ZwaWyYKS69IbPE6x3hL6sB+cwCgYIKoZIzj0EAwIw\n\
gZcxCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApXYXNoaW5ndG9uMRAwDgYDVQQHDAdT\n\
ZWF0dGxlMRAwDgYDVQQKDAd3b2xmU1NMMRQwEgYDVQQLDAtEZXZlbG9wbWVudDEY\n\
MBYGA1UEAwwPd3d3LndvbGZzc2wuY29tMR8wHQYJKoZIhvcNAQkBFhBpbmZvQHdv\n\
bGZzc2wuY29tMB4XDTIwMDYxOTEzMjM0MVoXDTIzMDMxNjEzMjM0MVowgZcxCzAJ\n\
BgNVBAYTAlVTMRMwEQYDVQQIDApXYXNoaW5ndG9uMRAwDgYDVQQHDAdTZWF0dGxl\n\
MRAwDgYDVQQKDAd3b2xmU1NMMRQwEgYDVQQLDAtEZXZlbG9wbWVudDEYMBYGA1UE\n\
AwwPd3d3LndvbGZzc2wuY29tMR8wHQYJKoZIhvcNAQkBFhBpbmZvQHdvbGZzc2wu\n\
Y29tMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEAtPZbtYBjkXIuZAx5cBM456t\n\
KTiYuhDW6QkqgKkuFyq5ir8zg0bjlQvkd0C1O0NFMw9hU3w3RMHL/IDK6EPqp6Nj\n\
MGEwHQYDVR0OBBYEFFaOmsPwQt4YuUVVbvmTz+rD86UhMB8GA1UdIwQYMBaAFFaO\n\
msPwQt4YuUVVbvmTz+rD86UhMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQD\n\
AgGGMAoGCCqGSM49BAMCA0cAMEQCIAbDCmkBVzEXCT8hlU8/xSDjIYZ1R16H90iK\n\
HgH60mfIAiB9yOktW3Qoh3Hbow4ZhMyzpoM1dfgF3fxeUSsYmMSslQ==\n\
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
MIIDXzCCAwSgAwIBAgIUYbMeWfNobKR5QoMvGlBxA74yqiwwCgYIKoZIzj0EAwIw\n\
gY0xCzAJBgNVBAYTAlVTMQ8wDQYDVQQIDAZPcmVnb24xDjAMBgNVBAcMBVNhbGVt\n\
MRMwEQYDVQQKDApDbGllbnQgRUNDMQ0wCwYDVQQLDARGYXN0MRgwFgYDVQQDDA93\n\
d3cud29sZnNzbC5jb20xHzAdBgkqhkiG9w0BCQEWEGluZm9Ad29sZnNzbC5jb20w\n\
HhcNMjAwNjE5MTMyMzQxWhcNMjMwMzE2MTMyMzQxWjCBjTELMAkGA1UEBhMCVVMx\n\
DzANBgNVBAgMBk9yZWdvbjEOMAwGA1UEBwwFU2FsZW0xEzARBgNVBAoMCkNsaWVu\n\
dCBFQ0MxDTALBgNVBAsMBEZhc3QxGDAWBgNVBAMMD3d3dy53b2xmc3NsLmNvbTEf\n\
MB0GCSqGSIb3DQEJARYQaW5mb0B3b2xmc3NsLmNvbTBZMBMGByqGSM49AgEGCCqG\n\
SM49AwEHA0IABFW/9A9EUJo9zpu38MVN9XB71OwkjhmA7FpMoiQDYiyb2u+iNRJD\n\
hHYWxlaVBswBqb32dRpC972psjYiX8ddf7SjggE+MIIBOjAdBgNVHQ4EFgQU69RL\n\
WWuVYT9RV7YETYlBiERcq/Iwgc0GA1UdIwSBxTCBwoAU69RLWWuVYT9RV7YETYlB\n\
iERcq/KhgZOkgZAwgY0xCzAJBgNVBAYTAlVTMQ8wDQYDVQQIDAZPcmVnb24xDjAM\n\
BgNVBAcMBVNhbGVtMRMwEQYDVQQKDApDbGllbnQgRUNDMQ0wCwYDVQQLDARGYXN0\n\
MRgwFgYDVQQDDA93d3cud29sZnNzbC5jb20xHzAdBgkqhkiG9w0BCQEWEGluZm9A\n\
d29sZnNzbC5jb22CFGGzHlnzaGykeUKDLxpQcQO+MqosMAwGA1UdEwQFMAMBAf8w\n\
HAYDVR0RBBUwE4ILZXhhbXBsZS5jb22HBH8AAAEwHQYDVR0lBBYwFAYIKwYBBQUH\n\
AwEGCCsGAQUFBwMCMAoGCCqGSM49BAMCA0kAMEYCIQDkoCMmKwtCD5c3bcsUI8PD\n\
5kTPX0wmo3JkepzLZKumvgIhAKrFo1D28aXbBeB10ve6SV+PfRxEsW7fyNoQSC1T\n\
CKi0\n\
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
