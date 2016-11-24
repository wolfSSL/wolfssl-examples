/* wolfsslCertSetup.c
 *
 * Copyright (C) 2006-2015 wolfSSL Inc.
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

#ifdef WOLFSSL_STATIC_MEMORY
    #include <wolfssl/wolfcrypt/memory.h>
    static WOLFSSL_HEAP_HINT* HEAP_HINT;
else
    #define HEAP_HINT NULL
#endif

int wolfsslCertSetup(int argc, char** argv)
{
    int i;
    char* inform;
    char* outform;

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
                printf("IDENTIFIED PEM\n");
            if (XSTRNCMP(inform, "der", 3) == 0)
                printf("IDENTIFIED DER\n");
        } else if (XSTRNCMP(argv[i], "-outform", 8) == 0) {

            convert_to_lower(argv[i+1], (int) XSTRLEN(argv[i+1]));
            outform = argv[i+1];

            printf("outform is %s\n", outform);

            if (XSTRNCMP(outform, "pem", 3) == 0)
                printf("IDENTIFIED PEM\n");
            if (XSTRNCMP(outform, "der", 3) == 0)
                printf("IDENTIFIED DER\n");
        }
    }

    return 0;
}

void convert_to_lower(char* s, int sSz)
{
    int i;
    for (i = 0; i < sSz; i++) {
        s[i] = tolower(s[i]);
    }
}

void wolfsslCertHelp()
{
    printf("\n\n\nThis would be the certificate help.\n\n\n");
}
