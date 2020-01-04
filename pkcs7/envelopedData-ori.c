/* envelopedData-ori.c
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

#define certFile "../certs/client-ecc-cert.der"
#define keyFile  "../certs/ecc-client-key.der"

#define encodedFileORI "envelopedDataORI.der"

static const byte data[] = { /* Hello World */
    0x48,0x65,0x6c,0x6c,0x6f,0x20,0x57,0x6f,
    0x72,0x6c,0x64
};

static const byte asnDataOid[] = {
    0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x01
};

static int load_certs(byte* cert, word32* certSz, byte* key, word32* keySz)
{
    FILE* file;

    /* certificate file */
    file = fopen(certFile, "rb");
    if (!file) {
        printf("ERROR: failed to open file: %s\n", certFile);
        return -1;
    }

    *certSz = (word32)fread(cert, 1, *certSz, file);
    fclose(file);

    /* key file */
    file = fopen(keyFile, "rb");
    if (!file) {
        printf("ERROR: failed to open file: %s\n", keyFile);
        return -1;
    }

    *keySz = (word32)fread(key, 1, *keySz, file);
    fclose(file);

    return 0;
}

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

/* ORI encrypt callback, responsible for encrypting content-encryption key (CEK)
 * and giving wolfCrypt the value for oriOID and oriValue to place in
 * OtherRecipientInfo.
 *
 * Returns 0 on success, negative upon error. */
static int myOriEncryptCb(PKCS7* pkcs7, byte* cek, word32 cekSz, byte* oriType,
                          word32* oriTypeSz, byte* oriValue, word32* oriValueSz,
                          void* ctx)
{
    int i;

    /* make sure buffers are large enough */
    if ((*oriValueSz < (2 + cekSz)) || (*oriTypeSz < sizeof(oriType)))
        return -1;

    /* our simple encryption algorithm will be take the bitwise complement */
    oriValue[0] = 0x04;         /*ASN OCTET STRING */
    oriValue[1] = (byte)cekSz;  /* length */
    for (i = 0; i < (int)cekSz; i++) {
        oriValue[2 + i] = ~cek[i];
    }
    *oriValueSz = 2 + cekSz;

    /* set oriType to ASN.1 encoded data OID */
    XMEMCPY(oriType, asnDataOid, sizeof(asnDataOid));
    *oriTypeSz = sizeof(asnDataOid);

    (void)pkcs7;
    (void)ctx;

    return 0;
}

/* ORI decrypt callback, responsible for providing a decrypted content
 * encryption key (CEK) placed into decryptedKey and size placed into
 * decryptedKeySz. oriOID and oriValue are given to the callback to help
 * in decrypting the encrypted CEK.
 *
 * Returns 0 on success, negative upon error. */
static int myOriDecryptCb(PKCS7* pkcs7, byte* oriType, word32 oriTypeSz,
                          byte* oriValue, word32 oriValueSz, byte* decryptedKey,
                          word32* decryptedKeySz, void* ctx)
{
    int i;

    /* make sure oriType matches what we expect */
    if (oriTypeSz != sizeof(asnDataOid))
        return -1;

    if (XMEMCMP(oriType, asnDataOid, sizeof(asnDataOid)) != 0)
        return -1;

    /* make sure decrypted buffer is large enough */
    if (*decryptedKeySz < oriValueSz)
        return -1;

    /* decrypt encrypted CEK using simple bitwise complement,
       only for example */
    for (i = 0; i < (int)oriValueSz - 2; i++) {
        decryptedKey[i] = ~oriValue[2 + i];
    }

    *decryptedKeySz = oriValueSz - 2;

    (void)pkcs7;
    (void)ctx;

    return 0;
}

static int envelopedData_encrypt(byte* cert, word32 certSz, byte* key,
                                 word32 keySz, byte* out, word32 outSz)
{
    int ret;
    PKCS7* pkcs7;

    pkcs7 = wc_PKCS7_New(NULL, INVALID_DEVID);
    if (pkcs7 == NULL)
        return -1;

    pkcs7->content     = (byte*)data;
    pkcs7->contentSz   = sizeof(data);
    pkcs7->contentOID  = DATA;
    pkcs7->encryptOID  = AES256CBCb;

    /* add recipient using otherRecipientInfo (ORI) with custom encrypt
     * callback to handle encryption. ORI is loosely defined, allowing
     * advanced users or future protocols to extend the CMS RecipientInfo
     * model. */
    ret = wc_PKCS7_AddRecipient_ORI(pkcs7, myOriEncryptCb, 0);
    if (ret < 0) {
        printf("wc_PKCS7_AddRecipient_ORI() failed, ret = %d\n", ret);
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
               encodedFileORI);

        if (write_file_buffer(encodedFileORI, out, ret) != 0) {
            printf("ERROR: error writing encoded to output file\n");
            return -1;
        }
    }

    wc_PKCS7_Free(pkcs7);

    return ret;
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

    /* set decrypt callback for decryption */
    ret = wc_PKCS7_SetOriDecryptCb(pkcs7, myOriDecryptCb);
    if (ret != 0) {
        printf("ERROR: wc_PKCS7_SetOriDecryptCb(), ret = %d\n", ret);
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
               encodedFileORI);
    }

    wc_PKCS7_Free(pkcs7);

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
    byte encrypted[1024];
    byte decrypted[1024];
    
#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    certSz = sizeof(cert);
    keySz = sizeof(key);
    ret = load_certs(cert, &certSz, key, &keySz);
    if (ret != 0)
        return -1;

    encryptedSz = envelopedData_encrypt(cert, certSz, key, keySz,
                                        encrypted, sizeof(encrypted));
    if (encryptedSz < 0)
        return -1;

#ifdef DEBUG_WOLFSSL
    printf("EnvelopedData DER (%d byte):\n", encryptedSz);
    WOLFSSL_BUFFER(encrypted, encryptedSz);
#endif

    decryptedSz = envelopedData_decrypt(encrypted, encryptedSz,
                                        cert, certSz, key, keySz,
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
    printf("Must build wolfSSL using ./configure --enable-pkcs7\n");
    return 0;
}

#endif

