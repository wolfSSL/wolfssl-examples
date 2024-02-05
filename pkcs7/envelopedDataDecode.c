/* envelopedDataDecode.c
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

static int load_certs(const char* certFile, byte* cert, word32* certSz,
    const char* keyFile, byte* key, word32* keySz)
{
    FILE* file;

    /* certificate file */
    file = fopen(certFile, "rb");
    if (!file)
        return -1;

    *certSz = (word32)fread(cert, 1, *certSz, file);
    fclose(file);

    /* key file */
    file = fopen(keyFile, "rb");
    if (!file)
        return -1;

    *keySz = (word32)fread(key, 1, *keySz, file);
    fclose(file);

    return 0;
}


static int envelopedData_decrypt(byte* in, word32 inSz, byte* cert,
                                word32 certSz, byte* key, word32 keySz,
                                byte* out, word32 outSz)
{
    int ret;
    PKCS7* pkcs7;

    pkcs7 = wc_PKCS7_New(NULL, INVALID_DEVID);
    if (pkcs7 == NULL)
        return -1;

    /* init with recipient cert */
    ret = wc_PKCS7_InitWithCert(pkcs7, cert, certSz);
    if (ret != 0) {
        wc_PKCS7_Free(pkcs7);
        return -1;
    }

    /* set recipient private key */
    ret = wc_PKCS7_SetKey(pkcs7, key, keySz);
    if (ret != 0) {
        wc_PKCS7_Free(pkcs7);
        return -1;
    }

    /* decode envelopedData, returns size */
    ret = wc_PKCS7_DecodeEnvelopedData(pkcs7, in, inSz, out, outSz);
    wc_PKCS7_Free(pkcs7);

    if (ret <= 0) {
        printf("Failed to decode EnvelopedData bundle error of %d\n", ret);
    }
    else {
        printf("Successfully decoded EnvelopedData bundle\n");
    }


    return ret;
}

#ifdef HAVE_PKCS7

int main(int argc, char** argv)
{
    int ret;
    int encryptedSz, decryptedSz;
    word32 certSz, keySz;

    byte cert[2048];
    byte key[2048];
    byte* encrypted;
    byte* decrypted;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    if (argc != 4) {
        printf("expecting DER cert, key, and encrypted bundle as args\n");
        printf("%s <DER cert> <DER key> <Encrypted bundle>\n", argv[0]);
        return -1;
    }

    certSz = sizeof(cert);
    keySz  = sizeof(key);
    ret = load_certs(argv[1], cert, &certSz, argv[2], key, &keySz);
    if (ret != 0) {
        printf("Error loading cert and key\n");
        return -1;
    }

    /* read encrypted bundle */
    {
        FILE* file;

        file = fopen(argv[3], "rb");
        if (!file) {
            printf("unable to open file %s\n", argv[3]);
            return -1;
        }
        fseek(file, 0, SEEK_END);
        encryptedSz = (int)ftell(file);
        rewind(file);

        encrypted = (byte*)malloc(encryptedSz);
        if (encrypted == NULL) {
            printf("malloc failed\n");
            return -1;
        }

        decryptedSz = encryptedSz;
        decrypted = (byte*)malloc(decryptedSz);
        if (decrypted == NULL) {
            printf("malloc failed\n");
            free(encrypted);
            return -1;
        }

        encryptedSz = (word32)fread(encrypted, 1, encryptedSz, file);
        printf("encrypted bundle size read = %d\n", encryptedSz);
        fclose(file);
    }

    decryptedSz = envelopedData_decrypt(encrypted, encryptedSz,
                                        cert, certSz, key, keySz,
                                        decrypted, decryptedSz);
    free(encrypted);
    if (decryptedSz < 0) {
        free(decrypted);
        return -1;
    }

#ifdef DEBUG_WOLFSSL
    printf("Decrypted content (%d byte):\n", decryptedSz);
    WOLFSSL_BUFFER(decrypted, decryptedSz);
#endif
    free(decrypted);

    return 0;
}

#else

int main(int argc, char** argv)
{
    printf("Must build wolfSSL using ./configure --enable-pkcs7\n");
    return 0;
}

#endif

