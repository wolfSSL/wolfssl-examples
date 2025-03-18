/* hash-file.c
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


#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#ifndef CHUNK_SIZE
#define CHUNK_SIZE 1024
#endif

#ifndef NO_HASH_WRAPPERS
enum wc_HashType hash_type_from_string(char* name)
{
    if (strcmp(name, "MD2") == 0) {
        return WC_HASH_TYPE_MD2;
    }
    else if (strcmp(name, "MD4") == 0) {
        return WC_HASH_TYPE_MD4;
    }
    else if (strcmp(name, "MD5") == 0) {
        return WC_HASH_TYPE_MD5;
    }
    else if (strcmp(name, "SHA") == 0) {
        return WC_HASH_TYPE_SHA;
    }
    else if (strcmp(name, "SHA224") == 0) {
        return WC_HASH_TYPE_SHA224;
    }
    else if (strcmp(name, "SHA256") == 0) {
        return WC_HASH_TYPE_SHA256;
    }
    else if (strcmp(name, "SHA384") == 0) {
        return WC_HASH_TYPE_SHA384;
    }
    else if (strcmp(name, "SHA512") == 0) {
        return WC_HASH_TYPE_SHA512;
    }
    else if (strcmp(name, "SHA3-224") == 0) {
        return WC_HASH_TYPE_SHA3_224;
    }
    else if (strcmp(name, "SHA3-256") == 0) {
        return WC_HASH_TYPE_SHA3_256;
    }
    else if (strcmp(name, "SHA3-384") == 0) {
        return WC_HASH_TYPE_SHA3_384;
    }
    else if (strcmp(name, "SHA3-512") == 0) {
        return WC_HASH_TYPE_SHA3_512;
    }
    else if (strcmp(name, "BLAKE2B") == 0) {
        return WC_HASH_TYPE_BLAKE2B;
    }
    else if (strcmp(name, "BLAKE2S") == 0) {
        return WC_HASH_TYPE_BLAKE2S;
    }
#ifndef WOLFSSL_NOSHA512_224
    else if (strcmp(name, "SHA512-224") == 0) {
        return WC_HASH_TYPE_SHA512_224;
    }
#endif
#ifndef WOLFSSL_NOSHA512_256
    else if (strcmp(name, "SHA512-256") == 0) {
        return WC_HASH_TYPE_SHA512_256;
    }
#endif
#ifdef WOLFSSL_SHAKE128
    else if (strcmp(name, "SHAKE128") == 0) {
        return WC_HASH_TYPE_SHAKE128;
    }
#endif
#ifdef WOLFSSL_SHAKE256
    else if (strcmp(name, "SHAKE256") == 0) {
        return WC_HASH_TYPE_SHAKE256;
    }
#endif
#ifdef WOLFSSL_SM3
    else if (strcmp(name, "SM3") == 0) {
        return WC_HASH_TYPE_SHAKE256;
    }
#endif
    else {
        return WC_HASH_TYPE_NONE;
    }
}

void print_wolfssl_error(const char* msg, int err)
{
#ifndef NO_ERROR_STRINGS
    printf("%s: %s (%d)\n", msg, wc_GetErrorString(err), err);
#else
    printf("%s: %d\n", msg, err);
#endif
}

void usage(void)
{
    printf("./hash-file <alg> <file to hash>\n");
    exit(-99);
}
#endif

int main(int argc, char** argv)
{
    int ret = -1;
#ifndef NO_HASH_WRAPPERS
    wc_HashAlg hashAlg;
    enum wc_HashType hashType;
    byte  hash[WC_MAX_DIGEST_SIZE];
    byte  rawInput[CHUNK_SIZE];
    FILE* inputStream;
    char* hashName = NULL;
    char* fName = NULL;
    int fileLength = 0;
    int i, chunkSz;

    if (argc < 3)
        usage();
    hashName = argv[1];
    fName = argv[2];

    printf("Hash algorithme %s\n", hashName);
    hashType = hash_type_from_string(hashName);
    if (hashType == WC_HASH_TYPE_NONE) {
        printf("ERROR: hash algorithm not recognized.\n");
        return -1;
    }

    printf("Hash input file %s\n", fName);
    inputStream = fopen(fName, "rb");
    if (inputStream == NULL) {
        printf("ERROR: Unable to open file\n");
        return -1;
    }

    /* find length of the file */
    fseek(inputStream, 0, SEEK_END);
    fileLength = (int) ftell(inputStream);
    fseek(inputStream, 0, SEEK_SET);

    ret = wc_HashInit(&hashAlg, hashType);
    if (ret != 0) {
        print_wolfssl_error("Failed to initialize hash structure", ret);
        fclose(inputStream);
        return -1;
    }

    /* Loop reading a block at a time, finishing with any excess */
    for (i = 0; i < fileLength; i += CHUNK_SIZE) {
        chunkSz = CHUNK_SIZE;
        if (chunkSz > fileLength - i)
            chunkSz = fileLength - i;

        ret = fread(rawInput, 1, chunkSz, inputStream);
        if (ret != chunkSz) {
            printf("ERROR: Failed to read the appropriate amount\n");
            ret = -1;
            break;
        }

        ret = wc_HashUpdate(&hashAlg, hashType, rawInput, chunkSz);
        if (ret != 0) {
            print_wolfssl_error("Failed to update the hash", ret);
            break;
        }
    }

    if (ret == 0) {
        ret = wc_HashFinal(&hashAlg, hashType, hash);
        if (ret != 0) {
            print_wolfssl_error("Failed to generate hash", ret);
        }
    }

    if (ret != 0) {
        printf("ERROR: Hash operation failed");
    }
    else {
        printf("Hash result is: ");
        int sz = wc_HashGetDigestSize(hashType);
        for (i = 0; i < sz; i++)
            printf("%02x", hash[i]);
        printf("\n");
    }

    fclose(inputStream);
    wc_HashFree(&hashAlg, hashType);
#else
    printf("Please remove NO_HASH_WRAPPERS from wolfCrypt configuration\n");
#endif
    return ret;
}
