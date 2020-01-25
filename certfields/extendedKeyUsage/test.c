/* test.c
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

#include <stdio.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>

#ifdef OPENSSL_EXTRA
void print_extended_key_use(WOLF_STACK_OF(WOLFSSL_ASN1_OBJECT)* sk, int crit);

void print_extended_key_use(WOLF_STACK_OF(WOLFSSL_ASN1_OBJECT)* sk, int crit)
{
    WOLFSSL_ASN1_OBJECT* obj = NULL;
    char DecodedString[MAX_OID_STRING_SZ] = {0};
    int outputSz = 0;

    if (crit)
        printf("ExtendedKeyUsage is marked critical\n");

    if (sk) {
        while ((obj = wolfSSL_sk_ASN1_OBJECT_pop(sk)) != NULL) {
            outputSz = wolfSSL_OBJ_obj2txt(DecodedString, MAX_OID_STRING_SZ,
                                           obj, 1);
            if (outputSz > 0)
                printf("extKeyUsage OID: %s\n", DecodedString);

            wolfSSL_ASN1_OBJECT_free(obj);
        }
    } else {
        /* silence unused warnings */
        (void) obj;
        (void) DecodedString;
        (void) outputSz;
    }
    return;
}

#endif

int main(int argc, char** argv)
{
#ifdef OPENSSL_EXTRA
    char cert1FName[] = "../../certs/server-ecc.pem";
    char cert2FName[] = "../../certs/ocsp-responder-cert.pem";
    int numCerts = 2;
    char* certFName = NULL;

    WOLFSSL_X509* x509 = NULL;
    WOLF_STACK_OF(WOLFSSL_ASN1_OBJECT)* sk;

    int crit = 0;
    int i;

    for (i = 0; i < numCerts; i++) {
        if (i == 0)
            certFName = cert1FName;
        else
            certFName = cert2FName;

        printf("-----------------------------------------------------------\n");
        printf("Looking at cert: %s\n", certFName);

        x509 = wolfSSL_X509_load_certificate_file(certFName, WOLFSSL_FILETYPE_PEM);
        if (x509 == NULL) {
            printf("Failed to load file %s\n", certFName);
            return -999;
        }

        sk = (WOLF_STACK_OF(WOLFSSL_ASN1_OBJECT)*)wolfSSL_X509_get_ext_d2i(x509,
                                                  NID_ext_key_usage,
                                                  &crit, NULL);

        print_extended_key_use(sk, crit);

        wolfSSL_sk_ASN1_OBJECT_free(sk);
        wolfSSL_X509_free(x509);
    }

#else
    printf("Please configure wolfSSL with --enable-opensslextra and try"
           " again\n");
#endif
    return 0;
}


