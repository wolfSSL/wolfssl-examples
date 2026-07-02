/* csr_example.c
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

/* Example of generating a PEM-encoded certificate signing request (CSR). */

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define LARGE_TEMP_SZ 4096

/* Forward declarations */
static void usage(void);
static int fill_csr_req(Cert* req, const char* keyType);
static int gen_csr(Cert* req);
static int init_pk_key(Cert* req, void** keyPtr, WC_RNG* rng, byte* der,
                word32 derSz);
static int build_csr(Cert* req, void* keyPtr, WC_RNG* rng,
                byte* der, word32 derBufSz);
static int check_requested_key_type(Cert* req, const char* keyType);
static int get_key_type(Cert* req);
static void free_pk_key(Cert* req, void* keyPtr);
#ifdef WOLFSSL_DER_TO_PEM
static const char* get_key_name(Cert* req);
static int save_pem(byte* der, int derSz, int pemType, const char* outFile);
#endif

/*---------------------------------------------------------------------------*/
/* main
 *
 * This example demonstrates a CSR (PKCS#10) workflow in two steps:
 *
 * 1. fill_csr_req: Populate the Cert struct with subject info and sigType.
 *    The Cert struct becomes the single source of truth. All downstream
 *    functions read from it with no per-type switching logic.
 *
 * 2. gen_csr: Generate a key, build the unsigned request body with
 *    wc_MakeCertReq_ex, then sign it with wc_SignCert_ex.
 *
 * Key API details:
 *   wc_MakeCertReq_ex does NOT read req->sigType. It only encodes the
 *   CertificationRequestInfo (version, subject, public key, attributes).
 *   It sets req->bodySz and req->keyType automatically.
 *
 *   wc_SignCert_ex reads req->bodySz to know where the body ends and
 *   req->sigType to encode the signature algorithm OID in the outer
 *   CertificationRequest wrapper. sigType is a parameter to SignCert,
 *   not to MakeCertReq.
 *---------------------------------------------------------------------------*/
int main(int argc, char** argv)
{
#if !defined(WOLFSSL_CERT_REQ) || !defined(WOLFSSL_CERT_GEN) || !defined(WOLFSSL_KEY_GEN)
    printf("Please compile wolfSSL with --enable-certreq --enable-certgen --enable-certext --enable-keygen\n");
    return 0;
#else
    int ret;
    Cert req;

    if (argc != 2) {
        usage();
        return 1;
    }

    ret = wolfCrypt_Init();
    if (ret != 0) {
        printf("wolfCrypt_Init failed: %d\n", ret);
        return ret;
    }

    /* Step 1: Fill out the request struct. Single source of truth.
     * After this, everything is driven by what's in req. */
    ret = fill_csr_req(&req, argv[1]);
    if (ret != 0) {
        wolfCrypt_Cleanup();
        return ret;
    }

    /* Step 2: Generate key, make request body, and sign the CSR */
    ret = gen_csr(&req);

    wolfCrypt_Cleanup();
    return ret;
#endif
}

/*---------------------------------------------------------------------------*/
/* fill_csr_req
 *
 * Fill out the Cert request struct based on the key type. This is the
 * single source of truth for the CSR. All per-type decisions (sigType)
 * are made here. Everything downstream reads from this struct.
 *
 * wc_InitCert sets defaults like sigType=CTC_SHAwRSA and keyType=RSA_KEY,
 * but wc_MakeCertReq_ex never reads sigType (it is only used by
 * wc_SignCert_ex), and it overwrites keyType based on the key passed in.
 *---------------------------------------------------------------------------*/
static int fill_csr_req(Cert* req, const char* keyType)
{
    int ret;

    if (req == NULL || keyType == NULL) {
        return BAD_FUNC_ARG;
    }

    ret = wc_InitCert(req);
    if (ret != 0) {
        printf("Init Cert failed: %d\n", ret);
        return ret;
    }

    /* Check the requested key type is valid and supported */
    ret = check_requested_key_type(req, keyType);
    if (ret != 0) {
        if (ret == NOT_COMPILED_IN) {
            printf("Requested key type not compiled in: %s\n", keyType);
            return NOT_COMPILED_IN;
        }
        else {
            usage();
            return ret;
        }
    }

    strncpy(req->subject.country, "US", CTC_NAME_SIZE);
    strncpy(req->subject.state, "OR", CTC_NAME_SIZE);
    strncpy(req->subject.locality, "Portland", CTC_NAME_SIZE);
    strncpy(req->subject.org, "wolfSSL", CTC_NAME_SIZE);
    strncpy(req->subject.unit, "Development", CTC_NAME_SIZE);
    strncpy(req->subject.commonName, "www.wolfssl.com", CTC_NAME_SIZE);
    strncpy(req->subject.email, "info@wolfssl.com", CTC_NAME_SIZE);
    req->version = 0;

    return 0;
}

/*---------------------------------------------------------------------------*/
/* gen_csr
 *
 * Generate key, build request, and sign. All driven by the filled req.
 *---------------------------------------------------------------------------*/
static int gen_csr(Cert* req)
{
    int ret;
    void* keyPtr = NULL;
    WC_RNG rng;
    byte der[LARGE_TEMP_SZ];

    XMEMSET(der, 0, LARGE_TEMP_SZ);

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("RNG initialization failed: %d\n", ret);
        return ret;
    }

    /* Generate key driven by req->sigType */
    ret = init_pk_key(req, &keyPtr, &rng, der, sizeof(der));
    if (ret != 0) {
        free_pk_key(req, keyPtr);
        wc_FreeRng(&rng);
        return ret;
    }

    /* Build the CSR: make the request body, then sign it */
    ret = build_csr(req, keyPtr, &rng, der, sizeof(der));

    free_pk_key(req, keyPtr);
    wc_FreeRng(&rng);

    return ret;
}

/*---------------------------------------------------------------------------*/
/* init_pk_key
 *
 * Init, generate, and export the key based on req->sigType.
 * Returns 0 on success and sets *keyPtr.
 *---------------------------------------------------------------------------*/
static int init_pk_key(Cert* req, void** keyPtr, WC_RNG* rng, byte* der,
    word32 derSz)
{
    int ret;
#ifdef WOLFSSL_DER_TO_PEM
    char outFile[255];
#endif

    switch (req->sigType) {
#ifdef HAVE_ECC
        case CTC_SHA256wECDSA:
            *keyPtr = XMALLOC(sizeof(ecc_key), NULL, DYNAMIC_TYPE_ECC);
            if (*keyPtr == NULL) {
                ret = MEMORY_E;
                break;
            }
            XMEMSET(*keyPtr, 0, sizeof(ecc_key));
            ret = wc_ecc_init((ecc_key*)*keyPtr);
            if (ret != 0) {
                XFREE(*keyPtr, NULL, DYNAMIC_TYPE_ECC);
                *keyPtr = NULL;
                break;
            }
            ret = wc_ecc_make_key_ex(rng, 32, (ecc_key*)*keyPtr, ECC_SECP256R1);
            if (ret != 0) {
                printf("Key generation failed: %d\n", ret);
                break;
            }
            ret = wc_EccKeyToDer((ecc_key*)*keyPtr, der, derSz);
            if (ret <= 0) {
                printf("Key To DER failed: %d\n", ret);
                break;
            }
        #ifdef WOLFSSL_DER_TO_PEM
            snprintf(outFile, sizeof(outFile), "%s-key.pem", get_key_name(req));
            ret = save_pem(der, ret, ECC_PRIVATEKEY_TYPE, outFile);
        #else
            ret = 0;
        #endif
            break;
#endif
#ifndef NO_RSA
        case CTC_SHA256wRSA:
            *keyPtr = XMALLOC(sizeof(RsaKey), NULL, DYNAMIC_TYPE_RSA);
            if (*keyPtr == NULL) {
                ret = MEMORY_E;
                break;
            }
            XMEMSET(*keyPtr, 0, sizeof(RsaKey));
            ret = wc_InitRsaKey((RsaKey*)*keyPtr, NULL);
            if (ret != 0) {
                XFREE(*keyPtr, NULL, DYNAMIC_TYPE_RSA);
                *keyPtr = NULL;
                break;
            }
            ret = wc_MakeRsaKey((RsaKey*)*keyPtr, 2048, WC_RSA_EXPONENT, rng);
            if (ret != 0) {
                printf("Key generation failed: %d\n", ret);
                break;
            }
            ret = wc_RsaKeyToDer((RsaKey*)*keyPtr, der, derSz);
            if (ret <= 0) {
                printf("Key To DER failed: %d\n", ret);
                break;
            }
        #ifdef WOLFSSL_DER_TO_PEM
            snprintf(outFile, sizeof(outFile), "%s-key.pem", get_key_name(req));
            ret = save_pem(der, ret, PRIVATEKEY_TYPE, outFile);
        #else
            ret = 0;
        #endif
            break;
#endif
#ifdef HAVE_ED25519
        case CTC_ED25519:
            *keyPtr = XMALLOC(sizeof(ed25519_key), NULL, DYNAMIC_TYPE_ED25519);
            if (*keyPtr == NULL) {
                ret = MEMORY_E;
                break;
            }
            XMEMSET(*keyPtr, 0, sizeof(ed25519_key));
            ret = wc_ed25519_init((ed25519_key*)*keyPtr);
            if (ret != 0) {
                XFREE(*keyPtr, NULL, DYNAMIC_TYPE_ED25519);
                *keyPtr = NULL;
                break;
            }
            ret = wc_ed25519_make_key(rng, ED25519_KEY_SIZE,
                (ed25519_key*)*keyPtr);
            if (ret != 0) {
                printf("Key generation failed: %d\n", ret);
                break;
            }
            ret = wc_Ed25519KeyToDer((ed25519_key*)*keyPtr, der, derSz);
            if (ret <= 0) {
                printf("Key To DER failed: %d\n", ret);
                break;
            }
        #ifdef WOLFSSL_DER_TO_PEM
            snprintf(outFile, sizeof(outFile), "%s-key.pem", get_key_name(req));
            ret = save_pem(der, ret, ED25519_TYPE, outFile);
        #else
            ret = 0;
        #endif
            break;
#endif
        default:
            ret = NOT_COMPILED_IN;
    }

    return ret;
}

/*---------------------------------------------------------------------------*/
/* build_csr
 *
 * Build the CSR: make the request body, then sign it.
 * No per-type switching. Everything comes from the filled req struct.
 *
 * wc_MakeCertReq_ex encodes the CertificationRequestInfo body into the DER
 * buffer. It reads subject, version, and attributes from req. It does NOT
 * read req->sigType. It sets req->keyType and req->bodySz.
 *
 * wc_SignCert_ex wraps the body with the signature algorithm OID and
 * signature. req->sigType (set during fill) controls which OID is encoded.
 * req->bodySz (set by MakeCertReq) tells SignCert where the body ends.
 *---------------------------------------------------------------------------*/
static int build_csr(Cert* req, void* keyPtr, WC_RNG* rng,
    byte* der, word32 derBufSz)
{
    int ret;
#ifdef WOLFSSL_DER_TO_PEM
    char outFile[255];
#endif

    /* Step 1: Generate the unsigned request body */
    ret = wc_MakeCertReq_ex(req, der, derBufSz, get_key_type(req), keyPtr);
    if (ret <= 0) {
        printf("Make Cert Req failed: %d\n", ret);
        return ret;
    }
    printf("Generated unsigned CSR request body (%d bytes)\n", ret);

    /* Step 2: Sign the request body */
    ret = wc_SignCert_ex(req->bodySz, req->sigType, der, derBufSz,
        get_key_type(req), keyPtr, rng);
    if (ret <= 0) {
        printf("Sign Cert failed: %d\n", ret);
        return ret;
    }
    printf("Signed CSR (%d bytes)\n", ret);

#ifdef WOLFSSL_DER_TO_PEM
    snprintf(outFile, sizeof(outFile), "%s-csr.pem", get_key_name(req));
    ret = save_pem(der, ret, CERTREQ_TYPE, outFile);
    if (ret != 0) {
        return ret;
    }
#endif

    return 0;
}

/*---------------------------------------------------------------------------*/
/* Helper functions
 *---------------------------------------------------------------------------*/

static void usage(void)
{
    printf("Invalid input supplied try one of the below examples\n");
    printf("Examples:\n\n");
    printf("./csr_example rsa\n");
    printf("./csr_example ecc\n");
    printf("./csr_example ed25519\n");
}

static int check_requested_key_type(Cert* req, const char* keyType)
{
    if (req == NULL || keyType == NULL) {
        return BAD_FUNC_ARG;
    }

    /* Validate input and determine signature type */
    if (XSTRNCMP(keyType, "ecc", 3) == 0) {
    #ifdef HAVE_ECC
        req->sigType = CTC_SHA256wECDSA;
        return 0;
    #else
        return NOT_COMPILED_IN;
    #endif
    }

    if (XSTRNCMP(keyType, "rsa", 3) == 0) {
    #ifndef NO_RSA
        req->sigType = CTC_SHA256wRSA;
        return 0;
    #else
        return NOT_COMPILED_IN;
    #endif
    }

    if (XSTRNCMP(keyType, "ed25519", 7) == 0) {
    #ifdef HAVE_ED25519
        req->sigType = CTC_ED25519;
        return 0;
    #else
        return NOT_COMPILED_IN;
    #endif
    }

    return BAD_FUNC_ARG;
}

/* Get the key type constant needed by the _ex APIs from req->sigType. */
static int get_key_type(Cert* req)
{
    switch (req->sigType) {
#ifdef HAVE_ECC
        case CTC_SHA256wECDSA:
            return ECC_TYPE;
#endif
#ifndef NO_RSA
        case CTC_SHA256wRSA:
            return RSA_TYPE;
#endif
#ifdef HAVE_ED25519
        case CTC_ED25519:
            return ED25519_TYPE;
#endif
        default:
            return NOT_COMPILED_IN;
    }
}

static void free_pk_key(Cert* req, void* keyPtr)
{
    if (keyPtr == NULL) {
        return;
    }

    switch (req->sigType) {
#ifdef HAVE_ECC
        case CTC_SHA256wECDSA:
            wc_ecc_free((ecc_key*)keyPtr);
            XFREE(keyPtr, NULL, DYNAMIC_TYPE_ECC);
            break;
#endif
#ifndef NO_RSA
        case CTC_SHA256wRSA:
            wc_FreeRsaKey((RsaKey*)keyPtr);
            XFREE(keyPtr, NULL, DYNAMIC_TYPE_RSA);
            break;
#endif
#ifdef HAVE_ED25519
        case CTC_ED25519:
            wc_ed25519_free((ed25519_key*)keyPtr);
            XFREE(keyPtr, NULL, DYNAMIC_TYPE_ED25519);
            break;
#endif
        default:
            break;
    }
}

#ifdef WOLFSSL_DER_TO_PEM
/* Get a human readable key name from req->sigType for filenames. */
static const char* get_key_name(Cert* req)
{
    switch (req->sigType) {
#ifdef HAVE_ECC
        case CTC_SHA256wECDSA:
            return "ecc";
#endif
#ifndef NO_RSA
        case CTC_SHA256wRSA:
            return "rsa";
#endif
#ifdef HAVE_ED25519
        case CTC_ED25519:
            return "ed25519";
#endif
        default:
            return "unknown";
    }
}

static int save_pem(byte* der, int derSz, int pemType, const char* outFile)
{
    byte pem[LARGE_TEMP_SZ];
    int  pemSz;
    FILE* file;

    XMEMSET(pem, 0, sizeof(pem));
    pemSz = wc_DerToPem(der, derSz, pem, sizeof(pem), pemType);
    if (pemSz <= 0) {
        printf("Key DER to PEM failed: %d\n", pemSz);
        return pemSz;
    }
    printf("%s (%d)\n", pem, pemSz);
    printf("Saved PEM to \"%s\"\n", outFile);
    file = fopen(outFile, "wb");
    if (file == NULL) {
        printf("Failed to open file: %s\n", outFile);
        return -1;
    }
    if ((int)fwrite(pem, 1, pemSz, file) != pemSz) {
        printf("Failed to write PEM to file: %s\n", outFile);
        fclose(file);
        return -1;
    }
    fclose(file);

    return 0;
}
#endif
