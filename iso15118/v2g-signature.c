/* v2g-signature.c
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

/* The cryptographic operation behind an ISO 15118 XML signature.
 *
 * V2G messages that need proof of origin -- an authorisation request, a
 * metering receipt, a sales tariff -- carry an XML signature over the EXI
 * encoding of the element being signed, using ECDSA over secp256r1 with
 * SHA-256 and the contract certificate's key.
 *
 * That is the part wolfSSL does, and it is all this example does. It takes a
 * FIXTURE: a byte string standing in for an already-canonicalised EXI
 * fragment, hashes it, signs it with the contract private key, and verifies
 * against the contract certificate's public key.
 *
 * What is missing, and why:
 *
 *   EXI encoding          wolfSSL has no EXI codec and should not grow one.
 *   XML canonicalisation  the c14n rules are an XML problem, not a crypto one.
 *   Reference digests     which elements are signed, and the SignedInfo
 *                         structure around them, is 15118's schema.
 *
 * Producing those badly would be worse than not producing them, so the input
 * here is a fixture and the example says so. Everything from the digest
 * onwards is real.
 *
 * Usage: ./v2g-signature
 */

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include <stdio.h>
#include <string.h>

#define CONTRACT_KEY  "certs/contract.key"
#define CONTRACT_CERT "certs/contract.pem"

#define MAX_PEM 4096
#define MAX_DER 2048
#define MAX_SIG 128

static int failures = 0;

static void check(const char* what, int ok)
{
    printf("  %-46s %s\n", what, ok ? "ok" : "FAILED");
    if (!ok) {
        failures++;
    }
}

/* Stand-in for a canonicalised EXI fragment: what an AuthorizationReq would
 * look like after encoding, as bytes to be digested. NOT real EXI. */
static const unsigned char exiFixture[] = {
    0x80, 0x98, 0x02, 0x11, 0xd7, 0x62, 0x30, 0x31,
    0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x11, 0x94, 0x40, 0x00
};

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

int main(void)
{
    unsigned char pem[MAX_PEM];
    unsigned char der[MAX_DER];
    unsigned char hash[WC_SHA256_DIGEST_SIZE];
    unsigned char sig[MAX_SIG];
    size_t pemLen;
    int derLen;
    word32 idx, sigLen;
    ecc_key key;
    WC_RNG rng;
    wc_Sha256 sha;
    int ret, verified = 0;
    int keyInit = 0, rngInit = 0;

    printf("ISO 15118 XML signature: the cryptographic part\n");
    printf("  ECDSA secp256r1 with SHA-256, contract certificate key\n");
    printf("  input is a %u byte FIXTURE, not real EXI\n\n",
            (unsigned int)sizeof(exiFixture));

    if (wolfCrypt_Init() != 0) {
        fprintf(stderr, "wolfCrypt_Init failed\n");
        return 1;
    }

    /* ---- digest the (fixture) EXI fragment ---- */
    ret = wc_InitSha256(&sha);
    if (ret == 0) {
        ret = wc_Sha256Update(&sha, exiFixture, (word32)sizeof(exiFixture));
        if (ret == 0) {
            ret = wc_Sha256Final(&sha, hash);
        }
        /* Inside the success branch: freeing a context wc_InitSha256() never
         * initialized would inspect whatever was on the stack. */
        wc_Sha256Free(&sha);
    }
    check("SHA-256 over the EXI fragment", ret == 0);
    if (ret != 0) {
        goto cleanup;
    }

    /* ---- sign with the contract private key ---- */
    if (read_file(CONTRACT_KEY, pem, sizeof(pem), &pemLen) != 0) {
        failures++;
        goto cleanup;
    }
    derLen = wc_KeyPemToDer(pem, (int)pemLen, der, sizeof(der), NULL);
    check("contract private key PEM -> DER", derLen > 0);
    if (derLen <= 0) {
        goto cleanup;
    }

    ret = wc_ecc_init(&key);
    if (ret != 0) {
        failures++;
        goto cleanup;
    }
    keyInit = 1;

    idx = 0;
    ret = wc_EccPrivateKeyDecode(der, &idx, &key, (word32)derLen);
    check("contract private key decoded", ret == 0);
    if (ret != 0) {
        goto cleanup;
    }

    /* The curve must be secp256r1; anything else is not 15118 conformant. */
    check("key is on secp256r1",
            wc_ecc_size(&key) == 32 &&
            wc_ecc_get_curve_id(key.idx) == ECC_SECP256R1);

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        fprintf(stderr, "wc_InitRng failed: %d\n", ret);
        failures++;
        goto cleanup;
    }
    rngInit = 1;

    sigLen = (word32)sizeof(sig);
    ret = wc_ecc_sign_hash(hash, (word32)sizeof(hash), sig, &sigLen,
            &rng, &key);
    check("ECDSA signature produced", ret == 0 && sigLen > 0);
    if (ret != 0) {
        goto cleanup;
    }
    printf("      %u byte DER-encoded signature\n", (unsigned int)sigLen);

    wc_ecc_free(&key);
    keyInit = 0;

    /* ---- verify with the public key from the contract certificate ---- */
    if (read_file(CONTRACT_CERT, pem, sizeof(pem), &pemLen) != 0) {
        failures++;
        goto cleanup;
    }
    derLen = wc_CertPemToDer(pem, (int)pemLen, der, sizeof(der), CERT_TYPE);
    check("contract certificate PEM -> DER", derLen > 0);
    if (derLen <= 0) {
        goto cleanup;
    }

    {
        /* wc_GetSubjectPubKeyInfoDerFromCert() hands back the
         * SubjectPublicKeyInfo straight from the certificate DER, which is
         * what wc_EccPublicKeyDecode() consumes. Using it avoids pulling in
         * asn.h and a DecodedCert just to read a public key. */
        unsigned char spki[MAX_DER];
        word32 spkiLen = (word32)sizeof(spki);

        ret = wc_GetSubjectPubKeyInfoDerFromCert(der, (word32)derLen,
                spki, &spkiLen);
        check("SubjectPublicKeyInfo read from the certificate", ret == 0);

        if (ret == 0) {
            ret = wc_ecc_init(&key);
            if (ret == 0) {
                keyInit = 1;
                idx = 0;
                ret = wc_EccPublicKeyDecode(spki, &idx, &key, spkiLen);
            }
            check("public key decoded", ret == 0);
        }
    }
    if (ret != 0) {
        goto cleanup;
    }

    ret = wc_ecc_verify_hash(sig, sigLen, hash, (word32)sizeof(hash),
            &verified, &key);
    check("signature verifies against the certificate",
            ret == 0 && verified == 1);

    /* A single flipped bit in the signed data must break it. Without this the
     * check above only proves the call returned zero. */
    if (ret == 0 && verified == 1) {
        unsigned char tampered[sizeof(exiFixture)];
        int stillValid = 0;
        int hret;

        memcpy(tampered, exiFixture, sizeof(tampered));
        tampered[0] ^= 0x01;

        /* Every call has to succeed as well as the signature having to be
         * invalid. Ignoring the return values would let a hashing or verify
         * ERROR leave stillValid at 0 and count as proof that the change was
         * detected -- the check would pass without testing anything. */
        hret = wc_InitSha256(&sha);
        if (hret == 0) {
            hret = wc_Sha256Update(&sha, tampered, (word32)sizeof(tampered));
            if (hret == 0) {
                hret = wc_Sha256Final(&sha, hash);
            }
            wc_Sha256Free(&sha);
        }
        if (hret == 0) {
            hret = wc_ecc_verify_hash(sig, sigLen, hash,
                    (word32)sizeof(hash), &stillValid, &key);
        }
        check("a modified fragment does NOT verify",
                hret == 0 && stillValid == 0);
    }

cleanup:
    if (keyInit) {
        wc_ecc_free(&key);
    }
    if (rngInit) {
        wc_FreeRng(&rng);
    }
    wolfCrypt_Cleanup();

    if (failures != 0) {
        printf("\n%d check(s) failed\n", failures);
        printf("\nv2g-signature: FAIL\n");
        return 1;
    }

    printf("\n  This is the operation an XML signature wraps. Building the\n");
    printf("  SignedInfo structure, canonicalising it and EXI-encoding\n");
    printf("  it is\n");
    printf("  a V2G stack's job, not wolfSSL's.\n");
    printf("\nv2g-signature: PASS\n");
    return 0;
}
