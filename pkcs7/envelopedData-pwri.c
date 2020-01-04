/* envelopedData-pwri.c
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
#include <wolfssl/wolfcrypt/pkcs7.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

#if defined(HAVE_PKCS7) && !defined(NO_PWDBASED)

#define encodedFilePWRI "envelopedDataPWRI.der"

static const byte data[] = { /* Hello World */
    0x48,0x65,0x6c,0x6c,0x6f,0x20,0x57,0x6f,
    0x72,0x6c,0x64
};

const char password[] = "wolfsslPassword";

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

static int envelopedData_encrypt(byte* out, word32 outSz)
{
    int ret;
    PKCS7* pkcs7;

    int kdfIterations = 5;

    byte salt[] = {
        0x12, 0x34, 0x56, 0x78, 0x56, 0x34, 0x12
    };

    pkcs7 = wc_PKCS7_New(NULL, INVALID_DEVID);
    if (pkcs7 == NULL)
        return -1;

    pkcs7->content     = (byte*)data;
    pkcs7->contentSz   = sizeof(data);
    pkcs7->contentOID  = DATA;
    pkcs7->encryptOID  = AES256CBCb;

    /* add recipient using password (PWRI type) */
    ret = wc_PKCS7_AddRecipient_PWRI(pkcs7, (byte*)password,
                                     (word32)XSTRLEN(password),
                                     salt, sizeof(salt),
                                     PBKDF2_OID, WC_SHA, kdfIterations,
                                     AES256CBCb, 0);
    if (ret < 0) {
        printf("wc_PKCS7_AddRecipient_PWRI() failed, ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        return -1;
    }

    /* encode envelopedData, returns size */
    ret = wc_PKCS7_EncodeEnvelopedData(pkcs7, out, outSz);
    if (ret <= 0) {
        printf("wc_PKCS7_EncodeEnvelopedData() failed, ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        return -1;

    } else {
        printf("Successfully encoded EnvelopedData bundle (%s)\n",
               encodedFilePWRI);

        if (write_file_buffer(encodedFilePWRI, out, ret) != 0) {
            printf("ERROR: error writing encoded to output file\n");
            return -1;
        }
    }

    wc_PKCS7_Free(pkcs7);

    return ret;
}

static int envelopedData_decrypt(byte* in, word32 inSz, byte* out, word32 outSz)
{
    int ret;
    PKCS7* pkcs7;

    pkcs7 = wc_PKCS7_New(NULL, INVALID_DEVID);
    if (pkcs7 == NULL)
        return -1;

    /* set password, for decryption */
    ret = wc_PKCS7_SetPassword(pkcs7, (byte*)password,
                               (word32)XSTRLEN(password));
    if (ret != 0) {
        printf("ERROR: wc_PKCS7_SetPassword(), ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        return -1;
    }

    /* decode envelopedData, returns size */
    ret = wc_PKCS7_DecodeEnvelopedData(pkcs7, in, inSz, out, outSz);
    if (ret <= 0 || (ret != sizeof(data)) || (XMEMCMP(out, data, ret) != 0)) {
        printf("ERROR: wc_PKCS7_DecodeEnvelopedData(), ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        return -1;

    } else {
        printf("Successfully decoded EnvelopedData bundle (%s)\n",
               encodedFilePWRI);
    }

    wc_PKCS7_Free(pkcs7);

    return ret;
}

int main(int argc, char** argv)
{
    int encryptedSz, decryptedSz;

    byte encrypted[1024];
    byte decrypted[1024];

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    encryptedSz = envelopedData_encrypt(encrypted, sizeof(encrypted));
    if (encryptedSz < 0)
        return -1;

#ifdef DEBUG_WOLFSSL
    printf("EnvelopedData DER (%d byte):\n", encryptedSz);
    WOLFSSL_BUFFER(encrypted, encryptedSz);
#endif

    decryptedSz = envelopedData_decrypt(encrypted, encryptedSz,
                                        decrypted, sizeof(decrypted));
    if (decryptedSz < 0)
        return -1;

#ifdef DEBUG_WOLFSSL
    printf("Decrypted content (%d byte):\n", decryptedSz);
    WOLFSSL_BUFFER(decrypted, decryptedSz);
#endif

    (void)argc;
    (void)argv;

    return 0;
}

#else

int main(int argc, char** argv)
{
    printf("Must build wolfSSL using ./configure --enable-pkcs7 "
           "--enable-pwdbased\n");
    return 0;
}

#endif /* HAVE_PKCS7 & !NO_PWDBASED */

