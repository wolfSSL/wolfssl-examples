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


#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/sha256.h>

#define CHUNK_SIZE 1024

void usage(void)
{
    printf("./run <file to hash>\n");
    exit(-99);
}

int main(int argc, char** argv)
{
    int ret = -1;
#ifndef NO_SHA256
    wc_Sha256 sha256;
    byte  hash[WC_SHA256_DIGEST_SIZE];
    byte* rawInput;
    FILE* inputStream;
    char* fName = NULL;
    int numBytes = 0;
    int fileLength = 0;
    int i, numberOfChunks, excess, currChunkSz;

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

    /* determine numberOfChunks per file */
    numberOfChunks = fileLength / CHUNK_SIZE;
    excess = fileLength % CHUNK_SIZE;

    /* allocate buffer based on CHUNK_SIZE size */
    rawInput = (byte*) XMALLOC(CHUNK_SIZE*sizeof(byte), NULL,
                               DYNAMIC_TYPE_TMP_BUFFER);
    if (rawInput == NULL) {
        printf("Failed to allocate buffer for reading chunks into\n");
        return -2;
    }

    ret = wc_InitSha256(&sha256);
    if (ret != 0) {
        printf("Failed to initialize sha structure\n");
        fclose(inputStream);
        XFREE(rawInput, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }

    if (fileLength > CHUNK_SIZE)
        currChunkSz = CHUNK_SIZE;
    else
        currChunkSz = excess;

    /* Loop reading a block at a time, finishing with any excess */
    for (i = 0; i < fileLength; i += currChunkSz) {

        XMEMSET(rawInput, 0, CHUNK_SIZE);

        numBytes = fread(rawInput, 1, CHUNK_SIZE, inputStream);
        if (numBytes != CHUNK_SIZE && numBytes != excess) {
            printf("ERROR: Failed to read the appropriate amount\n");
            fclose(inputStream);
            XFREE(rawInput, NULL, DYNAMIC_TYPE_TMP_BUFFER);
            return -2;
        }

        ret = wc_Sha256Update(&sha256, rawInput, numBytes);
        if (ret != 0) {
            printf("Failed to update the hash\n");
            fclose(inputStream);
            XFREE(rawInput, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        }
        currChunkSz = ((fileLength - i) >= CHUNK_SIZE) ? CHUNK_SIZE:excess;
    }

    ret = wc_Sha256Final(&sha256, hash);

    if (ret != 0)
        printf("ERROR: Hash operation failed");
    else {
        printf("Hash result is: ");
        for (i = 0; i < WC_SHA256_DIGEST_SIZE; i++)
            printf("%02x", hash[i]);
        printf("\n");
    }

    fclose(inputStream);
    XFREE(rawInput, NULL, DYNAMIC_TYPE_TMP_BUFFER);
#else
    printf("Please enabling sha256 and then try again\n");
#endif
    return ret;
}
