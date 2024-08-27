/* gen_rsa_mldsa_dual_keysig_cert.c
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
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/dilithium.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

#if defined(WOLFSSL_DUAL_ALG_CERTS) && defined(HAVE_DILITHIUM)

#define LARGE_TEMP_SZ 9216

#if defined(GEN_ROOT_CERT) && defined(GEN_SERVER_CERT)
    #error "Please only generate a root OR server certificate."
#endif

#define SUBJECT_COUNTRY "US"
#define SUBJECT_STATE "MT"
#define SUBJECT_LOCALITY "YourCity"
#define SUBJECT_ORG "YourOrgName"
#define SUBJECT_UNIT "YourUnitName"
#define SUBJECT_COMMONNAME "www.YourDomain.com"

#ifdef GEN_ROOT_CERT
#define SUBJECT_EMAIL "pq-root@YourDomain.com"
#else
#define SUBJECT_EMAIL "pq-server@YourDomain.com"
#endif

int readFileIntoBuffer(char *fname, byte *buf, int *sz)
{
    int ret;
    FILE *file;
    XMEMSET(buf, 0, *sz);
    file = fopen(fname, "rb");
    if (!file) {
        printf("failed to open file: %s\n", fname);
        return -1;
    }
    ret = fread(buf, 1, *sz, file);
    fclose(file);
    if (ret > 0)
        *sz = ret;
    return ret;
}

#ifdef HAVE_FIPS
    #include <wolfssl/wolfcrypt/fips_test.h>

    static void myFipsCb(int ok, int err, const char* hash)
    {
        printf("in my Fips callback, ok = %d, err = %d\n", ok, err);
        printf("message = %s\n", wc_GetErrorString(err));
        printf("hash = %s\n", hash);

        if (err == IN_CORE_FIPS_E) {
            printf("In core integrity hash check failure, copy above hash\n");
            printf("into verifyCore[] in fips_test.c and rebuild\n");
        }
    }
#endif

static int do_certgen(int argc, char** argv)
{
    int ret = 0;

    char caKeyFile[] = "./ca-key.der";
    char altPrivFile[] = "../certs/mldsa44_ca_key.der";
#ifdef GEN_ROOT_CERT
    char newCertOutput[] = "./ca-cert-pq.der";
    char sapkiFile[] = "../certs/mldsa44_ca_pubkey.der";
#else
    char caCert[] = "./ca-cert-pq.der";
    char newCertOutput[] = "./server-cert-pq.der";
    char serverKeyFile[] = "./server-key.der";
    char sapkiFile[] = "../certs/mldsa44_server_pubkey.der";
#endif
    FILE* file;
    Cert newCert;
    DecodedCert preTBS;

#ifndef GEN_ROOT_CERT 
    byte caCertBuf[LARGE_TEMP_SZ];
    int caCertSz = LARGE_TEMP_SZ;
    byte serverKeyBuf[LARGE_TEMP_SZ];
    int serverKeySz = LARGE_TEMP_SZ;
#endif /* !GEN_ROOT_CERT */

    byte caKeyBuf[LARGE_TEMP_SZ];
    int caKeySz = LARGE_TEMP_SZ;
    byte sapkiBuf[LARGE_TEMP_SZ];
    int sapkiSz = LARGE_TEMP_SZ;
    byte altPrivBuf[LARGE_TEMP_SZ];
    int altPrivSz = LARGE_TEMP_SZ;
    byte altSigAlgBuf[LARGE_TEMP_SZ];
    int altSigAlgSz = LARGE_TEMP_SZ;
    byte scratchBuf[LARGE_TEMP_SZ];
    int scratchSz = LARGE_TEMP_SZ;
    byte preTbsBuf[LARGE_TEMP_SZ];
    int preTbsSz = LARGE_TEMP_SZ;
    byte altSigValBuf[LARGE_TEMP_SZ];
    int altSigValSz = LARGE_TEMP_SZ;
    byte outBuf[LARGE_TEMP_SZ];
    int outSz = LARGE_TEMP_SZ;

    /* The are for MakeCert and SignCert. */
    WC_RNG rng;
    int initRng = 0;

    RsaKey caKey;
    int initCaKey = 0;
#ifndef GEN_ROOT_CERT
    RsaKey serverKey;
    int initServerKey = 0;
#endif /* !GEN_ROOT_CERT */
    int initPreTBS = 0;
    dilithium_key altCaKey;
    word32 idx = 0;

#if 0
    wolfSSL_Debugging_ON();
#endif

#ifdef WC_RNG_SEED_CB
 wc_SetSeed_Cb(wc_GenerateSeed);
#endif

#if defined(HAVE_FIPS)
    wolfCrypt_SetCb_fips(myFipsCb);
    #if FIPS_VERSION3_GE(6,0,0)
        printf("FIPS module version in use: %s\n",
               wolfCrypt_GetVersion_fips());
    #endif
#endif

    ret = wc_InitRng(&rng);
    if (ret != 0) goto exit;
    initRng = 1;

#ifndef GEN_ROOT_CERT
    /* Open the CA der formatted certificate. if we are generating the server
     * certificate. We need to get it's subject line to use in the new cert
     * we're creating as the "Issuer" line */
    printf("Loading CA certificate\n");
    ret = readFileIntoBuffer(caCert, caCertBuf, &caCertSz);
    if (ret <= 0) goto exit;
    printf("Successfully read %d bytes from %s\n\n", caCertSz, caCert);

    /* Open the server private key. We need this to embed the public part into
     * the certificate. */
    printf("Loading server private key\n");
    ret = readFileIntoBuffer(serverKeyFile, serverKeyBuf, &serverKeySz);
    if (ret <= 0) goto exit;
    printf("Successfully read %d bytes from %s\n\n", serverKeySz,
           serverKeyFile);

    printf("Decoding the server private key\n");
    ret = wc_InitRsaKey_ex(&serverKey, NULL, INVALID_DEVID);
    if (ret != 0) goto exit;
    initServerKey = 1;
    idx = 0;
    ret = wc_RsaPrivateKeyDecode(serverKeyBuf, &idx, &serverKey,
                                 (word32)serverKeySz);
    if (ret != 0) goto exit;
    printf("Successfully decoded server private key\n\n");
#endif /* !GEN_ROOT_CERT */

    /* Open caKey file and get the caKey, we need it to sign our new cert. */
    printf("Loading the CA key\n");
    ret = readFileIntoBuffer(caKeyFile, caKeyBuf, &caKeySz);
    if (ret <= 0) goto exit;
    printf("Successfully read %d bytes from %s\n", caKeySz, caKeyFile);

    printf("Decoding the CA private key\n");
    ret = wc_InitRsaKey_ex(&caKey, NULL, INVALID_DEVID);
    if (ret != 0) goto exit;
    initCaKey = 1;
    idx = 0;
    ret = wc_RsaPrivateKeyDecode(caKeyBuf, &idx, &caKey, (word32)caKeySz);
    if (ret != 0) goto exit;
    printf("Successfully decoded CA private key\n\n");

    /* Open the subject alternative public key file. */
    printf("Loading the subject alternative public key\n");
    ret = readFileIntoBuffer(sapkiFile, sapkiBuf, &sapkiSz);
    if (ret <= 0) goto exit;
    printf("Successfully read %d bytes from %s\n", sapkiSz, sapkiFile);

    /* Open the issuer's alternative private key file. */
    printf("Loading the alternative private key\n");
    ret = readFileIntoBuffer(altPrivFile, altPrivBuf, &altPrivSz);
    if (ret <= 0) goto exit;
    printf("Successfully read %d bytes from %s\n", altPrivSz, altPrivFile);

    printf("Decoding the CA alt private key\n");
    wc_dilithium_init(&altCaKey);
    ret = wc_dilithium_set_level(&altCaKey, 2);
    if (ret < 0) goto exit;

    idx = 0;
    ret = wc_Dilithium_PrivateKeyDecode(altPrivBuf, &idx, &altCaKey,
                                        (word32)altPrivSz);
    if (ret != 0) goto exit;
    printf("Successfully decoded CA alt private key\n");

    XMEMSET(altSigAlgBuf, 0, altSigAlgSz);

    altSigAlgSz = SetAlgoID(CTC_DILITHIUM_LEVEL2, altSigAlgBuf, oidSigType, 0);
    if (altSigAlgSz <= 0) goto exit;
    printf("Successfully generated alternative signature algorithm;");
    printf(" %d bytes.\n\n", altSigAlgSz);

    /* Create a new certificate. If server cert, use SUBJECT information from
     * ca cert for ISSUER information in generated cert. */
#ifdef GEN_ROOT_CERT
    printf("Generate self signed cert\n");
#else
    printf("Generate the server cert\n");
#endif
    wc_InitCert(&newCert);
    strncpy(newCert.subject.country, SUBJECT_COUNTRY, CTC_NAME_SIZE);
    strncpy(newCert.subject.state, SUBJECT_STATE, CTC_NAME_SIZE);
    strncpy(newCert.subject.locality, SUBJECT_LOCALITY, CTC_NAME_SIZE);
    strncpy(newCert.subject.org, SUBJECT_ORG, CTC_NAME_SIZE);
    strncpy(newCert.subject.unit, SUBJECT_UNIT, CTC_NAME_SIZE);
    strncpy(newCert.subject.commonName, SUBJECT_COMMONNAME, CTC_NAME_SIZE);
    strncpy(newCert.subject.email, SUBJECT_EMAIL, CTC_NAME_SIZE);

    newCert.sigType = CTC_SHA256wRSA;

#ifdef GEN_ROOT_CERT
    newCert.isCA    = 1;
#else
    newCert.isCA    = 0;
    ret = wc_SetIssuerBuffer(&newCert, caCertBuf, caCertSz);
    if (ret != 0) goto exit;
#endif

    ret = wc_SetCustomExtension(&newCert, 0, "2.5.29.72", sapkiBuf, sapkiSz);
    if (ret < 0) goto exit;
    ret = wc_SetCustomExtension(&newCert, 0, "2.5.29.73", altSigAlgBuf,
                                altSigAlgSz);
    if (ret < 0) goto exit;

    /* Generate a cert and then convert into a DecodedCert. */
    XMEMSET(scratchBuf, 0, scratchSz);
#ifdef GEN_ROOT_CERT
    ret = wc_MakeSelfCert(&newCert, scratchBuf, scratchSz, &caKey, &rng);
    if (ret <= 0) goto exit;
    printf("wc_MakeSelfCert for preTBS returned %d\n", ret);
#else
    ret = wc_MakeCert(&newCert, scratchBuf, scratchSz, &serverKey, NULL, &rng);
    if (ret <= 0) goto exit;
    printf("wc_MakeCert for preTBS returned %d\n", ret);

    /* Technically, we don't need to sign because as it stands now, the DER has
     * everything we need. However, when we call wc_ParseCert, the lack of a
     * signature will be fatal. */
    ret = wc_SignCert(newCert.bodySz, newCert.sigType, scratchBuf,
                      scratchSz, &caKey, NULL, &rng);
    if (ret < 0) goto exit;
    printf("wc_SignCert for preTBS returned %d\n", ret);
#endif
    scratchSz = ret;

    wc_InitDecodedCert(&preTBS, scratchBuf, scratchSz, 0);
    initPreTBS = 1;
    ret = wc_ParseCert(&preTBS, CERT_TYPE, NO_VERIFY, NULL);
    if (ret < 0) goto exit;

    /* Generate the DER for a pre-TBS. */
    XMEMSET(preTbsBuf, 0, preTbsSz);
    ret = wc_GeneratePreTBS(&preTBS, preTbsBuf, preTbsSz);
    if (ret < 0) goto exit;
    printf("PreTBS is %d bytes.\n", ret);
    preTbsSz = ret;

    /* Generate the contents of the altSigVal extension and inject into cert. */
    XMEMSET(altSigValBuf, 0, altSigValSz);
    ret = wc_MakeSigWithBitStr(altSigValBuf, altSigValSz, CTC_DILITHIUM_LEVEL2,
                               preTbsBuf, preTbsSz, DILITHIUM_LEVEL2_TYPE,
                               &altCaKey, &rng);
    if (ret < 0) goto exit;
    altSigValSz = ret;
    printf("altSigVal is %d bytes.\n", altSigValSz);

    ret = wc_SetCustomExtension(&newCert, 0, "2.5.29.74",
                                altSigValBuf, altSigValSz);
    if (ret < 0) goto exit;

    /* Finally, generate the new certificate. */
    XMEMSET(outBuf, 0, outSz);
#ifdef GEN_ROOT_CERT
    ret = wc_MakeSelfCert(&newCert, outBuf, outSz, &caKey, &rng);
    if (ret <= 0) goto exit;
    printf("wc_MakeSelfCert for preTBS returned %d\n", ret);
#else
    ret = wc_MakeCert(&newCert, outBuf, outSz, &serverKey, NULL, &rng);
    if (ret < 0) goto exit;
    printf("Make Cert returned %d\n", ret);

    ret = wc_SignCert(newCert.bodySz, newCert.sigType, outBuf, outSz, &caKey,
                      NULL, &rng);
    if (ret < 0) goto exit;
    printf("Sign Cert returned %d\n", ret);
#endif
    outSz = ret;

    printf("Successfully created new certificate\n\n");

    /* Write the new cert to file in der format. */
    printf("Writing newly generated DER certificate to file \"%s\"\n",
           newCertOutput);
    file = fopen(newCertOutput, "wb");
    if (!file) {
        printf("failed to open file: %s\n", newCertOutput);
        goto exit;
    }
    ret = fwrite(outBuf, 1, outSz, file);
    fclose(file);
    if (ret <= 0) goto exit;
    printf("Successfully output %d bytes\n", ret);

    ret = 0;
    printf("SUCCESS!\n");
exit:

    if (initCaKey)
        wc_FreeRsaKey(&caKey);
#ifndef GEN_ROOT_CERT
    if (initServerKey)
        wc_FreeRsaKey(&serverKey);
#endif
    if (initPreTBS)
        wc_FreeDecodedCert(&preTBS);
    if (initRng)
        wc_FreeRng(&rng);

    if (ret != 0)
        printf("Failure code was %d\n", ret);
    return ret;
}

int main(int argc, char** argv)
{
    return do_certgen(argc, argv);
}

#else

int main(int argc, char** argv)
{
    printf("Please compile wolfSSL with --enable-dual-alg-certs --with-liboqs "
           "or CFLAGS=\"-DWOLFSSL_DUAL_ALG_CERTS -DHAVE_LIBOQS\"");
    return 0;
}

#endif
