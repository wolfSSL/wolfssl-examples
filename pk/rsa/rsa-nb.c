/* rsa-nb.c
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/*
* An implementation of RSA non-blocking wolfSSL
* Usage:
./rsa-nb
*/

#include <stdio.h>
#include <string.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

static const char* kRsaKey = "../../certs/client-key.der";
#define RSA_TEST_BYTES 256

#define ENABLE_RSA_NONBLOCK_TIME

#if !defined(NO_RSA) && defined(USE_FAST_MATH) && defined(WC_RSA_NONBLOCK)
static int load_file_to_buffer(const char* filename, byte** fileBuf, int* fileLen)
{
    int ret = 0;
    FILE* file = NULL;

    /* Open file */
    file = fopen(filename, "rb");
    if (file == NULL) {
        printf("File %s does not exist!\n", filename);
        ret = EXIT_FAILURE;
        goto exit;
    }

    /* Determine length of file */
    fseek(file, 0, SEEK_END);
    *fileLen = (int) ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("File %s is %d bytes\n", filename, *fileLen);

    /* Allocate buffer for image */
    *fileBuf = malloc((size_t) *fileLen);
    if(!*fileBuf) {
        printf("File buffer malloc failed!\n");
        ret = EXIT_FAILURE;
        goto exit;
    }

    /* Load file into buffer */
    ret = (int)fread(*fileBuf, 1, (size_t) *fileLen, file);
    if(ret != *fileLen) {
        printf("Error reading file! %d", ret);
        ret = EXIT_FAILURE;
        goto exit;
    }

exit:

    if(file) {
        fclose(file);
    }

    return ret;
}
#endif

#ifdef WC_RSA_NONBLOCK_TIME
    #ifndef _MSC_VER
        #define cpuid(reg, leaf, sub)\
            __asm__ __volatile__ ("cpuid":\
                "=a" (reg[0]), "=b" (reg[1]), "=c" (reg[2]), "=d" (reg[3]) :\
                "a" (leaf), "c"(sub));

        #define XASM_LINK(f) asm(f)
    #elif defined(__linux__)
        #include <intrin.h>

        #define cpuid(a,b,c) __cpuidex((int*)a,b,c)

        #define XASM_LINK(f)
    #endif /* _MSC_VER */
#endif

int main(int argc, char** argv)
{
/* These examples require RSA, FastMath and Non-blocking */
#if !defined(NO_RSA) && defined(USE_FAST_MATH) && defined(WC_RSA_NONBLOCK)
    RsaKey key;
    RsaNb nb;
    WC_RNG rng;
    int ret = 0, count;
    int signSz, verifySz;
    byte* derBuf = NULL;
    int derSz = 0;
    word32 idx;
    const char* in = "Everyone gets Friday off.";
    word32      inSz = (word32)XSTRLEN((char*)in);
    byte out[RSA_TEST_BYTES];
    byte plain[RSA_TEST_BYTES];
    word32 outSz = (word32)sizeof(out);
    word32 plainSz = (word32)sizeof(plain);
#ifdef WC_RSA_NONBLOCK_TIME
    long cpuSpeed = 1000000; /* 1Mhz - default */
    word32 blockingIntervalMs = 1; /* 1ms */
#endif

    wolfSSL_Debugging_ON();

    wolfSSL_Init();

    ret = load_file_to_buffer(kRsaKey, &derBuf, &derSz);
    if (ret < 0) {
        return ret;
    }

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Init RNG failed %d\n", ret);
        return ret;
    }

    ret = wc_InitRsaKey(&key, NULL);
    if (ret != 0) {
        printf("Init RSA key failed %d\n", ret);
        return ret;
    }

    ret = wc_RsaSetRNG(&key, &rng);
    if (ret != 0) {
        printf("Set RSA RNG failed %d\n", ret);
        goto prog_end;
    }

    /* load RSA key */
    idx = 0;
    ret = wc_RsaPrivateKeyDecode(derBuf, &idx, &key, (word32)derSz);
    if (ret != 0) {
        printf("RSA Private Key Decode failure %d\n", ret);
        goto prog_end;
    }

    /* Enable non-blocking RSA mode - provide context */
    ret = wc_RsaSetNonBlock(&key, &nb);
    if (ret != 0) {
        goto prog_end;
    }

#ifdef WC_RSA_NONBLOCK_TIME
    #ifdef cpuid
    {
        unsigned int reg[5];
        reg[4] = '\0';
        cpuid(reg, 0, 0);
        if (reg[0] >= 22) {
            cpuid(reg, 22, 0);
            /* reg[0]=proc base freq
               reg[1]=max freq
               reg[2]=bus ref freq
            */
            cpuSpeed = reg[0] * 1000000;
        }
    }
    #endif

    printf("CPU Speed is %ld, blocking ms %d\n", cpuSpeed, blockingIntervalMs);

    /* Enable time based RSA blocking */
    ret = wc_RsaSetNonBlockTime(&key, blockingIntervalMs, cpuSpeed);
    if (ret != 0) {
        goto prog_end;
    }
#endif

    count = 0;
    do {
        ret = wc_RsaSSL_Sign((const byte*)in, inSz, out, outSz, &key, &rng);
        count++; /* track number of would blocks */
        if (ret == FP_WOULDBLOCK) {
            /* do "other" work here */
        }
    } while (ret == FP_WOULDBLOCK);
    if (ret < 0) {
        goto prog_end;
    }
    printf("RSA non-block sign: %d times\n", count);
    signSz = ret;
    ret = 0;

    /* Test non-blocking verify */
    XMEMSET(plain, 0, plainSz);
    count = 0;
    do {
        ret = wc_RsaSSL_Verify(out, (word32)signSz, plain, plainSz, &key);
        count++; /* track number of would blocks */
        if (ret == FP_WOULDBLOCK) {
            /* do "other" work here */
        }
    } while (ret == FP_WOULDBLOCK);
    if (ret < 0) {
        goto prog_end;
    }
    printf("RSA non-block verify: %d times\n", count);
    verifySz = ret;
    ret = 0;

    if (signSz == ret && XMEMCMP(plain, in, (size_t)ret)) {
        ret = SIG_VERIFY_E;
    }

prog_end:

    if (ret != 0) {
        printf("Failure %s (%d)\n", wc_GetErrorString(ret), ret);
    }

    if (derBuf) {
        free(derBuf);
        derBuf = NULL;
    }
    wc_FreeRsaKey(&key);
    wc_FreeRng(&rng);
    wolfSSL_Cleanup();

    (void)verifySz;

    return 0;
#else
    (void)kRsaKey;

    printf("wolfSSL missing build features.\n");
    printf("Please build using `./configure --enable-fastmath CFLAGS=\"-DWC_RSA_NONBLOCK\"`\n");
    return -1;
#endif
}
