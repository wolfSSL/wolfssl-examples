#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/wc_port.h>

#include <stdio.h>

#if defined(WOLFSSL_TEST_CERT) || defined(OPENSSL_EXTRA) || \
    defined(OPENSSL_EXTRA_X509_SMALL)

static void check_ret(char*, int);

int main(void)
{
    DecodedCert decodedCert;
    FILE* file;
    byte derBuffer[4096];
    size_t bytes;
    ecc_key eccKey;
    int ret;
    word32 inOutIdx = 0;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    wolfCrypt_Init();

    file = fopen("../certs/client-ecc-cert.der", "rb");
    if (!file) {
        printf("Failed to open file\n");
        exit(-99);
    }

    bytes = fread(derBuffer, 1, 4096, file);
    fclose(file);

    printf("read bytes = %d\n", (int) bytes);
    if (bytes <= 0) {
        return -1;
    }

    InitDecodedCert(&decodedCert, derBuffer, (word32) bytes, 0);

    ret = ParseCert(&decodedCert, CERT_TYPE, NO_VERIFY, NULL);
    check_ret("ParseCert", ret);

    ret = wc_ecc_init(&eccKey);
    check_ret("wc_ecc_init", ret);

    printf("decodedCert.pubKeySize %d\n", decodedCert.pubKeySize);
    WOLFSSL_BUFFER(decodedCert.publicKey, decodedCert.pubKeySize);

    ret = wc_EccPublicKeyDecode(decodedCert.publicKey, &inOutIdx, &eccKey, decodedCert.pubKeySize);
    check_ret("wc_EccPublicKeyDecode", ret);

    printf("publickey size: %d\n", wc_ecc_size(&eccKey));

    FreeDecodedCert(&decodedCert);

    wolfCrypt_Cleanup();

    printf("Success\n");

    return 0;
}


static void check_ret(char* call, int ret)
{
    if (ret != 0) {
        printf("call: %s\n", call);
        printf("ret = %d\n", ret);
        exit(-99);
    }
    return;
}

#else

int main(void)
{
    printf("Not compiled in: Must build wolfSSL using ./confgiure --enable-opensslextra or ./configure CFLAGS=-DWOLFSSL_TEST_CERT\n");
    return 0;
}

#endif
