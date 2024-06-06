/* sigvfycert.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
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

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>

#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/signature.h>

#define MAX_DER_SZ          4096

int load_file(const char* name, byte* buf, int bufSz)
{
    FILE* file;

    file = fopen(name, "rb");
    if (file == NULL) {
        return 0;
    }
    bufSz = fread(buf, 1, bufSz, file);
    fclose(file);

    return bufSz;
}

int get_rsa_public_key_from_ca(const char* caCert, RsaKey* rsaKey)
{
    int res = 0;
    int ret;
    byte caDer[MAX_DER_SZ];
    int caDerSz;
    DecodedCert ca;
    word32 idx;

    XMEMSET(&ca, 0, sizeof(ca));

    /* Load the DER encoded CA certificate. */
    caDerSz = load_file(caCert, caDer, (int)sizeof(caDer));
    if (caDerSz == 0) {
        printf("Failed to load CA file\n");
        res = 1;
        goto exit;
    }

    /* Put the CA certificate data into the object. */
    wc_InitDecodedCert(&ca, caDer, caDerSz, NULL);
    /* Parse fields of the certificate. */
    ret = wc_ParseCert(&ca, CERT_TYPE, 0, NULL);
    if (ret != 0) {
        printf("Parsing CA failed: %s (%d)\n", wc_GetErrorString(ret), ret);
        res = 1;
        goto exit;
    }

    /* Initialize an RSA key object. */
    ret = wc_InitRsaKey(rsaKey, NULL);
    if (ret != 0) {
        printf("Initializing RSA key failed: %s (%d)\n", wc_GetErrorString(ret),
            ret);
        res = 1;
        goto exit;
    }

    /* Decode the DER into RSA key object. */
    idx = 0;
    ret = wc_RsaPublicKeyDecode(ca.publicKey, &idx, rsaKey, ca.pubKeySize);
    if (ret != 0) {
        printf("Decoding RSA key failed: %s (%d)\n", wc_GetErrorString(ret),
            ret);
        res = 1;
        goto exit;
    }

exit:
    wc_FreeDecodedCert(&ca);

    return res;
}

int get_length(const byte* in, word32* inOutIdx, int* len, word32 maxIdx)
{
    word32 idx = *inOutIdx;
    int length = 0;
    byte b;

    /* Check data has length byte to read. */
    if (idx + 1 > maxIdx) {
        return ASN_PARSE_E;
    }
    /* Get first byte of length. */
    b = in[idx++];
    /* Check whether byte is real length. */
    if (b < ASN_LONG_LENGTH) {
        length = b;
    }
    /* Check whether byte is indefinte length. */
    else if (b == ASN_INDEF_LENGTH) {
        /* Indefinite length not supported. */
        return ASN_PARSE_E;
    }
    else {
        /* Calculate number of bytes make up length. */
        b -= ASN_LONG_LENGTH;
        /* Check we have enough input for length value. */
        if (idx + b > maxIdx) {
            return ASN_PARSE_E;
        }
        /* Reconstruct length value from one or more bytes of length. */
        while ((b--) > 0) {
            length = (length << 8) | in[idx++];
        }
    }
    /* Check enough data for length read. */
    if (idx + length > maxIdx) {
        return ASN_PARSE_E;
    }

    /* Return length and index after length. */
    *len = length;
    *inOutIdx = idx;
    return length;
}

int get_sequence(const byte* in, word32* inOutIdx, int* len, word32 maxIdx)
{
    word32 idx = *inOutIdx;

    /* Check data has tag byte to read. */
    if (idx + 1 > maxIdx) {
        return ASN_PARSE_E;
    }
    /* Check tag is a constructed SEQUENCE. */
    if (in[idx++] != (ASN_SEQUENCE | ASN_CONSTRUCTED)) {
        return ASN_PARSE_E;
    }

    /* Get the length of the data. */
    if (get_length(in, &idx, len, maxIdx) < 0) {
        return ASN_PARSE_E;
    }

    /* Return index of data in SEQUENCE and length of data. */
    *inOutIdx = idx;
    return *len;
}

int get_bitstring(const byte* in, word32* inOutIdx, int* len, word32 maxIdx,
    int zeroBits)
{
    word32 idx = *inOutIdx;

    /* Check data has tag byte to read. */
    if (idx + 1 > maxIdx) {
        return ASN_PARSE_E;
    }
    /* Check tag is a BIT_STING. */
    if (in[idx++] != (ASN_BIT_STRING)) {
        return ASN_PARSE_E;
    }

    /* Get the length of the data. */
    if (get_length(in, &idx, len, maxIdx) < 0) {
        return ASN_PARSE_E;
    }
    /* BIT_STRING has an extra byte at the front. */
    if (*len == 0) {
        return ASN_PARSE_E;
    }
    /* Check frst byte is zero when expected. */
    if (zeroBits && (in[idx] != 0x00)) {
        return ASN_PARSE_E;
    }
    /* Skip unused bit count. */
    idx++;
    *len -= 1;

    /* Return index of data in BIT_STRING and length of data. */
    *inOutIdx = idx;
    return *len;
}

int get_cert_tbs_sig(byte* certDer, int certDerSz, byte** tbs, int* tbsSz,
    byte** sig, int* sigSz)
{
    int res = 0;
    int ret;
    word32 tbsIdx = 0;
    word32 idx;
    word32 sigIdx;
    int tbsLen;
    int sigAlgLen;
    int sigLen;
    int len;

#ifdef WOLFSSL_TEST_STATIC_BUILD
    /* Parse outer SEQUENCE. */
    if ((ret = GetSequence(certDer, &tbsIdx, &len, certDerSz)) < 0) {
        printf("Failed to parse certificate: %s (%d)\n", wc_GetErrorString(ret),
            ret);
        res = 1;
        goto exit;
    }

    /* Get length of TBS (To Be Signed) SEQUENCE. */
    idx = tbsIdx;
    if ((ret = GetSequence(certDer, &idx, &tbsLen, certDerSz)) < 0) {
        printf("Failed to parse TBS: %s (%d)\n", wc_GetErrorString(ret),
            ret);
        res = 1;
        goto exit;
    }
    tbsLen += idx - tbsIdx;

    /* Get length of signature algorithm SEQUENCE. */
    sigIdx = tbsIdx + tbsLen;
    if ((ret = GetSequence(certDer, &sigIdx, &sigAlgLen, certDerSz)) < 0) {
        printf("Failed to parse SigAlg: %s (%d)\n", wc_GetErrorString(ret),
            ret);
        res = 1;
        goto exit;
    }
    sigIdx += sigAlgLen;

    /* Get signature data from BIT_STRING. */
    if ((ret = CheckBitString(certDer, &sigIdx, &sigLen, certDerSz, 1,
            NULL)) < 0) {
        printf("Failed to parse sig BIT_STRING: %s (%d)\n",
            wc_GetErrorString(ret), ret);
        res = 1;
        goto exit;
    }
#else
    /* Parse outer SEQUENCE. */
    if ((ret = get_sequence(certDer, &tbsIdx, &len, certDerSz)) < 0) {
        printf("Failed to parse certificate: %s (%d)\n", wc_GetErrorString(ret),
            ret);
        res = 1;
        goto exit;
    }

    /* Get length of TBS (To Be Signed) SEQUENCE. */
    idx = tbsIdx;
    if ((ret = get_sequence(certDer, &idx, &tbsLen, certDerSz)) < 0) {
        printf("Failed to parse TBS: %s (%d)\n", wc_GetErrorString(ret),
            ret);
        res = 1;
        goto exit;
    }
    tbsLen += idx - tbsIdx;

    /* Get length of signature algorithm SEQUENCE. */
    sigIdx = tbsIdx + tbsLen;
    if ((ret = get_sequence(certDer, &sigIdx, &sigAlgLen, certDerSz)) < 0) {
        printf("Failed to parse SigAlg: %s (%d)\n", wc_GetErrorString(ret),
            ret);
        res = 1;
        goto exit;
    }
    sigIdx += sigAlgLen;

    /* Get signature data from BIT_STRING. */
    if ((ret = get_bitstring(certDer, &sigIdx, &sigLen, certDerSz, 1)) < 0) {
        printf("Failed to parse sig BIT_STRING: %s (%d)\n",
            wc_GetErrorString(ret), ret);
        res = 1;
        goto exit;
    }
#endif

    fprintf(stderr, "TBS: %d %d\n", tbsIdx, tbsLen);
    fprintf(stderr, "SIG: %d %d\n", sigIdx, sigLen);

    *tbs = certDer + tbsIdx;
    *tbsSz = tbsLen;
    *sig = certDer + sigIdx;
    *sigSz = sigLen;

exit:
    return res;
}

int main(void)
{
    int res = 0;
    int ret;

    const char* caCert     = "../certs/ca-cert.der";
    const char* verifyCert = "../certs/server-cert.der";

    byte certDer[MAX_DER_SZ];
    int certDerSz;
    byte* tbs;
    int tbsSz;
    byte* sig;
    int sigSz;

    RsaKey rsaPubKey;


    wolfCrypt_Init();

    XMEMSET(&rsaPubKey, 0, sizeof(RsaKey));

    /* Get the RSA public key from the CA certificate. */
    res = get_rsa_public_key_from_ca(caCert, &rsaPubKey);
    if (res != 0) {
        goto exit;
    }

    /* Load the DER encoded certificate to verify. */
    certDerSz = load_file(verifyCert, certDer, (int)sizeof(certDer));
    if (certDerSz == 0) {
        printf("Failed to load certificate file\n");
        res = 1;
        goto exit;
    }

    /* Get the TBS (To Be Signed) and signature from certificate. */
    res = get_cert_tbs_sig(certDer, certDerSz, &tbs, &tbsSz, &sig, &sigSz);
    if (res != 0) {
        goto exit;
    }

    /* Verify signature of certificate. */
    ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA_W_ENC,
        tbs, tbsSz, sig, sigSz, &rsaPubKey, sizeof(rsaPubKey));
    if (ret != 0) {
        printf("Signature verification failed: %s (%d)\n",
            wc_GetErrorString(ret), ret);
        res = 1;
        goto exit;
    }
    printf("Verification Successful!\n");

exit:
    wc_FreeRsaKey(&rsaPubKey);
    wolfCrypt_Cleanup();

    return res;
}

