/* clu_cert_setup.c
 *
 * Copyright (C) 2006-2016 wolfSSL Inc.
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

#include "clu_include/x509/clu_cert.h"
#include <wolfssl/wolfcrypt/types.h>
#include "clu_include/clu_optargs.h"
#include "clu_include/clu_error_codes.h"

#ifdef WOLFSSL_STATIC_MEMORY
    #include <wolfssl/wolfcrypt/memory.h>
    static WOLFSSL_HEAP_HINT* HEAP_HINT;
else
    #define HEAP_HINT NULL
#endif

enum {
    INPEM_OUTPEM = 1,
    INPEM_OUTDER = 2,
    INDER_OUTPEM = 3,
    INDER_OUTDER = 4,
};

int wolfsslCertSetup(int argc, char** argv)
{
    int i, ret;
    char* inform;
    char* outform;
    int inder = 0, inpem = 0, outder = 0, outpem = 0;

    printf("In x509 loop\n");
    for (i = 2; i < argc; i++) {
        convert_to_lower(argv[i], (int) XSTRLEN(argv[i]));
        if (XSTRNCMP(argv[i], "-help", 5) == 0) {
            wolfsslCertHelp();
            return 0;
        } else if (XSTRNCMP(argv[i], "-h", 2) == 0) {
            wolfsslCertHelp();
            return 0;
        } else if (XSTRNCMP(argv[i], "-inform", 7) == 0) {

            convert_to_lower(argv[i+1], (int) XSTRLEN(argv[i+1]));
            inform = argv[i+1];

            printf("inform is %s\n", inform);

            if (XSTRNCMP(inform, "pem", 3) == 0)
                inpem = 1;
            if (XSTRNCMP(inform, "der", 3) == 0)
                inder = 1;
        } else if (XSTRNCMP(argv[i], "-outform", 8) == 0) {

            convert_to_lower(argv[i+1], (int) XSTRLEN(argv[i+1]));
            outform = argv[i+1];

            printf("outform is %s\n", outform);

            if (XSTRNCMP(outform, "pem", 3) == 0)
                outpem = 1;
            if (XSTRNCMP(outform, "der", 3) == 0)
                outder = 1;
        }
    }

    ret = error_check(inpem, inder, outpem, outder);
    printf("ret = %d\n", ret);
    switch (ret) {
        case INPEM_OUTPEM:
            ret = 0;
            printf("run inpem outpem\n");
            break;
        case INPEM_OUTDER:
            ret = 0;
            printf("run inpem outder\n");
            break;
        case INDER_OUTPEM:
            ret = 0;
            printf("run inder outpem\n");
            break;
        case INDER_OUTDER:
            ret = 0;
            printf("run inder outder\n");
            break;
        default:
            goto clu_cert_end;
            break;
    }
clu_cert_end:
    return ret;
}


void wolfsslCertHelp()
{
    printf("\n\n\nThis would be the certificate help.\n\n\n");
}

/*
 * @arg a: is inform set to pem
 * @arg b: is inform set to der
 * @arg c: is outform set to pem
 * @arg d: is outform set to der
 */
int error_check(int inpem, int inder, int outpem, int outder)
{
    int ret = USER_INPUT_ERROR;
    ret = ( inpem & inder);
    if (ret) {
        printf("ERROR: inform set to both PEM and DER format\n");
        return USER_INPUT_ERROR;
    }
    ret = ( inpem & outpem);
    if (ret) {
        printf("input is pem format, output is pem format\n");
        return INPEM_OUTPEM;
   }
    ret = (inpem & outder);
    if (ret) {
        printf("input is pem format, output is der format\n");
        return INPEM_OUTDER;
    }
    ret = (inder & outpem);
    if (ret) {
        printf("input is der format, output is pem format\n");
        return INDER_OUTPEM;
    }
    ret = (inder & outder);
    if (ret) {
        printf("input is der format, output is der format\n");
        return INDER_OUTDER;
    }
    ret = (outder & outpem);
    if (ret) {
        printf("ERROR: outform set to both DER and PEM format\n");
        return USER_INPUT_ERROR;
    }
    if (!ret) {
        ret = USER_INPUT_ERROR;
        if ( !inpem && !inder)
            printf("User failed to specify input format: -inform not set\n");
        else
            printf("User failed to specify output format: -outform not set\n");
    }
    return ret;
}
