/* compressedData.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/pkcs7.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

#define compressedFile "compressedData.der"

#if defined(HAVE_PKCS7) && defined(HAVE_LIBZ) && \
    !defined(NO_PKCS7_COMPRESSED_DATA)

static const byte data[] = { /* Hello World */
    0x48,0x65,0x6c,0x6c,0x6f,0x20,0x57,0x6f,
    0x72,0x6c,0x64
};

static int write_file_buffer(const char* fileName, byte* in, word32 inSz)
{
    int ret;
    FILE* file;

    file = fopen(fileName, "wb");
    if (file == NULL) {
        printf("ERROR: opening file for writing: %s\n", fileName);
        return -1;
    }

    ret = (int)fwrite(in, 1, inSz, file);
    if (ret == 0) {
        printf("ERROR: writing buffer to output file\n");
        return -1;
    }
    fclose(file);

    return 0;
}

static int compressedData_encode(byte* out, word32 outSz)
{
    int ret;
    PKCS7* pkcs7;

    pkcs7 = wc_PKCS7_New(NULL, 0);
    if (pkcs7 == NULL)
        return -1;

    pkcs7->content              = (byte*)data;
    pkcs7->contentSz            = sizeof(data);
    pkcs7->contentOID           = DATA;

    /* encode compressedData, returns size */
    ret = wc_PKCS7_EncodeCompressedData(pkcs7, out, outSz);
    if (ret <= 0) {
        wc_PKCS7_Free(pkcs7);
        return -1;
    } else {
        printf("Successfully encoded CompressedData bundle (%s)\n",
               compressedFile);

        if (write_file_buffer(compressedFile, out, ret) != 0) {
            printf("ERROR: error writing encoded to output file\n");
            return -1;
        }
    }

    wc_PKCS7_Free(pkcs7);

    return ret;
}

static int compressedData_decode(byte* in, word32 inSz, byte* out, word32 outSz)
{
    int ret;
    PKCS7* pkcs7;

    pkcs7 = wc_PKCS7_New(NULL, 0);
    if (pkcs7 == NULL)
        return -1;

    /* decode compressedData, returns size */
    ret = wc_PKCS7_DecodeCompressedData(pkcs7, in, inSz, out, outSz);
    if (ret <= 0 || (XMEMCMP(out, data, ret) != 0)) {
        printf("Failed to decode CompressedData bundle (%s)\n",
               compressedFile);
        wc_PKCS7_Free(pkcs7);
        return -1;

    } else {
        printf("Successfully decoded CompressedData bundle (%s)\n",
               compressedFile);
    }

    wc_PKCS7_Free(pkcs7);

    return ret;
}

int main(int argc, char** argv)
{
    int compressedSz, decodedSz;
    byte compressed[1024];
    byte decoded[1024];

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    compressedSz = compressedData_encode(compressed, sizeof(compressed));
    if (compressedSz < 0)
        return -1;

#ifdef DEBUG_WOLFSSL
    printf("CompressedData DER (%d bytes):\n", compressedSz);
    WOLFSSL_BUFFER(compressed, compressedSz);
#endif

    decodedSz = compressedData_decode(compressed, compressedSz,
                                      decoded, sizeof(decoded));
    if (decodedSz < 0)
        return -1;

#ifdef DEBUG_WOLFSSL
    printf("Decoded Data DER (%d bytes):\n", decodedSz);
    WOLFSSL_BUFFER(decoded, decodedSz);
#endif

    (void)argc;
    (void)argv;

    return 0;
}

#else

int main(int argc, char** argv)
{
    printf("Must build wolfSSL using ./configure --enable-pkcs7 --with-libz\n");
    return 0;
}

#endif /* HAVE_PKCS7 & HAVE_LIBZ & !NO_PKCS7_COMPRESSED_DATA */

