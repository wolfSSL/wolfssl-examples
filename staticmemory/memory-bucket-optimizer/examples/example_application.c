/* example_application.c
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

/* Check if static memory is enabled */
#ifdef WOLFSSL_STATIC_MEMORY
    #define STATIC_MEMORY_ENABLED 1
#else
    #define STATIC_MEMORY_ENABLED 0
    #warning "This example requires wolfSSL to be built with --enable-staticmemory"
#endif

/* Example optimized bucket configuration for TLS 1.2 client */
#define WOLFMEM_BUCKET_SIZES "16,22,30,40,86,133,184,256,344,512,864,1248,1812,3128,5518,8368"
#define WOLFMEM_DIST_SIZES  "2,3,7,7,7,7,7,7,7,7,7,7,8,8,8,8"

/* Buffer size for static memory */
#define STATIC_MEM_SIZE (1024*1024) /* 1MB */

/* Static memory buffer */
static unsigned char gStaticMemory[STATIC_MEM_SIZE];

int main(void) {
    int ret = 0;
#ifdef WOLFSSL_STATIC_MEMORY
    WOLFSSL_METHOD* method = NULL;
    WOLFSSL_CTX* ctx = NULL;
    void* heap = NULL;

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* Print memory configuration */
    printf("Static Memory Configuration:\n");
    printf("WOLFMEM_BUCKETS: %s\n", WOLFMEM_BUCKET_SIZES);
    printf("WOLFMEM_DIST:    %s\n", WOLFMEM_DIST_SIZES);
    printf("Total Memory:     %d bytes\n\n", STATIC_MEM_SIZE);

    /* Initialize static memory */
    ret = wolfSSL_CTX_load_static_memory(&method, NULL, gStaticMemory,
                                       STATIC_MEM_SIZE, 0, 1);
    if (ret != WOLFSSL_SUCCESS) {
        printf("Error: Failed to load static memory\n");
        ret = -1;
        goto cleanup;
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(method)) == NULL) {
        printf("Error: Failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto cleanup;
    }

    /* Get heap statistics */
    if (wolfSSL_CTX_GetStatics(ctx, &heap) != 1) {
        printf("Error: Failed to get heap statistics\n");
        ret = -1;
        goto cleanup;
    }

    /* Print memory usage statistics */
    printf("Memory Usage Statistics:\n");
    printf("Total Memory:     %d bytes\n", STATIC_MEM_SIZE);
    printf("Used Memory:      %d bytes\n", wolfSSL_GetStaticMemoryUsed(heap));
    printf("Available Memory: %d bytes\n",
           STATIC_MEM_SIZE - wolfSSL_GetStaticMemoryUsed(heap));

cleanup:
    if (ctx) wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
#else
    printf("Error: This example requires wolfSSL to be built with "
           "--enable-staticmemory\n");
    printf("Please rebuild wolfSSL with static memory support:\n");
    printf("  cd ../../../wolfssl\n");
    printf("  ./configure --enable-staticmemory\n");
    printf("  make\n");
    ret = -1;
#endif

    return ret;
}
