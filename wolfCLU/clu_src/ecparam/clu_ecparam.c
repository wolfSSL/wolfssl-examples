/* clu_ecparam.c
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

#include "clu_include/clu_header_main.h"
#include "clu_include/genkey/clu_genkey.h"
#include "clu_include/x509/clu_cert.h"    /* PER_FORM/DER_FORM */
#include "clu_include/clu_optargs.h"

static void wolfCLU_ecparamHelp(void)
{
    printf("./wolfssl ecparam\n");
    printf("\t-genkey create new key\n");
    printf("\t-out output file\n");
    printf("\t-name curve name i.e. secp384r1\n");
}


int wolfCLU_ecparam(int argc, char** argv)
{
    char  name[ECC_MAXNAME]; /* name of curve to use */
    char* out  = NULL;    /* default output file name */
    int   ret        = 0; /* return variable */
    int   long_index = 0;
    int   genKey     = 0;
    int   outForm    = PEM_FORM;
    int   i, option;
    WC_RNG rng;

    XMEMSET(name, 0, ECC_MAXNAME);
    ret = wolfCLU_checkForArg("-h", 2, argc, argv);
    if (ret > 0) {
        wolfCLU_ecparamHelp();
    }

    opterr = 0; /* do not display unrecognized options */
    optind = 0; /* start at indent 0 */
    while ((option = getopt_long_only(argc, argv, "",
                   ecparam_options, &long_index )) != -1) {

        switch (option) {
            case OUTFILE:
                out = optarg;
                break;

            case OUTFORM:
                outForm = wolfCLU_checkOutform(optarg);
                if (outForm < 0) {
                    printf("bad outform\n");
                    return USER_INPUT_ERROR;
                }
                break;

            case WOLFCLU_GEN_KEY:
                genKey = 1;
                break;

            case WOLFCLU_CURVE_NAME:
                XSTRNCPY(name, optarg, ECC_MAXNAME);

                /* convert name to upper case */
                for (i = 0; i < XSTRLEN(name); i++)
                    (void)toupper(name[i]);

                #if 0
                /* way to get the key size if needed in the future */
                keySz = wc_ecc_get_curve_size_from_name(name);
                #endif

                break;

            case ':':
            case '?':
                break;

            default:
                /* do nothing. */
                (void)ret;
        }
    }

    if (genKey == 0) {
        printf("only supporting genkey so far\n");
        return 0;
    }

    ret = wc_InitRng(&rng);
    if (ret == 0) {
        ret = wolfCLU_genKey_ECC_ex(&rng, out, ECPARAM, outForm, name);
        wc_FreeRng(&rng);
    }
    return ret;
}

