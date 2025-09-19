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

static unsigned char* ocsp_resp = NULL;
static int ocsp_resp_sz = 0;

static int cert_cb(WOLFSSL* ssl, void* arg)
{
    (void)arg;
    if (wolfSSL_use_certificate_file(ssl, SERVER_CERT, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS) {
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
    (void)ctx;
    if (url == NULL || urlSz <= 0 ||ocspReqBuf == NULL || ocspReqSz <= 0 ||
        ocspRespBuf == NULL) {
        fprintf(stderr, "ocsp_cb: invalid input\n");
        return -1;
    }

    // Only support http://
    const char* prefix = "http://";
    size_t prefix_len = strlen(prefix);
    if (urlSz <= (int)prefix_len || strncmp(url, prefix, prefix_len) != 0) {
        fprintf(stderr, "ocsp_cb: only http:// URLs are supported\n");
        return -1;
    }

    // Find domain and port
    const char* host_start = url + prefix_len;
    const char* url_end = url + urlSz;
    const char* colon = memchr(host_start, ':', url_end - host_start);
    if (!colon) {
        fprintf(stderr, "ocsp_cb: URL missing port\n");
        return -1;
    }
    const char* slash = memchr(colon, '/', url_end - colon);
    size_t domain_len = colon - host_start;
    size_t port_len = (slash ? (size_t)(slash - colon - 1) : (size_t)(url_end - colon - 1));

    if (domain_len == 0 || port_len == 0) {
        fprintf(stderr, "ocsp_cb: invalid domain or port in URL\n");
        return -1;
    }

    char domain[256];
    char port[16];
    if (domain_len >= sizeof(domain) || port_len >= sizeof(port)) {
        fprintf(stderr, "ocsp_cb: domain or port too long\n");
        return -1;
    }
    memcpy(domain, host_start, domain_len);
    domain[domain_len] = '\0';
    memcpy(port, colon + 1, port_len);
    port[port_len] = '\0';

    // Resolve domain and port to IP address
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    int gai_ret = getaddrinfo(domain, port, &hints, &res);
    if (gai_ret != 0) {
        fprintf(stderr, "ocsp_cb: getaddrinfo failed: %s\n", gai_strerror(gai_ret));
        return -1;
    }

    // Create a socket
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        perror("ocsp_cb: socket");
        freeaddrinfo(res);
        return -1;
    }

    // Connect to the server
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("ocsp_cb: connect");
        close(sock);
        freeaddrinfo(res);
        return -1;
    }

    // Prepare HTTP POST header
    char http_header[512];
    int header_len = snprintf(
        http_header, sizeof(http_header),
        "POST / HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Content-Type: application/ocsp-request\r\n"
        "Content-Length: %d\r\n"
        "\r\n",
        domain, ocspReqSz
    );
    if (header_len < 0 || (size_t)header_len >= sizeof(http_header)) {
        fprintf(stderr, "ocsp_cb: HTTP header too long\n");
        close(sock);
        freeaddrinfo(res);
        return -1;
    }

    // Send HTTP header
    if (send(sock, http_header, header_len, 0) != header_len) {
        perror("ocsp_cb: send header");
        close(sock);
        freeaddrinfo(res);
        return -1;
    }
    // Send OCSP request body
    if (send(sock, ocspReqBuf, ocspReqSz, 0) != ocspReqSz) {
        perror("ocsp_cb: send body");
        close(sock);
        freeaddrinfo(res);
        return -1;
    }
    // Read HTTP response
    char resp_buf[4096];
    int resp_len = 0;
    int n;
    while ((n = recv(sock, resp_buf + resp_len, sizeof(resp_buf) - resp_len, 0)) > 0) {
        resp_len += n;
        if (resp_len >= (int)sizeof(resp_buf)) {
            fprintf(stderr, "ocsp_cb: response too large\n");
            close(sock);
            freeaddrinfo(res);
            return -1;
        }
    }
    if (n < 0) {
        perror("ocsp_cb: recv");
        close(sock);
        freeaddrinfo(res);
        return -1;
    }
    close(sock);
    freeaddrinfo(res);

    // Find end of HTTP headers
    char* body = NULL;
    int body_len = 0;
    char* header_end = NULL;
    header_end = strstr(resp_buf, "\r\n\r\n");
    if (!header_end) {
        fprintf(stderr, "ocsp_cb: malformed HTTP response\n");
        return -1;
    }
    body = header_end + 4;
    body_len = resp_len - (body - resp_buf);
    if (body_len <= 0) {
        fprintf(stderr, "ocsp_cb: empty HTTP body\n");
        return -1;
    }

    // Allocate and copy OCSP response body
    ocsp_resp = *ocspRespBuf = (byte*)malloc(body_len);
    ocsp_resp_sz = body_len;
    if (!*ocspRespBuf) {
        fprintf(stderr, "ocsp_cb: malloc failed\n");
        return -1;
    }
    memcpy(*ocspRespBuf, body, body_len);

    // Return the length of the OCSP response body
    return body_len;
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

    /* Note: This does not return the raw OCSP response, just verifies. */
    if (wolfSSL_CertManagerCheckOCSP(cm, certDer, certDerSz) == WOLFSSL_SUCCESS) {
        /* For real stapling, you would fetch and set the actual OCSP response here. */
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

    // wolfSSL takes ownership of resp_buf, so do not free it here
    return WOLFSSL_OCSP_STATUS_CB_OK;
}

int main()
{
    int listenfd = -1, connfd = -1;
    struct sockaddr_in serv_addr;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;

    if (fetch_ocsp_response(&ocsp_resp, &ocsp_resp_sz) != 0) {
        fprintf(stderr, "Failed to fetch OCSP response at startup\n");
        goto cleanup;
    }

    wolfSSL_Init();
    ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
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
        if (wolfSSL_write(ssl, "hello", 5) != 5) {
            fprintf(stderr, "Server: wolfSSL_write failed\n");
        }
    } else {
        fprintf(stderr, "Server: TLS handshake failed: %s\n", wolfSSL_ERR_reason_error_string(wolfSSL_get_error(ssl, 0)));
    }

cleanup:
    if (ssl) wolfSSL_free(ssl);
    if (connfd >= 0) close(connfd);
    if (listenfd >= 0) close(listenfd);
    if (ctx) wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    return 0;
}
