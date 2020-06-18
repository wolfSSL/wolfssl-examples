/* srp_gen.c
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

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/srp.h>

#include <stdio.h>

#ifdef WOLFCRYPT_HAVE_SRP

#include "srp_params.h"

/* Generate a new random salt */
static int generate_random_salt(byte *buf, word32 size)
{
    int ret;
    WC_RNG rng;

    ret = wc_InitRng(&rng);
    if (ret == 0) {
        ret = wc_RNG_GenerateBlock(&rng, (byte *)buf, size);

        wc_FreeRng(&rng);
    }

    return ret;
}

/* Print out the buffer as a C variable */
static void print_buf(char *name, byte* buf, word32 sz)
{
    int i;

    printf("static const byte %s[%d] = {", name, sz);
    for (i = 0; i < (int)sz; i++) {
        if ((i % 12) == 0) {
            printf("\n    ");
        }
        printf("0x%02x,", buf[i]);
    }
    printf("\n};\n");
}

int main(int argc, char* argv[])
{
    int ret;
    Srp srp;
    byte salt[20];
    byte verifier[80];
    word32 vSz = (word32)sizeof(verifier);
    char* username;
    char* password;

    if (argc < 3) {
        fprintf(stderr, "usage: %s <username> <password>\n", argv[0]);
        return 1;
    }

    username = argv[1];
    password = argv[2];

    ret = generate_random_salt(salt, sizeof(salt));
    if (ret == 0) {
        ret = wc_SrpInit(&srp, SRP_TYPE_SHA, SRP_CLIENT_SIDE);
    }
    if (ret == 0) {
        ret = wc_SrpSetUsername(&srp, (byte*)username, XSTRLEN(username));
        if (ret == 0) {
            ret = wc_SrpSetParams(&srp, srp_n_640, sizeof(srp_n_640),
                                  srp_g_640, sizeof(srp_g_640),
                                  salt, sizeof(salt));
        }
        if (ret == 0) {
            ret = wc_SrpSetPassword(&srp, (byte*)password, XSTRLEN(password));
        }
        if (ret == 0) {
            ret = wc_SrpGetVerifier(&srp, verifier, &vSz);
        }

        wc_SrpTerm(&srp);
    }

    /* Output the details to store on the server */
    printf("static const char* srp_username = \"%s\";\n", username);
    print_buf("salt", salt, sizeof(salt));
    print_buf("verifier", verifier, vSz);

    return (ret == 0) ? 0 : 1;
}

#else

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    fprintf(stderr, "Must build wolfSSL with SRP enabled for this example\n");
    return 0;
}

#endif



