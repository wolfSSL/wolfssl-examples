#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/asn_public.h>

#define MAX_TEMP_SIZE 1024

int main(void)
{
    int ret;
    ecc_key key;
    WC_RNG rng;
    Cert req;
    byte der[MAX_TEMP_SIZE], pem[MAX_TEMP_SIZE];
    int  derSz, pemSz;

    ret = wc_ecc_init(&key);
    if (ret != 0) {
        printf("ECC init key failed: %d\n", ret);
        goto exit;
    }

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Init rng failed: %d\n", ret);
        goto exit;
    }

    ret = wc_ecc_make_key_ex(&rng, 32, &key, ECC_SECP256R1);
    if (ret != 0) {
        printf("ECC make key failed: %d\n", ret);
        goto exit;
    }

    ret = wc_EccKeyToDer(&key, der, sizeof(der));
    if (ret <= 0) {
        printf("ECC Key To DER failed: %d\n", ret);
        goto exit;
    }
    derSz = ret;

    memset(pem, 0, sizeof(pem));
    ret = wc_DerToPem(der, derSz, pem, sizeof(pem), ECC_PRIVATEKEY_TYPE);
    if (ret <= 0) {
        printf("DER to PEM failed: %d\n", ret);
        goto exit;
    }
    pemSz = ret;
    printf("%s", pem);

    ret = wc_InitCert(&req);
    if (ret != 0) {
        printf("Init Cert failed: %d\n", ret);
        goto exit;
    }
    strncpy(req.subject.country, "US", CTC_NAME_SIZE);
    strncpy(req.subject.state, "OR", CTC_NAME_SIZE);
    strncpy(req.subject.locality, "Portland", CTC_NAME_SIZE);
    strncpy(req.subject.org, "wolfSSL", CTC_NAME_SIZE);
    strncpy(req.subject.unit, "Development", CTC_NAME_SIZE);
    strncpy(req.subject.commonName, "www.wolfssl.com", CTC_NAME_SIZE);
    strncpy(req.subject.email, "info@wolfssl.com", CTC_NAME_SIZE);
    ret = wc_MakeCertReq(&req, der, sizeof(der), NULL, &key);
    if (ret <= 0) {
        printf("Make Cert Req failed: %d\n", ret);
        goto exit;
    }
    derSz = ret;

    req.sigType = CTC_SHA256wECDSA;
    ret = wc_SignCert(req.bodySz, req.sigType, der, sizeof(der), NULL, &key, &rng);
    if (ret <= 0) {
        printf("Sign Cert failed: %d\n", ret);
        goto exit;
    }
    derSz = ret;

    ret = wc_DerToPem(der, derSz, pem, sizeof(pem), CERTREQ_TYPE);
    if (ret <= 0) {
        printf("DER to PEM failed: %d\n", ret);
        goto exit;
    }
    pemSz = ret;
    printf("%s", pem);

exit:
    wc_ecc_free(&key);
    wc_FreeRng(&rng);

    return ret;
}
