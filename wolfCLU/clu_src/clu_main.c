/* clu_main.c
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.
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

#include "clu_include/clu_header_main.h"
#include "clu_include/x509/clu_cert.h"
#include "clu_include/clu_optargs.h"
#include "clu_include/clu_error_codes.h"
#include "clu_include/x509/clu_request.h"
#include "clu_include/genkey/clu_genkey.h"
#include "clu_include/sign-verify/clu_sign_verify_setup.h"
/* enumerate optionals beyond ascii range to dis-allow use of alias IE we
 * do not want "-e" to work for encrypt, user must use "encrypt"
 */

int main(int argc, char** argv)
{
    int     flag = 0;
    char*   mode = "";
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

    while ((option = getopt_long_only(argc, argv,"",
                   long_options, &long_index )) != -1) {

        switch (option) {

            /* @temporary: implement the modes as arguments */
        case ENCRYPT:
        case DECRYPT:
        case BENCHMARK:
        case HASH:
        case X509:
        case REQUEST:
        case GEN_KEY:
        case RSA:

            if (!flag) flag = option;

            /*
             * Ignore the following arguments for now. They will be handled by
             * their respective setups (e.g. Crypto setup, Benchmark setup, or
             * Hash Setup)
             */

        case PASSWORD: /* Password                                  */
        case KEY:      /* Key if used must be in hex                */
        case IV:       /* IV if used must be in hex                 */
        case ALL:      /* Opt to benchmark all available algorithms */
        case SIZE:     /* size for hash or key to output            */
        case EXPONENT: /* exponent for generating RSA key           */
        case TIME:     /* Time to benchmark for                     */
        case SIGN:
        case VERIFY:   /* Verify results, used with -iv and -key    */
        case INFORM:   /* Certificate Stuff                         */
        case OUTFORM:
        case OUTPUT:
        case NOOUT:
        case TEXT_OUT:
        case SILENT:
        case PUBIN:

            /* The cases above have their arguments converted to lower case */
            if (optarg) convert_to_lower(optarg, (int)XSTRLEN(optarg));
            /* The cases below won't have their argument's molested */

        case INFILE:   /* File passed in by user                    */
        case OUTFILE:  /* Output file                               */
        case INKEY:

            /* do nothing. */

            break; /* note: this is the first break in the block */

            /*
             * End of ignored arguments
             */

        case HELP:
            /* only print for -help if no mode has been declared */
            if (!flag) {
                printf("Main help menu:\n");
                wolfCLU_help();
                return 0;
            }
            break;

        case VERBOSE:
            wolfCLU_verboseHelp();
            return 0;

        case 'v':
            wolfCLU_version();
            return 0;

        default:
            printf("Main help default.\n");
            wolfCLU_help();
            return 0;
        }
    }

    /* @temporary: implement mode as a flag */
    switch (flag) {
    case 0:
        printf("No mode provided.\n");
        ret = 0;
        break;

    case ENCRYPT:
        ret = wolfCLU_setup(argc, argv, 'e');
        break;

    case DECRYPT:
        ret = wolfCLU_setup(argc, argv, 'd');
        break;

    case BENCHMARK:
        ret = wolfCLU_benchSetup(argc, argv);
        break;

    case HASH:
        ret = wolfCLU_hashSetup(argc, argv);
        break;

    case X509:
        ret = wolfCLU_certSetup(argc, argv);
        break;

    case REQUEST:
        ret = wolfCLU_requestSetup(argc, argv);
        break;

    case GEN_KEY:
        ret = wolfCLU_genKeySetup(argc, argv);
        break;
    
    case RSA:
        ret = wolfCLU_sign_verify_setup(argc, argv);
        break;
    
    }
    
    if (ret != 0)
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

