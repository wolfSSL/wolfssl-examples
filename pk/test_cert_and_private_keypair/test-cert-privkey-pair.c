/* test-cert-privkey-pair.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/*
* An implementation that checks if a private key and cert are a pair
* Usage:
./test-cert-privkey-pair key.pem cert.pem
*/

#include <stdio.h>
#include <string.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>

#define LARGEST_FILE_PATH 1024

void Usage(void);
int main(int argc, char** argv)
{
    WOLFSSL_CTX* ctx;
    int ret;
    char* svrKeyFile = NULL;
    char* svrCertFile = NULL;

    if (argc == 3) {
        (void) svrKeyFile;
        (void) svrCertFile;
        svrKeyFile = argv[1];
        svrCertFile = argv[2];
    } else {
        Usage();
        return -1;
    }

    ctx = wolfSSL_CTX_new(wolfSSLv23_server_method());

    ret = wolfSSL_CTX_use_PrivateKey_file(ctx, svrKeyFile,
                                          WOLFSSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        printf("Failed to load PRIVATE KEY %s, error was: %d\n", svrKeyFile,
                ret);
        ret = -1;
        goto cleanup;
    }

    ret = wolfSSL_CTX_use_certificate_file(ctx, svrCertFile,
                                           WOLFSSL_FILETYPE_PEM);
    if (ret != WOLFSSL_SUCCESS) {
        printf("Failed to load CERTIFICATE %s, error was: %d\n", svrCertFile,
                ret);
        ret = -2;
        goto cleanup;
    }

    ret = wolfSSL_CTX_check_private_key(ctx);
    if (ret != WOLFSSL_SUCCESS) {
        printf("This key and cert are not a pair\n");
        printf("Error: %d\n", ret);
    } else {
        printf("Congratulations, this private key and cert are a pair!\n");
    }

cleanup:
    wolfSSL_CTX_free(ctx);
    if (ret == WOLFSSL_SUCCESS)
        ret = 0;
    return ret;
}

void Usage(void)
{
    printf("Please specify a cert and key to check\n");
    printf("Example: ./test-cert-privkey-pair ../../certs/server-key.pem "
           "../../certs/server-cert.pem\n");
}


