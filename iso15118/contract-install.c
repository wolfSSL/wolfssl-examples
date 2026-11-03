/* contract-install.c
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Contract certificate installation: how a vehicle gets the key it charges
 * with, without that key ever travelling in the clear.
 *
 * This is the provisioning step behind Plug & Charge. The vehicle leaves the
 * factory holding an OEM provisioning certificate and its private key. To get
 * a charging contract it asks for one, and the backend has to deliver a
 * contract certificate AND the matching private key -- over a path it does not
 * control. So the private key is wrapped to the provisioning certificate's
 * key, and only the vehicle that holds the provisioning private key can
 * unwrap it.
 *
 *   vehicle  -- CertificateInstallationReq ------>  backend
 *               PCID, provisioning chain, trusted roots
 *
 *   vehicle  <-- CertificateInstallationRes ------  backend
 *               contract chain, eMAID,
 *               DHpublickey,
 *               ContractSignatureEncryptedPrivateKey
 *
 * The wrapping, which is what this example actually does:
 *
 *   backend   ephemeral EC key pair                      -> DHpublickey
 *             ECDH(ephemeral private, provisioning public) -> shared secret
 *             X9.63 KDF with SHA-256                      -> session key
 *             AES-CBC(session key, random IV)             -> encrypted key
 *
 *   vehicle   ECDH(provisioning private, DHpublickey)     -> same secret
 *             same KDF                                    -> same session key
 *             AES-CBC decrypt                             -> contract key
 *
 * Nothing about the transport is here -- the messages are EXI over the TLS
 * session that secc-server.c and evcc-client.c set up, and wolfSSL has no EXI
 * codec. What is here is every cryptographic step, end to end, with a real
 * proof that the recovered key is the contract key.
 *
 * One caveat, stated plainly: the KDF's SharedInfo input is what the standard
 * pins down, and this example uses the eMAID for it. Check that against the
 * revision you are implementing -- get it wrong and you will derive a session
 * key that no real backend agrees with. Everything else here follows from the
 * primitives.
 *
 * Requires wolfSSL built with --enable-x963kdf.
 *
 * Usage: ./contract-install
 */

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>

#include <stdio.h>
#include <string.h>

#ifndef HAVE_X963_KDF

int main(void)
{
    printf("This example needs the X9.63 KDF.\n");
    printf("Rebuild wolfSSL with: ./configure --enable-x963kdf\n");
    printf("\ncontract-install: SKIP\n");
    return 0;
}

#else /* HAVE_X963_KDF */

#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/memory.h>  /* wc_ForceZero */

#define PROV_KEY       "certs/provisioning.key"
#define PROV_CERT      "certs/provisioning.pem"
#define OTHER_PROV_KEY "certs/other-provisioning.key"
#define CONTRACT_KEY   "certs/contract.key"
#define CONTRACT_CERT  "certs/contract.pem"

/* The e-mobility account identifier the contract is issued against. Also used
 * as the KDF SharedInfo here -- see the caveat at the top of this file. */
#define EMAID "DE-MO-C0123456789-3"

#define MAX_PEM     4096
#define MAX_DER     2048
#define P256_SCALAR 32   /* secp256r1 private key */
#define SESSION_KEY 16   /* AES-128 */
#define SECRET_SZ   32   /* ECDH shared secret on P-256 */

/* IV followed by the encrypted 32 byte scalar: 48 bytes on the wire. */
#define WRAPPED_SZ (WC_AES_BLOCK_SIZE + P256_SCALAR)

static int failures = 0;

static void check(const char* what, int ok)
{
    printf("  %-50s %s\n", what, ok ? "ok" : "FAILED");
    if (!ok) {
        failures++;
    }
}

static void print_hex(const char* label, const unsigned char* b, int len)
{
    int i;

    printf("      %-22s", label);
    for (i = 0; i < len; i++) {
        printf("%02x", b[i]);
    }
    printf("\n");
}

static int read_file(const char* path, unsigned char* buf, size_t max,
        size_t* outLen)
{
    FILE* f = fopen(path, "rb");
    size_t n;

    if (f == NULL) {
        fprintf(stderr, "could not open %s -- run ./generate_v2g_certs.sh\n",
                path);
        return -1;
    }
    n = fread(buf, 1, max, f);
    fclose(f);
    if (n == 0) {
        return -1;
    }
    *outLen = n;
    return 0;
}

/* Loads an EC private key from a PEM file. */
static int load_private_key(const char* path, ecc_key* key)
{
    unsigned char pem[MAX_PEM];
    unsigned char der[MAX_DER];
    size_t pemLen;
    int derLen;
    word32 idx = 0;

    if (read_file(path, pem, sizeof(pem), &pemLen) != 0) {
        return -1;
    }
    derLen = wc_KeyPemToDer(pem, (int)pemLen, der, sizeof(der), NULL);
    if (derLen <= 0) {
        return -1;
    }
    if (wc_ecc_init(key) != 0) {
        return -1;
    }
    if (wc_EccPrivateKeyDecode(der, &idx, key, (word32)derLen) != 0) {
        wc_ecc_free(key);
        return -1;
    }
    return 0;
}

/* Loads the public key out of a certificate PEM. */
static int load_cert_public_key(const char* path, ecc_key* key)
{
    unsigned char pem[MAX_PEM];
    unsigned char der[MAX_DER];
    unsigned char spki[MAX_DER];
    size_t pemLen;
    int derLen;
    word32 spkiLen = (word32)sizeof(spki);
    word32 idx = 0;

    if (read_file(path, pem, sizeof(pem), &pemLen) != 0) {
        return -1;
    }
    derLen = wc_CertPemToDer(pem, (int)pemLen, der, sizeof(der), CERT_TYPE);
    if (derLen <= 0) {
        return -1;
    }
    if (wc_GetSubjectPubKeyInfoDerFromCert(der, (word32)derLen, spki,
                &spkiLen) != 0) {
        return -1;
    }
    if (wc_ecc_init(key) != 0) {
        return -1;
    }
    if (wc_EccPublicKeyDecode(spki, &idx, key, spkiLen) != 0) {
        wc_ecc_free(key);
        return -1;
    }
    return 0;
}

/* ECDH then the X9.63 KDF: the session key both ends must agree on.
 * Returns 0 on success. */
static int derive_session_key(ecc_key* priv, ecc_key* pub, WC_RNG* rng,
        unsigned char* sessionKey, unsigned char* secretOut)
{
    unsigned char secret[SECRET_SZ];
    word32 secretLen = (word32)sizeof(secret);
    int ret;

    /* wolfSSL is built with ECC_TIMING_RESISTANT by default (--enable-harden),
     * which makes wc_ecc_shared_secret() blind the scalar multiplication --
     * and that needs an RNG on the private key. Without this it returns
     * MISSING_RNG_E (-236), which is easy to misread as a bad key. A key from
     * wc_ecc_make_key_ex() does not carry one either. */
    ret = wc_ecc_set_rng(priv, rng);
    if (ret != 0) {
        return ret;
    }

    ret = wc_ecc_shared_secret(priv, pub, secret, &secretLen);
    if (ret != 0) {
        return ret;
    }
    if (secretOut != NULL) {
        XMEMCPY(secretOut, secret, secretLen);
    }

    /* SharedInfo is the part the standard pins down; see the file header. */
    ret = wc_X963_KDF(WC_HASH_TYPE_SHA256, secret, secretLen,
            (const byte*)EMAID, (word32)XSTRLEN(EMAID),
            sessionKey, SESSION_KEY);
    if (ret != 0) {
        /* a failed KDF can still have written part of the output */
        wc_ForceZero(sessionKey, SESSION_KEY);
    }

    wc_ForceZero(secret, sizeof(secret));
    return ret;
}

/* The backend side: wrap the contract private key for this vehicle.
 * Writes DHpublickey in X9.63 form and the 48 byte wrapped key.
 * Returns 0 on success. */
static int backend_wrap(WC_RNG* rng, ecc_key* provisioningPub,
        const unsigned char* contractScalar,
        unsigned char* dhPub, word32* dhPubLen,
        unsigned char* wrapped)
{
    ecc_key ephemeral;
    unsigned char sessionKey[SESSION_KEY];
    unsigned char secret[SECRET_SZ];
    Aes aes;
    int ret;

    if (wc_ecc_init(&ephemeral) != 0) {
        return -1;
    }

    /* A fresh key pair per installation. Reusing one would let two
     * installations share a session key. */
    ret = wc_ecc_make_key_ex(rng, P256_SCALAR, &ephemeral, ECC_SECP256R1);
    if (ret == 0) {
        ret = wc_ecc_export_x963(&ephemeral, dhPub, dhPubLen);
    }
    if (ret == 0) {
        ret = derive_session_key(&ephemeral, provisioningPub, rng,
                sessionKey, secret);
    }
    if (ret != 0) {
        /* derive_session_key() wipes its own copy of the ECDH secret, but
         * these two are ours and a partial derivation may have filled them. */
        wc_ForceZero(sessionKey, sizeof(sessionKey));
        wc_ForceZero(secret, sizeof(secret));
        wc_ecc_free(&ephemeral);
        return ret;
    }

    printf("      ephemeral key pair generated, %u byte DHpublickey\n",
            (unsigned int)*dhPubLen);
    /* Neither the ECDH secret nor the session key is printed: this key wraps
     * the contract private key, so a log carrying it plus the wrapped blob is
     * the charging key in plaintext. An example that dumped it would be
     * teaching the opposite of what it sets out to show. */
    printf("      shared secret and session key derived\n");

    /* Random IV, then AES-CBC over the 32 byte scalar: exactly two blocks, so
     * no padding is involved and the result is IV || ciphertext. */
    ret = wc_RNG_GenerateBlock(rng, wrapped, WC_AES_BLOCK_SIZE);
    if (ret == 0) {
        ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
        if (ret == 0) {
            ret = wc_AesSetKey(&aes, sessionKey, SESSION_KEY, wrapped,
                    AES_ENCRYPTION);
            if (ret == 0) {
                ret = wc_AesCbcEncrypt(&aes, wrapped + WC_AES_BLOCK_SIZE,
                        contractScalar, P256_SCALAR);
            }
            wc_AesFree(&aes);
        }
    }

    wc_ForceZero(sessionKey, sizeof(sessionKey));
    wc_ForceZero(secret, sizeof(secret));
    wc_ecc_free(&ephemeral);
    return ret;
}

/* The vehicle side: unwrap with the provisioning private key.
 * Returns 0 on success, with the 32 byte scalar in 'scalar'. */
static int vehicle_unwrap(ecc_key* provisioningPriv, WC_RNG* rng,
        const unsigned char* dhPub, word32 dhPubLen,
        const unsigned char* wrapped, unsigned char* scalar, int verbose)
{
    ecc_key ephemeralPub;
    unsigned char sessionKey[SESSION_KEY];
    unsigned char secret[SECRET_SZ];
    Aes aes;
    int ret;

    if (wc_ecc_init(&ephemeralPub) != 0) {
        return -1;
    }

    /* DHpublickey arrives as an X9.63 point. */
    ret = wc_ecc_import_x963_ex(dhPub, dhPubLen, &ephemeralPub,
            ECC_SECP256R1);
    if (ret == 0) {
        ret = derive_session_key(provisioningPriv, &ephemeralPub, rng,
                sessionKey, secret);
    }
    if (ret != 0) {
        wc_ForceZero(sessionKey, sizeof(sessionKey));
        wc_ForceZero(secret, sizeof(secret));
        wc_ecc_free(&ephemeralPub);
        return ret;
    }

    if (verbose) {
        /* As on the backend side: not printed. */
        printf("      same shared secret and session key derived\n");
    }

    ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (ret == 0) {
        /* the IV is the first block of the wrapped blob */
        ret = wc_AesSetKey(&aes, sessionKey, SESSION_KEY, wrapped,
                AES_DECRYPTION);
        if (ret == 0) {
            ret = wc_AesCbcDecrypt(&aes, scalar,
                    wrapped + WC_AES_BLOCK_SIZE, P256_SCALAR);
        }
        wc_AesFree(&aes);
    }

    wc_ForceZero(sessionKey, sizeof(sessionKey));
    wc_ForceZero(secret, sizeof(secret));
    wc_ecc_free(&ephemeralPub);
    return ret;
}

/* The real test: does the recovered scalar belong to the contract
 * certificate? Sign with it and verify against the certificate's public key.
 * Returns 1 if it is the contract key. */
static int scalar_matches_cert(const unsigned char* scalar, WC_RNG* rng)
{
    ecc_key recovered;
    ecc_key certPub;
    unsigned char hash[WC_SHA256_DIGEST_SIZE];
    unsigned char sig[128];
    word32 sigLen = (word32)sizeof(sig);
    int ret, verified = 0;
    int haveRecovered = 0, haveCertPub = 0;

    XMEMSET(hash, 0xA5, sizeof(hash)); /* any fixed value will do */

    if (wc_ecc_init(&recovered) != 0) {
        return 0;
    }
    haveRecovered = 1;

    /* A wrong scalar may not even be a valid private key, which is itself a
     * correct rejection. */
    ret = wc_ecc_import_private_key_ex(scalar, P256_SCALAR, NULL, 0,
            &recovered, ECC_SECP256R1);
    if (ret == 0) {
        ret = wc_ecc_sign_hash(hash, (word32)sizeof(hash), sig, &sigLen, rng,
                &recovered);
    }
    if (ret == 0 && load_cert_public_key(CONTRACT_CERT, &certPub) == 0) {
        haveCertPub = 1;
        ret = wc_ecc_verify_hash(sig, sigLen, hash, (word32)sizeof(hash),
                &verified, &certPub);
        if (ret != 0) {
            verified = 0;
        }
    }
    else {
        verified = 0;
    }

    if (haveCertPub) {
        wc_ecc_free(&certPub);
    }
    if (haveRecovered) {
        wc_ecc_free(&recovered);
    }
    return verified == 1;
}

int main(void)
{
    ecc_key provisioningPub;
    ecc_key provisioningPriv;
    ecc_key contractKey;
    WC_RNG rng;
    unsigned char contractScalar[P256_SCALAR];
    unsigned char recoveredScalar[P256_SCALAR];
    unsigned char dhPub[128];
    unsigned char wrapped[WRAPPED_SZ];
    word32 dhPubLen = (word32)sizeof(dhPub);
    word32 scalarLen = (word32)sizeof(contractScalar);
    int rngInit = 0;
    int haveProvPub = 0, haveProvPriv = 0, haveContract = 0;

    printf("ISO 15118 contract certificate installation\n");
    printf("  ECDH secp256r1 + X9.63 KDF (SHA-256) + AES-128-CBC\n\n");

    if (wolfCrypt_Init() != 0) {
        fprintf(stderr, "wolfCrypt_Init failed\n");
        return 1;
    }
    if (wc_InitRng(&rng) != 0) {
        fprintf(stderr, "wc_InitRng failed\n");
        /* Count it: every other early exit goes through check(), and without
         * this the run would print the closing narrative and PASS with
         * nothing having been done -- which the Makefile's PASS|SKIP grep
         * would accept. */
        failures++;
        goto cleanup;
    }
    rngInit = 1;

    printf("== the vehicle as built ==\n");
    if (load_cert_public_key(PROV_CERT, &provisioningPub) != 0) {
        check("OEM provisioning certificate loaded", 0);
        goto cleanup;
    }
    haveProvPub = 1;
    check("OEM provisioning certificate loaded", 1);

    if (load_private_key(PROV_KEY, &provisioningPriv) != 0) {
        check("OEM provisioning private key loaded", 0);
        goto cleanup;
    }
    haveProvPriv = 1;
    check("OEM provisioning private key loaded", 1);
    printf("      PCID WOLFSSL0000000001, chains to the V2G root via OEM\n");

    /* Stands in for the contract certificate pool's copy of the key it is
     * about to deliver. In reality the backend generates this. */
    printf("\n== the backend holds the contract key ==\n");
    if (load_private_key(CONTRACT_KEY, &contractKey) != 0) {
        check("contract private key available to the backend", 0);
        goto cleanup;
    }
    haveContract = 1;
    if (wc_ecc_export_private_only(&contractKey, contractScalar, &scalarLen)
            != 0 || scalarLen != P256_SCALAR) {
        check("contract private key exported as a 32 byte scalar", 0);
        goto cleanup;
    }
    check("contract private key exported as a 32 byte scalar", 1);
    printf("      eMAID %s\n", EMAID);

    printf("\n== backend wraps it for this vehicle ==\n");
    if (backend_wrap(&rng, &provisioningPub, contractScalar, dhPub, &dhPubLen,
                wrapped) != 0) {
        check("contract private key wrapped", 0);
        goto cleanup;
    }
    check("contract private key wrapped", 1);
    printf("      ContractSignatureEncryptedPrivateKey is %d bytes "
           "(%d IV + %d)\n", WRAPPED_SZ, WC_AES_BLOCK_SIZE, P256_SCALAR);
    print_hex("wrapped (first 16)", wrapped, 16);

    printf("\n== the vehicle installs the contract ==\n");
    if (vehicle_unwrap(&provisioningPriv, &rng, dhPub, dhPubLen, wrapped,
                recoveredScalar, 1) != 0) {
        check("contract private key unwrapped", 0);
        goto cleanup;
    }
    check("contract private key unwrapped", 1);
    /* This compares the unwrapped scalar with the original, which is what
     * shows the session keys agreed -- the two keys themselves are derived
     * inside backend_wrap()/vehicle_unwrap() and never leave them. Labelled
     * for what it actually compares. */
    check("unwrapped scalar matches the contract key",
            XMEMCMP(recoveredScalar, contractScalar, P256_SCALAR) == 0);

    /* The proof that matters: the recovered key is the contract key, shown by
     * signing with it and verifying against the certificate. Comparing the
     * scalars above only works because this example happens to know the
     * original; a real vehicle does not. */
    check("recovered key signs, contract certificate verifies",
            scalar_matches_cert(recoveredScalar, &rng));

    printf("\n== only this vehicle can do it ==\n");
    {
        ecc_key otherPriv;
        unsigned char wrongScalar[P256_SCALAR];

        if (load_private_key(OTHER_PROV_KEY, &otherPriv) != 0) {
            check("second provisioning key loaded", 0);
        }
        else {
            check("second provisioning key loaded", 1);

            /* Same wrapped blob, same DHpublickey, different provisioning
             * private key. ECDH yields a different secret, so the KDF yields
             * a different session key. */
            /* The unwrap has to SUCCEED here and produce the wrong bytes.
             * AES-CBC without padding cannot reject a wrong key and ECDH on a
             * valid point does not fail on a key mismatch, so a non-zero
             * return means something genuinely broke -- wc_ecc_set_rng,
             * wc_ecc_shared_secret, wc_X963_KDF, wc_AesInit -- and counting
             * that as the expected outcome would turn a real fault into a
             * pass. */
            if (vehicle_unwrap(&otherPriv, &rng, dhPub, dhPubLen, wrapped,
                        wrongScalar, 0) != 0) {
                check("unwrap with the wrong key ran", 0);
            }
            else {
                check("wrong key recovers different bytes",
                        XMEMCMP(wrongScalar, contractScalar, P256_SCALAR)
                            != 0);
                /* scalar_matches_cert() returns 0 both for "does not match"
                 * and for an internal error, so confirm it says yes to the
                 * right scalar before trusting its no. */
                check("the check that says no also says yes to the real key",
                        scalar_matches_cert(contractScalar, &rng));
                check("those bytes are NOT the contract key",
                        !scalar_matches_cert(wrongScalar, &rng));
            }
            wc_ForceZero(wrongScalar, sizeof(wrongScalar));
            wc_ecc_free(&otherPriv);
        }
    }

cleanup:
    /* In the cleanup block, not before it: every error path above reaches
     * here by goto, and private key bytes should not be left on the stack
     * just because something failed. */
    wc_ForceZero(contractScalar, sizeof(contractScalar));
    wc_ForceZero(recoveredScalar, sizeof(recoveredScalar));

    if (haveContract) {
        wc_ecc_free(&contractKey);
    }
    if (haveProvPriv) {
        wc_ecc_free(&provisioningPriv);
    }
    if (haveProvPub) {
        wc_ecc_free(&provisioningPub);
    }
    if (rngInit) {
        wc_FreeRng(&rng);
    }
    wolfCrypt_Cleanup();

    if (failures != 0) {
        printf("\n%d check(s) failed\n", failures);
        printf("\ncontract-install: FAIL\n");
        return 1;
    }

    printf("\n  The contract private key crossed the link only ever\n");
    printf("  wrapped to the provisioning certificate's key. Missing here\n");
    printf("  is the EXI encoding of the two messages and the XML\n");
    printf("  signature over the response -- see v2g-signature.c for\n");
    printf("  that operation.\n");
    printf("\ncontract-install: PASS\n");
    return 0;
}

#endif /* HAVE_X963_KDF */
