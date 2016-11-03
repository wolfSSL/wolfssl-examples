#include <stdio.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define HEAP_HINT NULL
#define FOURK_SZ 4096

void free_things(byte* a, byte* b, byte* c, ecc_key* d, ecc_key* e, WC_RNG* f);

int main(void) {

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
/* open and read the der formatted certificate */
/*---------------------------------------------------------------------------*/
    printf("Open and read in der formatted certificate\n");
    derBuf = (byte*) XMALLOC(FOURK_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    XMEMSET(derBuf, 0, FOURK_SZ);

    file = fopen(certToUse, "rb");
    if (!file) {
        printf("failed to find file: %s\n", certToUse);
        goto fail;
    }

    derBufSz = fread(derBuf, 1, FOURK_SZ, file);

    fclose(file);
    printf("Successfully read %d bytes\n\n", derBufSz);
/*---------------------------------------------------------------------------*/
/* END */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* open caKey file and get the caKey */
/*---------------------------------------------------------------------------*/
    printf("Getting the caKey from %s\n", caKeyFile);

    caKeyBuf = (byte*) XMALLOC(FOURK_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

    file = fopen(caKeyFile, "rb");
    if (!file) {
        printf("failed to open file: %s\n", caKeyFile);
        goto fail;
    }

    caKeySz = fread(caKeyBuf, 1, FOURK_SZ, file);
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
/* Generate new ecc key */
/*---------------------------------------------------------------------------*/
    printf("initializing the rng\n");
    ret = wc_InitRng(&rng);
    if (ret != 0) goto fail;

    printf("Generating a new ecc key\n");
    wc_ecc_init(&newKey);

    ret = wc_ecc_make_key(&rng, 32, &newKey);
    if (ret != 0) goto fail;

    printf("Successfully created new ecc key\n\n");
/*---------------------------------------------------------------------------*/
/* END */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Create a new certificate using header information from der cert */
/*---------------------------------------------------------------------------*/
    printf("Setting new cert issuer to subject of signer\n");

    wc_InitCert(&newCert);

    newCert.sigType = CTC_SHA256wECDSA;

    ret = wc_SetIssuerBuffer(&newCert, derBuf, derBufSz);
    if (ret != 0) goto fail;

    ret = wc_MakeCert(&newCert, derBuf, FOURK_SZ, NULL, &newKey, &rng); //ecc certificate
    if (ret < 0) goto fail;

    derBufSz = ret;
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
    free_things(derBuf, pemBuf, caKeyBuf, &caKey, &newKey, &rng);
    printf("Failure code was %d\n", ret);
    return -1;

success:
    free_things(derBuf, pemBuf, caKeyBuf, &caKey, &newKey, &rng);
    printf("Tests passed\n");
    return 0;
}

void free_things(byte* a, byte* b, byte* c, ecc_key* d, ecc_key* e, WC_RNG* f)
{
    if (a != NULL) {
        XFREE(a, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        a = NULL;
    }
    if (b != NULL) {
        XFREE(b, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        b = NULL;
    }
    if (c != NULL) {
        XFREE(c, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        c = NULL;
    }

    wc_ecc_free(d);
    wc_ecc_free(e);
    wc_FreeRng(f);

}
