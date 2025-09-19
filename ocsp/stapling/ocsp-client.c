/* ocsp-client.c
 *
 * Standalone OCSP stapling client example.
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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
#include <wolfssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/types.h>
#include <wolfssl/ocsp.h>

#define CA_CERT     "client-certs/root-ca-cert.pem"
#define INTERMEDIATE_CA_CERT "client-certs/intermediate1-ca-cert.pem"
#define SERVER_PORT 11111

/* Certificate verify callback for wolfSSL */
static int cert_verify_cb(int preverify, WOLFSSL_X509_STORE_CTX* store)
{
    int err = wolfSSL_X509_STORE_CTX_get_error(store);
    int depth = wolfSSL_X509_STORE_CTX_get_error_depth(store);

    printf("Client: cert_verify_cb called. preverify=%d, err=%d, depth=%d\n", preverify, err, depth);

    /* If CA not found, try manual validation using CertManager */
    /* Error codes: ASN_NO_SIGNER_E, ASN_SELF_SIGNED_E, and similar */
    /* These are typically negative values, but may depend on wolfSSL build */
    if (preverify == 0) {
        int ret = 1;
        if (err == ASN_NO_SIGNER_E ||
            err == ASN_SELF_SIGNED_E
#ifdef OPENSSL_EXTRA
            || err == WOLFSSL_X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY
            || err == WOLFSSL_X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT
#endif
            ) {

            WOLFSSL_BUFFER_INFO* bInfo = &store->certs[depth];
            WOLFSSL_CERT_MANAGER* cm = NULL;
            DecodedCert cert;
            byte certInit = 0;
            WOLFSSL* ssl = (WOLFSSL*)store->userCtx;

            cm = wolfSSL_CertManagerNew();
            if (cm == NULL)
                ret = 0;
            if (ret == 1 &&
                wolfSSL_CertManagerLoadCA(cm, CA_CERT, NULL) != WOLFSSL_SUCCESS)
                ret = 0;
            /* If verifying leaf cert then we need to load the intermediate CA */
            if (ret == 1 && depth == 0 &&
                wolfSSL_CertManagerLoadCA(cm, INTERMEDIATE_CA_CERT, NULL) != WOLFSSL_SUCCESS)
                ret = 0;

            /* Verify cert with CA */
            if (ret == 1) {
                wc_InitDecodedCert(&cert, bInfo->buffer, bInfo->length, NULL);
                certInit = 1;
            }
            if (ret == 1 && wc_ParseCert(&cert, CERT_TYPE, VERIFY, cm) != 0)
                ret = 0;

            if (ret == 1 && (wolfSSL_version(ssl) == TLS1_3_VERSION ||
                    wolfSSL_version(ssl) == DTLS1_3_VERSION)) {
                WOLFSSL_BUFFER_INFO* ocspStaple = wolfSSL_GetTls13OcspStatusResp(ssl, (word32)depth);
                WOLFSSL_OCSP* ocsp = NULL;

                if (ocspStaple == NULL || ocspStaple->buffer == NULL ||ocspStaple->length == 0)
                    ret = 0;
                if (ret == 1 && (ocsp = wc_NewOCSP(cm)) == NULL)
                    ret = 0;
                if (ret == 1 &&
                    wc_CheckCertOcspResponse(ocsp, &cert, ocspStaple->buffer,
                            ocspStaple->length, NULL) != 0)
                    ret = 0;
                wc_FreeOCSP(ocsp);

                if (ret == 1)
                    printf("Client: Manual OCSP staple verification succeeded at depth %d\n", depth);
                else
                    printf("Client: Manual OCSP staple verification failed at depth %d\n", depth);
            }

            if (certInit)
                wc_FreeDecodedCert(&cert);
            wolfSSL_CertManagerFree(cm);

            if (ret == 1) {
                printf("Client: Manual certificate validation succeeded at depth %d\n", depth);
                return 1;
            } else {
                printf("Client: Manual certificate validation failed at depth %d\n", depth);
                return 0;
            }
        } else {
            printf("Client: Certificate verification failed at depth %d, error %d\n", depth, err);
            return 0;
        }
    }
    return 1;
}

static int ocsp_verify_cb(WOLFSSL* ssl, int err, unsigned char* staple, unsigned int stapleSz, unsigned int idx, void* arg)
{
    (void)arg;
    if (err == 0 && staple && stapleSz > 0) {
        printf("Client: OCSP staple received, size=%u\n", stapleSz);
        return 0;
    }
    /* Manual OCSP staple verification if err != 0 */
    if (err != 0 && staple && stapleSz > 0) {
        WOLFSSL_CERT_MANAGER* cm = NULL;
        DecodedCert cert;
        byte certInit = 0;
        WOLFSSL_OCSP* ocsp;
        WOLFSSL_X509_CHAIN* peerCerts;

        cm = wolfSSL_CertManagerNew();
        if (cm == NULL)
            goto cleanup;
        if (wolfSSL_CertManagerLoadCA(cm, CA_CERT, NULL) != WOLFSSL_SUCCESS)
            goto cleanup;
        if (wolfSSL_CertManagerLoadCA(cm, INTERMEDIATE_CA_CERT, NULL) != WOLFSSL_SUCCESS)
            goto cleanup;

        peerCerts = wolfSSL_get_peer_chain(ssl);
        if (peerCerts == NULL || wolfSSL_get_chain_count(peerCerts) <= (int)idx)
            goto cleanup;

        wc_InitDecodedCert(&cert, wolfSSL_get_chain_cert(peerCerts, idx), wolfSSL_get_chain_length(peerCerts, idx), NULL);
        certInit = 1;
        if (wc_ParseCert(&cert, CERT_TYPE, VERIFY, cm) != 0)
            goto cleanup;
        if ((ocsp = wc_NewOCSP(cm)) == NULL)
            goto cleanup;
        if (wc_CheckCertOcspResponse(ocsp, &cert, staple, stapleSz, NULL) != 0)
            goto cleanup;

        printf("Client: Manual OCSP staple verification succeeded for idx=%u\n", idx);
        err = 0;
cleanup:
        wc_FreeOCSP(ocsp);
        if (certInit)
            wc_FreeDecodedCert(&cert);
        wolfSSL_CertManagerFree(cm);
        if (err == 0)
            return 0;
        printf("Client: Manual OCSP staple verification failed for idx=%u\n", idx);
    }
    printf("Client: OCSP staple verify error=%d\n", err);
    return err;
}

int main(int argc, char** argv)
{
    int sockfd = -1;
    struct sockaddr_in serv_addr;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;
    char buf[32];
    int use_tls13 = 0;

    if (argc != 2) {
        printf("Usage: %s [--tls12|--tls13]\n", argv[0]);
        return 0;
    }
    if (strcmp(argv[1], "--tls13") == 0) {
        use_tls13 = 1;
    } else if (strcmp(argv[1], "--tls12") == 0) {
        use_tls13 = 0;
    } else {
        printf("Usage: %s [--tls12|--tls13]\n", argv[0]);
        return 0;
    }

    wolfSSL_Init();
    ctx = wolfSSL_CTX_new(use_tls13 ? wolfTLSv1_3_client_method() : wolfTLSv1_2_client_method());
    if (!ctx) {
        fprintf(stderr, "wolfSSL_CTX_new (client) failed\n");
        goto cleanup;
    }
    wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, cert_verify_cb);
    if (wolfSSL_CTX_EnableOCSPStapling(ctx) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_EnableOCSPStapling failed\n");
        goto cleanup;
    }
    if (wolfSSL_CTX_EnableOCSPMustStaple(ctx) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_EnableOCSPMustStaple failed\n");
        goto cleanup;
    }
    if (use_tls13) {
        if (wolfSSL_CTX_UseOCSPStapling(ctx, WOLFSSL_CSR_OCSP, 0) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_CTX_UseOCSPStaplingV2 failed\n");
            goto cleanup;
        }
    }
    else {
        if (wolfSSL_CTX_UseOCSPStaplingV2(ctx, WOLFSSL_CSR2_OCSP_MULTI, 0) != WOLFSSL_SUCCESS) {
            fprintf(stderr, "wolfSSL_CTX_UseOCSPStaplingV2 failed\n");
            goto cleanup;
        }
    }
    wolfSSL_CTX_set_tls12_ocsp_status_verify_cb(ctx, ocsp_verify_cb, NULL);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        goto cleanup;
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) != 1) {
        fprintf(stderr, "inet_pton failed for client\n");
        goto cleanup;
    }

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        goto cleanup;
    }
    ssl = wolfSSL_new(ctx);
    if (!ssl) {
        fprintf(stderr, "wolfSSL_new (client) failed\n");
        goto cleanup;
    }
    wolfSSL_set_fd(ssl, sockfd);
    /* No way to get ssl from the store without OPENSSL_EXTRA */
    wolfSSL_SetCertCbCtx(ssl, ssl);

    if (wolfSSL_connect(ssl) == WOLFSSL_SUCCESS) {
        printf("Client: TLS handshake success\n");
        printf("Negotiated TLS version: %s\n", wolfSSL_get_version(ssl));
        int n = wolfSSL_read(ssl, buf, sizeof(buf)-1);
        if (n > 0) {
            buf[n] = 0;
            printf("Client: received: %s\n", buf);
        } else if (n < 0) {
            fprintf(stderr, "Client: wolfSSL_read failed: %s\n", wolfSSL_ERR_reason_error_string(wolfSSL_get_error(ssl, 0)));
        }
    } else {
        fprintf(stderr, "Client: TLS handshake failed: %s\n", wolfSSL_ERR_reason_error_string(wolfSSL_get_error(ssl, 0)));
    }

cleanup:
    if (ssl) wolfSSL_free(ssl);
    if (sockfd >= 0) close(sockfd);
    if (ctx) wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    return 0;
}
