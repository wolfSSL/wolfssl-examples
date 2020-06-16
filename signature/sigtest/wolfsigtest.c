#include <stdio.h>
#include <stdlib.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/signature.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/rsa.h>

/* Set this define to show RSA verify only */
//#define DEMO_RSA_VERIFY_ONLY

/* this is from ./certs/ca-key.pem */
const char* pubPemKey  = "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvwzKLRSyHoRCW804H0ry\n"
    "TXUQ8bY1n9/KfQOY06zeA2buKvHYsH1uB1QLEJghTYDLEiDnzE/eRX3Jcncy6sqQ\n"
    "u2lSEAMvqPOVxfGLYlYb72dvpBBBla0Km+OlwLDScHZQMFuo6AgsfO2nonqNOCkc\n"
    "rMft8nyVsJWCfUlcOM13Je+9gHVTlDw9ymNbnxW10x0TLxnRPNt2Osy4fcnlwtfa\n"
    "QG/YIdxzG0ItU5z+Gvx9q3o2P5jehHwFZ85qFDiHqfGMtWjLaH9xICv1oGP1Vi+j\n"
    "JtK3b7FaF9c4mQj+k1hv/sMTSQgWC6dNZwBSMWcjTpjtUUUduQTZC+zYKLNLve02\n"
    "eQIDAQAB\n"
    "-----END PUBLIC KEY-----\n";

const byte pubKeyModulus[] = {
    0x00, 0xbf, 0x0c, 0xca, 0x2d, 0x14, 0xb2, 0x1e, 0x84, 0x42, 0x5b, 0xcd, 0x38, 0x1f, 0x4a,
    0xf2, 0x4d, 0x75, 0x10, 0xf1, 0xb6, 0x35, 0x9f, 0xdf, 0xca, 0x7d, 0x03, 0x98, 0xd3, 0xac,
    0xde, 0x03, 0x66, 0xee, 0x2a, 0xf1, 0xd8, 0xb0, 0x7d, 0x6e, 0x07, 0x54, 0x0b, 0x10, 0x98,
    0x21, 0x4d, 0x80, 0xcb, 0x12, 0x20, 0xe7, 0xcc, 0x4f, 0xde, 0x45, 0x7d, 0xc9, 0x72, 0x77,
    0x32, 0xea, 0xca, 0x90, 0xbb, 0x69, 0x52, 0x10, 0x03, 0x2f, 0xa8, 0xf3, 0x95, 0xc5, 0xf1,
    0x8b, 0x62, 0x56, 0x1b, 0xef, 0x67, 0x6f, 0xa4, 0x10, 0x41, 0x95, 0xad, 0x0a, 0x9b, 0xe3,
    0xa5, 0xc0, 0xb0, 0xd2, 0x70, 0x76, 0x50, 0x30, 0x5b, 0xa8, 0xe8, 0x08, 0x2c, 0x7c, 0xed,
    0xa7, 0xa2, 0x7a, 0x8d, 0x38, 0x29, 0x1c, 0xac, 0xc7, 0xed, 0xf2, 0x7c, 0x95, 0xb0, 0x95,
    0x82, 0x7d, 0x49, 0x5c, 0x38, 0xcd, 0x77, 0x25, 0xef, 0xbd, 0x80, 0x75, 0x53, 0x94, 0x3c,
    0x3d, 0xca, 0x63, 0x5b, 0x9f, 0x15, 0xb5, 0xd3, 0x1d, 0x13, 0x2f, 0x19, 0xd1, 0x3c, 0xdb,
    0x76, 0x3a, 0xcc, 0xb8, 0x7d, 0xc9, 0xe5, 0xc2, 0xd7, 0xda, 0x40, 0x6f, 0xd8, 0x21, 0xdc,
    0x73, 0x1b, 0x42, 0x2d, 0x53, 0x9c, 0xfe, 0x1a, 0xfc, 0x7d, 0xab, 0x7a, 0x36, 0x3f, 0x98,
    0xde, 0x84, 0x7c, 0x05, 0x67, 0xce, 0x6a, 0x14, 0x38, 0x87, 0xa9, 0xf1, 0x8c, 0xb5, 0x68,
    0xcb, 0x68, 0x7f, 0x71, 0x20, 0x2b, 0xf5, 0xa0, 0x63, 0xf5, 0x56, 0x2f, 0xa3, 0x26, 0xd2,
    0xb7, 0x6f, 0xb1, 0x5a, 0x17, 0xd7, 0x38, 0x99, 0x08, 0xfe, 0x93, 0x58, 0x6f, 0xfe, 0xc3,
    0x13, 0x49, 0x08, 0x16, 0x0b, 0xa7, 0x4d, 0x67, 0x00, 0x52, 0x31, 0x67, 0x23, 0x4e, 0x98,
    0xed, 0x51, 0x45, 0x1d, 0xb9, 0x04, 0xd9, 0x0b, 0xec, 0xd8, 0x28, 0xb3, 0x4b, 0xbd, 0xed,
    0x36, 0x79
};
const byte pubKeyExponent[] = { 0x01, 0x00, 0x01 };
const long pubKeyExponentLong = 65537; /* 0x10001 */

const char* privPemKey = "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEpAIBAAKCAQEAvwzKLRSyHoRCW804H0ryTXUQ8bY1n9/KfQOY06zeA2buKvHY\n"
    "sH1uB1QLEJghTYDLEiDnzE/eRX3Jcncy6sqQu2lSEAMvqPOVxfGLYlYb72dvpBBB\n"
    "la0Km+OlwLDScHZQMFuo6AgsfO2nonqNOCkcrMft8nyVsJWCfUlcOM13Je+9gHVT\n"
    "lDw9ymNbnxW10x0TLxnRPNt2Osy4fcnlwtfaQG/YIdxzG0ItU5z+Gvx9q3o2P5je\n"
    "hHwFZ85qFDiHqfGMtWjLaH9xICv1oGP1Vi+jJtK3b7FaF9c4mQj+k1hv/sMTSQgW\n"
    "C6dNZwBSMWcjTpjtUUUduQTZC+zYKLNLve02eQIDAQABAoIBAD1uTmAahH+dhXzh\n"
    "Swd84NaZKt6d+TY0DncOPgjqT+UGJtT2OPffDQ8cLgai9CponGNy4zXmBJGRtcGx\n"
    "pFSs18b7QaDWdW+9C06/sVLoX0kmmFZHx97p6jxgAb8o3DG/SV+TSYd6gVuWS03K\n"
    "XDhPt+Gy08ch2jwShwfkG9xD7OjsVGHn9u2mCy7134J/xh9hGZykgznfIYWJb3ev\n"
    "hhUyCKJaCyZh+3AMypw4fbwi7uujqBYA+YqAHgCEqEpB+IQDZy8jWy+baybDBzSU\n"
    "owM7ctWfcuCtzDSrvcfV9SYwhQ8wIzlS/zzLmSFNiKWr7mK5x+C7R4fBac9z8zC+\n"
    "zjkEnOUCgYEA4XZFgFm200nfCu8S1g/wt8sqN7+n+LVN9TE1reSjlKHb8ZattQVk\n"
    "hYP8G1spqr74Jj92fq0c8MvXJrQbBY5Whn4IYiHBhtZHeT63XaTGOtexdCD2UJdB\n"
    "BFPtPybWb5H6aCbsKtya8efc+3PweUMbIaNZBGNSB8nX5tEbXV6W+lMCgYEA2O1O\n"
    "ZGFrkQxhAbUPu0RnUx7cB8Qkfp5shCORDOQSBBZNeJjMlj0gTg9Fmrb4s5MNsqIb\n"
    "KfImecjF0nh+XnPy13Bhu0DOYQX+aR6CKeYUuKHnltAjPwWTAPLhTX7tt5Zs9/Dk\n"
    "0c8BmE/cdFSqbV5aQTH+/5q2oAXdqRBU+GvQqoMCgYAh0wSKROtQt3xmv4cr5ihO\n"
    "6oPi6TXh8hFH/6H1/J8t5TqB/AEDb1OtVCe2Uu7lVtETq+GzD3WQCoS0ocCMDNae\n"
    "RrorPrUx7WO7pNUNj3LN0R4mNeu+G3L9mzm0h7cT9eqDRZOYuo/kSsy0TKh/CLpB\n"
    "SahJKD1ePcHONwDL+SzdUQKBgQChV58+udavg22DP4/70NyozgMJI7GhG2PKxElW\n"
    "NSvRLmVglQVVmRE1/dXfRMeliHJfsoJRqHFFkzbPXB9hUQwFgOivxXu6XiLjPHXD\n"
    "hAVVbdY6LYSJkzPLONqqMQXNzmwt3VXTVwvwpTVqsK4xukOWygDHS+MZEkPTQvpv\n"
    "6oDA0QKBgQC524kgNCdwYjTqXyViEvOdgb9I7poOwY0Q/2WanS0aipRayMClpYRh\n"
    "ntQkue+pncl3C8dwZj26yFTf0jPh9X/5J2G+V0Xdt0UXJPUj5DgOkSfu4yDYFMiU\n"
    "R3dAd0UYng3OeT9XMVYJSWe+lFhP9sSr4onj44rABVUsJMBKlwQnmg==\n"
    "-----END RSA PRIVATE KEY-----\n";

byte Digest_given[] = { /* 44 bytes */
    0x2C,0x05,0x16,0x39,0x9F,0x0C,0x02,0xD0,0xf9,0xba,0x90,0x37,0x0f,0xc1,0x4f,0xcc,
    0x31,0x4b,0x42,0x32,0x00,0x00,0x36,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x49,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x21,0x00,0x3f,0x19
};

byte expected_signed_results[] = {
    0x15,0xEB,0xE3,0x5A,0xA7,0x82,0x97,0x7C,0x3E,0x6D,0x3E,0x30,0xFB,0x3D,0x01,0x2C,
    0x71,0x3A,0x47,0x84,0x2B,0xB4,0x99,0x35,0xA3,0x2E,0x91,0xE4,0xF0,0x77,0x29,0x8A,
    0x63,0x51,0x33,0xB0,0x4F,0xBE,0x92,0xDB,0x17,0x3B,0xD6,0x3E,0x45,0x06,0x4E,0xAB,
    0x57,0x39,0x89,0x53,0x7B,0x54,0x56,0xC4,0xE8,0xA1,0x9E,0xA2,0x4C,0x21,0x53,0x4D,
    0xD7,0xC6,0x2A,0x94,0x48,0x62,0x6F,0x78,0x94,0x1B,0x2F,0xF4,0x48,0x2F,0xF7,0x37,
    0x32,0x32,0x35,0x52,0x08,0xAF,0xA0,0xF0,0xAF,0xFC,0x75,0x91,0x3B,0xBD,0x59,0xB3,
    0x24,0x74,0x1D,0xE8,0xFA,0xCB,0x00,0x93,0xFC,0x41,0x28,0x0C,0x13,0xEE,0xAB,0x02,
    0x1B,0xAB,0xA9,0xB4,0x7E,0xA7,0x97,0x81,0xC1,0x15,0x85,0xA2,0xA5,0x49,0x45,0x11,
    0xF2,0x76,0xAA,0xFB,0x00,0x7A,0xE7,0xD9,0x19,0x31,0xC6,0xF3,0xB9,0x07,0xA1,0x69,
    0x17,0xD9,0x79,0x72,0x84,0x9E,0xC2,0x6F,0xA0,0x2F,0x66,0xDD,0x7F,0xB3,0x0D,0xFE,
    0x34,0x1D,0x5C,0x0B,0xE5,0x06,0xEF,0x08,0xE7,0xE3,0xFA,0x79,0xB2,0xA6,0xD1,0xC1,
    0x31,0x9A,0xA4,0x47,0x5D,0x78,0x93,0xCB,0x56,0xB1,0x6D,0xC9,0xD7,0x11,0xD8,0x32,
    0x3A,0x2C,0x15,0xC1,0x26,0x19,0xDB,0x96,0xCA,0x2F,0x33,0xF9,0x6F,0xC4,0x3A,0x19,
    0x87,0x24,0xF4,0x1D,0x2E,0x99,0x4A,0x98,0x6B,0xE4,0x50,0x2B,0xB4,0xBF,0x8B,0x95,
    0xEC,0x9F,0x36,0x2D,0x14,0x58,0x6D,0xBF,0x4E,0x63,0xB6,0x26,0x8B,0x9C,0x22,0xFC,
    0xF4,0xC1,0x08,0xA1,0x70,0xDA,0x62,0x45,0x8F,0x3E,0x53,0x81,0x5D,0x4D,0xDA,0xF3
};

#define SHA1_HASH_LEN  20
#define DATA_BLOCK_LEN 44
#define SIGNED_LEN     256

static void print_buf(char *str, byte *buf, int blen)
{
   int i, j;

   printf("%s\n", str);
   for (i = 0, j = 0; i < blen; i++)
   {
      printf("%02X ", *buf++);
      if (++j == 16)
      {
         j = 0;
         printf("\n");
      }
   }
   printf("\n");
}

int main(int argc, char** argv)
{
    int ret = 0;
    WC_RNG rng;
    byte DER_buf[2048]; word32 DER_len = 0;
    byte Sig_buf[SIGNED_LEN]; word32 Sig_len = sizeof(Sig_buf);
    byte Hash_buf[SHA1_HASH_LEN]; word32 Hash_len = sizeof(Hash_buf);
    byte Digest_buf[SHA1_HASH_LEN+DATA_BLOCK_LEN]; word32 Digest_len = sizeof(Digest_buf);
    byte DigestVer_buf[SHA1_HASH_LEN+DATA_BLOCK_LEN]; word32 DigestVer_len = sizeof(DigestVer_buf);
    word32 inOutIdx=0;
    RsaKey rsakey;
    byte pemPublic = 0;
    enum wc_HashType hash_type = WC_HASH_TYPE_SHA;
    enum wc_SignatureType sig_type = WC_SIGNATURE_TYPE_RSA_W_ENC;

    /* Create input data (44 bytes) */
    print_buf("Digest Input Data:", Digest_given, DATA_BLOCK_LEN);

    /* Init */
    wc_InitRng(&rng);

    /* Init Rsa Key */
    wc_InitRsaKey(&rsakey, NULL);

    XMEMSET(DER_buf, 0, sizeof(DER_buf));
    
#ifndef DEMO_RSA_VERIFY_ONLY
    ret = wc_KeyPemToDer((const byte*)privPemKey, strlen(privPemKey), 
        DER_buf, sizeof(DER_buf), NULL);
    if (ret < 0)
#endif
    {
        pemPublic = 1;

#ifdef WOLFSSL_CERT_EXT
        /* Needs WOLFSSL_CERT_EXT defined or --enable-certgen --enable-certext  */
        ret = wc_PubKeyPemToDer((const byte*)pubPemKey, strlen(pubPemKey), 
            DER_buf, sizeof(DER_buf));
#else
        ret = 0; /* NOT_COMPILED_IN */
#endif
    }
    if (ret >= 0) {
        DER_len = ret;
    }
    printf("Key Pem to Der ret %d\n", ret);

    if (ret < 0) goto exit;
    if (DER_len > 0) {
        printf("DER_len = %d DER_buf:\n", DER_len);
        print_buf("DER:", DER_buf, DER_len);
    }

    /* PEM key selection */
    if (!pemPublic) {
        ret = wc_RsaPrivateKeyDecode(DER_buf, &inOutIdx, &rsakey, DER_len);
    }
    else {
        /* Three Examples for loading an RSA public key */

        /* 1. Decode DER key */
        if (DER_len > 0) {
            ret = wc_RsaPublicKeyDecode(DER_buf, &inOutIdx, &rsakey, DER_len);
        }
        else {
        #if 1
            /* 2. Decode Raw: Example for loading RSA public key with modulus and exponenet only */
            ret = wc_RsaPublicKeyDecodeRaw(pubKeyModulus, sizeof(pubKeyModulus), 
                pubKeyExponent, sizeof(pubKeyExponent), &rsakey);
        #else
            /* 3. Manual Math: Manually setting with math API's */
            ret = mp_set_int(&rsakey.e, pubKeyExponentLong);
            if (ret == 0) {
                ret = mp_read_unsigned_bin(&rsakey.n, pubKeyModulus, 
                    sizeof(pubKeyModulus));
            }
        #endif
        }
    }
    printf("decode %s key =%d\n", pemPublic ? "public" : "private", ret);

    /* Get signature length and allocate buffer */
    ret = wc_SignatureGetSize(sig_type, &rsakey, sizeof(rsakey));
    printf("Sig Len: %d\n", ret);
    if (ret < 0) goto exit;
    Sig_len = ret;

    /* Get Hash length */
    ret = wc_HashGetDigestSize(hash_type);
    printf("Hash Digest Len: %d\n", ret);
    if (ret < 0) goto exit;
    Hash_len = ret;

    /* Hash digest with SHA1 */
    ret = wc_Hash(hash_type, Digest_given, sizeof(Digest_given), Hash_buf, Hash_len);
    printf("Digest SHA1 Hash: %d\n", ret);
    if (ret < 0) goto exit;
    print_buf("Digest Output 20 Data:", Hash_buf, Hash_len);

    /* Add ASN digest info header */
    ret = wc_EncodeSignature(Digest_buf, Hash_buf, Hash_len, SHAh);
    printf("Digest Header: %d\n", ret);
    if (ret <= 0) goto exit;
    Digest_len = ret;
    print_buf("Signed data results:", Digest_buf, Digest_len);

    if (!pemPublic) {
        /* Perform hash and sign to create signature */
        ret = wc_RsaSSL_Sign(Digest_buf, Digest_len, Sig_buf, Sig_len, &rsakey, &rng);
        printf("RSA Sign Result: %d\n", ret);
        if (ret < 0) goto exit;
        Sig_len = ret;

        print_buf("RSA Sign Data:", Sig_buf, Sig_len);

        ret = wc_SignatureGenerate(hash_type, sig_type, 
            Digest_given, sizeof(Digest_given),
            Sig_buf, &Sig_len,
            &rsakey, sizeof(rsakey), &rng);
        printf("Sig Generation: ret %d, Sig_len=%d\n", ret, Sig_len);
        print_buf("Sign Data:", Sig_buf, Sig_len);

        /* Verify against expected signature */
        print_buf("Expected Signature:", expected_signed_results, sizeof(expected_signed_results));
        if (XMEMCMP(Sig_buf, expected_signed_results, Sig_len) == 0) {
            printf("Signatures match!\n");
        }
        else {
            printf("Signature invalid!\n");
        }
    }
    else {
        /* Use digest for RSA verify */
        ret = wc_RsaSSL_Verify(expected_signed_results, sizeof(expected_signed_results), 
            DigestVer_buf, DigestVer_len, &rsakey);
        if (ret != Digest_len || XMEMCMP(DigestVer_buf, Digest_buf, Digest_len) != 0) {
            printf("RSA Verify Failed! %d\n", ret);
        }
        else {
            printf("RSA Verify Success!\n");
            ret = 0;
        }
        print_buf("Expected Verify Data:", DigestVer_buf, DigestVer_len);
        print_buf("RSA Verify Data:", Digest_buf, Digest_len);

        if (ret == 0) {
            ret = wc_SignatureVerify(hash_type, sig_type, 
                Digest_given, sizeof(Digest_given),
                expected_signed_results, sizeof(expected_signed_results),
                &rsakey, sizeof(rsakey));
            printf("Sig Verify: %s (%d)\n", (ret == 0) ? "Pass" : "Fail", ret);

            /* Example for validating hash directly */
            ret = wc_SignatureVerifyHash(hash_type, sig_type, 
                Digest_buf, Digest_len,
                expected_signed_results, sizeof(expected_signed_results),
                &rsakey, sizeof(rsakey));
            printf("Sig Verify Hash: %s (%d)\n", (ret == 0) ? "Pass" : "Fail", ret);
        }
    }

exit:
    wc_FreeRsaKey(&rsakey);
    wc_FreeRng(&rng);
    return 0;
}
