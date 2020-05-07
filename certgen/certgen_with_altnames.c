#include <stdio.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define HEAP_HINT NULL
#define FOURK_SZ 4096

#if defined(WOLFSSL_CERT_REQ) && defined(WOLFSSL_CERT_GEN)
void free_things(byte** a, byte** b, byte** c, ecc_key* d, ecc_key* e,
                 WC_RNG* f);
#endif

int main(void) {
#if !defined(WOLFSSL_CERT_REQ) || !defined(WOLFSSL_CERT_GEN) && \
    !defined(WOLFSSL_ALT_NAMES)
  printf("Please compile wolfSSL with the following and try again:\n");
  printf(" --enable-certreq --enable-certgen CFLAGS=-DWOLFSSL_ALT_NAMES\n");
  return 0;
#else

    int ret = 0;

    Cert newCert;

    FILE* file;
    char certToUse[] = "./ca-cert.der";
    char caKeyFile[] = "./ca-key.der";
    char newCertOutput[] = "./newCert.der";

    int derBufSz;
    int caKeySz;

    byte* derBuf   = NULL;
    byte* pemBuf   = NULL;
    byte* caKeyBuf = NULL;

    /* for MakeCert and SignCert */
    WC_RNG rng;
    ecc_key caKey;
    ecc_key newKey;
    word32 idx3 = 0;

/*---------------------------------------------------------------------------*/
/* open the CA der formatted certificate, we need to get it's subject line to
 * use in the new cert we're creating as the "Issuer" line */
/*---------------------------------------------------------------------------*/
    printf("Open and read in der formatted certificate\n");

    derBuf = (byte*) XMALLOC(FOURK_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (derBuf == NULL) goto fail;

    XMEMSET(derBuf, 0, FOURK_SZ);

    file = fopen(certToUse, "rb");
    if (!file) {
        printf("failed to find file: %s\n", certToUse);
        goto fail;
    }

    derBufSz = fread(derBuf, 1, FOURK_SZ, file);

    fclose(file);
    printf("Successfully read the CA cert we are using to sign our new cert\n");
    printf("Cert was %d bytes\n\n", derBufSz);
    derBufSz = FOURK_SZ;
/*---------------------------------------------------------------------------*/
/* END */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* open caKey file and get the caKey, we need it to sign our new cert */
/*---------------------------------------------------------------------------*/
    printf("Getting the caKey from %s\n", caKeyFile);

    caKeyBuf = (byte*) XMALLOC(FOURK_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (caKeyBuf == NULL) goto fail;

    file = fopen(caKeyFile, "rb");
    if (!file) {
        printf("failed to open file: %s\n", caKeyFile);
        goto fail;
    }

    caKeySz = fread(caKeyBuf, 1, FOURK_SZ, file);
    if (caKeySz <= 0) {
        printf("Failed to read caKey from file\n");
        goto fail;
    }

    fclose(file);
    printf("Successfully read %d bytes\n", caKeySz);

    printf("Init ecc Key\n");
    wc_ecc_init(&caKey);

    printf("Decode the private key\n");
    ret = wc_EccPrivateKeyDecode(caKeyBuf, &idx3, &caKey, (word32)caKeySz);
    if (ret != 0) goto fail;

    printf("Successfully retrieved caKey\n\n");
/*---------------------------------------------------------------------------*/
/* END */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Generate new private key to go with our new cert */
/*---------------------------------------------------------------------------*/
    printf("initializing the rng\n");
    ret = wc_InitRng(&rng);
    if (ret != 0) goto fail;

    printf("Generating a new ecc key\n");
    ret = wc_ecc_init(&newKey);
    if (ret != 0) goto fail;

    ret = wc_ecc_make_key(&rng, 32, &newKey);
    if (ret != 0) goto fail;

    printf("Successfully created new ecc key\n\n");
/*---------------------------------------------------------------------------*/
/* END */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Create a new certificate using SUBJECT information from ca cert
 * for ISSUER information in generated cert */
/*---------------------------------------------------------------------------*/
    printf("Setting new cert issuer to subject of signer\n");

    wc_InitCert(&newCert);

    strncpy(newCert.subject.country, "US", CTC_NAME_SIZE);
    strncpy(newCert.subject.state, "MT", CTC_NAME_SIZE);
    strncpy(newCert.subject.locality, "Bozeman", CTC_NAME_SIZE);
    strncpy(newCert.subject.org, "yourOrgNameHere", CTC_NAME_SIZE);
    strncpy(newCert.subject.unit, "yourUnitNameHere", CTC_NAME_SIZE);
    strncpy(newCert.subject.commonName, "www.yourDomain.com", CTC_NAME_SIZE);
    strncpy(newCert.subject.email, "yourEmail@yourDomain.com", CTC_NAME_SIZE);

    /* Add some alt names to our cert: */
    char myAltNames[] = {
         /* SEQUENCE (1 element with 3 segements. Entire length is 35
          * (0x29 in hex))
          */
         /* 0x30 -> SEQUENCE, 0x23 -> Length of entire sequence */
         0x30, 0x23,
         /* This is a String 0x8, it denotes a DNSName 0x2 -> 0x82
          * This strings' length is 9 (0x09)
          */
         0x82, 0x09,
         /* This strings value is "localhost" (in hex) */
         0x6C, 0x6F, 0x63, 0x61, 0x6C, 0x68, 0x6F, 0x73, 0x74,
         /* This is a String 0x8, it denotes a DNSName 0x2 -> 0x82
          * This strings' length is 11 (0x0B)
          */
         0x82, 0x0B,
         /* This strings value is "example.com" (in hex) */
         0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d,
         /* This is a String 0x8, it denotes a DNSName 0x2 -> 0x82
          * This strings' length is 9 (0x09)
          */
         0x82, 0x09,
         /* This strings value is "127.0.0.1" (in hex) */
         0x31, 0x32, 0x37, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x31
    };
    XMEMCPY(newCert.altNames, myAltNames, sizeof(myAltNames));
    newCert.altNamesSz = (int) sizeof(myAltNames);

    newCert.isCA    = 0;
    newCert.sigType = CTC_SHA256wECDSA;

    ret = wc_SetIssuerBuffer(&newCert, derBuf, derBufSz);
    if (ret != 0) goto fail;

    ret = wc_MakeCert(&newCert, derBuf, FOURK_SZ, NULL, &newKey, &rng); //ecc certificate
    if (ret < 0) goto fail;

    printf("MakeCert returned %d\n", ret);

    ret = wc_SignCert(newCert.bodySz, newCert.sigType, derBuf, FOURK_SZ, NULL,
                                                              &caKey, &rng);
    if (ret < 0) goto fail;
    printf("SignCert returned %d\n", ret);

    derBufSz = ret;

    printf("Successfully created new certificate\n");
/*---------------------------------------------------------------------------*/
/* END */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* write the new cert to file in der format */
/*---------------------------------------------------------------------------*/
    printf("Writing newly generated certificate to file \"%s\"\n",
                                                                 newCertOutput);
    file = fopen(newCertOutput, "wb");
    if (!file) {
        printf("failed to open file: %s\n", newCertOutput);
        goto fail;
    }

    ret = (int) fwrite(derBuf, 1, derBufSz, file);
    fclose(file);
    printf("Successfully output %d bytes\n", ret);
/*---------------------------------------------------------------------------*/
/* END */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* convert the der to a pem and write it to a file */
/*---------------------------------------------------------------------------*/
    {
        char pemOutput[] = "./newCert.pem";
        int pemBufSz;

        printf("Convert the der cert to pem formatted cert\n");

        pemBuf = (byte*) XMALLOC(FOURK_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (pemBuf == NULL) goto fail;

        XMEMSET(pemBuf, 0, FOURK_SZ);

        pemBufSz = wc_DerToPem(derBuf, derBufSz, pemBuf, FOURK_SZ, CERT_TYPE);
        if (pemBufSz < 0) goto fail;

        printf("Resulting pem buffer is %d bytes\n", pemBufSz);

        file = fopen(pemOutput, "wb");
        if (!file) {
            printf("failed to open file: %s\n", pemOutput);
            goto fail;
        }
        fwrite(pemBuf, 1, pemBufSz, file);
        fclose(file);
        printf("Successfully converted the der to pem. Result is in:  %s\n\n",
                                                                     pemOutput);
    }
/*---------------------------------------------------------------------------*/
/* END */
/*---------------------------------------------------------------------------*/

    goto success;

fail:
    free_things(&derBuf, &pemBuf, &caKeyBuf, &caKey, &newKey, &rng);
    printf("Failure code was %d\n", ret);
    return -1;

success:
    free_things(&derBuf, &pemBuf, &caKeyBuf, &caKey, &newKey, &rng);
    printf("Tests passed\n");
    return 0;
}

void free_things(byte** a, byte** b, byte** c, ecc_key* d, ecc_key* e,
                                                                      WC_RNG* f)
{
    if (a != NULL) {
        if (*a != NULL) {
            XFREE(*a, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            *a = NULL;
        }
    }
    if (b != NULL) {
        if (*b != NULL) {
            XFREE(*b, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            *b = NULL;
        }
    }
    if (c != NULL) {
        if (*c != NULL) {
            XFREE(*c, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            *c = NULL;
        }
    }

    wc_ecc_free(d);
    wc_ecc_free(e);
    wc_FreeRng(f);
#endif
}
