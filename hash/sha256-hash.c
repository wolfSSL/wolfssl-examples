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
#include <wolfssl/wolfcrypt/hash.h>

#ifndef NO_SHA256
void usage(void)
{
    printf("./run <file to hash>\n");
    exit(-99);
}
#endif

int main(int argc, char** argv)
{
    int ret = -1;
#ifndef NO_SHA256
    byte  hash[WC_SHA256_DIGEST_SIZE];
    byte* rawInput;
    FILE* inputStream;
    char* fName = NULL;
    int numBytes = 0;
    int fileLength = 0;

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

    /* allocate buffer based on file size */
    rawInput = (byte*) XMALLOC(fileLength*sizeof(byte), NULL,
                               DYNAMIC_TYPE_TMP_BUFFER);

    numBytes = fread(rawInput, 1, fileLength, inputStream);
    if (numBytes != fileLength) {
        printf("ERROR: Failed to read the entire file\n");
        fclose(inputStream);
        XFREE(rawInput, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        return -2;
    }

    ret = wc_Sha256Hash(rawInput, numBytes, hash);
    if (ret != 0)
        printf("ERROR: Hash operation failed");
    else {
        int i;
        printf("Hash result is: ");
        for (i = 0; i < WC_SHA256_DIGEST_SIZE; i++)
            printf("%02x", hash[i]);
        printf("\n");
    }

    fclose(inputStream);
    XFREE(rawInput, NULL, DYNAMIC_TYPE_TMP_BUFFER);
#else
    printf("Please enable sha256 and then try again\n");
#endif
    return ret;
}
