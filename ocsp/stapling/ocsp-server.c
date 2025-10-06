/* ocsp-server.c
 *
 * Standalone OCSP stapling server example.
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
#include <wolfssl/ocsp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#define SERVER_CERT "server-certs/server1-cert.pem"
#define SERVER_KEY  "server-certs/server1-key.pem"
#define SERVER_ISSUER_CERT "client-certs/intermediate1-ca-cert.pem"
#define SERVER_PORT 11111
#define HTTP_TMP_BUFFER_SIZE 512
#define URL_SIZE 128

static unsigned char* ocsp_resp = NULL;
static int ocsp_resp_sz = 0;

/* This callback can be used to choose between multiple certs/keys. It can
 * be used to select certs based on SNI, ciphersuites, etc. Here we just
 * load a single cert/key. */
static int cert_cb(WOLFSSL* ssl, void* arg)
{
    (void)arg;
    if (wolfSSL_use_certificate_chain_file(ssl, SERVER_CERT) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error loading server certificate: %s\n", wolfSSL_ERR_reason_error_string(wolfSSL_get_error(ssl, 0)));
        return 0;
    }
    if (wolfSSL_use_PrivateKey_file(ssl, SERVER_KEY, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error loading server private key: %s\n", wolfSSL_ERR_reason_error_string(wolfSSL_get_error(ssl, 0)));
        return 0;
    }
    return 1;
}

int ocsp_cb(void* ctx, const char* url, int urlSz,
                        byte* ocspReqBuf, int ocspReqSz, byte** ocspRespBuf)
{
    int      httpBufSz = 0;
    byte     httpBuf[HTTP_TMP_BUFFER_SIZE];
    char     path[URL_SIZE];
    char     domainName[URL_SIZE];
    word16   port = 0;
    SOCKET_T sfd = SOCKET_INVALID;
    int      ret = -1;
    int      respSz = 0;

    if (wolfIO_DecodeUrl(url, urlSz, domainName, path, &port) != 0) {
        WOLFSSL_MSG("Unable to decode OCSP URL");
        goto cleanup;
    }

    httpBufSz = wolfIO_HttpBuildRequestOcsp(domainName, path, ocspReqSz,
        httpBuf, HTTP_TMP_BUFFER_SIZE);
    if (wolfIO_TcpConnect(&sfd, domainName, port, 0) != 0) {
        WOLFSSL_MSG("OCSP Responder connection failed");
        goto cleanup;
    }

    if (wolfIO_Send(sfd, (char*)httpBuf, httpBufSz, 0) != httpBufSz) {
        WOLFSSL_MSG("OCSP http request failed");
        goto cleanup;
    }

    if (wolfIO_Send(sfd, (char*)ocspReqBuf, ocspReqSz, 0) != ocspReqSz) {
        WOLFSSL_MSG("OCSP ocsp request failed");
        goto cleanup;
    }
    if ((respSz = wolfIO_HttpProcessResponseOcsp((int)sfd, ocspRespBuf, httpBuf,
        HTTP_TMP_BUFFER_SIZE, ctx)) <= 0) {
        WOLFSSL_MSG("OCSP http response failed");
        goto cleanup;
    }
    ocsp_resp = *ocspRespBuf;
    ocsp_resp_sz = ret = respSz;
cleanup:
    if (sfd != SOCKET_INVALID)
        CloseSocket(sfd);
    return ret;
}

static int fetch_ocsp_response(unsigned char** resp, int* respSz)
{
    int ret = -1;
    WOLFSSL_CERT_MANAGER* cm = NULL;
    unsigned char* certPem = NULL;
    unsigned char* certDer = NULL;
    long certPemSz = 0;
    int certDerSz = 0;
    FILE* f = NULL;

    *resp = NULL;
    *respSz = 0;

    f = fopen(SERVER_CERT, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open server cert: %s\n", SERVER_CERT);
        goto cleanup;
    }
    fseek(f, 0, SEEK_END);
    certPemSz = ftell(f);
    fseek(f, 0, SEEK_SET);
    certPem = (unsigned char*)malloc(certPemSz);
    if (!certPem) {
        fprintf(stderr, "malloc failed\n");
        goto cleanup;
    }
    if (fread(certPem, 1, certPemSz, f) != (size_t)certPemSz) {
        fprintf(stderr, "fread failed\n");
        goto cleanup;
    }

    certDer = (unsigned char*)malloc(certPemSz);
    if (!certDer) {
        fprintf(stderr, "malloc failed\n");
        goto cleanup;
    }
    certDerSz = wolfSSL_CertPemToDer(certPem, (int)certPemSz, certDer, (int)certPemSz, CERT_TYPE);
    if (certDerSz <= 0) {
        fprintf(stderr, "wolfSSL_CertPemToDer failed\n");
        goto cleanup;
    }

    cm = wolfSSL_CertManagerNew();
    if (!cm) {
        fprintf(stderr, "wolfSSL_CertManagerNew failed\n");
        goto cleanup;
    }
    if (wolfSSL_CertManagerEnableOCSP(cm, WOLFSSL_OCSP_NO_NONCE) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CertManagerEnableOCSP failed\n");
        goto cleanup;
    }
    if (wolfSSL_CertManagerLoadCA(cm, SERVER_ISSUER_CERT, NULL) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CertManagerLoadCA failed for issuer cert: %s\n", SERVER_ISSUER_CERT);
        goto cleanup;
    }
    wolfSSL_CertManagerSetOCSP_Cb(cm, ocsp_cb, NULL, NULL);

    /* This calls ocsp_cb to fetch the response and verifies it. ocsp_cb stores the response globally. */
    if (wolfSSL_CertManagerCheckOCSP(cm, certDer, certDerSz) == WOLFSSL_SUCCESS) {
        ret = 0;
    } else {
        fprintf(stderr, "wolfSSL_CertManagerCheckOCSP failed or OCSP not verified\n");
    }

cleanup:
    if (f) fclose(f);
    if (certPem) free(certPem);
    if (certDer) free(certDer);
    if (cm) wolfSSL_CertManagerFree(cm);
    return ret;
}

static int status_cb(WOLFSSL* ssl, void* ctx)
{
    unsigned char* resp_buf;

    (void)ctx;
    if (ocsp_resp == NULL || ocsp_resp_sz == 0)
        return WOLFSSL_OCSP_STATUS_CB_ALERT_FATAL;

    resp_buf = (unsigned char*)malloc(ocsp_resp_sz);
    if (resp_buf == NULL)
        return WOLFSSL_OCSP_STATUS_CB_ALERT_FATAL;

    memcpy(resp_buf, ocsp_resp, ocsp_resp_sz);
    if (wolfSSL_set_tlsext_status_ocsp_resp(ssl, resp_buf, ocsp_resp_sz) != WOLFSSL_SUCCESS) {
        free(resp_buf);
        return WOLFSSL_OCSP_STATUS_CB_ALERT_FATAL;
    }

    /* wolfSSL takes ownership of resp_buf, so do not free it here */
    return WOLFSSL_OCSP_STATUS_CB_OK;
}

int main()
{
    int listenfd = -1, connfd = -1;
    struct sockaddr_in serv_addr;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;
    int ret = 1;

    if (fetch_ocsp_response(&ocsp_resp, &ocsp_resp_sz) != 0) {
        fprintf(stderr, "Failed to fetch OCSP response at startup\n");
        goto cleanup;
    }

    wolfSSL_Init();
    ctx = wolfSSL_CTX_new(wolfTLS_server_method());
    if (!ctx) {
        fprintf(stderr, "wolfSSL_CTX_new failed\n");
        goto cleanup;
    }
    wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
    wolfSSL_CTX_set_cert_cb(ctx, cert_cb, NULL);

    if (wolfSSL_CTX_set_tlsext_status_cb(ctx, status_cb) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_set_tlsext_status_cb failed\n");
        goto cleanup;
    }
    if (wolfSSL_CTX_set_tlsext_status_arg(ctx, NULL) <= 0) {
        fprintf(stderr, "wolfSSL_CTX_set_tlsext_status_arg failed\n");
        goto cleanup;
    }
    if (wolfSSL_CTX_EnableOCSPStapling(ctx) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_CTX_EnableOCSPStapling failed\n");
        goto cleanup;
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        goto cleanup;
    }
    int optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt");
        goto cleanup;
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(SERVER_PORT);
    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        goto cleanup;
    }
    if (listen(listenfd, 1) < 0) {
        perror("listen");
        goto cleanup;
    }

    printf("Server: waiting for connection...\n");
    connfd = accept(listenfd, NULL, NULL);
    if (connfd < 0) {
        perror("accept");
        goto cleanup;
    }
    ssl = wolfSSL_new(ctx);
    if (!ssl) {
        fprintf(stderr, "wolfSSL_new failed\n");
        goto cleanup;
    }
    wolfSSL_set_fd(ssl, connfd);

    if (wolfSSL_accept(ssl) == WOLFSSL_SUCCESS) {
        printf("Server: TLS handshake success\n");
        printf("Negotiated TLS version: %s\n", wolfSSL_get_version(ssl));
        if (wolfSSL_write(ssl, "hello", 5) != 5) {
            fprintf(stderr, "Server: wolfSSL_write failed\n");
            goto cleanup;
        }
    } else {
        fprintf(stderr, "Server: TLS handshake failed: %s\n", wolfSSL_ERR_reason_error_string(wolfSSL_get_error(ssl, 0)));
        goto cleanup;
    }

    ret = 0;
cleanup:
    if (ssl) wolfSSL_free(ssl);
    if (connfd >= 0) close(connfd);
    if (listenfd >= 0) close(listenfd);
    if (ctx) wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    return ret;
}
