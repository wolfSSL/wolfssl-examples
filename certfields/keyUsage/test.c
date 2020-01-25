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
void print_use(unsigned int usageMask, char* usage);

void print_use(unsigned int usageMask, char* usage)
{
    if (usageMask)
        printf("cert can be used for %s\n", usage);
}
#endif

int main(int argc, char** argv)
{
#ifdef OPENSSL_EXTRA
    char certFName[] = "../../certs/test-intermediate.pem";
    WOLFSSL_X509* x509 = NULL;
    unsigned int keyUsage = 0;

    x509 = wolfSSL_X509_load_certificate_file(certFName, WOLFSSL_FILETYPE_PEM);
    if (x509 == NULL) {
        printf("Failed to load file %s\n", certFName);
        return -999;
    }

    keyUsage = wolfSSL_X509_get_keyUsage(x509);

    print_use((keyUsage & KEYUSE_DIGITAL_SIG), "DIGITAL SIGNATURE");
    print_use((keyUsage & KEYUSE_CONTENT_COMMIT), "CONTENT COMMIT");
    print_use((keyUsage & KEYUSE_KEY_ENCIPHER),"KEY ENCRYPTION");
    print_use((keyUsage & KEYUSE_DATA_ENCIPHER), "DATA ENCRYPTION");
    print_use((keyUsage & KEYUSE_KEY_AGREE), "KEY AGREEMENT");
    print_use((keyUsage & KEYUSE_KEY_CERT_SIGN), "CERTIFICATE SIGNING");
    print_use((keyUsage & KEYUSE_CRL_SIGN), "CRL SIGNING");
    print_use((keyUsage & KEYUSE_ENCIPHER_ONLY), "ENCRYPT ONLY");
    print_use((keyUsage & KEYUSE_DECIPHER_ONLY), "DECRYPT ONLY");
#else
    printf("Please configure wolfSSL with --enable-opensslextra and try"
           " again\n");
#endif
    return 0;
}


