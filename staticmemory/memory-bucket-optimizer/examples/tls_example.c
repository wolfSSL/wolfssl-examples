/* tls_example.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/memory.h>

/* Example optimized bucket configurations */
/* TLS 1.2 Configuration */
#define TLS12_BUCKETS "16,22,30,40,86,133,184,256,344,512,864,1248,1812,3128,5518,8368"
#define TLS12_DIST    "2,3,7,7,7,7,7,7,7,7,7,7,8,8,8,8"

/* TLS 1.3 Configuration */
#define TLS13_BUCKETS "16,24,32,48,96,144,192,288,384,576,864,1296,1944,2916,4374,6561"
#define TLS13_DIST    "2,3,7,7,7,7,7,7,7,7,7,7,8,8,8,8"

/* Buffer size for static memory */
#define STATIC_MEM_SIZE (1024*1024) /* 1MB */

#ifdef WOLFSSL_STATIC_MEMORY
/* Static memory buffers */
static unsigned char gStaticMemoryTLS12[STATIC_MEM_SIZE];
static unsigned char gStaticMemoryTLS13[STATIC_MEM_SIZE];

/* Print memory usage statistics */
static void print_memory_stats(const char* name, void* heap, int total_size) {
    int used;
    
    printf("\nMemory Usage Statistics for %s:\n", name);
    printf("Total Memory:     %d bytes\n", total_size);
    
    used = wolfSSL_GetStaticMemoryUsed(heap);
    printf("Used Memory:      %d bytes\n", used);
    printf("Available Memory: %d bytes\n", total_size - used);
    printf("\n");
}
#endif

int main(void) {
    int ret = 0;

#ifdef WOLFSSL_STATIC_MEMORY
    WOLFSSL_METHOD* method12 = NULL;
    WOLFSSL_METHOD* method13 = NULL;
    WOLFSSL_CTX* ctx12 = NULL;
    WOLFSSL_CTX* ctx13 = NULL;
    void* heap12 = NULL;
    void* heap13 = NULL;

    /* Initialize wolfSSL */
    wolfSSL_Init();

    printf("Static Memory Example\n");
    printf("====================\n\n");

    /* Print configurations */
    printf("TLS 1.2 Configuration:\n");
    printf("WOLFMEM_BUCKETS: %s\n", TLS12_BUCKETS);
    printf("WOLFMEM_DIST:    %s\n\n", TLS12_DIST);

    printf("TLS 1.3 Configuration:\n");
    printf("WOLFMEM_BUCKETS: %s\n", TLS13_BUCKETS);
    printf("WOLFMEM_DIST:    %s\n\n", TLS13_DIST);

    /* Initialize TLS 1.2 static memory */
    ret = wolfSSL_CTX_load_static_memory(&method12, NULL,
                                       gStaticMemoryTLS12,
                                       STATIC_MEM_SIZE, 0, 1);
    if (ret != WOLFSSL_SUCCESS) {
        printf("Error: Failed to load TLS 1.2 static memory\n");
        ret = -1;
        goto cleanup;
    }

    /* Initialize TLS 1.3 static memory */
    ret = wolfSSL_CTX_load_static_memory(&method13, NULL,
                                       gStaticMemoryTLS13,
                                       STATIC_MEM_SIZE, 0, 1);
    if (ret != WOLFSSL_SUCCESS) {
        printf("Error: Failed to load TLS 1.3 static memory\n");
        ret = -1;
        goto cleanup;
    }

    /* Create TLS 1.2 context */
    if ((ctx12 = wolfSSL_CTX_new(method12)) == NULL) {
        printf("Error: Failed to create TLS 1.2 context\n");
        ret = -1;
        goto cleanup;
    }

    /* Create TLS 1.3 context */
    if ((ctx13 = wolfSSL_CTX_new(method13)) == NULL) {
        printf("Error: Failed to create TLS 1.3 context\n");
        ret = -1;
        goto cleanup;
    }

    /* Get heap statistics */
    if (wolfSSL_CTX_GetStatics(ctx12, &heap12) != 1) {
        printf("Error: Failed to get TLS 1.2 heap statistics\n");
        ret = -1;
        goto cleanup;
    }

    if (wolfSSL_CTX_GetStatics(ctx13, &heap13) != 1) {
        printf("Error: Failed to get TLS 1.3 heap statistics\n");
        ret = -1;
        goto cleanup;
    }

    /* Print memory usage statistics */
    print_memory_stats("TLS 1.2", heap12, STATIC_MEM_SIZE);
    print_memory_stats("TLS 1.3", heap13, STATIC_MEM_SIZE);

cleanup:
    if (ctx12) wolfSSL_CTX_free(ctx12);
    if (ctx13) wolfSSL_CTX_free(ctx13);
    wolfSSL_Cleanup();
#else
    printf("Error: This example requires wolfSSL to be built with "
           "--enable-staticmemory\n");
    printf("\nTo rebuild wolfSSL with static memory support:\n");
    printf("  cd ../../../wolfssl\n");
    printf("  ./configure --enable-staticmemory\n");
    printf("  make\n\n");
    printf("Example bucket configurations that will be used:\n\n");
    printf("TLS 1.2:\n");
    printf("  WOLFMEM_BUCKETS: %s\n", TLS12_BUCKETS);
    printf("  WOLFMEM_DIST:    %s\n\n", TLS12_DIST);
    printf("TLS 1.3:\n");
    printf("  WOLFMEM_BUCKETS: %s\n", TLS13_BUCKETS);
    printf("  WOLFMEM_DIST:    %s\n", TLS13_DIST);
    ret = -1;
#endif

    return ret;
}
