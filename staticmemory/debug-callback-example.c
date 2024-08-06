#include <stdio.h>
#include <stdlib.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/memory.h>

#ifndef WOLFSSL_STATIC_MEMORY
    #error requires --enable-staticmemory
#endif

#ifdef WOLFSSL_STATIC_MEMORY_DEBUG_CALLBACK
static void MemoryDebug(size_t reqSz, int buckSz, byte memAction, int heapType)
{
    switch (memAction) {
        case WOLFSSL_DEBUG_MEMORY_ALLOC:
            printf("Malloc request of %zu bytes using bucket size of %d and heap"
                   " type of %d\n", reqSz, buckSz, heapType);
            break;

        case WOLFSSL_DEBUG_MEMORY_FAIL:
            printf("Failed on malloc request of %zu bytes\n", reqSz);
            break;

        case WOLFSSL_DEBUG_MEMORY_FREE:
        #ifdef WOLFSSL_DEBUG_MEMORY
            printf("Free'ing %zu bytes, bucket size used was %d bytes\n",
                reqSz, buckSz);
        #else
            printf("Free'ing bucket size %d bytes\n", buckSz);
        #endif
            break;

        case WOLFSSL_DEBUG_MEMORY_INIT:
            printf("Creating bucket of size %d\n", buckSz);
            break;
    }
}

int main(int argc, char** argv)
{
    int padSz;
    const unsigned int dist[4] = {1, 1, 2, 1};
    const unsigned int buck[4] = {10, 15, 20, 25};
    byte* buf;
    byte* testPtr[5];
    int   totalMem;
    int   i;
    WOLFSSL_HEAP_HINT* heapHint = NULL;

    wolfCrypt_Init();

    wolfSSL_SetDebugMemoryCb(MemoryDebug);

    /* get padding size per bucket */
    padSz = wolfSSL_MemoryPaddingSz();
    totalMem = (padSz * 5) + buck[0] + buck[1] + buck[2] + buck[2] + buck[3]
        + sizeof(WOLFSSL_HEAP) + sizeof(WOLFSSL_HEAP_HINT)
        + (WOLFSSL_STATIC_ALIGN - 1);


    buf = (byte*)malloc(totalMem);

    /* divide up the buffer into individual buckets and manager */
    if (wc_LoadStaticMemory_ex(&heapHint, 4, buck, dist, buf, totalMem, 0,
            0) != 0) {
        printf("Failed to load up static memory\n");
    }

    /* test checking out all buckets */
    testPtr[0] = XMALLOC(6, heapHint, DYNAMIC_TYPE_HASHES);
    testPtr[1] = XMALLOC(2, heapHint, DYNAMIC_TYPE_TMP_BUFFER);
    testPtr[2] = XMALLOC(200, heapHint, DYNAMIC_TYPE_LOG);
    testPtr[3] = XMALLOC(21, heapHint, DYNAMIC_TYPE_LOG);
    testPtr[4] = XMALLOC(1, heapHint, DYNAMIC_TYPE_HMAC);

    /* free all buckets back to heap hint manager */
    for (i = 0; i < sizeof(testPtr)/sizeof(byte*); i++) {
        XFREE(testPtr[i], heapHint, DYNAMIC_TYPE_TMP_BUFFER);
    }

    free(buf);
    wolfCrypt_Cleanup();
    return 0;
}
#else
int main(int argc, char** argv)
{
    printf("Requires WOLFSSL_STATIC_MEMORY_DEBUG_CALLBACK defined\n");
    return 0;
}
#endif
