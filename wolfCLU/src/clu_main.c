/* clu_main.c
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

#include <wolfclu/clu_header_main.h>
#include <wolfclu/x509/clu_cert.h>
#include <wolfclu/clu_optargs.h>
#include <wolfclu/clu_error_codes.h>
#include <wolfclu/x509/clu_request.h>
#include <wolfclu/genkey/clu_genkey.h>
#include <wolfclu/pkey/clu_pkey.h>
#include <wolfclu/sign-verify/clu_sign_verify_setup.h>
/* enumerate optionals beyond ascii range to dis-allow use of alias IE we
 * do not want "-e" to work for encrypt, user must use "encrypt"
 */


/**
 * Takes in the second string passed into the function and compares it to known
 * modes. When a match is found the modes value is returned, otherwise a
 * negative value is returned.
 */
static int getMode(char* arg)
{
    int ret = -1, i = 0;

    if (arg != NULL) {
        int argSz = XSTRLEN(arg);
        struct option current = mode_options[i];
        while (current.name != NULL) {
            if (XSTRLEN(current.name) == argSz &&
                    XSTRNCMP(arg, current.name, argSz) == 0) {
                ret = current.val;
                break;
            }
            current = mode_options[i];
            i = i+1;
        }
    }
    return ret;
}



int main(int argc, char** argv)
{
    int     flag = 0;
    char*   mode;
    int     ret = 0;
    int     option = 0;
    int     ignoreIn = 0;
    int     ignoreOut = 0;
    int     long_index = 0;
    int     i;

    if (argc == 1) {
        printf("Main Help.\n");
        wolfCLU_help();
    }

    /* If the first string does not have a '-' in front of it then try to
     * get the mode to use i.e. x509, req, version ... this is for
     * compatibility with the behavior of the OpenSSL command line utility
     */
    if (argc > 1 && argv[1] != NULL && argv[1][0] != '-') {
        flag = getMode(argv[1]);
    }
    else {
        /* retain old version of modes where '-' is used. i.e -x509, -req */
        flag = getopt_long_only(argc, argv,"", mode_options, &long_index);
    }

    switch (flag) {
        case 0:
            printf("No mode provided.\n");
            ret = 0;
            break;

        case WOLFCLU_CRYPT:
            /* generic 'enc' used, default to encrypt unless -d was used */
            ret = wolfCLU_checkForArg("-d", 2, argc, argv);
            if (ret > 0) {
                ret = wolfCLU_setup(argc, argv, 'd');
            }
            else {
                ret = wolfCLU_setup(argc, argv, 'e');
            }
            break;

        case WOLFCLU_ENCRYPT:
            ret = wolfCLU_setup(argc, argv, 'e');
            break;

        case WOLFCLU_DECRYPT:
            ret = wolfCLU_setup(argc, argv, 'd');
            break;

        case WOLFCLU_BENCHMARK:
            ret = wolfCLU_benchSetup(argc, argv);
            break;

        case WOLFCLU_HASH:
            ret = wolfCLU_hashSetup(argc, argv);
            break;

        case WOLFCLU_MD5:
            ret = wolfCLU_md5Setup(argc, argv);
            break;

        case WOLFCLU_X509:
            ret = wolfCLU_certSetup(argc, argv);
            break;

        case WOLFCLU_REQUEST:
            ret = wolfCLU_requestSetup(argc, argv);
            break;

        case WOLFCLU_GEN_KEY:
            ret = wolfCLU_genKeySetup(argc, argv);
            break;

        case WOLFCLU_ECPARAM:
            ret = wolfCLU_ecparam(argc, argv);
            break;

        case WOLFCLU_PKEY:
            ret = wolfCLU_pKeySetup(argc, argv);
            break;

        case WOLFCLU_DGST:
            ret = wolfCLU_dgst_setup(argc, argv);
            break;

        case WOLFCLU_RSA:
        case WOLFCLU_ECC:
        case WOLFCLU_ED25519:
            ret = wolfCLU_sign_verify_setup(argc, argv);
            break;

        case WOLFCLU_HELP:
            /* only print for -help if no mode has been declared */
            printf("Main help menu:\n");
            wolfCLU_help();
            return 0;

        case WOLFCLU_VERBOSE:
            wolfCLU_verboseHelp();
            return 0;

        case 'v':
            (void)wolfCLU_version();
            return 0;

        default:
            printf("Unknown mode\n");
            wolfCLU_help();
            return -1;
    }

    if (ret < 0)
        printf("Error returned: %d.\n", ret);

    return ret;
}

void convert_to_lower(char* s, int sSz)
{
    int i;
    for (i = 0; i < sSz; i++) {
        s[i] = tolower(s[i]);
    }
}

