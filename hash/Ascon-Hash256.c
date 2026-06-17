/* Ascon-Hash256.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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


#ifdef NO_INLINE
    #include <wolfssl/wolfcrypt/misc.h>
#endif
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ascon.h>

#define BLOCK_SIZE 4096

#ifdef HAVE_ASCON
void usage(void)
{
    printf("./Ascon-Hash256 <file to hash>\n");
    exit(-99);
}
#endif

void free_mem(wc_AsconHash256 *asconHash, byte *rawInput, FILE *inputStream) {
    if (asconHash != NULL) {
        wc_AsconHash256_Free(asconHash);
    }

    if (rawInput != NULL) {
        free(rawInput);
    }

    if (inputStream != NULL) {
        fclose(inputStream);
    }
}

int main(int argc, char** argv)
{
    int ret = 1;
#ifdef HAVE_ASCON
    wc_AsconHash256* asconHash = NULL;
    byte             hash[ASCON_HASH256_SZ] = {0};
    byte*            rawInput = NULL;
    FILE*            inputStream = NULL;
    char*            fName = NULL;
    long             fileLength = 0;
    int              chunkRead = BLOCK_SIZE;
    long             i;
    int              j;
    size_t           read;

    if (argc < 2)
        usage();
    fName = argv[1];
    printf("Hash input file %s\n", fName);


    inputStream = fopen(fName, "rb");
    if (inputStream == NULL) {
        printf("ERROR: Unable to open file\n");
        goto cleanup;
    }

    /* find length of the file */
    if (fseek(inputStream, 0, SEEK_END) != 0 ) {
        printf("fseek failed\n");
        goto cleanup;
    }

    fileLength = ftell(inputStream);
    if (fileLength < 0) {
        printf("ftell failed\n");
        goto cleanup;
    }

    if (fseek(inputStream, 0, SEEK_SET) != 0) {
        printf("fseek failed\n");
        goto cleanup;
    }

    /* Create and initialize hash context */
    asconHash = wc_AsconHash256_New();
    if (asconHash == NULL) {
        printf("ERROR: Unable to create the hash context\n");
        goto cleanup;
    }

    rawInput = (byte*) malloc(BLOCK_SIZE);
    if (rawInput == NULL) {
        printf("ERROR: Unable to allocate space for raw input\n");
        goto cleanup;
    }

    for (i = 0; i < fileLength; i += BLOCK_SIZE) {
        if (chunkRead > fileLength - i)
            chunkRead = fileLength - i;

        /* Read blocks from input file into a byte array*/
        read = fread(rawInput, 1, chunkRead, inputStream);
        if (read != chunkRead) {
            printf("ERROR: Failed to read the size of input file\n");
            goto cleanup;
        }

        ret = wc_AsconHash256_Update(asconHash, rawInput, chunkRead);
        if (ret != 0) {
            printf("ERROR: Hash update failed\n");
            ret = 1;
            goto cleanup;
        }
    }

    ret = wc_AsconHash256_Final(asconHash, hash);
    if (ret != 0) {
        printf("ERROR: Hash operation failed");
        ret = 1;
        goto cleanup;
    }

    printf("Hash result is: ");
    for (j = 0; j < ASCON_HASH256_SZ; j++)
        printf("%02x", hash[j]);
    printf("\n");

    cleanup:
        free_mem(asconHash, rawInput, inputStream);
#else
    printf("Please enable Ascon-Hash256 (--enable-ascon --enable-experimental) in wolfCrypt\n");
#endif
    return ret;
}
