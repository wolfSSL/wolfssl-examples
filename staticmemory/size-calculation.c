#include <stdio.h>
#include <stdlib.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/memory.h>

#ifndef WOLFSSL_STATIC_MEMORY
    #error requires --enable-staticmemory
#endif

int main(int argc, char** argv)
{
    int padSz;
    const unsigned int dist[4] = {1, 1, 2, 1};
    const unsigned int buck[4] = {10, 15, 20, 25};
    byte* buf;
    byte* testPtr[5];
    int   buckTotalMem = 0; /* sum of bucket sizes */
    int   totalMem;         /* sum of buckets and padding */
    int   totalMemAll;      /* sum of buckets, padding, and managing structs */
    int   i, k = 0;
    WOLFSSL_HEAP_HINT* heapHint = NULL;

    wolfCrypt_Init();

    /* get padding size per bucket */
    padSz = wolfSSL_MemoryPaddingSz();
    printf("Padding size per bucket is %d\n", padSz);

    /* calculate total size needed for individual bucket structs */
    padSz = padSz * 5;
    printf("Total padding needed for 5 buckets would be %d\n", padSz);

    /* sum of all bucket sizes */
    buckTotalMem = buck[0] + buck[1] + buck[2] + buck[2] + buck[3];
    printf("Total memory in buckets is %d\n", buckTotalMem);

    /* adding sum of bucket sizes with struct padding size */
    totalMem = buckTotalMem + padSz;
    printf("Calculated total memory for buckets is %d\n", totalMem);

    /* adding in size of over all managing structs used */
    totalMemAll = totalMem + sizeof(WOLFSSL_HEAP) + sizeof(WOLFSSL_HEAP_HINT);

    /* account for max size needed for alignment of pointer */
    totalMemAll += WOLFSSL_STATIC_ALIGN - 1;

    printf("Calculated total memory for buckets, heap hint struct and "
            "alignment is %d\n", totalMemAll);

    buf = (byte*)malloc(totalMemAll);
    printf("Return of static buffer sz is %d\n",
        wolfSSL_StaticBufferSz_ex(4, buck, dist, buf, totalMem, 0));
    printf("\n");

    /* divide up the buffer into individual buckets and manager */
    if (wc_LoadStaticMemory_ex(&heapHint, 4, buck, dist, buf, totalMemAll, 0,
            0) != 0) {
        printf("Failed to load up static memory\n");
    }

    /* test checking out all buckets */
    for (i = 0; i < sizeof(testPtr)/sizeof(byte*); i++) {
        printf("Checking out bucket %d size of %d...", i, buck[k]);
        testPtr[i] = (byte*)XMALLOC(buck[k], heapHint, DYNAMIC_TYPE_TMP_BUFFER);
        if (testPtr[i] == NULL) {
            printf("fail\n");
        }
        else {
            printf("ok\n");
        }

        k++;
        if (i == 2) k--;
    }

    /* free all buckets back to heap hint manager */
    for (i = 0; i < sizeof(testPtr)/sizeof(byte*); i++) {
        XFREE(testPtr[i], heapHint, DYNAMIC_TYPE_TMP_BUFFER);
    }

    free(buf);
    wolfCrypt_Cleanup();
    return 0;
}
