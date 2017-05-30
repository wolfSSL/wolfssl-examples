

#include <stdio.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/ssl.h>
#include <wolfssl/test.h>


#define HEAP_HINT NULL
#define FOURK_SZ 4096


typedef struct CertData CertData;

/* Data for certificate generation. */
struct CertData {
    ed25519_key  key;
    ed25519_key* signKey;
    Cert         cert;
    byte*        der;
    int          derSz;
    CertName     issuer;
    CertName     subject;
    int          isSelfSigned;
    int          isCA;
    char         certName[80];
    char         keyUsage[80];
    CertData*    next;
};

static const char caKeyUsage[]   =
                          "digitalSignature,nonRepudiation,keyCertSign,cRLSign";
static const char leafKeyUsage[] = "digitalSignature,nonRepudiation";

static const CertName defaultName = {
    "US",               CTC_PRINTABLE,
    "Montana",          CTC_UTF8,
    "Bozeman",          CTC_UTF8,
    "",                 CTC_UTF8,
    "wolfSSL",          CTC_UTF8,
    "ED25519",          CTC_UTF8,
    "www.wolfssl.com",  CTC_UTF8,
    "info@wolfssl.com"
};


int myoptind = 0;
char* myoptarg = NULL;

/*---------------------------------------------------------------------------*/

/* Write data to a file.
 *
 * filename  Name of the file to create.
 * buffer    Data to write.
 * bufferSz  Number of bytes of data to write.
 * returns the length of data written, otherwise -1 on failure.
 */
static int write_file(const char* fileName, byte* buffer, int bufferSz)
{
    FILE* file;
    int   sz;

    file = fopen(fileName, "wb");
    if (!file) {
        printf("\nFailed to open file: %s\n", fileName);
        return -1;
    }

    sz = (int)fwrite(buffer, 1, bufferSz, file);
    fclose(file);
    printf(" %d bytes\n", sz);

    return sz;
}

/* Write DER data to a file as PEM.
 *
 * filename  Name of the file to create.
 * buffer    DER data to write.
 * bufferSz  Number of bytes of DER data to write.
 * type      Type of data: certificate or key.
 * returns the length of data written, otherwise -1 on failure.
 */
static int write_file_as_pem(const char* fileName, byte* buffer, int bufferSz,
                             int type)
{
    byte* pemBuf;
    int   pemBufSz, sz = -1;

    pemBuf = (byte*) XMALLOC(FOURK_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (pemBuf == NULL)
        return -1;

    XMEMSET(pemBuf, 0, FOURK_SZ);

    pemBufSz = wc_DerToPem(buffer, bufferSz, pemBuf, FOURK_SZ, type);
    if (pemBufSz >= 0)
        sz = write_file(fileName, pemBuf, pemBufSz);

    XFREE(pemBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    return sz;
}

/*---------------------------------------------------------------------------*/

/* Make an Ed25519 key.
 *
 * key  Ed25519 key object.
 * rng  Random number generator.
 * returns 0 on success, otherwise negative on failure.
 */
static int Key_Make(ed25519_key* key, WC_RNG* rng)
{
    int ret;

    printf("Generating a new ed25519 key ...");

    ret = wc_ed25519_make_key(rng, ED25519_KEY_SIZE, key);
    if (ret != 0)
        return ret;

    printf(" done\n");

    return 0;
}

/* Decode the DER encoded Ed25519 private key.
 *
 * buffer    DER data to decode.
 * bufferSz  Number of bytes of DER data to decode.
 * retuens 0 on success, otherwise negative on failure.
 */
static int Key_Decode(byte* derBuf, int derBufSz)
{
    ed25519_key newKey;
    int         ret;
    word32      idx;

    ret = wc_ed25519_init(&newKey);
    if (ret != 0)
        return ret;

    idx = 0;
    ret = wc_Ed25519PrivateKeyDecode(derBuf, &idx, &newKey, derBufSz);
    wc_ed25519_free(&newKey);

    return ret;
}

/* Write the Ed25519 private key to file.
 * A DER and PEM encoded version of the key is written out.
 *
 * key   Ed25519 key object.
 * name  Base name of the files.
 * returns 0 on success, otherwise negative on failure.
 */
static int Key_Write(ed25519_key* key, const char* name)
{
    int         ret = -1;
    byte*       derBuf;
    int         derBufSz;
    char        fileName[80];

    derBuf = (byte*)XMALLOC(FOURK_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (derBuf == NULL)
        goto end;
    XMEMSET(derBuf, 0, FOURK_SZ);

    XMEMCPY(fileName, name, XSTRLEN(name) + 1);
    XSTRNCAT(fileName, "-key.der", 9);
    printf("Key file DER: \"%s\" ...", fileName);
    derBufSz = ret = wc_Ed25519KeyToDer(key, derBuf, FOURK_SZ);
    if (ret < 0) {
        printf("\nFailed to encode private key to DER\n");
        goto end;
    }

    /* Check private and public key DER can be decoded. */
    ret = Key_Decode(derBuf, derBufSz);
    if (ret < 0) {
        printf("\nFailed to decode private key with public key\n");
        goto end;
    }

    ret = write_file(fileName, derBuf, derBufSz);
    if (ret < 0)
        goto end;

    XMEMCPY(fileName, name, XSTRLEN(name) + 1);
    XSTRNCAT(fileName, "-key.pem", 9);
    printf("Key file PEM: \"%s\" ...", fileName);
    ret = write_file_as_pem(fileName, derBuf, derBufSz, EDDSA_PRIVATEKEY_TYPE);
    if (ret < 0)
        goto end;

    /* Check private key only DER can be encoded and decoded. */
    derBufSz = ret = wc_Ed25519PrivateKeyToDer(key, derBuf, FOURK_SZ);
    if (ret < 0) {
        printf("\nFailed to encode private key only to DER\n");
        goto end;
    }

    ret = Key_Decode(derBuf, derBufSz);
    if (ret < 0) {
        printf("\nFailed to decode private key only\n");
        goto end;
    }

end:
    if (derBuf != NULL)
        XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    return ret;
}

/* Read the Ed25519 private key from file.
 *
 * key    Ed25519 key object.
 * name   Name of file containing Ed25519 private key.
 * returns 0 on success, otherwise negative on failure.
 */
static int Key_Read(ed25519_key* caKey, char* name)
{
    byte*  buffer;
    FILE*  file;
    int    ret;
    int    sz;
    word32 idx;

    buffer = XMALLOC(FOURK_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (buffer == NULL) {
        printf("\nFailed to allocate memory - Key_Read\n");
        return -1;
    }

    file = fopen(name, "rb");
    if (file == NULL) {
        XFREE(buffer, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        printf("\nFailed to open file: %s\n", name);
        return -1;
    }

    sz = (int)fread(buffer, 1, FOURK_SZ, file);
    fclose(file);
    printf(" %d bytes\n", sz);

    idx = 0;
    ret = wc_Ed25519PrivateKeyDecode(buffer, &idx, caKey, sz);
    if (ret < 0)
        printf("\nFailed to decode private key\n");

    XFREE(buffer, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    return ret;
}

/*---------------------------------------------------------------------------*/

/* Make a certifcate and sign it.
 *
 * cert      Certificate object.
 * signKey   The private key to sign with.
 * certKey   The public key to put into the certificate.
 * rng       Random number generator.
 * buffer    Buffer with the encoded data.
 * bufferSz  Size of the encoded data.
 * returns 0 on success, otherwise negative on failure.
 */
static int Cert_Make(Cert* cert, ed25519_key* signKey, ed25519_key* certKey,
                     WC_RNG* rng, byte** buffer, int* bufferSz)
{
    int   ret = -1;
    byte* derBuf;

    printf("Making cert ...");
    derBuf = (byte*) XMALLOC(FOURK_SZ, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    if (derBuf == NULL)
        goto end;
    XMEMSET(derBuf, 0, FOURK_SZ);

    cert->sigType = CTC_ED25519;
    ret = wc_MakeCert_ex(cert, derBuf, FOURK_SZ, ED25519_TYPE, certKey, rng);
    if (ret < 0)
        goto end;
    printf(" MakeCert: %d ...", ret);

    ret = wc_SignCert_ex(cert->bodySz, cert->sigType, derBuf, FOURK_SZ,
                                                    ED25519_TYPE, signKey, rng);
    if (ret < 0)
        goto end;
    printf(" SignCert: %d ... done\n", ret);

    *buffer = derBuf;
    *bufferSz = ret;
    derBuf = NULL;

end:
    if (derBuf != NULL)
        XFREE(derBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    return ret;
}

/*---------------------------------------------------------------------------*/

/* Verify a chain of certificates.
 * The first certificate should be the self-signed root.
 *
 * chain  Chain of certificate data.
 * @returns 0 on success, otherwise negative on failure.
 */
static int Chain_Verify(CertData* chain)
{
    int ret;
    int i = 1;
    WOLFSSL_CERT_MANAGER* cm;
    DecodedCert cert;

    cm = wolfSSL_CertManagerNew_ex(NULL);
    if (cm == NULL) {
        printf("\nFailed to create certificate manager\n");
        return -1;
    }

    if (chain->isSelfSigned) {
        ret = wolfSSL_CertManagerLoadCABuffer(cm, chain->der, chain->derSz,
                                                             SSL_FILETYPE_ASN1);
        if (ret != SSL_SUCCESS) {
            printf("\nFailed to load Root: %d\n", ret);
            ret = -1;
            goto end;
        }
    }

    while (chain != NULL) {
        printf(" %d", i);
        InitDecodedCert(&cert, chain->der, chain->derSz, NULL);
        ret = ParseCert(&cert, CERT_TYPE, VERIFY, cm);
        FreeDecodedCert(&cert);
        if (ret != 0) {
            printf("\nFailed to parse cert: %d\n", ret);
            goto end;
        }

        if (chain->isCA) {
            ret = wolfSSL_CertManagerLoadCABuffer(cm, chain->der, chain->derSz,
                                                             SSL_FILETYPE_ASN1);
            if (ret != SSL_SUCCESS) {
                printf("\nFailed to load CA: %d\n", ret);
                ret = -1;
                goto end;
            }
            ret = 0;
        }

        chain = chain->next;
        i++;
    }

    printf(" done\n");
end:
    wolfSSL_CertManagerFree(cm);
    return ret;
}

/*---------------------------------------------------------------------------*/

/* Initialize the certificate data object.
 *
 * certData  Certificate data object.
 */
static void CertData_Init(CertData* certData)
{
    XMEMSET(certData, 0, sizeof(*certData));

    wc_ed25519_init(&certData->key);
    wc_InitCert(&certData->cert);

    certData->der = NULL;
    certData->derSz = 0;

    XMEMCPY(&certData->issuer, &defaultName, sizeof(CertName));
    XMEMCPY(&certData->subject, &defaultName, sizeof(CertName));
    XMEMCPY(certData->issuer.sur, "CA", 3);
    XMEMCPY(certData->subject.sur, "LEAF", 5);

    certData->isSelfSigned = 0;
    certData->isCA = 0;
    XMEMCPY(certData->certName, "cert", 5);
    XMEMCPY(certData->keyUsage, leafKeyUsage, sizeof(leafKeyUsage));
    wc_InitCert(&certData->cert);
}

/* Free the dynamic data associated with the certificate data object.
 *
 * certData  Certificate data object.
 */
static void CertData_Free(CertData* certData)
{
    if (certData->der != NULL)
        XFREE(certData->der, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
    wc_ed25519_free(&certData->key);
}

/*---------------------------------------------------------------------------*/

/* Buffer holding the data read from the configuration file. */
static char readBuf[FOURK_SZ] = { '\0', };
/* Index to put read data at. */
static int readIdx = 0;

/* Find a character in a string.
 *
 * str  String to search.
 * ch   The character to find.
 * returns NULL on failure, otherwise a pointer into string of character.
 */
static char* xstrchr(char* str, int ch)
{
    int i;

    for (i = 0; str[i] != '\0'; i++)
        if (str[i] == ch)
            return str + i;

    return NULL;
}

/* Read a line from the configuration file.
 * Excludes the '\n' character.
 *
 * fp   File pointer to configuration file.
 * buf  The buffer to put the line into.
 * len  The length of the buffer.
 * returns -1 on failure, otherwise the length of the line data.
 */
static int Config_ReadLine(FILE* fp, char* buf, int len)
{
    char* newLine;
    int lineLen, inLen;

    newLine = xstrchr(readBuf, '\n');
    if (newLine == NULL) {
        inLen = fread(&readBuf[readIdx], 1, sizeof(readBuf) - readIdx - 1, fp);
        readIdx += inLen;
        readBuf[readIdx] = '\0';
        newLine = xstrchr(readBuf, '\n');
        if (newLine == NULL)
            return -1;
    }

    lineLen = newLine - readBuf;
    if (len - 1 < lineLen)
        return -1;
    XMEMCPY(buf, readBuf, lineLen);
    buf[lineLen++] = '\0';

    readIdx -= lineLen;
    XMEMMOVE(readBuf, &readBuf[lineLen], readIdx + 1);

    return lineLen;
}

/* Unread the line back into the read buffer.
 *
 * line  The line of data.
 * returns -1 when the line is too long to fit in read buffer, 0 on success.
 */
static int Config_Unread(char* line)
{
    int len = XSTRLEN(line) + 1;
    if (len + readIdx > sizeof(readBuf) - 1)
        return -1;
    XMEMMOVE(&readBuf[len], readBuf, readIdx);
    readIdx += len;
    XMEMCPY(readBuf, line, len);
    readBuf[len - 1] = '\n';

    return 0;
}

/* Split the line into variable and value.
 * Skips whitespace.
 *
 * line  Line of data.
 * var   Buffer to hold variable name.
 * val   Buffer to hold value.
 * returns -1 on bad format, 0 on success.
 */
static int Config_SplitVar(char* line, char* var, char* val)
{
    int varO = 0;
    int valO = 0;
    int o = 0;

    while (line[o] != ' ' && line[o] != '\t' && line[o] != '\0')
        var[varO++] = line[o++];
    if (line[o] == '\0')
        return -1;
    var[varO] = '\0';

    while (line[o] == ' ' || line[o] == '\t' || line[o] == '\0') {
        if (line[o] == '\0')
            return -1;
        o++;
    }
    if (line[o++] != '=')
        return -1;
    while (line[o] == ' ' || line[o] == '\t' || line[o] == '\0') {
        if (line[o] == '\0')
            return -1;
        o++;
    }

    while (line[o] != ' ' && line[o] != '\t' && line[o] != '\0')
        val[valO++] = line[o++];
    val[valO] = '\0';

    while (line[o] == ' ' || line[o] == '\t')
        o++;
    if (line[o] != '\0')
        return -1;

    return 0;
}

/* Compare two string.
 *
 * a  A NUL terminated string.
 * b  A NUL terminated string.
 * returns an integer less than, equal to, or greater than zero if s1 (or the
 * first n bytes thereof) is found, respectively, to be less than, to match, or
 * be greater than s2.
 */
static int xstrcmp(char* s1, char* s2)
{
    int s1Sz = XSTRLEN(s1);
    int s2Sz = XSTRLEN(s2);

    if (s1Sz != s2Sz)
        return s1Sz - s2Sz;
    return XMEMCMP(s1, s2, s1Sz);
}

/* Read the configuration for a certificate from the file.
 *
 * fp           File pointer.
 * retCertData  The certificate data created.
 * returns -1 on failure, 0 on success.
 */
static int Config_ReadCert(FILE* fp, CertData** retCertData)
{
    char      line[80], var[80], val[80];
    int       ret, idx;
    CertName* name = NULL;
    int       seenCert = 0;
    int       keyUsageSet = 0;
    CertData* certData = NULL;

    while ((ret = Config_ReadLine(fp, line, sizeof(line))) >= 0) {
        idx = 0;
        while (line[idx] == ' ' || line[idx] == '\t')
            idx++;
        if (line[idx] == '\0')
            continue;

        if (XMEMCMP(line + idx, "[Cert]", 7) == 0) {
            if (seenCert) {
                if (Config_Unread(line + idx) < 0)
                    return -1;
                return 1;
            }
            printf("\nNew cert ...");
            certData = (CertData*)XMALLOC(sizeof(*certData), NULL,
                                                       DYNAMIC_TYPE_TMP_BUFFER);
            if (certData == NULL)
                return -1;
            CertData_Init(certData);
            *retCertData = certData;
            seenCert = 1;
            continue;
        }
        if (!seenCert)
            return -1;

        if (XMEMCMP(line + idx, "[Issuer]", 9) == 0) {
            printf(", Issuer:");
            name = &certData->issuer;
            continue;
        }
        if (XMEMCMP(line + idx, "[Subject]", 10) == 0) {
            printf(", Subject:");
            name = &certData->subject;
            continue;
        }

        if (Config_SplitVar(line + idx, var, val) < 0) {
            printf("\nBad config line:\n  %s\n", line);
            CertData_Free(certData);
            XFREE(certData, NULL, DYNAMIC_TYPE_TMP_BUFFER);
            return -1;
        }

        if (xstrcmp(var, "country") == 0) {
            printf(" C");
            XMEMCPY(name->country, val, XSTRLEN(val) + 1);
        }
        else if (xstrcmp(var, "state") == 0) {
            printf(" S");
            XMEMCPY(name->state, val, XSTRLEN(val) + 1);
        }
        else if (xstrcmp(var, "locality") == 0) {
            printf(" L");
            XMEMCPY(name->locality, val, XSTRLEN(val) + 1);
        }
        else if (xstrcmp(var, "surname") == 0) {
            printf(" SN");
            XMEMCPY(name->sur, val, XSTRLEN(val) + 1);
        }
        else if (xstrcmp(var, "org") == 0) {
            printf(" O");
            XMEMCPY(name->org, val, XSTRLEN(val) + 1);
        }
        else if (xstrcmp(var, "unit") == 0) {
            printf(" OU");
            XMEMCPY(name->unit, val, XSTRLEN(val) + 1);
        }
        else if (xstrcmp(var, "commonName") == 0) {
            printf(" CN");
            XMEMCPY(name->commonName, val, XSTRLEN(val) + 1);
        }
        else if (xstrcmp(var, "email") == 0) {
            printf(" E");
            XMEMCPY(name->email, val, XSTRLEN(val) + 1);
        }

        else if (xstrcmp(var, "name") == 0) {
            printf(" %s", val);
            XMEMCPY(certData->certName, val, XSTRLEN(val) + 1);
        }
        else if (xstrcmp(var, "ca") == 0 && strcmp(val, "yes") == 0) {
            printf(" CA");
            certData->isCA = 1;
            if (!keyUsageSet)
                XMEMCPY(certData->keyUsage, caKeyUsage, sizeof(caKeyUsage));
        }
        else if (xstrcmp(var, "selfSigned") == 0 && strcmp(val, "yes") == 0) {
            printf(" SS");
            certData->isSelfSigned = 1;
        }
        else if (xstrcmp(var, "keyUsage") == 0) {
            printf(" KU");
            XMEMCPY(certData->keyUsage, val, XSTRLEN(val) + 1);
            keyUsageSet = 1;
        }

        else {
            printf("\nUnrecognized var:\n  %s\n", line);
            return -1;
        }
    }

    return 0;
}

/* Read the configuration from the file.
 * A linked list of certificate data objects is created.
 *
 * name  Name of configuration file.
 * res   The certificate data result.
 */
static int Config_Read(char* name, CertData** res)
{
    int       ret;
    FILE*     file;
    CertData* nextCert;
    CertData* curr;
    CertData* chain = NULL;

    file = fopen(name, "rb");
    if (!file) {
        printf("\nFailed to open file: %s\n", name);
        return -1;
    }

    while ((ret = Config_ReadCert(file, &nextCert)) >= 0) {
        if (chain == NULL)
            chain = nextCert;
        else
            curr->next = nextCert;
        curr = nextCert;

        if (ret == 0)
            break;
    }
    printf ("\n");

    fclose(file);

    *res = chain;

    return ret;
}

/*---------------------------------------------------------------------------*/

/* Display the usage message for this program. */
static void Usage(void)
{
    printf("ed25519_cert "    LIBWOLFSSL_VERSION_STRING
           "\n");
    printf("-?          Help, print this usage\n");
    printf("-c <file>   Configuration file\n");
    printf("-k <file>   Key file. Used for signing certificate\n");
    printf("-s          Self-signed certificate\n");
};

/* Create Ed25519 certificates.
 *
 * argc  The number of command line arguments.
 * argv  The array of command line arguments.
 */
int main(int argc, char* argv[])
{
    int          ret = 0;
    int          i = 1;
    CertData*    chain = NULL;
    CertData*    curr;
    Cert*        cert;
    WC_RNG       rng;
    int          ch;
    char*        config = NULL;
    char*        keyFileName = NULL;
    ed25519_key  caKey;
    ed25519_key* key = NULL;
    ed25519_key* prevKey = NULL;
    int          selfSigned = 0;
    char         fileName[80];

    while ((ch = mygetopt(argc, argv, "?c:k:s")) != -1) {
        switch (ch) {
            case '?':
                Usage();
                return 0;

            case 'c':
                config = myoptarg;
                break;

            case 'k':
                keyFileName = myoptarg;
                break;

            case 's':
                selfSigned = 1;
                break;

            default:
                Usage();
                return -1;
        }
    }

    printf("Initializing the RNG\n");
    ret = wc_InitRng(&rng);
    if (ret != 0)
        goto fail;

    if (config == NULL && !selfSigned && keyFileName == NULL) {
        printf("Need a key to sign with\n");
        ret = -1;
        goto fail;
    }

    ret = wc_ed25519_init(&caKey);
    if (ret != 0)
        goto fail;
    key = &caKey;

    if (config != NULL) {
        ret = Config_Read(config, &chain);
        if (ret < 0)
            goto fail;
    }
    else {
        chain = XMALLOC(sizeof(CertData), NULL, DYNAMIC_TYPE_TMP_BUFFER);
        if (chain == NULL) {
            printf("Failed to allocate memory for chain\n");
            goto fail;
        }
        CertData_Init(chain);

        if (selfSigned) {
            chain->isSelfSigned = 1;
            XMEMCPY(chain->issuer.sur, chain->subject.sur,
                                                     sizeof(chain->issuer.sur));
        }
    }

    if (keyFileName != NULL) {
        ret = Key_Read(&caKey, keyFileName);
        if (ret < 0)
            goto fail;
        prevKey = &caKey;
    }


    /* Construct certificates */
    curr = chain;
    while (curr != NULL) {
        printf("\nCreating certificate: %s (%d)\n", curr->certName, i);

        ret = Key_Make(&curr->key, &rng);
        if (ret != 0)
            goto fail;
        Key_Write(&curr->key, curr->certName);

        cert = &curr->cert;
        prevKey = curr->isSelfSigned ? &curr->key : prevKey;

        cert->daysValid = 365 * 2;
        cert->selfSigned = curr->isSelfSigned;
        XMEMCPY(&cert->issuer, &curr->issuer, sizeof(CertName));
        XMEMCPY(&cert->subject, &curr->subject, sizeof(CertName));
        cert->isCA = curr->isCA;
        ret = wc_SetKeyUsage(cert, curr->keyUsage);
        if (ret < 0)
            goto fail;
        ret = wc_SetSubjectKeyIdFromPublicKey_ex(cert, ED25519_TYPE,
                                                                    &curr->key);
        if (ret < 0)
            goto fail;
        ret = wc_SetAuthKeyIdFromPublicKey_ex(cert, ED25519_TYPE, prevKey);
        if (ret < 0)
            goto fail;

        ret = Cert_Make(cert, prevKey, &curr->key, &rng, &curr->der,
                                                                  &curr->derSz);
        if (ret < 0)
            goto fail;

        XMEMCPY(fileName, curr->certName, XSTRLEN(curr->certName) + 1);
        XSTRNCAT(fileName, ".der", 9);
        printf("Certificate file DER: \"%s\" ...", fileName);
        if (write_file(fileName, curr->der, curr->derSz) < 0)
            goto fail;

        XMEMCPY(fileName, curr->certName, XSTRLEN(curr->certName) + 1);
        XSTRNCAT(fileName, ".pem", 9);
        printf("Certificate file DER: \"%s\" ...", fileName);
        if (write_file_as_pem(fileName, curr->der, curr->derSz,
                                                               CERT_TYPE) < 0) {
            goto fail;
        }

        prevKey = &curr->key;
        curr = curr->next;
        i++;
    }


    /* Verify a chain */
    if (chain != NULL && chain->isSelfSigned) {
        printf("\nVerify chain ...");
        ret = Chain_Verify(chain);
        if (ret < 0)
            goto fail;
    }


    /* Cleanup */
    if (0) {
fail:
        printf("\nFailure code was %d\n", ret);
        ret = -1;
    }
    else {
        printf("\nTests passed\n");
        ret = 0;
    }

    while (chain != NULL) {
        curr = chain->next;
        CertData_Free(chain);
        XFREE(chain, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        chain = curr;
    }
    wc_ed25519_free(key);
    wc_FreeRng(&rng);

    return ret;
}
