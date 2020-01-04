/* pkcs12-example.c
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


#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/pkcs12.h>
#include <wolfssl/wolfcrypt/types.h>



/* This is an example with using wc_ function for PKCS12. To see an example of
 * wolfSSL_PKCS12 functions look in tests/api.c */
int main()
{
    WC_DerCertList* list;
    WC_PKCS12*      pkcs12;
    byte* keyDer  = NULL;
    byte* certDer = NULL;
    word32 keySz;
    word32 certSz;
    word32 i;
    byte buffer[5300];
    char file[] = "./test-servercert.p12";
    FILE *f;
    int  bytes, ret;

    printf("extracting private key and certificate from PKCS12 (test-servercert.p12)\n");

    pkcs12 = wc_PKCS12_new();
    if (pkcs12 == NULL) {
        printf("issue creating pkcs12 object\n");
        return -1;
    }

    /* open PKCS12 file */
    f = fopen(file, "rb");
    if (f == NULL) {
        printf("error opening test-servercert.p12\n");
        wc_PKCS12_free(pkcs12);
        return -1;
    }
    bytes = (int)fread(buffer, 1, sizeof(buffer), f);
    fclose(f);

    /* convert the DER file into an internal structure */
    ret = wc_d2i_PKCS12(buffer, bytes, pkcs12);
    printf("return value of d2i pkcs12 = %d %s\n", ret, (ret == 1)? "SUCCESS": "FAIL");
    if (ret != 1) {
        printf("\t error converting pkcs12 to an internal structure\n");
        wc_PKCS12_free(pkcs12);
        return -1;
    }

    /* parse the internal structure into its parts */
    ret = wc_PKCS12_parse(pkcs12, "wolfSSL test", &keyDer, &keySz,
            &certDer, &certSz, &list);
    printf("return value of parsing pkcs12 = %d %s\n", ret, (ret == 1)? "SUCCESS": "FAIL");
    if (ret != 1 || keyDer == NULL || certDer == NULL) {
        printf("\t error parsing pkcs12\n");
        wc_PKCS12_free(pkcs12);
        return -1;
    }

    /* print out key and cert found */
    printf("HEX of Private Key Read (DER format) :\n");
    for (i = 0; i < keySz; i++) {
        if (i != 0 && !(i%16)) printf("\n");
        printf("%02X", keyDer[i]);
    }
    printf("\n");

    printf("\nHEX of Certificate Read (DER format) :\n");
    for (i = 0; i < certSz; i++) {
        if (i != 0 && !(i%16)) printf("\n");
        printf("%02X", certDer[i]);
    }
    printf("\n");

    if (keyDer != NULL) {
        XFREE(keyDer, NULL, DYNAMIC_TYPE_PKCS);
    }

    if (certDer != NULL) {
        XFREE(certDer, NULL, DYNAMIC_TYPE_PKCS);
    }

    /* itterate through list if was not passed as null and free each node */
    if (list != NULL) {
        WC_DerCertList* current;
        current = list;
        while (current != NULL) {
            WC_DerCertList* next = current->next;
            if (current->buffer != NULL) {
                XFREE(current->buffer, NULL, DYNAMIC_TYPE_PKCS);
            }
            XFREE(current, NULL, DYNAMIC_TYPE_PKCS);
            current = next;
        }
    }

    wc_PKCS12_free(pkcs12);

    return 1;
}
