/* clu_md5.c
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

#define MAX_BUFSIZE 8192

int wolfCLU_md5Setup(int argc, char** argv)
{
    WOLFSSL_BIO *bioIn  = NULL;
    WOLFSSL_BIO *bioOut = NULL;
    int     ret         = 0;

#ifdef NO_MD5
    printf("wolfCrypt compiled without MD5 support\n");
#endif

    /* was a file input provided? if so read from file */
    if (argc >= 3) {
        bioIn = wolfSSL_BIO_new_file(argv[2], "rb");
        if (bioIn == NULL) {
            printf("unable to open file %s\n", argv[2]);
            return USER_INPUT_ERROR;
        }
    }

    /* hashing function */
    ret = wolfCLU_hash(bioIn, bioOut, "md5", WC_MD5_DIGEST_SIZE);
    wolfSSL_BIO_free(bioIn);
    return ret;
}

