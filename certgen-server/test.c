/* test.c
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
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include "server-tls.h"

#define HEAP_HINT NULL
#define FOURK_SZ 4096

void free_things(RsaKey* d, RsaKey* e, WC_RNG* f);

/* modified version of the certgen.c example
 *
 * This function takes in a signer certificate and outputs a created certificate
 * and key.
 *
 * caBuf    DER buffer of the certificate to sign with
 * caBufSz  size of caBuf
 * caKeyBuf DER buffer of the private CA key for signing
 * caKeySz  size of caKeyBuf
 * derOut   [in/out] buffer to hold DER certificate created
 * derOutSz [in/out] holds original derOut size then gets updated with result
 * pemOut   [in/out] buffer to hold PEM certificate created
 * pemOutSz [in/out] size of PEM buffer
 * keyOut   [in/out] buffer to hold RSA 2048 bit DER key created
 * keyOutSz [in/out] size of DER key created
 * cn       common name to use when creating certificate
 * isCa     CA basic constraint
 *
 * Returns 0 on success
 */
int createSignedCert(unsigned char* caBuf,     int caBufSz,
                     unsigned char* caKeyBuf,  int caKeySz,
                     unsigned char* derOut,    int* derOutSz,
                     unsigned char* pemOut,    int* pemOutSz,
                     unsigned char* keyOut,    int* keyOutSz,
                     char* cn, int isCa) {

    int ret = 0;
    Cert newCert;

    /* for MakeCert and SignCert */
    WC_RNG rng;
    RsaKey caKey;
    RsaKey newKey;
    word32 idx3 = 0;


/*---------------------------------------------------------------------------*/
/* open caKey file and get the caKey */
/*---------------------------------------------------------------------------*/
    printf("Init RSA Key\n");
    wc_InitRsaKey(&caKey, NULL);

    printf("Decode the private key\n");
    ret = wc_RsaPrivateKeyDecode(caKeyBuf, &idx3, &caKey, (word32)caKeySz);
    if (ret != 0) goto fail;

    printf("Successfully retrieved caKey\n\n");
/*---------------------------------------------------------------------------*/
/* END */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Generate new RSA key */
/*---------------------------------------------------------------------------*/
    printf("initializing the rng\n");
    ret = wc_InitRng(&rng);
    if (ret != 0) goto fail;

    printf("Generating a new RSA key\n");
    ret = wc_InitRsaKey(&newKey, NULL);
    if (ret != 0) goto fail;

    ret = wc_MakeRsaKey(&newKey, 2048, 3, &rng);
    if (ret != 0) goto fail;

    /* write key to output buffer */
    ret = wc_RsaKeyToDer(&newKey, keyOut, *keyOutSz);
    if (ret <= 0) goto fail;
    *keyOutSz = ret;

    printf("Successfully created new RSA key\n\n");
/*---------------------------------------------------------------------------*/
/* END */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Create a new certificate using header information from der cert */
/*---------------------------------------------------------------------------*/
    printf("Setting new cert issuer to subject of signer\n");

    wc_InitCert(&newCert);

    strncpy(newCert.subject.country, "US", CTC_NAME_SIZE);
    strncpy(newCert.subject.state, "MT", CTC_NAME_SIZE);
    strncpy(newCert.subject.locality, "Bozeman", CTC_NAME_SIZE);
    strncpy(newCert.subject.org, "yourOrgNameHere", CTC_NAME_SIZE);
    strncpy(newCert.subject.unit, "yourUnitNameHere", CTC_NAME_SIZE);
    strncpy(newCert.subject.commonName, cn, CTC_NAME_SIZE);
    strncpy(newCert.subject.email, "yourEmail@yourDomain.com", CTC_NAME_SIZE);
    newCert.isCA    = isCa;
    newCert.sigType = CTC_SHA256wRSA;

    ret = wc_SetIssuerBuffer(&newCert, caBuf, caBufSz);
    if (ret != 0) goto fail;

#ifdef WOLFSSL_CERT_EXT
    if (wc_SetSubjectKeyIdFromPublicKey(&newCert, &newKey, NULL) != 0) {
        printf("unable to set SKID\n");
    }

    if (wc_SetAuthKeyIdFromPublicKey(&newCert, &caKey, NULL) != 0) {
        printf("unable to set AKID\n");
    }
#endif
#ifdef WOLFSSL_ALT_NAMES
    /* google chrome currently needs the SAN set */
    ret = wc_SetAltNames(&newCert, "./san.pem");
    if (ret != 0) goto fail;
#endif
    ret = wc_MakeCert(&newCert, derOut, *derOutSz, &newKey, NULL, &rng); //ecc certificate
    if (ret < 0) goto fail;

    printf("MakeCert returned %d\n", ret);

    ret = wc_SignCert(newCert.bodySz, newCert.sigType, derOut, *derOutSz,
                                                            &caKey, NULL, &rng);
    if (ret < 0) goto fail;
    printf("SignCert returned %d\n", ret);
    *derOutSz = ret;

    ret = wc_DerToPem(derOut, *derOutSz, pemOut, *pemOutSz, CERT_TYPE);
    if (ret < 0) goto fail;
    *pemOutSz = ret;

    printf("Successfully created new certificate\n");
/*---------------------------------------------------------------------------*/
/* END */
/*---------------------------------------------------------------------------*/

    goto success;

fail:
    free_things(&caKey, &newKey, &rng);
    printf("Failure code was %d\n", ret);
    return -1;

success:
    free_things(&caKey, &newKey, &rng);
    printf("Successfully created certificate\n");
    return 0;
}

void free_things(RsaKey* d, RsaKey* e, WC_RNG* f)
{
    wc_FreeRsaKey(d);
    wc_FreeRsaKey(e);
    wc_FreeRng(f);

}
