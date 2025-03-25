/* Example showing ECC shared secret and HKDF to generate a symmetric key */
/*
gcc -Wall -okeywrap -lwolfssl keywrap.c
*/
#include <stdio.h>
#include <stdint.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/hmac.h>
#include <wolfssl/wolfcrypt/sha512.h>
#include <wolfssl/wolfcrypt/aes.h>

#if defined(HAVE_ECC) && !defined(NO_HMAC) && defined(HAVE_HKDF) && defined(HAVE_AESGCM)

#define ECC_CURVE_ID ECC_SECP384R1
#define USE_PEM /* USE_PEM, USE_DER else KeyGen */

/* from server-ecc384-key.pem */
#ifdef USE_PEM
static const char* privKeyPem =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIG2AgEAMBAGByqGSM49AgEGBSuBBAAiBIGeMIGbAgEBBDCk5QboBhY+q4n4YEPA\n"
    "YCXbunv+GTUIVWV24tzgAYtraN/Pb4ASznk36yuce8RoHHShZANiAATqz5NPLAm7\n"
    "ORQPVmTDQLTfDmOu5XFLAMwEl//h6TiWu1+RsmrMtTlfj3BZ8QH2WisBbGgLz1Ul\n"
    "r22YSAqodMmpF6AMw/vTI2j+BDxjUIg7uU98ZzT3O6lz5xvDUV4iGOw=\n"
    "-----END PRIVATE KEY-----";
#endif
#ifdef USE_DER
static const uint8_t privKeyDer[] = {
    0x30, 0x81, 0xA4, 0x02, 0x01, 0x01, 0x04, 0x30, 0xA4, 0xE5, 0x06, 0xE8, 0x06,
    0x16, 0x3E, 0xAB, 0x89, 0xF8, 0x60, 0x43, 0xC0, 0x60, 0x25, 0xDB, 0xBA, 0x7B,
    0xFE, 0x19, 0x35, 0x08, 0x55, 0x65, 0x76, 0xE2, 0xDC, 0xE0, 0x01, 0x8B, 0x6B,
    0x68, 0xDF, 0xCF, 0x6F, 0x80, 0x12, 0xCE, 0x79, 0x37, 0xEB, 0x2B, 0x9C, 0x7B,
    0xC4, 0x68, 0x1C, 0x74, 0xA0, 0x07, 0x06, 0x05, 0x2B, 0x81, 0x04, 0x00, 0x22,
    0xA1, 0x64, 0x03, 0x62, 0x00, 0x04, 0xEA, 0xCF, 0x93, 0x4F, 0x2C, 0x09, 0xBB,
    0x39, 0x14, 0x0F, 0x56, 0x64, 0xC3, 0x40, 0xB4, 0xDF, 0x0E, 0x63, 0xAE, 0xE5,
    0x71, 0x4B, 0x00, 0xCC, 0x04, 0x97, 0xFF, 0xE1, 0xE9, 0x38, 0x96, 0xBB, 0x5F,
    0x91, 0xB2, 0x6A, 0xCC, 0xB5, 0x39, 0x5F, 0x8F, 0x70, 0x59, 0xF1, 0x01, 0xF6,
    0x5A, 0x2B, 0x01, 0x6C, 0x68, 0x0B, 0xCF, 0x55, 0x25, 0xAF, 0x6D, 0x98, 0x48,
    0x0A, 0xA8, 0x74, 0xC9, 0xA9, 0x17, 0xA0, 0x0C, 0xC3, 0xFB, 0xD3, 0x23, 0x68,
    0xFE, 0x04, 0x3C, 0x63, 0x50, 0x88, 0x3B, 0xB9, 0x4F, 0x7C, 0x67, 0x34, 0xF7,
    0x3B, 0xA9, 0x73, 0xE7, 0x1B, 0xC3, 0x51, 0x5E, 0x22, 0x18, 0xEC
};
#endif

/* from client-ecc384-key.pem */
#ifdef USE_PEM
/* openssl ec -in ./certs/client-ecc384-key.pem -pubout -text */
static const char* pubKeyPem =
    "-----BEGIN PUBLIC KEY-----\n"
    "MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEZsQIPWanoRXUUwojs60Lzo/I9Jgdptiy\n"
    "biIR+rnvmcD6KT5IAPn+wqZKG6cSqGuQTBy7rF1uDmLOcCD3Q3fYl8d002j+iex3\n"
    "yxkviUodd/mXS2YCaKVir5WBy+MkNuuF\n"
    "-----END PUBLIC KEY-----";
#endif
#ifdef USE_DER
static uint8_t pubKeyDer[] = {
    0x30, 0x76, 0x30, 0x10, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01,
    0x06, 0x05, 0x2B, 0x81, 0x04, 0x00, 0x22, 0x03, 0x62, 0x00, 0x04, 0x66, 0xC4,
    0x08, 0x3D, 0x66, 0xA7, 0xA1, 0x15, 0xD4, 0x53, 0x0A, 0x23, 0xB3, 0xAD, 0x0B,
    0xCE, 0x8F, 0xC8, 0xF4, 0x98, 0x1D, 0xA6, 0xD8, 0xB2, 0x6E, 0x22, 0x11, 0xFA,
    0xB9, 0xEF, 0x99, 0xC0, 0xFA, 0x29, 0x3E, 0x48, 0x00, 0xF9, 0xFE, 0xC2, 0xA6,
    0x4A, 0x1B, 0xA7, 0x12, 0xA8, 0x6B, 0x90, 0x4C, 0x1C, 0xBB, 0xAC, 0x5D, 0x6E,
    0x0E, 0x62, 0xCE, 0x70, 0x20, 0xF7, 0x43, 0x77, 0xD8, 0x97, 0xC7, 0x74, 0xD3,
    0x68, 0xFE, 0x89, 0xEC, 0x77, 0xCB, 0x19, 0x2F, 0x89, 0x4A, 0x1D, 0x77, 0xF9,
    0x97, 0x4B, 0x66, 0x02, 0x68, 0xA5, 0x62, 0xAF, 0x95, 0x81, 0xCB, 0xE3, 0x24,
    0x36, 0xEB, 0x85
};
#endif

static void print_bin(char* desc, uint8_t* s, int sLen)
{
    int i;
    printf("%s: ", desc);
    for (i = 0; i < sLen; i++)
        printf("%02x", s[i]);
    printf("\n");
}


int do_ecdh(WC_RNG* rng, int devId, uint8_t* secret, uint32_t* secretLen)
{
    int ret;
    ecc_key myKey, peerKey;
#if defined(USE_PEM) || defined(USE_DER)
    uint32_t idx;
    #ifdef USE_PEM
    uint8_t  der[ECC_BUFSIZE];
    uint32_t derSz = (uint32_t)sizeof(der);
    #else
    uint8_t* der;
    uint32_t derSz;
    #endif
#endif

    memset(&myKey, 0, sizeof(myKey));
    memset(&peerKey, 0, sizeof(peerKey));

    ret = wc_ecc_init_ex(&myKey, NULL, devId);
    if (ret == 0)
        ret = wc_ecc_init_ex(&peerKey, NULL, devId);

    /* load (or generate) private key */
    if (ret == 0) {
    #ifdef USE_PEM
        ret = wc_KeyPemToDer((const uint8_t*)privKeyPem, strlen(privKeyPem),
            der, derSz, NULL);
        if (ret > 0) {
            derSz = ret;
            ret = 0;
        }
    #elif defined(USE_DER)
        der = (uint8_t*)privKeyDer;
        derSz = sizeof(privKeyDer);
    #endif
    }
    if (ret == 0) {
    #if defined(USE_DER) || defined(USE_PEM)
        idx = 0;
        ret = wc_EccPrivateKeyDecode(der, &idx, &myKey, derSz);
    #else
        /* don't use fixed key, just generate a key to throw away (ephemeral) */
        ret = wc_ecc_make_key_ex(rng, 0, &myKey, ECC_CURVE_ID);
    #endif
    }

    /* load (or generate) public key */
    if (ret == 0) {
    #ifdef USE_PEM
        ret = wc_PubKeyPemToDer((const uint8_t*)pubKeyPem, strlen(pubKeyPem),
            der, derSz);
        if (ret > 0) {
            derSz = ret;
            ret = 0;
        }
    #elif defined(USE_DER)
        der = (uint8_t*)pubKeyDer;
        derSz = sizeof(pubKeyDer);
    #endif
    }
    if (ret == 0) {
    #if defined(USE_DER) || defined(USE_PEM)
        idx = 0;
        ret = wc_EccPublicKeyDecode(der, &idx, &peerKey, derSz);
    #else
        /* don't use fixed key, just generate a key to throw away (ephemeral) */
        ret = wc_ecc_make_key_ex(rng, 0, &peerKey, ECC_CURVE_ID);
    #endif
    }

    /* compute shared secret */
    if (ret == 0) {
        *secretLen = wc_ecc_size(&myKey);
        wc_ecc_set_rng(&myKey, rng);
        ret = wc_ecc_shared_secret(&myKey, &peerKey, secret, secretLen);
    }

    wc_ecc_free(&peerKey);
    wc_ecc_free(&myKey);

    return ret;
}

#if 0
int wc_HKDF(int type, const uint8_t* inKey, uint32_t inKeySz,
                       const uint8_t* salt,  uint32_t saltSz,
                       const uint8_t* info,  uint32_t infoSz,
                       uint8_t* out,         uint32_t outSz)
#endif

int do_example(void)
{
    int ret;
    uint8_t  secret[MAX_ECC_BYTES];
    uint32_t secretLen = (uint32_t)sizeof(secret);
    const uint8_t* kdfSalt = NULL; /* optional salt for kdf */
    const uint8_t* kdfInfo = NULL; /* optional info for kdf */
    uint32_t kdfSaltSz = 0; /* size of kdfSalt */
    uint32_t kdfInfoSz = 0; /* size of kdfInfo */
    WC_RNG rng;
    uint8_t key[AES_256_KEY_SIZE];
    int     keyLen = (int)sizeof(key);
    uint8_t wrapKey[AES_256_KEY_SIZE];
    uint8_t wrapKeyEnc[AES_256_KEY_SIZE];
    int     wrapKeyLen = (int)sizeof(wrapKey);
    Aes aes;
    uint8_t tag[AES_BLOCK_SIZE];
    uint8_t iv[GCM_NONCE_MID_SZ];
    uint8_t* aad = NULL; /* optional additional auth used in tag generation */
    uint32_t aadLen = 0;
    int devId = INVALID_DEVID;


    ret = wc_InitRng_ex(&rng, NULL, devId);
    if (ret != 0) {
        printf("RNG Init failed! %d (%s)\n", ret, wc_GetErrorString(ret));
        return ret;
    }


    /* create a shared secret between a private and public key */
    ret = do_ecdh(&rng, devId, secret, &secretLen);
    if (ret == 0) {
        print_bin("ECDH Secret", secret, (int)secretLen);
    }
    else {
        printf("ECDH failed! %d (%s)\n", ret, wc_GetErrorString(ret));
        goto exit;
    }

    /* derive a key that can be used for symmetric */
    ret = wc_HKDF(WC_SHA384,
        secret, secretLen,
        kdfSalt, kdfSaltSz,
        kdfInfo, kdfInfoSz,
        key, (uint32_t)keyLen
    );
    if (ret == 0) {
        print_bin("HKDF Derived Key", key, keyLen);
    }
    else {
        printf("HKDF failed! %d (%s)\n", ret, wc_GetErrorString(ret));
        goto exit;
    }

    /* generate random value */
    ret = wc_RNG_GenerateBlock(&rng, wrapKey, wrapKeyLen);
    if (ret == 0) {
        print_bin("Random Key", wrapKey, wrapKeyLen);
    }
    else {
        printf("Random Key failed! %d (%s)\n", ret, wc_GetErrorString(ret));
        goto exit;
    }

    /* IV */
    memset(iv, 0, sizeof(iv));

    /* encrypt random value */
    ret = wc_AesInit(&aes, NULL, devId);
    if (ret == 0) {
        ret = wc_AesGcmSetKey(&aes, key, keyLen);
        if (ret == 0) {
            ret = wc_AesGcmEncrypt(&aes,
                wrapKeyEnc, wrapKey, wrapKeyLen, /* out, in, len */
                iv, sizeof(iv), /* IV = should be unique for each key, this will use zero's */
                tag, sizeof(tag),  /* output: tag used to validate integrity of data */
                aad, aadLen /* additional authentication data (optional) and mixed with tag */
            );
        }
        wc_AesFree(&aes);
    }
    if (ret == 0) {
        print_bin("AES GCM Key Wrap", wrapKeyEnc, wrapKeyLen);
    }
    else {
        printf("AES GCM Key Wrap failed! %d (%s)\n", ret, wc_GetErrorString(ret));
        goto exit;
    }

    /* test decrypt */
    ret = wc_AesInit(&aes, NULL, devId);
    if (ret == 0) {
        ret = wc_AesGcmSetKey(&aes, key, keyLen);
        if (ret == 0) {
            ret = wc_AesGcmDecrypt(&aes,
                wrapKey, wrapKeyEnc, wrapKeyLen, /* out, in, len */
                iv, sizeof(iv), /* IV = should be unique for each key, this will use zero's */
                tag, sizeof(tag),  /* output: tag used to validate integrity of data */
                aad, aadLen /* additional authentication data (optional) and mixed with tag */
            );
        }
        wc_AesFree(&aes);
    }
    if (ret == 0) {
        print_bin("Decrypted Random Key", wrapKey, wrapKeyLen);
    }
    else {
        printf("AES GCM Key Unwrap failed! %d (%s)\n", ret, wc_GetErrorString(ret));
        goto exit;
    }

    /* PKCS7 create bundle using my key and cert */
    /* see: pkcs7/authEnvelopedData-ktri.c */


exit:
    wc_FreeRng(&rng);

    return ret;
}
#endif

int main(int argc, char** argv)
{
    int ret;
#if defined(HAVE_ECC) && !defined(NO_HMAC) && defined(HAVE_HKDF) && defined(HAVE_AESGCM)
    ret = do_example();
#else
    printf("Example requires ECC, HMAC, HKDF and AES GCM\n");
    ret = -1;
#endif
    return ret;
}
