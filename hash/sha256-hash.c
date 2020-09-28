/* sha256-hash.c
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
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#ifndef CHUNK_SIZE
#define CHUNK_SIZE 1024
#endif

#ifndef NO_SHA256
void usage(void)
{
    printf("./sha256-hash <file to hash>\n");
    exit(-99);
}
#endif

int main(int argc, char** argv)
{
    int ret = -1;
#ifndef NO_SHA256
    wc_Sha256 sha256;
    byte  hash[WC_SHA256_DIGEST_SIZE];
    byte  rawInput[CHUNK_SIZE];
    FILE* inputStream;
    char* fName = NULL;
    int fileLength = 0;
    int i, chunkSz;

    if (argc < 2)
        usage();
    fName = argv[1];
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

    ret = wc_InitSha256(&sha256);
    if (ret != 0) {
        printf("Failed to initialize sha structure\n");
        fclose(inputStream);
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

        ret = wc_Sha256Update(&sha256, rawInput, chunkSz);
        if (ret != 0) {
            printf("Failed to update the hash\n");
            break;
        }
    }

    if (ret == 0) {
        ret = wc_Sha256Final(&sha256, hash);
    }
    if (ret != 0) {
        printf("ERROR: Hash operation failed");
    }
    else {
        printf("Hash result is: ");
        for (i = 0; i < WC_SHA256_DIGEST_SIZE; i++)
            printf("%02x", hash[i]);
        printf("\n");
    }

    fclose(inputStream);
    wc_Sha256Free(&sha256);
#else
    printf("Please enable sha256 (--enable-sha256) in wolfCrypt\n");
#endif
    return ret;
}
