/*
 * benchmark-client-dtls13.c
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
 *
 */

#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#define USE_CERT_BUFFERS_256
#include <wolfssl/certs_test.h>

#define SERV_PORT   11111
#define PACKET_SIZE 15360  /* 15KB = 15 * 1024 */
#define DEFAULT_MAX_TRIES 100
#define SEND_INTERVAL_US 2000  /* 2ms = 2000 microseconds */
#define DTLS_MAX_RECORD_SIZE 1300  /* Very small size to avoid DTLS datagram limit */

/* Get current time in microseconds */
static long long get_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* Write data in chunks to fit DTLS record size limits */
/* Loop over wolfSSL_write to send the 15KB packet in chunks */
static int dtls_write_chunked(WOLFSSL* ssl, const unsigned char* buf, int len)
{
    int total_written = 0;
    int remaining = len;
    const unsigned char* ptr = buf;
    
    while (remaining > 0) {
        int chunk_size = (remaining > DTLS_MAX_RECORD_SIZE) ? DTLS_MAX_RECORD_SIZE : remaining;
        int written = wolfSSL_write(ssl, ptr, chunk_size);
        
        if (written < 0) {
            return written;  /* Error */
        }
        
        if (written == 0) {
            /* Should not happen with blocking socket, but handle it */
            break;
        }
        
        total_written += written;
        remaining -= written;
        ptr += written;
    }
    
    return total_written;
}

/* Read data in chunks to handle DTLS record size limits */
static int dtls_read_chunked(WOLFSSL* ssl, unsigned char* buf, int len)
{
    int total_read = 0;
    int remaining = len;
    unsigned char* ptr = buf;
    
    while (remaining > 0) {
        int chunk_size = (remaining > DTLS_MAX_RECORD_SIZE) ? DTLS_MAX_RECORD_SIZE : remaining;
        int read = wolfSSL_read(ssl, ptr, chunk_size);
        
        if (read <= 0) {
            return read;  /* Error or EOF */
        }
        
        total_read += read;
        remaining -= read;
        ptr += read;
    }
    
    return total_read;
}

/* Run a single benchmark with a specific cipher suite */
static int run_benchmark(const char* server_ip, int max_tries, 
                         const char* cipher_suite, const char* cipher_name,
                         int curve_id, int use_ecdsa_cert)
{
    (void)use_ecdsa_cert;  /* Reserved for future ECDSA certificate support */
    int             sockfd = INVALID_SOCKET;
    int             err;
    int             ret;
    int             exitVal = 1;
    struct          sockaddr_in servAddr;
    WOLFSSL*        ssl = NULL;
    WOLFSSL_CTX*    ctx = NULL;
    unsigned char   sendBuf[PACKET_SIZE];
    unsigned char   recvBuf[PACKET_SIZE];
    long long       total_send_time = 0;
    long long       total_recv_time = 0;
    long long       connection_time = 0;
    int             successful_sends = 0;
    int             successful_recvs = 0;
    int             send_errors = 0;
    int             recv_errors = 0;
    int             size_mismatches = 0;
    long long       send_start, send_end, recv_start, recv_end;
    long long       conn_start, conn_end;
    int             recvlen;
    int             first_send_error = 0;
    int             first_recv_error = 0;

    if ((ctx = wolfSSL_CTX_new(
#ifdef WOLFSSL_DTLS13
            wolfDTLSv1_3_client_method()
#else
            wolfDTLSv1_2_client_method()
#endif
            )) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        goto cleanup;
    }

    /* Set cipher suite */
    if (wolfSSL_CTX_set_cipher_list(ctx, cipher_suite) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error setting cipher suite: %s\n", cipher_suite);
        goto cleanup;
    }
    
    /* Set supported groups (key exchange) */
    if (wolfSSL_CTX_UseSupportedCurve(ctx, curve_id) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error setting supported curve\n");
        goto cleanup;
    }
    
    /* Load CA certificate from memory buffer */
    if (wolfSSL_CTX_load_verify_buffer(ctx, ca_ecc_cert_der_256, sizeof_ca_ecc_cert_der_256,
            WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "Error loading CA certificate from buffer.\n");
        goto cleanup;
    }

    /* Assign ssl variable */
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        fprintf(stderr, "unable to get ssl object\n");
        goto cleanup;
    }

    /* servAddr setup */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, server_ip, &servAddr.sin_addr) < 1) {
        perror("inet_pton()");
        goto cleanup;
    }

    if (wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr))
            != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_dtls_set_peer failed\n");
        goto cleanup;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket()");
        goto cleanup;
    }

    /* Increase socket buffer sizes to handle 15KB packets */
    int buffer_size = PACKET_SIZE * 10;  /* 10x packet size for buffer */
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size)) < 0) {
        perror("setsockopt SO_SNDBUF failed");
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) < 0) {
        perror("setsockopt SO_RCVBUF failed");
    }

    /* Set the file descriptor for ssl */
    if (wolfSSL_set_fd(ssl, sockfd) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "cannot set socket file descriptor\n");
        goto cleanup;
    }

    /* Measure connection establishment time (handshake) */
    conn_start = get_time_us();
    if (wolfSSL_connect(ssl) != WOLFSSL_SUCCESS) {
        err = wolfSSL_get_error(ssl, 0);
        fprintf(stderr, "err = %d, %s\n", err, wolfSSL_ERR_reason_error_string(err));
        fprintf(stderr, "wolfSSL_connect failed\n");
        goto cleanup;
    }
    conn_end = get_time_us();
    connection_time = conn_end - conn_start;

    /* Initialize send buffer with some data */
    memset(sendBuf, 0xAA, PACKET_SIZE);

    printf("\n=== Testing Cipher: %s ===\n", cipher_name);
    printf("Keyshare: %s\n", 
           curve_id == WOLFSSL_ECC_SECP256R1 ? "secp256r1" :
           curve_id == WOLFSSL_ECC_SECP384R1 ? "secp384r1" :
           curve_id == WOLFSSL_ECC_SECP521R1 ? "secp521r1" :
           curve_id == WOLFSSL_ECC_X25519 ? "x25519" :
           curve_id == WOLFSSL_ECC_X448 ? "x448" : "unknown");

    /* Send and receive packets */
    for (int i = 0; i < max_tries; i++) {
        /* Measure send time - no printf/perror in this section */
        send_start = get_time_us();
        ret = dtls_write_chunked(ssl, sendBuf, PACKET_SIZE);
        send_end = get_time_us();
        
        if (ret != PACKET_SIZE) {
            if (ret < 0) {
                send_errors++;
                if (first_send_error == 0) {
                    first_send_error = wolfSSL_get_error(ssl, 0);
                }
            } else {
                size_mismatches++;
            }
            continue;
        }
        total_send_time += (send_end - send_start);
        successful_sends++;

        /* Measure receive time - no printf/perror in this section */
        recv_start = get_time_us();
        recvlen = dtls_read_chunked(ssl, recvBuf, PACKET_SIZE);
        recv_end = get_time_us();
        
        if (recvlen < 0) {
            recv_errors++;
            if (first_recv_error == 0) {
                first_recv_error = wolfSSL_get_error(ssl, 0);
            }
        } else if (recvlen == PACKET_SIZE) {
            total_recv_time += (recv_end - recv_start);
            successful_recvs++;
        } else {
            size_mismatches++;
        }

        /* Wait 2ms before next send (except for the last iteration) */
        if (i < max_tries - 1) {
            usleep(SEND_INTERVAL_US);
        }
    }

    /* Calculate and print averages - all output after benchmark */
    printf("Connection establishment time: %.3f microseconds (%.3f ms)\n",
           (double)connection_time, (double)connection_time / 1000.0);
    printf("Total packets sent: %d\n", successful_sends);
    printf("Total packets received: %d\n", successful_recvs);
    
    if (send_errors > 0) {
        printf("Send errors: %d", send_errors);
        if (first_send_error != 0) {
            printf(" (first error: %d - %s)", first_send_error,
                   wolfSSL_ERR_reason_error_string(first_send_error));
        }
        printf("\n");
    }
    if (recv_errors > 0) {
        printf("Receive errors: %d", recv_errors);
        if (first_recv_error != 0) {
            printf(" (first error: %d - %s)", first_recv_error,
                   wolfSSL_ERR_reason_error_string(first_recv_error));
        }
        printf("\n");
    }
    if (size_mismatches > 0) {
        printf("Size mismatches: %d\n", size_mismatches);
    }
    
    if (successful_sends > 0) {
        double avg_send_us = (double)total_send_time / successful_sends;
        printf("Average send time: %.3f microseconds (%.3f ms)\n", 
               avg_send_us, avg_send_us / 1000.0);
    } else {
        printf("No successful sends to calculate average\n");
    }

    if (successful_recvs > 0) {
        double avg_recv_us = (double)total_recv_time / successful_recvs;
        printf("Average receive time: %.3f microseconds (%.3f ms)\n", 
               avg_recv_us, avg_recv_us / 1000.0);
    } else {
        printf("No successful receives to calculate average\n");
    }

    exitVal = 0;
cleanup:
    if (ssl != NULL) {
        /* Attempt a full shutdown */
        ret = wolfSSL_shutdown(ssl);
        if (ret == WOLFSSL_SHUTDOWN_NOT_DONE)
            ret = wolfSSL_shutdown(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            err = wolfSSL_get_error(ssl, 0);
            fprintf(stderr, "err = %d, %s\n", err,
                wolfSSL_ERR_reason_error_string(err));
            fprintf(stderr, "wolfSSL_shutdown failed\n");
        }
        wolfSSL_free(ssl);
    }
    if (sockfd != INVALID_SOCKET)
        close(sockfd);
    if (ctx != NULL)
        wolfSSL_CTX_free(ctx);

    return exitVal;
}

int main(int argc, char** argv)
{
    int             exitVal = 1;
    int             max_tries = DEFAULT_MAX_TRIES;
    int             curve_id = WOLFSSL_ECC_SECP256R1;  /* Default ECC curve */
    
    /* Define all cipher suites to test */
    const char* ciphers[] = {
        "TLS13-AES128-GCM-SHA256",
        "TLS13-AES256-GCM-SHA384",
        "TLS13-CHACHA20-POLY1305-SHA256"
    };
    const char* cipher_names[] = {
        "AES128-GCM",
        "AES256-GCM",
        "ChaCha20-Poly1305"
    };
    int num_ciphers = 3;
    
    /* Program argument checking */
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s <IP address> [max_tries]\n", argv[0]);
        fprintf(stderr, "  max_tries defaults to %d\n", DEFAULT_MAX_TRIES);
        fprintf(stderr, "\nNote: Will automatically test all three ciphers: AES128, AES256, ChaCha20\n");
        return exitVal;
    }

    if (argc >= 3) {
        max_tries = atoi(argv[2]);
        if (max_tries <= 0) {
            fprintf(stderr, "Error: max_tries must be positive\n");
            return exitVal;
        }
    }
    

    /* Initialize wolfSSL */
    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_Init error.\n");
        return exitVal;
    }

    printf("Starting DTLS 1.3 benchmark: %d packets of %d bytes, %d us interval\n",
           max_tries, PACKET_SIZE, SEND_INTERVAL_US);
    printf("Server: %s:%d\n", argv[1], SERV_PORT);
    printf("Testing all three cipher suites automatically...\n\n");

    /* Run benchmark for each cipher suite */
    for (int i = 0; i < num_ciphers; i++) {
        if (run_benchmark(argv[1], max_tries, ciphers[i], cipher_names[i], 
                         curve_id, 1) != 0) {
            fprintf(stderr, "Benchmark failed for %s\n", cipher_names[i]);
            exitVal = 1;
        }
    }

    printf("\n=== All benchmarks completed ===\n");
    wolfSSL_Cleanup();
    return 0;
}
