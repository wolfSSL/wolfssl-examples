/* wolfsslBenchmark.c
 *
 * Copyright (C) 2006-2014 wolfSSL Inc.
 * This file is part of CyaSSL.
 *
 * CyaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CyaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,USA
 */

#include "include/wolfssl.h"

#define DES3_BLOCK_SIZE 24

#ifdef HAVE_BLAKE2

#define BLAKE_DIGEST_SIZE 64

#endif /* HAVE_BLAKE2 */

/*
 * benchmarking funciton 
 */
int wolfsslBenchmark(int timer, int* option)
{
    int i              =   0;       /* A looping variable */

    int     loop       =   1;       /* benchmarking loop */
    int64_t blocks     =   0;      /* blocks used during benchmarking */
#ifndef NO_AES
    Aes aes;                        /* aes declaration */
#endif

#ifndef NO_DES3
    Des3 des3;                      /* 3des declaration */
#endif

    RNG rng;                        /* random number generator */

    int             ret  = 0;        /* return variable */
    double          stop = 0.0;     /* stop breaks loop */
    double          start;          /* start time */
    double          currTime;       /* current time*/
    

    ALIGN16 byte*   plain;          /* plain text */
    ALIGN16 byte*   cipher;         /* cipher */
    ALIGN16 byte*   key;            /* key for testing */
    ALIGN16 byte*   iv;             /* iv for initial encoding */
    
    byte*           digest;         /* message digest */

    InitRng(&rng);

    signal(SIGALRM, wolfsslStop);
    i = 0;
#ifndef NO_AES
    /* aes test */
    if (option[i] == 1) {
        plain = malloc(AES_BLOCK_SIZE);
        cipher = malloc(AES_BLOCK_SIZE);
        key = malloc(AES_BLOCK_SIZE);
        iv = malloc(AES_BLOCK_SIZE);

        RNG_GenerateBlock(&rng, plain, AES_BLOCK_SIZE);
        RNG_GenerateBlock(&rng, cipher, AES_BLOCK_SIZE);
        RNG_GenerateBlock(&rng, key, AES_BLOCK_SIZE);
        RNG_GenerateBlock(&rng, iv, AES_BLOCK_SIZE);
        start = wolfsslGetTime();
        alarm(timer);

        AesSetKey(&aes, key, AES_BLOCK_SIZE, iv, AES_ENCRYPTION);

        while (loop) {
            AesCbcEncrypt(&aes, cipher, plain, AES_BLOCK_SIZE);
            blocks++;
            currTime = wolfsslGetTime();
            stop = currTime - start;
            /* if stop >= timer, loop = 0 */
            loop = (stop >= timer) ? 0 : 1;
        }
        printf("\n");
        printf("AES-CBC ");
        wolfsslStats(start, AES_BLOCK_SIZE, blocks);
        memset(plain, 0, AES_BLOCK_SIZE);
        memset(cipher, 0, AES_BLOCK_SIZE);
        memset(key, 0, AES_BLOCK_SIZE);
        memset(iv, 0, AES_BLOCK_SIZE);
        free(plain);
        free(cipher);
        free(key);
        free(iv);
        blocks = 0;
        loop = 1;
    }
    i++;
#endif
#ifdef CYASSL_AES_COUNTER
    /* aes-ctr test */
    if (option[i] == 1) {
        plain = malloc(AES_BLOCK_SIZE);
        cipher = malloc(AES_BLOCK_SIZE);
        key = malloc(AES_BLOCK_SIZE);
        iv = malloc(AES_BLOCK_SIZE);

        RNG_GenerateBlock(&rng, plain, AES_BLOCK_SIZE);
        RNG_GenerateBlock(&rng, cipher, AES_BLOCK_SIZE);
        RNG_GenerateBlock(&rng, key, AES_BLOCK_SIZE);
        RNG_GenerateBlock(&rng, iv, AES_BLOCK_SIZE);
        start = wolfsslGetTime();
        alarm(timer);

        AesSetKeyDirect(&aes, key, AES_BLOCK_SIZE, iv, AES_ENCRYPTION);
        while (loop) {
            AesCtrEncrypt(&aes, cipher, plain, AES_BLOCK_SIZE);
            blocks++;
            currTime = wolfsslGetTime();
            stop = currTime - start;
            /* if stop >= timer, loop = 0 */
            loop = (stop >= timer) ? 0 : 1;
        }
        printf("AES-CTR ");
        wolfsslStats(start, AES_BLOCK_SIZE, blocks);
        memset(plain, 0, AES_BLOCK_SIZE);
        memset(cipher, 0, AES_BLOCK_SIZE);
        memset(key, 0, AES_BLOCK_SIZE);
        memset(iv, 0, AES_BLOCK_SIZE);
        free(plain);
        free(cipher);
        free(key);
        free(iv);
        blocks = 0;
        loop = 1;
    }
    i++;
#endif
#ifndef NO_DES3
    /* 3des test */
    if (option[i] == 1) {   
        plain = malloc(DES3_BLOCK_SIZE);
        cipher = malloc(DES3_BLOCK_SIZE);
        key = malloc(DES3_BLOCK_SIZE);
        iv = malloc(DES3_BLOCK_SIZE);

        RNG_GenerateBlock(&rng, plain, DES3_BLOCK_SIZE);
        RNG_GenerateBlock(&rng, cipher, DES3_BLOCK_SIZE);
        RNG_GenerateBlock(&rng, key, DES3_BLOCK_SIZE);
        RNG_GenerateBlock(&rng, iv, DES3_BLOCK_SIZE);

        start = wolfsslGetTime();
        alarm(timer);

        Des3_SetKey(&des3, key, iv, DES_ENCRYPTION);
        while (loop) {
            Des3_CbcEncrypt(&des3, cipher, plain, DES3_BLOCK_SIZE);
            blocks++;
            currTime = wolfsslGetTime();
            stop = currTime - start;
            /* if stop >= timer, loop = 0 */
            loop = (stop >= timer) ? 0 : 1;
        }
        printf("3DES ");
        wolfsslStats(start, DES3_BLOCK_SIZE, blocks);
        memset(plain, 0, DES3_BLOCK_SIZE);
        memset(cipher, 0, DES3_BLOCK_SIZE);
        memset(key, 0, DES3_BLOCK_SIZE);
        memset(iv, 0, DES3_BLOCK_SIZE);
        free(plain);
        free(cipher);
        free(key);
        free(iv);
        blocks = 0;
        loop = 1;
    }
    i++;
#endif
#ifdef HAVE_CAMELLIA
    /* camellia test */
    if (option[i] == 1) {
        Camellia camellia;

        plain = malloc(CAMELLIA_BLOCK_SIZE);
        cipher = malloc(CAMELLIA_BLOCK_SIZE);
        key = malloc(CAMELLIA_BLOCK_SIZE);
        iv = malloc(CAMELLIA_BLOCK_SIZE);

        RNG_GenerateBlock(&rng, plain, CAMELLIA_BLOCK_SIZE);
        RNG_GenerateBlock(&rng, cipher, CAMELLIA_BLOCK_SIZE);
        RNG_GenerateBlock(&rng, key, CAMELLIA_BLOCK_SIZE);
        RNG_GenerateBlock(&rng, iv, CAMELLIA_BLOCK_SIZE);

        start = wolfsslGetTime();
        alarm(timer);

        CamelliaSetKey(&camellia, key, CAMELLIA_BLOCK_SIZE, iv);
        while (loop) {
            CamelliaCbcEncrypt(&camellia, cipher, plain, CAMELLIA_BLOCK_SIZE);
            blocks++;
            currTime = wolfsslGetTime();
            stop = currTime - start;
            /* if stop >= timer, loop = 0 */
            loop = (stop >= timer) ? 0 : 1;
        }
        printf("Camellia ");
        wolfsslStats(start, CAMELLIA_BLOCK_SIZE, blocks);
        memset(plain, 0, CAMELLIA_BLOCK_SIZE);
        memset(cipher, 0, CAMELLIA_BLOCK_SIZE);
        memset(key, 0, CAMELLIA_BLOCK_SIZE);
        memset(iv, 0, CAMELLIA_BLOCK_SIZE);
        free(plain);
        free(cipher);
        free(key);
        free(iv);
        blocks = 0;
        loop = 1;
    }
    i++;
#endif
#ifndef NO_MD5
    /* md5 test */
    if (option[i] == 1) {
        Md5 md5;

        digest = malloc(MD5_DIGEST_SIZE);
        plain = malloc(MEGABYTE);
        RNG_GenerateBlock(&rng, plain, MEGABYTE);

        InitMd5(&md5);
        start = wolfsslGetTime();
        alarm(timer);

        while (loop) {
            Md5Update(&md5, plain, MEGABYTE);
            blocks++;
            currTime = wolfsslGetTime();
            stop = currTime - start;
            /* if stop >= timer, loop = 0 */
            loop = (stop >= timer) ? 0 : 1;
        }
        Md5Final(&md5, digest);
        printf("MD5 ");
        wolfsslStats(start, MEGABYTE, blocks);
        memset(plain, 0, MEGABYTE);
        memset(digest, 0, MD5_DIGEST_SIZE);
        free(plain);
        free(digest);
        blocks = 0;
        loop = 1;
    }
    i++;
#endif
#ifndef NO_SHA
    /* sha test */
    if (option[i] == 1) {
        Sha sha;

        digest = malloc(SHA_DIGEST_SIZE);
        plain = malloc(MEGABYTE);
        RNG_GenerateBlock(&rng, plain, MEGABYTE);

        InitSha(&sha);
        start = wolfsslGetTime();
        alarm(timer);

        while (loop) {
            ShaUpdate(&sha, plain, MEGABYTE);
            blocks++;
            currTime = wolfsslGetTime();
            stop = currTime - start;
            /* if stop >= timer, loop = 0 */
            loop = (stop >= timer) ? 0 : 1;
        }
        ShaFinal(&sha, digest);
        printf("Sha ");
        wolfsslStats(start, MEGABYTE, blocks);
        memset(plain, 0, MEGABYTE);
        memset(digest, 0, SHA_DIGEST_SIZE);
        free(plain);
        free(digest);
        blocks = 0;
        loop = 1;
    }
    i++;
#endif
#ifndef NO_SHA256
    /* sha256 test */
    if (option[i] == 1) {
        Sha256 sha256;

        digest = malloc(SHA256_DIGEST_SIZE);
        plain = malloc(MEGABYTE);
        RNG_GenerateBlock(&rng, plain, MEGABYTE);

        InitSha256(&sha256);
        start = wolfsslGetTime();
        alarm(timer);

        while (loop) {
            Sha256Update(&sha256, plain, MEGABYTE);
            blocks++;
            currTime = wolfsslGetTime();
            stop = currTime - start;
            /* if stop >= timer, loop = 0 */
            loop = (stop >= timer) ? 0 : 1;
        }
        Sha256Final(&sha256, digest);
        printf("Sha256 ");
        wolfsslStats(start, MEGABYTE, blocks);
        memset(plain, 0, MEGABYTE);
        memset(digest, 0, SHA256_DIGEST_SIZE);
        free(plain);
        free(digest);
        /* resets used for debug, uncomment if needed */
        /* blocks = 0; */
        /* loop = 1; */
    }
    i++;
#endif
#ifdef CYASSL_SHA384
    /* sha384 test */
    if (option[i] == 1) {
        Sha384 sha384;

        digest = malloc(SHA384_DIGEST_SIZE);
        plain = malloc(MEGABYTE);
        RNG_GenerateBlock(&rng, plain, MEGABYTE);

        InitSha384(&sha384);
        start = wolfsslGetTime();
        alarm(timer);

        while (loop) {
            Sha384Update(&sha384, plain, MEGABYTE);
            blocks++;
            currTime = wolfsslGetTime();
            stop = currTime - start;
            /* if stop >= timer, loop = 0 */
            loop = (stop >= timer) ? 0 : 1;
        }
        Sha384Final(&sha384, digest);
        printf("Sha384 ");
        wolfsslStats(start, MEGABYTE, blocks);
        memset(plain, 0, MEGABYTE);
        memset(digest, 0, SHA384_DIGEST_SIZE);
        free(plain);
        free(digest);
        blocks = 0;
        loop = 1;
    }
    i++;
#endif
#ifdef CYASSL_SHA512
    /* sha512 test */
    if (option[i] == 1) {
        Sha512 sha512;

        digest = malloc(SHA512_DIGEST_SIZE);
        plain = malloc(MEGABYTE);
        RNG_GenerateBlock(&rng, plain, MEGABYTE);

        InitSha512(&sha512);
        start = wolfsslGetTime();
        alarm(timer);

        while (loop) {
            Sha512Update(&sha512, plain, MEGABYTE);
            blocks++;
            currTime = wolfsslGetTime();
            stop = currTime - start;
            /* if stop >= timer, loop = 0 */
            loop = (stop >= timer) ? 0 : 1;
        }
        Sha512Final(&sha512, digest);
        printf("Sha512 ");
        wolfsslStats(start, MEGABYTE, blocks);
        memset(plain, 0, MEGABYTE);
        memset(digest, 0, SHA512_DIGEST_SIZE);
        free(plain);
        free(digest);
        blocks = 0;
        loop = 1;
    }
    i++;
#endif
#ifdef HAVE_BLAKE2
    /* blake2b test */
    if (option[i] == 1) {
        Blake2b  b2b;

        digest = malloc(BLAKE_DIGEST_SIZE);
        plain = malloc(MEGABYTE);
        RNG_GenerateBlock(&rng, plain, MEGABYTE);

        InitBlake2b(&b2b, BLAKE_DIGEST_SIZE);
        start = wolfsslGetTime();
        alarm(timer);

        while (loop) {
            Blake2bUpdate(&b2b, plain, MEGABYTE);
            blocks++;
            currTime = wolfsslGetTime();
            stop = currTime - start;
            /* if stop >= timer, loop = 0 */
            loop = (stop >= timer) ? 0 : 1;
        }
        Blake2bFinal(&b2b, digest, BLAKE_DIGEST_SIZE);
        printf("Blake2b ");
        wolfsslStats(start, MEGABYTE, blocks);
        memset(plain, 0, MEGABYTE);
        memset(digest, 0, BLAKE_DIGEST_SIZE);
        free(plain);
        free(digest);
    }
#endif
    return ret;
}
