/* certverify_crl_ocsp.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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

/* This example demonstrates certificate verification using the wolfSSL
 * Certificate Manager API with CRL (Certificate Revocation List) and
 * OCSP (Online Certificate Status Protocol) checking.
 *
 * For OCSP testing, this example requires an OpenSSL OCSP responder.
 * See README.md for instructions on starting the OCSP responder.
 *
 * Build wolfSSL with:
 *   ./configure --enable-crl --enable-ocsp
 *   make && sudo make install
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/test.h>

#ifdef HAVE_OCSP
/* Certificate paths for OCSP testing
 * These certificates are from wolfSSL's certs/ocsp directory and are
 * configured to work with an OpenSSL OCSP responder.
 * The server1-cert.pem certificate has an AIA extension with OCSP URL:
 *     http://127.0.0.1:22221
 *
 * Note: This assumes wolfssl and wolfssl-examples repos are in the same
 * parent directory.
 */
#define OCSP_ROOT_CA      "../../wolfssl/certs/ocsp/root-ca-cert.pem"
#define OCSP_INTER_CA     "../../wolfssl/certs/ocsp/intermediate1-ca-cert.pem"
#define OCSP_SERVER_CERT  "../../wolfssl/certs/ocsp/server1-cert.pem"

/* Demonstrate OCSP checking with a real OCSP responder
 *
 * Before running this demo, start the OpenSSL OCSP responder:
 *   cd <wolfssl-dir>
 *   openssl ocsp -port 22221 -ndays 365 \
 *       -index certs/ocsp/index-intermediate1-ca-issued-certs.txt \
 *       -rsigner certs/ocsp/ocsp-responder-cert.pem \
 *       -rkey certs/ocsp/ocsp-responder-key.pem \
 *       -CA certs/ocsp/intermediate1-ca-cert.pem
 */
static int demoOCSPCheck(void)
{
    int ret;
    WOLFSSL_CERT_MANAGER* cm = NULL;
    FILE* fp = NULL;
    long certSz;
    unsigned char* certBuf = NULL;

    /* Create a new Certificate Manager for OCSP testing
     * We use a separate CM because we need different CA certificates
     * for the OCSP certificate chain.
     */
    cm = wolfSSL_CertManagerNew();
    if (cm == NULL) {
        printf("Failed to create Certificate Manager for OCSP\n");
        return -1;
    }

    /* Load the root CA certificate */
    ret = wolfSSL_CertManagerLoadCA(cm, OCSP_ROOT_CA, NULL);
    if (ret != WOLFSSL_SUCCESS) {
        printf("Failed to load OCSP root CA (%d): %s\n",
               ret, wolfSSL_ERR_reason_error_string(ret));
        wolfSSL_CertManagerFree(cm);
        return ret;
    }
    printf("Loaded root CA: %s\n", OCSP_ROOT_CA);

    /* Load the intermediate CA certificate */
    ret = wolfSSL_CertManagerLoadCA(cm, OCSP_INTER_CA, NULL);
    if (ret != WOLFSSL_SUCCESS) {
        printf("Failed to load OCSP intermediate CA (%d): %s\n",
               ret, wolfSSL_ERR_reason_error_string(ret));
        wolfSSL_CertManagerFree(cm);
        return ret;
    }
    printf("Loaded intermediate CA: %s\n", OCSP_INTER_CA);

    /* Enable OCSP checking
     * Options (can be combined with |):
     *   WOLFSSL_OCSP_URL_OVERRIDE - Use override URL instead of cert's AIA
     *   WOLFSSL_OCSP_NO_NONCE     - Don't include nonce in request
     *   WOLFSSL_OCSP_CHECKALL     - Check all certificates in chain
     *
     * We use WOLFSSL_OCSP_NO_NONCE because OpenSSL's OCSP responder
     * may not support nonces by default.
     */
    ret = wolfSSL_CertManagerEnableOCSP(cm, WOLFSSL_OCSP_NO_NONCE);
    if (ret != WOLFSSL_SUCCESS) {
        printf("Failed to enable OCSP (%d): %s\n",
               ret, wolfSSL_ERR_reason_error_string(ret));
        wolfSSL_CertManagerFree(cm);
        return ret;
    }
    printf("OCSP checking enabled\n");

    /* First, verify the certificate chain without OCSP.
     * Note: wolfSSL_CertManagerVerify() does NOT perform OCSP checks.
     */
    printf("Verifying certificate: %s\n", OCSP_SERVER_CERT);
    ret = wolfSSL_CertManagerVerify(cm, OCSP_SERVER_CERT, WOLFSSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        printf("Certificate chain verification failed (%d): %s\n",
               ret, wolfSSL_ERR_reason_error_string(ret));
        wolfSSL_CertManagerFree(cm);
        return ret;
    }
    printf("Certificate chain verification: PASSED\n");

    /* Now perform explicit OCSP check using wolfSSL_CertManagerCheckOCSP()
     * This function requires DER-encoded certificate data, so we need to
     * load the certificate file and convert it.
     */
    printf("Performing OCSP check...\n");

    /* Load the certificate file into memory */
    fp = fopen(OCSP_SERVER_CERT, "rb");
    if (fp == NULL) {
        printf("Failed to open certificate file: %s\n", OCSP_SERVER_CERT);
        wolfSSL_CertManagerFree(cm);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    certSz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    certBuf = (unsigned char*)malloc(certSz);
    if (certBuf == NULL) {
        printf("Failed to allocate memory for certificate\n");
        fclose(fp);
        wolfSSL_CertManagerFree(cm);
        return -1;
    }
    if (fread(certBuf, 1, certSz, fp) != (size_t)certSz) {
        printf("Failed to read certificate file\n");
        free(certBuf);
        fclose(fp);
        wolfSSL_CertManagerFree(cm);
        return -1;
    }
    fclose(fp);

    /* Convert PEM to DER if needed and check OCSP
     * wolfSSL_CertManagerCheckOCSP requires DER format, but we can use
     * wolfSSL_CertManagerCheckOCSPResponse for more control, or convert
     * the PEM to DER first. For simplicity, let's use the internal
     * conversion by loading the cert into a buffer.
     */
    {
        DerBuffer* derCert = NULL;

        /* Convert PEM to DER */
        ret = wc_PemToDer(certBuf, certSz, CERT_TYPE, &derCert, NULL, NULL,
                          NULL);
        if (ret != 0) {
            printf("Failed to convert PEM to DER (%d)\n", ret);
            free(certBuf);
            wolfSSL_CertManagerFree(cm);
            return ret;
        }

        /* Perform OCSP check - this will contact the OCSP responder */
        ret = wolfSSL_CertManagerCheckOCSP(cm, derCert->buffer,
                                           derCert->length);
        wc_FreeDer(&derCert);
    }
    free(certBuf);

    if (ret == WOLFSSL_SUCCESS) {
        printf("OCSP check: PASSED\n");
        printf("Certificate status: GOOD (not revoked)\n");
    } else {
        printf("OCSP check failed (%d): %s\n",
               ret, wolfSSL_ERR_reason_error_string(ret));

        /* Provide helpful error messages */
        if (ret == OCSP_CERT_REVOKED) {
            printf("\nCertificate has been REVOKED!\n");
        }
        else if (ret == OCSP_LOOKUP_FAIL || ret == OCSP_INVALID_STATUS) {
            printf("\nOCSP lookup failed!\n");
            printf("Make sure the OCSP responder is running:\n");
            printf("  cd <wolfssl-dir>\n");
            printf("  openssl ocsp -port 22221 -ndays 365 \\\n");
            printf("      -index certs/ocsp/"
                   "index-intermediate1-ca-issued-certs.txt \\\n");
            printf("      -rsigner certs/ocsp/ocsp-responder-cert.pem \\\n");
            printf("      -rkey certs/ocsp/ocsp-responder-key.pem \\\n");
            printf("      -CA certs/ocsp/intermediate1-ca-cert.pem\n");
        }
    }

    wolfSSL_CertManagerFree(cm);
    return ret;
}
#endif /* HAVE_OCSP */

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    /* Initialize wolfSSL library */
    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        printf("wolfSSL_Init() failed\n");
        return -1;
    }

#ifdef DEBUG_WOLFSSL
    //wolfSSL_Debugging_ON();
#endif

#ifdef HAVE_OCSP
    demoOCSPCheck();
#else
    printf("OCSP checking not available (compile with --enable-ocsp)\n");
#endif

    /* Cleanup wolfSSL */
    wolfSSL_Cleanup();

}
