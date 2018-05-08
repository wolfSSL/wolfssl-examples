#include "clu_include/certgen/clu_certgen_ecc.h"

void free_things(byte** a, byte** b, byte** c, ecc_key* d, ecc_key* e,
                                                                     WC_RNG* f);

int make_self_signed_ecc_certificate(char* keyPath, char* certOut) {
    int ret = 0;
    word32 index = 0;
    
    Cert newCert;
    ecc_key key;
    WC_RNG rng;
    
    int keyFileSz;
    FILE* keyFile = fopen(keyPath,"rb");
    fseek(keyFile, 0, SEEK_END);
    keyFileSz = ftell(keyFile);
    byte keyBuf[keyFileSz];
    fseek(keyFile, 0, SEEK_SET);
    fread(keyBuf, 1, keyFileSz, keyFile);
    fclose(keyFile);
    
    ret = wc_ecc_init(&key);
    if (ret != 0) {
        printf("Failed to initialize ecc key\nRET: %d\n", ret);
        return ret;
    }
    
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Failed to initialize rng.\nRET: %d\n", ret);
        return ret;
    }
    
    ret = wc_EccPrivateKeyDecode(keyBuf, &index, &key, keyFileSz);
    if (ret != 0 ) {
        printf("Failed to decode private key.\nRET: %d\n", ret);
        return ret;
    }
    
    wc_InitCert(&newCert);
    char country[3];
    char province[CTC_NAME_SIZE];
    char city[CTC_NAME_SIZE];
    char org[CTC_NAME_SIZE];
    char unit[CTC_NAME_SIZE];
    char commonName[CTC_NAME_SIZE];
    char email[CTC_NAME_SIZE];
    char daysValid[CTC_NAME_SIZE];
    
    printf("Enter your countries 2 digit code (ex: United States -> US): ");
    fgets(country,CTC_NAME_SIZE,stdin);
    country[sizeof(country)-1] = '\0';
    printf("Enter the name of the province you are located at: ");
    fgets(province,CTC_NAME_SIZE,stdin);
    printf("Enter the name of the city you are located at: ");
    fgets(city,CTC_NAME_SIZE,stdin);
    printf("Enter the name of your orginization: ");
    fgets(org,CTC_NAME_SIZE,stdin);
    printf("Enter the name of your unit: ");
    fgets(unit,CTC_NAME_SIZE,stdin);
    printf("Enter the common name of your domain: ");
    fgets(commonName,CTC_NAME_SIZE,stdin);
    printf("Enter your email address: ");
    fgets(email,CTC_NAME_SIZE,stdin);
    printf("Enter the number of days this certificate should be valid: ");
    fgets(daysValid,CTC_NAME_SIZE,stdin);
    
    strncpy(newCert.subject.country, country, sizeof(country));
    strncpy(newCert.subject.state, province, CTC_NAME_SIZE);
    strncpy(newCert.subject.locality, city, CTC_NAME_SIZE);
    strncpy(newCert.subject.org, org, CTC_NAME_SIZE);
    strncpy(newCert.subject.unit, unit, CTC_NAME_SIZE);
    strncpy(newCert.subject.commonName, commonName, CTC_NAME_SIZE);
    strncpy(newCert.subject.email, email, CTC_NAME_SIZE);
    newCert.daysValid = atoi(daysValid);
    newCert.isCA    = 0;
    newCert.sigType = CTC_SHA256wECDSA;
    
    byte* certBuf = (byte*) XMALLOC(FOURK_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (certBuf == NULL) {
        printf("Failed to initialize buffer to stort certificate.\n");
        return -1;
    }

    XMEMSET(certBuf, 0, FOURK_SZ);
    int certBufSz = FOURK_SZ;

    ret = wc_MakeCert(&newCert, certBuf, FOURK_SZ, NULL, &key, &rng); //ecc certificate
    if (ret < 0) {
        printf("Failed to make certificate.\n");
        return ret;
    }
    printf("MakeCert returned %d\n", ret);

    ret = wc_SignCert(newCert.bodySz, newCert.sigType, certBuf, FOURK_SZ, NULL,
                                                              &key, &rng);
    if (ret < 0) {
        printf("Failed to sign certificate.\n");
        return ret;
    }
    printf("SignCert returned %d\n", ret);

    certBufSz = ret;

    printf("Successfully created new certificate\n");
    
    printf("Writing newly generated certificate to file \"%s\"\n",
                                                                 certOut);
    FILE* file = fopen(certOut, "wb");
    if (!file) {
        printf("failed to open file: %s\n", certOut);
        return -1;
    }

    ret = (int) fwrite(certBuf, 1, certBufSz, file);
    fclose(file);
    printf("Successfully output %d bytes\n", ret);

/*---------------------------------------------------------------------------*/
/* convert the der to a pem and write it to a file */
/*---------------------------------------------------------------------------*/
    {
        int pemBufSz;

        printf("Convert the der cert to pem formatted cert\n");

        byte* pemBuf = (byte*) XMALLOC(FOURK_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (pemBuf == NULL) {
            printf("Failed to initialize pem buffer.\n");
            return -1;
        }

        XMEMSET(pemBuf, 0, FOURK_SZ);

        pemBufSz = wc_DerToPem(certBuf, certBufSz, pemBuf, FOURK_SZ, CERT_TYPE);
        if (pemBufSz < 0) {
            printf("Failed to convert from der to pem.\n");
            return -1;
        }

        printf("Resulting pem buffer is %d bytes\n", pemBufSz);

        FILE* pemFile = fopen(certOut, "wb");
        if (!pemFile) {
            printf("failed to open file: %s\n", certOut);
            return -1;
        }
        fwrite(pemBuf, 1, pemBufSz, pemFile);
        fclose(pemFile);
        printf("Successfully converted the der to pem. Result is in:  %s\n\n",
                                                                     certOut);
    }

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

}
