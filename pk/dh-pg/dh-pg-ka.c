/* dh-pg-ka.c
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
#include <wolfssl/wolfcrypt/dh.h>

#include "dh-params.h"

#define MAX_DH_BITS       4096
#define MAX_DH_Q_SIZE     256
#define DEF_DH_SIZE       2048
#define DEF_KA_CHECKS     512
#define DEF_PARAMS_GEN    8

int load_dh_params(DhKey* key1, DhKey* key2, WC_RNG* rng)
{
    int ret;

    ret = wc_DhSetCheckKey(key1, dh_p, sizeof(dh_p), dh_g, sizeof(dh_g),
                           dh_q, sizeof(dh_q), 0, rng);
    if (ret == 0) {
        ret = wc_DhSetCheckKey(key2, dh_p, sizeof(dh_p), dh_g, sizeof(dh_g),
                               NULL, 0, 0, rng);
    }

    return ret;
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

/* Print the DH parameters */
void print_dh(DhKey *key)
{
    int ret;
    static unsigned char p[MAX_DH_BITS/8];
    static unsigned char g[MAX_DH_BITS/8];
    static unsigned char q[MAX_DH_Q_SIZE/8];
    word32 p_len, g_len, q_len;

    /* Export the DH parameters */
    p_len = sizeof(p);
    g_len = sizeof(g);
    q_len = sizeof(q);
    ret = wc_DhExportParamsRaw(key, p, &p_len, q, &q_len, g, &g_len);
    if (ret != 0) {
        fprintf(stderr, "Failed to export parameters\n");
        return;
    }

    /* Print out parameters. */
    printf("\n");
    print_data("dh_p", p, p_len);
    print_data("dh_g", g, g_len);
    print_data("dh_q", q, q_len);
}

/* Show usage information */
void usage()
{
    fprintf(stderr, "dh-pg-ka <options>:\n");
    fprintf(stderr, "  -load            Load the parameters\n");
    fprintf(stderr, "  -bits <num>      Size in bits of RSA keys\n");
    fprintf(stderr, "  -num-gen <num>   Number of params to generate\n");
    fprintf(stderr, "  -checks <num>    Number of key exchanges to do\n");
    fprintf(stderr, "  -ffdhe           Used pre-defined FFDHE params\n");
    fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
    int ec = 0;
    int ret;
    static DhKey key1, key2;
    const DhParams *dhparams;
    WC_RNG rng;
    static unsigned char p[MAX_DH_BITS/8];
    static unsigned char g[MAX_DH_BITS/8];
    static unsigned char q[MAX_DH_Q_SIZE/8];
    word32 p_len, g_len, q_len;
    static unsigned char priv1[MAX_DH_BITS/8];
    static unsigned char priv2[MAX_DH_BITS/8];
    word32 priv1_len, priv2_len;
    static unsigned char pub1[MAX_DH_BITS/8];
    static unsigned char pub2[MAX_DH_BITS/8];
    word32 pub1_len, pub2_len;
    static unsigned char secret1[MAX_DH_BITS/8];
    static unsigned char secret2[MAX_DH_BITS/8];
    word32 secret1_len, secret2_len;
    int i, cnt;
    int bits = DEF_DH_SIZE;
    int numParams = DEF_PARAMS_GEN;
    int checks = DEF_KA_CHECKS;
    int gen_params = 1;
    int load_params = 0;

    /* Skip the program name */
    --argc;
    ++argv;

    /* Process the command line arguments */
    while (argc > 0) {
        /* Load the parameters from dh-params.h */
        if (XSTRNCMP(*argv, "-load", 7) == 0) {
            load_params = 1;
            gen_params = 0;
        }
        /* Number of bits in DH parameters to generate */
        else if (XSTRNCMP(*argv, "-bits", 6) == 0) {
            ++argv;
            if (--argc == 0) {
                fprintf(stderr, "Missing bits value\n");
                usage();
                return 1;
            }
            bits = atoi(*argv);
        }
        /* Number of DH parameters to generate */
        else if (XSTRNCMP(*argv, "-num-gen", 9) == 0) {
            ++argv;
            if (--argc == 0) {
                fprintf(stderr, "Missing loops value\n");
                usage();
                return 1;
            }
            numParams = atoi(*argv);
        }
        /* Number of key agreement checks to perform */
        else if (XSTRNCMP(*argv, "-checks", 7) == 0) {
            ++argv;
            if (--argc == 0) {
                fprintf(stderr, "Missing check count value\n");
                usage();
                return 1;
            }
            checks = atoi(*argv);
        }
        /* Use the pre-defined FFDHE parameters */
        else if (XSTRNCMP(*argv, "-ffdhe", 7) == 0) {
            gen_params = 0;
        }
        /* Display usage information */
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

    /* Check bits are valid */
    if (bits != 1024 && bits != 2048 && bits != 3072 && bits != 4096) {
        fprintf(stderr, "Bits out of range (1024, 2048, 3072 or 4086): %d\n",
                bits);
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
    if (bits > MAX_DH_BITS) {
        fprintf(stderr, "Program isn't supporting bit sizes greater than %d\n",
                MAX_DH_BITS);
        return 1;
    }
    /* Only dping one set of parameters unless generating */
    if (!gen_params) {
        numParams = 1;
    }

    if (gen_params) {
        /* Check valid bits for DH parameter generation */
        if (bits == 4096) {
            fprintf(stderr, "4096-bit parameter generation not supported\n");
            return 1;
        }
    }
    else if (load_params) {
        /* Nothing to do */
    }
#ifdef HAVE_FFDHE_2048
    else if (bits == 2048) {
        /* Set FFDHE-2048 */
        dhparams = wc_Dh_ffdhe2048_Get();
    }
#endif
#ifdef HAVE_FFDHE_3072
    else if (bits == 3072) {
        /* Set FFDHE-3072 */
        dhparams = wc_Dh_ffdhe3072_Get();
    }
#endif
#ifdef HAVE_FFDHE_4096
    else if (bits == 4096) {
        /* Set FFDHE-4096 */
        dhparams = wc_Dh_ffdhe4096_Get();
    }
#endif
    else {
        /* Set FFDHE size not supported */
        fprintf(stderr, "Unsupported FFDHE parameters: %d\n", bits);
        return 1;
    }

    /* Display the options to use */
    if (gen_params) {
        printf("Bits:     %d\n", bits);
        printf("#Params:  %d\n", numParams);
    }
    else if (!load_params) {
        printf("FFHDE:    %d\n", bits);
    }
    else {
        printf("Load parameters\n");
    }
    printf("Checks:   %d\n", checks);

    /* Initialise a random number generator for generation */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize random\n");
        return 1;
    }

    /* Initialise DH key */
    ret = wc_InitDhKey(&key1);
    if (ret != 0) {
        wc_FreeRng(&rng);
        fprintf(stderr, "Failed to initialize DH key\n");
        return 1;
    }
    /* Initialise DH key for key agreement */
    ret = wc_InitDhKey(&key2);
    if (ret != 0) {
        wc_FreeDhKey(&key1);
        wc_FreeRng(&rng);
        fprintf(stderr, "Failed to initialize DH key\n");
        return 1;
    }

    /* Perform operations for specified number of parameters */
    for (cnt = 0; cnt < numParams; cnt++) {
        if (gen_params) {
            fprintf(stderr, "%d: ", cnt + 1);

            /* Generate a set of DH parameters */
            ret = wc_DhGenerateParams(&rng, bits, &key1);
            if (ret != 0) {
                fprintf(stderr, "Failed to generate DH params\n");
                fprintf(stderr, "%d\n", ret);
                ec = 1;
                break;
            }

            /* Export DH parameters */
            p_len = sizeof(p);
            q_len = sizeof(q);
            g_len = sizeof(g);
            ret = wc_DhExportParamsRaw(&key1, p, &p_len, q, &q_len, g, &g_len);
            if (ret != 0) {
                fprintf(stderr, "Failed to export DH params\n");
                ec = 1;
                break;
            }

            /* Set only p and g and check key */
            ret = wc_DhSetCheckKey(&key2, p, p_len, g, g_len, NULL, 0, 0, &rng);
            if (ret != 0) {
                fprintf(stderr, "Failed to set/check DH params\n");
                ec = 1;
                break;
            }
        }
        else if (!load_params) {
            /* Set p and g - trust key */
            ret = wc_DhSetCheckKey(&key2, dhparams->p, dhparams->p_len,
                                   dhparams->g, dhparams->g_len, NULL, 0, 1,
                                   &rng);
            if (ret != 0) {
                fprintf(stderr, "Failed to set/check DH params\n");
                ec = 1;
                break;
            }
        }
        else {
            /* Set p and g - trust key */
            ret = load_dh_params(&key1, &key2, &rng);
            if (ret != 0) {
                fprintf(stderr, "Failed to load DH params\n");
                ec = 1;
                break;
            }
        }

        /* Perform a number of key agreements with parameters */
        for (i = 0; i < checks; i++) {
            /* Generate first peer's key */
            priv1_len = sizeof(priv1);
            pub1_len = sizeof(pub1);
            ret = wc_DhGenerateKeyPair(&key2, &rng, priv1, &priv1_len, pub1,
                                       &pub1_len);
            if (ret != 0) {
                fprintf(stderr, "Failed to generate key pair\n");
                print_dh(&key1);
                ec = 1;
                break;
            }
    
            /* Generate second peer's key */
            priv2_len = sizeof(priv2);
            pub2_len = sizeof(pub2);
            ret = wc_DhGenerateKeyPair(&key2, &rng, priv2, &priv2_len, pub2,
                                       &pub2_len);
            if (ret != 0) {
                fprintf(stderr, "Failed to generate key pair\n");
                print_dh(&key1);
                ec = 1;
                break;
            }
    
            /* Calculate first peer's secret */
            secret1_len = sizeof(secret1);
            ret = wc_DhAgree(&key2, secret1, &secret1_len, priv1, priv1_len,
                             pub2, pub2_len);
            if (ret != 0) {
                fprintf(stderr, "Failed to calculate secret\n");
                print_dh(&key1);
                ec = 1;
                break;
            }
    
            /* Calculate second peer's secret */
            secret2_len = sizeof(secret2);
            ret = wc_DhAgree(&key2, secret2, &secret2_len, priv2, priv2_len,
                             pub1, pub1_len);
            if (ret != 0) {
                fprintf(stderr, "Failed to calculate secret\n");
                print_dh(&key1);
                ec = 1;
                break;
            }

            /* Secret's should be the same */
            if ((secret1_len != secret2_len) || (XMEMCMP(secret1, secret2,
                                                         secret1_len) != 0)) {
                fprintf(stderr, "Secrets different\n");
                print_dh(&key1);
                ec = 1;
                break;
            }
    
            fprintf(stderr, ".");
        }

        /* Error  during key generation or key agreement */
        if (ec) {
            break;
        }

        fprintf(stderr, "\n");
    }

    /* Free allocated items */
    wc_FreeDhKey(&key1);
    wc_FreeDhKey(&key1);
    wc_FreeRng(&rng);

    return 0;
}

