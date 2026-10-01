/* csr_ecc_rawpub.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

/* Certificate signing request built around an ECC public key supplied as raw
 * bytes - no DER SubjectPublicKeyInfo and no PEM. A secure element or HSM
 * typically hands back either the affine coordinates Qx and Qy or the X9.63
 * point 0x04||X||Y; both forms are shown here.
 *
 * The key material below is a throw-away P-256 key, standing in for whatever
 * the device holds. Where the private key lives in hardware, drop the private
 * import and give the public key a devId instead, so wc_SignCert_ex() calls a
 * crypto callback - see csr_cryptocb.c.
 */

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include <stdio.h>
#include <string.h>

#if defined(WOLFSSL_CERT_REQ) && defined(WOLFSSL_CERT_GEN) && \
    defined(HAVE_ECC) && !defined(NO_SHA256) && !defined(WC_NO_RNG)

#define CSR_BUF_SZ 1024

/* P-256 public key, affine coordinates, big endian, 32 bytes each. */
static const byte eccQx[] = {
    0xFE, 0xB1, 0x7F, 0xFF, 0x8D, 0x8F, 0xBD, 0x09,
    0xB3, 0x2D, 0xA4, 0xE4, 0x1E, 0x6E, 0xD4, 0xD9,
    0xB0, 0xF6, 0x18, 0xF4, 0x70, 0xE2, 0xCC, 0xEF,
    0xEC, 0x02, 0x9F, 0x16, 0xD8, 0x62, 0x31, 0xD4
};
static const byte eccQy[] = {
    0xF4, 0x31, 0x8E, 0xB5, 0xD8, 0x20, 0xE5, 0xE0,
    0x12, 0x0C, 0xEA, 0xB4, 0x38, 0xE0, 0x20, 0x1F,
    0x1F, 0x50, 0x80, 0xDA, 0xEA, 0xE1, 0x20, 0x19,
    0x46, 0x09, 0xD3, 0x25, 0xA8, 0xC7, 0x05, 0x14
};
/* Matching private scalar, used here only to sign the request. */
static const byte eccD[] = {
    0xF2, 0x52, 0xDE, 0x2D, 0x5E, 0x75, 0xDD, 0x94,
    0xA7, 0xE1, 0x56, 0xDE, 0x8D, 0x12, 0x91, 0x0A,
    0x26, 0x54, 0x0F, 0xCD, 0x58, 0x30, 0xB6, 0xAF,
    0xF8, 0x41, 0xDA, 0x8C, 0x16, 0xCE, 0x46, 0xD1
};

/* Same public key as an X9.63 uncompressed point: 0x04 || X || Y. */
static byte eccPoint[1 + sizeof(eccQx) + sizeof(eccQy)];

static void make_point(void)
{
    eccPoint[0] = 0x04;
    XMEMCPY(eccPoint + 1, eccQx, sizeof(eccQx));
    XMEMCPY(eccPoint + 1 + sizeof(eccQx), eccQy, sizeof(eccQy));
}

static void fill_subject(Cert* req)
{
    strncpy(req->subject.country,    "US",              CTC_NAME_SIZE);
    strncpy(req->subject.state,      "OR",              CTC_NAME_SIZE);
    strncpy(req->subject.locality,   "Portland",        CTC_NAME_SIZE);
    strncpy(req->subject.org,        "wolfSSL",         CTC_NAME_SIZE);
    strncpy(req->subject.unit,       "Development",     CTC_NAME_SIZE);
    strncpy(req->subject.commonName, "www.wolfssl.com", CTC_NAME_SIZE);
    req->version = 0;
    req->sigType = CTC_SHA256wECDSA;
}

/* Encode the unsigned request body around pubKey. Returns its size. */
static int make_req_body(ecc_key* pubKey, Cert* req, byte* der, word32 derBufSz)
{
    int ret;

    ret = wc_InitCert(req);
    if (ret != 0) {
        printf("wc_InitCert failed: %d\n", ret);
        return ret;
    }
    fill_subject(req);

    ret = wc_MakeCertReq_ex(req, der, derBufSz, ECC_TYPE, pubKey);
    if (ret <= 0) {
        printf("wc_MakeCertReq_ex failed: %d\n", ret);
    }
    return ret;
}

#ifdef WOLFSSL_DER_TO_PEM
static int print_pem(byte* der, int derSz)
{
    byte pem[CSR_BUF_SZ * 2];
    int  pemSz;

    pemSz = wc_DerToPem(der, (word32)derSz, pem, (word32)sizeof(pem),
                        CERTREQ_TYPE);
    if (pemSz <= 0) {
        printf("wc_DerToPem failed: %d\n", pemSz);
        return pemSz;
    }
    printf("%.*s\n", pemSz, pem);
    return 0;
}
#endif

int main(void)
{
    ecc_key pubKey;
    ecc_key pointKey;
    ecc_key signKey;
    WC_RNG  rng;
    Cert    req;
    static DecodedCert decoded;
    byte    der[CSR_BUF_SZ];
    byte    derPoint[CSR_BUF_SZ];
    int     ret;
    int     derSz;
    int     pointSz;

    make_point();

    ret = wolfCrypt_Init();
    if (ret != 0) {
        printf("wolfCrypt_Init failed: %d\n", ret);
        return ret;
    }

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("wc_InitRng failed: %d\n", ret);
        wolfCrypt_Cleanup();
        return ret;
    }

    /* Zero every key first so the cleanup below is safe even if one of the
     * inits fails partway. */
    XMEMSET(&signKey, 0, sizeof(signKey));
    XMEMSET(&pubKey, 0, sizeof(pubKey));
    XMEMSET(&pointKey, 0, sizeof(pointKey));

    ret = wc_ecc_init(&signKey);
    if (ret == 0)
        ret = wc_ecc_init(&pubKey);
    if (ret == 0)
        ret = wc_ecc_init(&pointKey);
    if (ret != 0) {
        printf("wc_ecc_init failed: %d\n", ret);
        goto exit;
    }

    /* The signer. A device would leave this out and route wc_SignCert_ex()
     * to its secure element through a crypto callback. */
    ret = wc_ecc_import_unsigned(&signKey, eccQx, eccQy, eccD,
                                 ECC_SECP256R1);
    if (ret != 0) {
        printf("private wc_ecc_import_unsigned failed: %d\n", ret);
        goto exit;
    }

    /* Public key from bare Qx and Qy. A NULL private scalar leaves the key
     * public only, which is all the request body needs. */
    ret = wc_ecc_import_unsigned(&pubKey, eccQx, eccQy, NULL,
                                 ECC_SECP256R1);
    if (ret != 0) {
        printf("wc_ecc_import_unsigned failed: %d\n", ret);
        goto exit;
    }

    /* The same key from the X9.63 point. */
    ret = wc_ecc_import_x963_ex(eccPoint, (word32)sizeof(eccPoint), &pointKey,
                                ECC_SECP256R1);
    if (ret != 0) {
        printf("wc_ecc_import_x963_ex failed: %d\n", ret);
        goto exit;
    }

    derSz = make_req_body(&pubKey, &req, der, (word32)sizeof(der));
    if (derSz <= 0) {
        ret = derSz;
        goto exit;
    }
    pointSz = make_req_body(&pointKey, &req, derPoint,
                            (word32)sizeof(derPoint));
    if (pointSz <= 0) {
        ret = pointSz;
        goto exit;
    }

    /* The two import forms describe the same key, so the request bodies
     * must be byte for byte identical. */
    if ((derSz != pointSz) || (XMEMCMP(der, derPoint, (size_t)derSz) != 0)) {
        printf("Request bodies differ: %d vs %d bytes\n", derSz, pointSz);
        ret = -1;
        goto exit;
    }
    printf("Request body from Qx/Qy and from the X9.63 point match: "
           "%d bytes\n", derSz);

    /* Rebuild the body for the key being signed, so req describes der. */
    derSz = make_req_body(&pubKey, &req, der, (word32)sizeof(der));
    if (derSz <= 0) {
        ret = derSz;
        goto exit;
    }

    ret = wc_SignCert_ex(req.bodySz, req.sigType, der, (word32)sizeof(der),
                         ECC_TYPE, &signKey, &rng);
    if (ret <= 0) {
        printf("wc_SignCert_ex failed: %d\n", ret);
        goto exit;
    }
    derSz = ret;
    printf("Signed CSR: %d bytes\n", derSz);

    /* Parse the request back and check its signature, so a malformed CSR
     * fails rather than reporting success. */
    wc_InitDecodedCert(&decoded, der, (word32)derSz, NULL);
    ret = wc_ParseCert(&decoded, CERTREQ_TYPE, VERIFY, NULL);
    if (ret == 0) {
        printf("Parsed back and signature verified\n");
    }
    else {
        printf("wc_ParseCert failed: %d\n", ret);
    }
    wc_FreeDecodedCert(&decoded);
    if (ret != 0) {
        goto exit;
    }

#ifdef WOLFSSL_DER_TO_PEM
    ret = print_pem(der, derSz);
    if (ret != 0) {
        goto exit;
    }
#endif

    printf("Tests passed\n");
    ret = 0;

exit:
    wc_ecc_free(&pointKey);
    wc_ecc_free(&pubKey);
    wc_ecc_free(&signKey);
    wc_FreeRng(&rng);
    wolfCrypt_Cleanup();
    return ret;
}

#else

int main(void)
{
    printf("Please compile wolfSSL with --enable-certgen --enable-certreq "
           "--enable-certext --enable-ecc, and with SHA-256 and the RNG "
           "enabled\n");
    return 0;
}

#endif
