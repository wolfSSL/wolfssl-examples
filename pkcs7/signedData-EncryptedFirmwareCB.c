/* signedData-EncryptedFirmwareCB.c
 *
 * Copyright (C) 2006-2018 wolfSSL Inc.
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

#ifdef HAVE_PKCS7

#define certFile "../certs/client-cert.der"
#define keyFile  "../certs/client-key.der"

static byte defKey[] = {
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08
};

static byte altKey[] = {
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08
};

#define OUTPUT_FILE "signedData_EncryptedFPD_callback.der"

static int load_certs(byte* cert, word32* certSz, byte* key, word32* keySz)
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


static int myDecryptionFunc(PKCS7* pkcs7, int encryptOID, byte* iv, int ivSz,
        byte* aad, word32 aadSz, byte* authTag, word32 authTagSz,
        byte* in, int inSz, byte* out, void* usrCtx)
{
    int i, keyId = -1, ret, keySz;
    word32 keyIdSz = 8;
    byte*  key;
    byte   keyIdRaw[8];
    Aes    aes;

    /* looking for KEY ID
     * fwDecryptKeyID OID "1.2.840.113549.1.9.16.2.37
     */
    unsigned char OID[] = {
        /* 0x06, 0x0B do not pass in tag and length */
        0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D,
        0x01, 0x09, 0x10, 0x02, 0x25
    };

    /* if needing to find keyIdSz can call with NULL */
    ret = wc_PKCS7_GetAttributeValue(pkcs7, OID, sizeof(OID), NULL,
            &keyIdSz);
    if (ret != LENGTH_ONLY_E) {
        printf("Unexpected error %d when getting keyIdSz\n", ret);
        printf("Possibly no KEY ID attribute set\n");
    }
    else {
        printf("Found keyID OID expected size of = %d bytes\n", keyIdSz);
        memset(keyIdRaw, 0, sizeof(keyIdRaw));
        ret = wc_PKCS7_GetAttributeValue(pkcs7, OID, sizeof(OID), keyIdRaw,
                &keyIdSz);
        if (keyIdSz < 3) {
            printf("keyIdSz is smaller than expected\n");
            return -1;
        }
        if (keyIdSz > 2 + sizeof(int)) {
            printf("example case was only expecting a keyId of int size\n");
            return -1;
        }

        /* keyIdRaw[0] OCTET TAG */
        /* keyIdRaw[1] Length */

        if (ret > 0) {
            printf("Found KEY ID RAW ASN1 :");
            for (i = 0; i < ret; i++)
                printf("%02X", keyIdRaw[i]);
            printf("\n");
        }
        keyId = *(int*)(keyIdRaw + 2);
        printf("\tstripping off OCTET TAG and length the keyId = %d\n", keyId);
    }


    /* Use keyID here if found to select key and decrypt in HSM or in this
     * example just select key and do software decryption */
    if (keyId == 1) {
        printf("Using AES-CBC 128 bit key\n");
        key = altKey;
        keySz = sizeof(altKey);
    }
    else {
        printf("Using default 256 bit AES key\n");
        key = defKey;
        keySz = sizeof(defKey);
    }


    switch (encryptOID) {
        case AES256CBCb:
            if ((encryptOID == AES256CBCb && keySz != 32 ) ||
                    (ivSz  != AES_BLOCK_SIZE) )
                return BAD_FUNC_ARG;
            /* fall through */

        case AES128CBCb:

            ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
            if (ret == 0) {
                ret = wc_AesSetKey(&aes, key, keySz, iv, AES_DECRYPTION);
                if (ret == 0)
                    ret = wc_AesCbcDecrypt(&aes, out, in, inSz);
                wc_AesFree(&aes);
            }
            break;
        default:
            printf("Unsupported content cipher type for example");
            return ALGO_ID_E;
    };

    return ret;
}


/*
 * keyHint is the KeyID to be set in the fwDecryptKeyID attribute
 * returns size of buffer output on success
 */
static int generateBundle(byte* out, word32 *outSz, byte* encryptKey,
        word32 encryptKeySz, byte keyHint, char* fileName)
{
    int ret;
    PKCS7* pkcs7;

    /* KEY ID
     * fwDecryptKeyID OID 1.2.840.113549.1.9.16.2.37
     */
    unsigned char keyOID[] = {
        0x06, 0x0B,
        0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D,
        0x01, 0x09, 0x10, 0x02, 0x25
    };
    byte keyID[] = { 0x04, 0x01, 0x00 };
    byte data[] = "Test of wolfSSL PKCS7 decrypt callback";

    PKCS7Attrib attribs[] =
    {
        { keyOID, sizeof(keyOID), keyID, sizeof(keyID) }
    };

    word32 certSz, keySz;

    byte cert[2048];
    byte key[2048];

    keyID[2] = keyHint;
    certSz = sizeof(cert);
    keySz = sizeof(key);
    ret = load_certs(cert, &certSz, key, &keySz);
    if (ret != 0)
        return -1;

    /* init PKCS7 */
    pkcs7 = wc_PKCS7_New(NULL, INVALID_DEVID);
    if (pkcs7 == NULL)
        return -1;

    ret = wc_PKCS7_InitWithCert(pkcs7, cert, certSz);
    if (ret != 0) {
        printf("ERROR: wc_PKCS7_InitWithCert() failed, ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        return -1;
    }

    /* encode Signed Encrypted FirmwarePkgData */
    if (encryptKeySz == 16) {
        ret = wc_PKCS7_EncodeSignedEncryptedFPD(pkcs7, encryptKey, encryptKeySz,
                                            key, keySz,
                                            AES128CBCb, RSAk, SHA256h,
                                            (byte*)data, sizeof(data),
                                            attribs,
                                            sizeof(attribs)/sizeof(PKCS7Attrib),
                                            attribs,
                                            sizeof(attribs)/sizeof(PKCS7Attrib),
                                            out, *outSz);
    }
    else {
        ret = wc_PKCS7_EncodeSignedEncryptedFPD(pkcs7, encryptKey, encryptKeySz,
                                            key, keySz,
                                            AES256CBCb, RSAk, SHA256h,
                                            (byte*)data, sizeof(data),
                                            attribs,
                                            sizeof(attribs)/sizeof(PKCS7Attrib),
                                            attribs,
                                            sizeof(attribs)/sizeof(PKCS7Attrib),
                                            out, *outSz);
    }
    if (ret <= 0) {
        printf("ERROR: wc_PKCS7_EncodeSignedEncryptedFPD() failed, "
                "ret = %d\n", ret);
        wc_PKCS7_Free(pkcs7);
        return -1;

    } else {
        printf("Successfully encoded Signed Encrypted FirmwarePkgData (%s) with"
               " key ID attribute\n", fileName);

#ifdef DEBUG_WOLFSSL
        printf("Encoded DER (%d bytes):\n", ret);
        WOLFSSL_BUFFER(out, ret);
#endif

        *outSz = ret;
        if (write_file_buffer(fileName, out, ret) != 0) {
            printf("ERROR: error writing encoded to output file\n");
            return -1;
        }

    }

    wc_PKCS7_Free(pkcs7);

    return ret;
}


static int verifyBundle(byte* derBuf, word32 derSz)
{
    int ret = 0;
    PKCS7 pkcs7;
    byte*  sid;
    word32 sidSz;
    word32 i;

    byte decoded[2048];
    int  decodedSz = 2048;

    /* Test verify */
    ret = wc_PKCS7_Init(&pkcs7, NULL, INVALID_DEVID);
    if (ret != 0) goto exit;
    ret = wc_PKCS7_InitWithCert(&pkcs7, NULL, 0);
    if (ret != 0) goto exit;
    ret = wc_PKCS7_VerifySignedData(&pkcs7, derBuf, derSz);
    if (ret != 0) goto exit;

    printf("PKCS7 Verify Success\n");

    /* Get size of SID and print it out */
    ret = wc_PKCS7_GetSignerSID(&pkcs7, NULL, &sidSz);
    if (ret != LENGTH_ONLY_E) goto exit;

    sid = (byte*)XMALLOC(sidSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (sid == NULL) {
        ret = MEMORY_E;
        goto exit;
    }

    ret = wc_PKCS7_GetSignerSID(&pkcs7, sid, &sidSz);
    if (ret == 0) {
        printf("Signers SID (SKID or issuerAndSerial):");
        for (i = 0; i < sidSz; i++)
            printf("%02X", sid[i]);
        printf("\n");
    }
    XFREE(sid, NULL, DYNAMIC_TYPE_TMP_BUFFER);

    printf("\nTrying to decode the enveloped data\n");
    decodedSz = sizeof(decoded);
    ret = wc_PKCS7_SetDecodeEncryptedCb(&pkcs7, myDecryptionFunc);
    decodedSz = wc_PKCS7_DecodeEncryptedData(&pkcs7, pkcs7.content,
            pkcs7.contentSz, decoded, decodedSz);
    if (decodedSz < 0) {
        ret = decodedSz;
        printf("unable to decode enveloped data\n");
    }
    else {
        printf("Decoded successfully\n");
    }
exit:

    if (ret != 0)
        printf("RC=%d\n", ret);

    wc_PKCS7_Free(&pkcs7);
    return ret;
}

int main(int argc, char** argv)
{
    int ret = 0;
    XFILE derFile;
    byte derBuf[2048];
    word32 derSz = 2048;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    if (argc < 2) {
        printf("Doing default generation and verify\n");
        ret = generateBundle(derBuf, &derSz, defKey, sizeof(defKey), 0,
                "signedData_EncryptedFPD_callback.der");
        if (ret <= 0) {
            printf("unable to generate AES CBC bundle\n");
            return ret;
        }

        derSz = 2048;
        ret = generateBundle(derBuf, &derSz, altKey, sizeof(altKey), 1,
                "signedData_EncryptedFPD_AES128_callback.der");
        if (ret <= 0) {
            printf("unable to generate AES GCM bundle\n");
            return ret;
        }
    }
    else {
        printf("Trying to verify %s\n", argv[1]);
        derFile = fopen(argv[1], "rb");
        if (derFile) {
            ret = (int)fread(derBuf, 1, derSz, derFile);
            fclose(derFile);

            if (ret >= derSz || ret < 0) {
                printf("Failed to read der file!\n");
                return -1;
            }
            derSz = ret;
        }
    }
    ret = verifyBundle(derBuf, derSz);
    if (ret != 0) {
        printf("Unable to verify bundle, error [%d]\n", ret);
    }

    return 0;
}

#else

int main(int argc, char** argv)
{
    printf("Not compiled in: Must build wolfSSL using ./configure --enable-pkcs7\n");
    return 0;
}

#endif
