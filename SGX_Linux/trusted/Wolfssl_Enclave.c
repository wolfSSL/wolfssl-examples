/* Wolfssl_Enclave.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include "Wolfssl_Enclave_t.h"

#include "sgx_trts.h"


#if defined(XMALLOC_USER) || defined(XMALLOC_OVERRIDE)
    #warning verification of heap hint pointers needed when overriding default malloc/free
#endif


/* Max number of WOLFSSL_CTX's */
#ifndef MAX_WOLFSSL_CTX
#define MAX_WOLFSSL_CTX 2
#endif
WOLFSSL_CTX* CTX_TABLE[MAX_WOLFSSL_CTX];

/* Max number of WOLFSSL's */
#ifndef MAX_WOLFSSL
#define MAX_WOLFSSL 2
#endif
WOLFSSL* SSL_TABLE[MAX_WOLFSSL];

/* returns ID assigned on success and -1 on failure
 * @TODO mutex for threaded use cases */
static long AddCTX(WOLFSSL_CTX* ctx)
{
    long i;
    for (i = 0; i < MAX_WOLFSSL_CTX; i++) {
         if (CTX_TABLE[i] == NULL) {
             CTX_TABLE[i] = ctx;
             return i;
         }
    }
    return -1;
}


/* returns ID assigned on success and -1 on failure
 * @TODO mutex for threaded use cases */
static long AddSSL(WOLFSSL* ssl)
{
    long i;
    for (i = 0; i < MAX_WOLFSSL; i++) {
         if (SSL_TABLE[i] == NULL) {
             SSL_TABLE[i] = ssl;
             return i;
         }
    }
    return -1;
}


/* returns the WOLFSSL_CTX pointer on success and NULL on failure */
static WOLFSSL_CTX* GetCTX(long id)
{
    if (id >= MAX_WOLFSSL_CTX || id < 0)
        return NULL;
    return CTX_TABLE[id];
}


/* returns the WOLFSSL pointer on success and NULL on failure */
static WOLFSSL* GetSSL(long id)
{
    if (id >= MAX_WOLFSSL || id < 0)
        return NULL;
    return SSL_TABLE[id];
}


/* Free's and removes the WOLFSSL_CTX associated with 'id' */
static void RemoveCTX(long id)
{
    if (id >= MAX_WOLFSSL_CTX || id < 0)
        return;
    wolfSSL_CTX_free(CTX_TABLE[id]);
    CTX_TABLE[id] = NULL;
}


/* Free's and removes the WOLFSSL associated with 'id' */
static void RemoveSSL(long id)
{
    if (id >= MAX_WOLFSSL || id < 0)
        return;
    wolfSSL_free(SSL_TABLE[id]);
    SSL_TABLE[id] = NULL;
}

#if defined(WOLFSSL_STATIC_MEMORY)
/* check on heap hint when used, aborts if pointer is not in Enclave.
 * In the default case where wolfSSL_Malloc is used the heap hint pointer is not
 * used.*/
static void checkHeapHint(WOLFSSL_CTX* ctx, WOLFSSL* ssl)
{
    WOLFSSL_HEAP_HINT* heap;
    if ((heap = (WOLFSSL_HEAP_HINT*)wolfSSL_CTX_GetHeap(ctx, ssl)) != NULL) {
        if(sgx_is_within_enclave(heap, sizeof(WOLFSSL_HEAP_HINT)) != 1)
            abort();
        if(sgx_is_within_enclave(heap->memory, sizeof(WOLFSSL_HEAP)) != 1)
            abort();
    }
}
#endif /* WOLFSSL_STATIC_MEMORY */


int wc_test(void* args)
{
#ifdef HAVE_WOLFSSL_TEST
	return wolfcrypt_test(args);
#else
    /* wolfSSL test not compiled in! */
    return -1;
#endif /* HAVE_WOLFSSL_TEST */
}

int wc_benchmark_test(void* args)
{

#ifdef HAVE_WOLFSSL_BENCHMARK
    return benchmark_test(args);
#else
    /* wolfSSL benchmark not compiled in! */
    return -1;
#endif /* HAVE_WOLFSSL_BENCHMARK */
}

void enc_wolfSSL_Debugging_ON(void)
{
    wolfSSL_Debugging_ON();
}

void enc_wolfSSL_Debugging_OFF(void)
{
    wolfSSL_Debugging_OFF();
}

int enc_wolfSSL_Init(void)
{
    return wolfSSL_Init();
}


#define WOLFTLSv12_CLIENT 1
#define WOLFTLSv12_SERVER 2

long enc_wolfTLSv1_2_client_method(void)
{
    return WOLFTLSv12_CLIENT;
}

long enc_wolfTLSv1_2_server_method(void)
{
    return WOLFTLSv12_SERVER;
}


/* returns method related to id */
static WOLFSSL_METHOD* GetMethod(long id)
{
    switch (id) {
        case WOLFTLSv12_CLIENT: return wolfTLSv1_2_client_method();
        case WOLFTLSv12_SERVER: return wolfTLSv1_2_server_method();
        default:
            return NULL;
    }
}


long enc_wolfSSL_CTX_new(long method)
{
    WOLFSSL_CTX* ctx;
    long id = -1;

    ctx = wolfSSL_CTX_new(GetMethod(method));
    if (ctx != NULL) {
        id = AddCTX(ctx);
    }
    return id;
}

int enc_wolfSSL_CTX_use_certificate_chain_buffer_format(long id,
        const unsigned char* buf, long sz, int type)
{
    WOLFSSL_CTX* ctx = GetCTX(id);
    if (ctx == NULL) {
        return -1;
    }
    return wolfSSL_CTX_use_certificate_chain_buffer_format(ctx, buf, sz, type);
}

int enc_wolfSSL_CTX_use_certificate_buffer(long id,
        const unsigned char* buf, long sz, int type)
{
    WOLFSSL_CTX* ctx = GetCTX(id);
    if (ctx == NULL) {
        return -1;
    }
    return wolfSSL_CTX_use_certificate_buffer(ctx, buf, sz, type);
}

int enc_wolfSSL_CTX_use_PrivateKey_buffer(long id, const unsigned char* buf,
                                            long sz, int type)
{
    WOLFSSL_CTX* ctx = GetCTX(id);
    if (ctx == NULL) {
        return -1;
    }
    return wolfSSL_CTX_use_PrivateKey_buffer(ctx, buf, sz, type);
}

int enc_wolfSSL_CTX_load_verify_buffer(long id, const unsigned char* in,
                                       long sz, int format)
{
    WOLFSSL_CTX* ctx = GetCTX(id);
    if (ctx == NULL) {
        return -1;
    }
    return wolfSSL_CTX_load_verify_buffer(ctx, in, sz, format);
}


int enc_wolfSSL_CTX_set_cipher_list(long id, const char* list)
{
    WOLFSSL_CTX* ctx = GetCTX(id);
    if (ctx == NULL) {
        return -1;
    }
    return wolfSSL_CTX_set_cipher_list(ctx, list);
}

long enc_wolfSSL_new(long id)
{
    WOLFSSL_CTX* ctx;
    WOLFSSL* ssl;
    long ret = -1;

    ctx = GetCTX(id);
    if (ctx == NULL) {
        return -1;
    }
    ssl = wolfSSL_new(ctx);
    if (ssl != NULL) {
        ret = AddSSL(ssl);
    }
    return ret;
}

int enc_wolfSSL_set_fd(long sslId, int fd)
{
    WOLFSSL* ssl = GetSSL(sslId);
    if (ssl == NULL) {
        return -1;
    }
    return wolfSSL_set_fd(ssl, fd);
}

int enc_wolfSSL_connect(long sslId)
{
    WOLFSSL* ssl = GetSSL(sslId);
    if (ssl == NULL) {
        return -1;
    }
    return wolfSSL_connect(ssl);
}

int enc_wolfSSL_write(long sslId, const void* in, int sz)
{
    WOLFSSL* ssl = GetSSL(sslId);
    if (ssl == NULL) {
        return -1;
    }
    return wolfSSL_write(ssl, in, sz);
}

int enc_wolfSSL_get_error(long sslId, int ret)
{
    WOLFSSL* ssl = GetSSL(sslId);
    if (ssl == NULL) {
        return -1;
    }
    return wolfSSL_get_error(ssl, ret);
}

int enc_wolfSSL_read(long sslId, void* data, int sz)
{
    WOLFSSL* ssl = GetSSL(sslId);
    if (ssl == NULL) {
        return -1;
    }
    return wolfSSL_read(ssl, data, sz);
}

void enc_wolfSSL_free(long sslId)
{
    RemoveSSL(sslId);
}

void enc_wolfSSL_CTX_free(long id)
{
    RemoveCTX(id);
}

int enc_wolfSSL_Cleanup(void)
{
    long id;

    /* free up all WOLFSSL's */
    for (id = 0; id < MAX_WOLFSSL; id++)
        RemoveSSL(id);

    /* free up all WOLFSSL_CTX's */
    for (id = 0; id < MAX_WOLFSSL_CTX; id++)
        RemoveCTX(id);
    wolfSSL_Cleanup();
}

void printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

int sprintf(char* buf, const char *fmt, ...)
{
    va_list ap;
    int ret;
    va_start(ap, fmt);
    ret = vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    return ret;
}

double current_time(void)
{
    double curr;
    ocall_current_time(&curr);
    return curr;
}

int LowResTimer(void) /* low_res timer */
{
    int time;
    ocall_low_res_time(&time);
    return time;
}

size_t recv(int sockfd, void *buf, size_t len, int flags)
{
    size_t ret;
    int sgxStatus;
    sgxStatus = ocall_recv(&ret, sockfd, buf, len, flags);
    return ret;
}

size_t send(int sockfd, const void *buf, size_t len, int flags)
{
    size_t ret;
    int sgxStatus;
    sgxStatus = ocall_send(&ret, sockfd, buf, len, flags);
    return ret;
}
