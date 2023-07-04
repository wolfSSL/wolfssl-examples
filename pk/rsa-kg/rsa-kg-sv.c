/* rsa-kg-sv.c
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


#include <stdio.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/hash.h>

#include "rsa-key.h"

#define MIN_RSA_BITS        1024
#define MAX_RSA_BITS        4096
#define DEF_RSA_BITS        2048
#define DEF_KEYS_GEN        128
#define DEF_SV_CHECKS       512

/* Load the RSA private key */
int load_rsa_key(RsaKey *key)
{
    word32 idx = 0;
    return wc_RsaPrivateKeyDecode(rsa_priv_key, &idx, key,
                                  sizeof(rsa_priv_key));
}

/* Print the buffer as bytes */
void print_data(char *name, unsigned char *data, int len)
{
    int i;
    printf("static unsigned char %s[%d] = {\n", name, len);
    for (i = 0; i < len; i++) {
        if ((i & 7) == 0) {
            printf("    ");
        }
        printf("0x%02x, ", data[i]);
        if ((i & 7) == 7) {
            printf("\n");
        }
    }
    if ((i & 7) != 0) {
        printf("\n");
    }
    printf("};\n");
}

/* Print the components of an RSA key */
void print_rsa(RsaKey *key)
{
    printf("\n");
    unsigned char buf[2400];
    int len;

    len = wc_RsaKeyToDer(key, buf, sizeof(buf));
    if (len < 0) {
        fprintf(stderr, "Failed to encode RSA key\n");
    }

    print_data("rsa_priv_key", buf, len);
}

/* Shows usage information */
void usage()
{
    fprintf(stderr, "rsa_kg_sv <options>:\n");
    fprintf(stderr, "  -load-key         Load RSA key from rsa-key.h\n");
    fprintf(stderr, "  -bits <num>       Size in bits of RSA keys generated\n");
    fprintf(stderr, "                    Range: 1024-4096\n");
    fprintf(stderr, "  -num-keys <num>   Number of keys to generate\n");
    fprintf(stderr, "  -checks <num>     Number of signs/verifies to do\n");
    fprintf(stderr, "  -hash-size <num>  Number of bytes in hash\n");
    fprintf(stderr, "                    Range: 1-64. Default: 64\n");
    fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
    int ec = 0;
    int ret;
    RsaKey key;
    WC_RNG rng;
    unsigned char hash[WC_SHA512_DIGEST_SIZE];
    int hashSz = WC_SHA512_DIGEST_SIZE;
    int hashAlg = WC_HASH_TYPE_SHA512;
    unsigned char sig[MAX_RSA_BITS/8];
    int sig_len;
    unsigned char *out = NULL;
    int i, cnt;
    int bits = DEF_RSA_BITS;
    int numKeys = DEF_KEYS_GEN;
    int checks = DEF_SV_CHECKS;
    int load_key = 0;

    /* Skip program name */
    --argc;
    ++argv;
    /* Process command line arguments */
    while (argc > 0) {
        /* Load the key rather than generate */
        if (XSTRNCMP(*argv, "-load-key", 10) == 0) {
            load_key = 1;
        }
        /* Number of bits in RSA key to generate */
        else if (XSTRNCMP(*argv, "-bits", 6) == 0) {
            ++argv;
            if (--argc == 0) {
                fprintf(stderr, "Missing bits value\n");
                usage();
                return 1;
            }
            bits = atoi(*argv);
        }
        /* Number of keys to generate */
        else if (XSTRNCMP(*argv, "-num-keys", 10) == 0) {
            ++argv;
            if (--argc == 0) {
                fprintf(stderr, "Missing number of keys value\n");
                usage();
                return 1;
            }
            numKeys = atoi(*argv);
        }
        /* Number of sign/verify checks to perform */
        else if (XSTRNCMP(*argv, "-checks", 7) == 0) {
            ++argv;
            if (--argc == 0) {
                fprintf(stderr, "Missing check count value\n");
                usage();
                return 1;
            }
            checks = atoi(*argv);
        }
        /* Size of hash to use */
        else if (XSTRNCMP(*argv, "-hash-size", 11) == 0) {
            ++argv;
            if (--argc == 0) {
                fprintf(stderr, "Missing hash size value\n");
                usage();
                return 1;
            }
            hashSz = atoi(*argv);
        }
        else if (XSTRNCMP(*argv, "-help", 6) == 0) {
            usage();
            return 0;
        }
        else {
            fprintf(stderr, "Unrecognized option: %s\n", *argv);
            usage();
            return 1;
        }

        --argc;
        ++argv;
    }

    /* Only use one key when loading */
    if (load_key) {
        numKeys = 1;
    }

    /* Check bit count if generating keys */
    if (!load_key && (bits < MIN_RSA_BITS || bits > MAX_RSA_BITS)) {
        fprintf(stderr, "Bits out of range (%d-%d): %d\n", MIN_RSA_BITS,
                MAX_RSA_BITS, bits);
        usage();
        return 1;
    }
#ifdef WOLFSSL_SP_MATH
    if (0) {
    }
#ifndef WOLFSSL_SP_NO_2048
    else if (bits == 2048) {
    }
#endif
#ifndef WOLFSSL_SP_NO_3072
    else if (bits == 3072) {
    }
#endif
#ifdef WOLFSSL_SP_4096
    else if (bits == 4096) {
    }
#endif
    else {
        fprintf(stderr, "Bit size not supported with SP_MATH: %d\n", bits);
        fprintf(stderr, " wolfSSL compiled to support, in bits:");
#ifndef WOLFSSL_SP_NO_2048
        fprintf(stderr, " 2048");
#endif
#ifndef WOLFSSL_SP_NO_3072
        fprintf(stderr, " 3072");
#endif
#ifdef WOLFSSL_SP_4096
        fprintf(stderr, " 4096");
#endif
        fprintf(stderr, "\n");
        return 1;
    }
#endif
    /* Check number of keys */
    if (numKeys < 1) {
        fprintf(stderr, "Number of key out of range (1+): %d\n", numKeys);
        usage();
        return 1;
    }
    /* Check count */
    if (checks < 0) {
        fprintf(stderr, "Number of checks out of range (0+): %d\n", checks);
        usage();
        return 1;
    }
    /* Check hash size is valid */
    if (hashSz < 1 || hashSz > WC_SHA512_DIGEST_SIZE) {
        fprintf(stderr, "Hash size out of range (1-%d): %d\n",
                WC_SHA512_DIGEST_SIZE, hashSz);
        usage();
        return 1;
    }
    /* Display the options in use */
    if (!load_key) {
        printf("Bits:       %d\n", bits);
        printf("#Keys:      %d\n", numKeys);
    }
    else {
        printf("Loading key\n");
    }
    printf("Checks:     %d\n", checks);
    printf("Hash Size:  %d\n", hashSz);

    /* Initialize random number generator */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize random\n");
        return 1;
    }

    /* Initialize RSA key structure */
    ret = wc_InitRsaKey(&key, NULL);
    if (ret != 0) {
        wc_FreeRng(&rng);
        fprintf(stderr, "Failed to initialize RSA key\n");
        return 1;
    }

    /* Perform operations for specified number of keys */
    for (cnt = 0; cnt < numKeys; cnt++) {
        if (!load_key) {
            fprintf(stderr, "%d: ", cnt + 1);

            /* Generate a new RSA key with the default public exponent */
            ret = wc_MakeRsaKey(&key, bits, WC_RSA_EXPONENT, &rng);
            if (ret != 0) {
                fprintf(stderr, "Failed to make RSA key\n");
                ec = 1;
                break;
            }

            /* Check the RSA key */
            ret = wc_CheckRsaKey(&key);
            if (ret != 0) {
                fprintf(stderr, "Key failed checks\n");
                ec = 1;
                break;
            }

            /* Generate a new hash */
            ret = wc_RNG_GenerateBlock(&rng, hash, sizeof(hash));
            if (ret != 0) {
                fprintf(stderr, "Failed to initialize random\n");
                ec = 1;
                break;
            }
        }
        else {
            /* Load RSA key from rsa-key.h */
            ret = load_rsa_key(&key);
            if (ret != 0) {
                fprintf(stderr, "Failed to load RSA key\n");
                ec = 1;
                break;
            }

            /* From rsa-key.h */
            XMEMCPY(hash, digest, digestSz);
        }

        /* Perform a number of sign-verify checks */
        for (i = 0; i < checks; i++) {
            /* Sign with RSA key */
            ret = wc_RsaSSL_Sign(hash, hashSz, sig, sizeof(sig), &key, &rng);
            if (ret < 0) {
                print_rsa(&key);
                print_data("digest", hash, hashSz);
                printf("static int digestSz = %d;\n", hashSz);
                fprintf(stderr, "Failed to sign with RSA key\n");
                ec = 1;
                break;
            }
            sig_len = ret;
            if (load_key && i == 0) {
                print_data("sig", sig, sig_len);
            }

            /* Verify with RSA key */
            ret = wc_RsaSSL_VerifyInline(sig, sig_len, &out, &key);
            if (ret < 0) {
                print_rsa(&key);
                print_data("digest", hash, hashSz);
                printf("static int digestSz = %d;\n", hashSz);
                printf("\n");
                print_data("sig", sig, sig_len);
                fprintf(stderr, "Failed to verify with RSA key\n");
                ec = 1;
                break;
            }

            /* Check hash is the same */
            if ((ret != hashSz) || (XMEMCMP(out, hash, hashSz) != 0)) {
                print_rsa(&key);
                print_data("digest", hash, hashSz);
                printf("static int digestSz = %d;\n", hashSz);
                printf("\n");
                print_data("sig", sig, sig_len);
                fprintf(stderr, "Failed to verify with RSA key\n");
                ec = 1;
                break;
            }

            /* Generate a new hash to sign */
            ret = wc_Hash(hashAlg, hash, hashSz, hash, sizeof(hash));
            if (ret != 0) {
                fprintf(stderr, "Failed to make new hash\n");
                ec = 1;
                break;
            }
    
            fprintf(stderr, ".");
        }

        /* Error during check - break out. */
        if (ec) {
            break;
        }
        fprintf(stderr, "\n");
    }

    /* Free allocated items */
    wc_FreeRsaKey(&key);
    wc_FreeRng(&rng);

    return ec;
}

