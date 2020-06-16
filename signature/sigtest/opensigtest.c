#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <openssl/err.h> /* Error codes */
#include <openssl/evp.h> /* For crypto engine functions */
#include <openssl/pem.h> /* For PEM file access functions */
#include <openssl/rsa.h> /* For RSA functions */
#include <openssl/sha.h> /* For SHA-1 functions */


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

uint8_t Digest_given[] = { /* 44 bytes */
    0x2C,0x05,0x16,0x39,0x9F,0x0C,0x02,0xD0,0xf9,0xba,0x90,0x37,0x0f,0xc1,0x4f,0xcc,
    0x31,0x4b,0x42,0x32,0x00,0x00,0x36,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x49,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x21,0x00,0x3f,0x19
};

uint8_t expected_signed_results[] = {
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

static void print_buf(char *str, uint8_t *buf, int blen)
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

int SignVerify_OpenSSL(void)
{
   unsigned int iSigLen;
   int iRetval = -1;
   RSA *rsa;
   uint8_t abDigest[SHA1_HASH_LEN];
   uint8_t signedData[SIGNED_LEN];

   if ( (rsa = RSA_new()) != NULL )
   {
      BIO * bio;
      if ( (bio = BIO_new_mem_buf((void*)privPemKey, -1)) != NULL )
      {
         /* Create input data (44 bytes) */
         print_buf("Digest Input Data:", Digest_given, DATA_BLOCK_LEN);

         rsa = PEM_read_bio_RSAPrivateKey(bio, &rsa, NULL, NULL);
         BIO_free(bio);
         if (rsa == NULL) {
             char buffer[120];
             ERR_error_string(ERR_get_error(), buffer);
             printf("OpenSSL error: %s", buffer);
         }

         /* Create Digest (20 bytes) */
         iRetval = EVP_Digest(&Digest_given[0], DATA_BLOCK_LEN, abDigest, NULL, EVP_sha1(), NULL);
         printf("Digest SHA1 result %d\n", iRetval);
         print_buf("Digest Output 20 Data:", abDigest, sizeof(abDigest));

         /* Sign hash (128 bytes) */
         iRetval = RSA_sign(NID_sha1, abDigest, sizeof(abDigest), &signedData[0], &iSigLen, rsa);
         printf("RSA Sign result %d\n", iRetval);
         print_buf("OpenSSL Signed data results:", &signedData[0], iSigLen);

         /* Verify Signature */
         iRetval = RSA_verify(NID_sha1, abDigest, sizeof(abDigest), &signedData[0], SIGNED_LEN, rsa);
         if(iRetval != 1)
         {
            printf("CRYPTO: Internal error, signature does not verify\n");
            iRetval = -2;
         }
         else
         {
            printf("CRYPTO: signature verify OK! %d\n", iRetval);
            iRetval = 0;
         }
#if 1
         if (iRetval == 0) {
            /* Verify Signature with "expected_signature_results" */
            iRetval = RSA_verify(NID_sha1, abDigest, SHA1_HASH_LEN, &expected_signed_results[0], SIGNED_LEN, rsa);
            if(iRetval != 1)
            {
              printf("CRYPTO: Internal error, EXPECTED signature does not verify\n");
              iRetval = -2;
            }
            else
            {
              printf("CRYPTO: EXPECTED signature verify OK! %d\n", iRetval);
              iRetval = 0;
            }
         }
#endif
      }
      RSA_free(rsa);
   }
   return iRetval;
}

int main(int argc, char * argv[])
{
   return SignVerify_OpenSSL();
}
