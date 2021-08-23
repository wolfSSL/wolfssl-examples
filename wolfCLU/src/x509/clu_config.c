/* clu_config.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
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

#include <wolfclu/clu_header_main.h>
#include <wolfclu/clu_error_codes.h>
#include <wolfclu/x509/clu_parse.h>


static int wolfCLU_setAttributes(WOLFSSL_X509* x509, WOLFSSL_CONF* conf,
            char* sect)
{

    (void)x509;
    (void)conf;
    (void)sect;
#if 0
// @TODO
//[ req_attributes ]
//challengePassword               = A challenge password
//challengePassword_min           = 4
//challengePassword_max           = 20
//unstructuredName                = An optional company name
#endif
    return 0;
}


#ifdef WOLFSSL_CERT_EXT
static WOLFSSL_X509_EXTENSION* wolfCLU_parseBasicConstraint(char* str, int crit)
{
    char* word, *end;
    char* deli = ":";
    WOLFSSL_X509_EXTENSION *ext;
    WOLFSSL_ASN1_OBJECT *obj;

    ext = wolfSSL_X509_EXTENSION_new();
    if (ext == NULL || str == NULL)
        return NULL;

    wolfSSL_X509_EXTENSION_set_critical(ext, crit);
    if (wolfSSL_X509_EXTENSION_set_object(ext,
               wolfSSL_OBJ_nid2obj(NID_basic_constraints)) != WOLFSSL_SUCCESS) {
        wolfSSL_X509_EXTENSION_free(ext);
        return NULL;
    }

    obj = wolfSSL_X509_EXTENSION_get_object(ext);
    if (obj == NULL) {
        wolfSSL_X509_EXTENSION_free(ext);
        return NULL;
    }

    for (word = strtok_r(str, deli, &end); word != NULL;
            word = strtok_r(NULL, deli, &end)) {
        if (word != NULL && strncmp(word, "CA", strlen(word)) == 0) {
            word = strtok_r(NULL, deli, &end);
            if (word != NULL && strncmp(word, "TRUE", strlen(word)) == 0) {
                obj->ca = 1;
            }
        }

        if (word != NULL && strncmp(word, "pathlen", strlen(word)) == 0) {
            word = strtok_r(NULL, deli, &end);
            if (word != NULL) {
                if (obj->pathlen != NULL)
                    wolfSSL_ASN1_INTEGER_free(obj->pathlen);
                obj->pathlen = wolfSSL_ASN1_INTEGER_new();
                wolfSSL_ASN1_INTEGER_set(obj->pathlen, atoi(word));
            }
        }
    }

    return ext;
}


static WOLFSSL_X509_EXTENSION* wolfCLU_parseSubjectKeyID(char* str, int crit,
        WOLFSSL_X509* x509)
{
    Cert cert; /* temporary to use existing subject key id api */
    WOLFSSL_ASN1_OBJECT *obj = NULL;
    WOLFSSL_X509_EXTENSION *ext = NULL;
    WOLFSSL_EVP_PKEY *pkey = NULL;
    char* word, *end;
    char* deli = ",";

    if (x509 == NULL || str == NULL)
        return NULL;

    for (word = strtok_r(str, deli, &end); word != NULL;
            word = strtok_r(NULL, deli, &end)) {

        if (strncmp(word, "hash", strlen(word)) == 0) {
            WOLFSSL_ASN1_STRING *data;
            int  keyType;
            void *key = NULL;

            XMEMSET(&cert, 0, sizeof(Cert));
            keyType = wolfSSL_X509_get_pubkey_type(x509);

            pkey = wolfSSL_X509_get_pubkey(x509);
            if (pkey == NULL) {
                printf("no public key set to hash for subject key id\n");
                return NULL;
            }

            switch (keyType) {
                case RSAk:
                    key = pkey->rsa->internal;
                    keyType = RSA_TYPE;
                    break;

                case ECDSAk:
                    key = pkey->ecc->internal;
                    keyType = ECC_TYPE;
                    break;

                default:
                    printf("key type not yet supported\n");
            }

            if (wc_SetSubjectKeyIdFromPublicKey_ex(&cert, keyType, key) < 0) {
                printf("error hashing public key\n");
            }
            else {
                data = wolfSSL_ASN1_STRING_new();
                if (data != NULL) {
                    if (wolfSSL_ASN1_STRING_set(data, cert.skid, cert.skidSz)
                            != WOLFSSL_SUCCESS) {
                        printf("error setting the skid\n");
                    }
                    else {
                        ext = wolfSSL_X509V3_EXT_i2d(NID_subject_key_identifier,
                                crit, data);
                    }
                    wolfSSL_ASN1_STRING_free(data);
                }
            }
        }
    }

    return ext;
}


static WOLFSSL_X509_EXTENSION* wolfCLU_parseKeyUsage(char* str, int crit,
        WOLFSSL_X509* x509)
{
    WOLFSSL_ASN1_STRING *data;
    WOLFSSL_X509_EXTENSION *ext = NULL;
    char* word, *end;
    char* deli = ",";
    word16 keyUseFlag = 0;

    if (x509 == NULL || str == NULL)
        return NULL;

    for (word = strtok_r(str, deli, &end); word != NULL;
            word = strtok_r(NULL, deli, &end)) {

        /* remove empty spaces at beginning of word */
        int mxSz = (int)XSTRLEN(word);
        while (word[0] == ' ' && mxSz > 0) {
            word++;
            mxSz--;
        }

        if (strncmp(word, "digitalSignature", XSTRLEN(word)) == 0) {
            keyUseFlag |= KEYUSE_DIGITAL_SIG;
        }

        if (strncmp(word, "nonRepudiation", XSTRLEN(word)) == 0 ||
                strncmp(word, "contentCommitment", XSTRLEN(word)) == 0) {
            keyUseFlag |= KEYUSE_CONTENT_COMMIT;
        }

        if (strncmp(word, "keyEncipherment", XSTRLEN(word)) == 0) {
            keyUseFlag |= KEYUSE_KEY_ENCIPHER;
        }

        if (strncmp(word, "dataEncipherment", XSTRLEN(word)) == 0) {
            keyUseFlag |= KEYUSE_DATA_ENCIPHER;
        }

        if (strncmp(word, "keyAgreement", XSTRLEN(word)) == 0) {
            keyUseFlag |= KEYUSE_KEY_AGREE;
        }

        if (strncmp(word, "keyCertSign", XSTRLEN(word)) == 0) {
            keyUseFlag |= KEYUSE_KEY_CERT_SIGN;
        }

        if (strncmp(word, "cRLSign", XSTRLEN(word)) == 0) {
            keyUseFlag |= KEYUSE_CRL_SIGN;
        }

        if (strncmp(word, "encipherOnly", XSTRLEN(word)) == 0) {
            keyUseFlag |= KEYUSE_ENCIPHER_ONLY;
        }

        if (strncmp(word, "decipherOnly", XSTRLEN(word)) == 0) {
            keyUseFlag |= KEYUSE_DECIPHER_ONLY;
        }
    }

    data = wolfSSL_ASN1_STRING_new();
    if (data != NULL) {
        if (wolfSSL_ASN1_STRING_set(data, (byte*)&keyUseFlag, sizeof(word16))
                        != WOLFSSL_SUCCESS) {
            printf("error setting the key use\n");
        }
        else {
            ext = wolfSSL_X509V3_EXT_i2d(NID_key_usage, crit, data);
        }
        wolfSSL_ASN1_STRING_free(data);
    }
    return ext;
}


static int wolfCLU_parseExtension(WOLFSSL_X509* x509, char* str, int nid,
        int* idx)
{
    WOLFSSL_X509_EXTENSION *ext = NULL;
    int   ret, crit = 0;

    if (strstr("critical", str) != NULL) {
        crit = 1;
    }
    switch (nid) {
        case NID_basic_constraints:
            ext = wolfCLU_parseBasicConstraint(str, crit);
            break;
        case NID_subject_key_identifier:
            ext = wolfCLU_parseSubjectKeyID(str, crit, x509);
            break;
        case NID_authority_key_identifier:
            /* @TODO */
            break;
        case NID_key_usage:
            ext = wolfCLU_parseKeyUsage(str, crit, x509);
            break;

        default:
            printf("unknown / supported nid %d value for extension\n",
                    nid);
    }

    if (ext != NULL) {
        ret = wolfSSL_X509_add_ext(x509, ext, -1);
        if (ret != WOLFSSL_SUCCESS) {
            printf("error %d adding extesion\n", ret);
        }
        *idx = *idx + 1;
    }
    return WOLFSSL_SUCCESS;
}


/* return 0 on success, searches for IP's and DNS's */
static int wolfCLU_setAltNames(WOLFSSL_X509* x509, WOLFSSL_CONF* conf,
            char* sect)
{
    WOLFSSL_X509_NAME *name;
    WOLFSSL_X509_NAME_ENTRY *entry;
    char *current;
    int  idx = 1;
    int  i;

    if (sect == NULL) {
        return 0; /* none set */
    }

    /* get DNS names */
    i = 1;
    do {
        char name[7];
        snprintf(name, 6, "DNS.%d", i);
        current = wolfSSL_NCONF_get_string(conf, sect, name);
        if (current != NULL) {
            if (wolfSSL_X509_add_altname(x509, current, ASN_DNS_TYPE)
                    != WOLFSSL_SUCCESS) {
                printf("error adding alt name %s\n", current);
            }
        }
        i++;
    } while(current != NULL);

    /* get IP names */
    i = 1;
    do {
        char name[7];
        snprintf(name, 6, "IP.%d", i);
        current = wolfSSL_NCONF_get_string(conf, sect, name);
        if (current != NULL) {
            /* convert to hex value */
            WOLFSSL_ASN1_STRING *str = wolfSSL_a2i_IPADDRESS(current);

            if (str != NULL) {
                unsigned char *data;
                int dataSz;

                data   = wolfSSL_ASN1_STRING_data(str);
                dataSz = wolfSSL_ASN1_STRING_length(str);

                if (wolfSSL_X509_add_altname_ex(x509, data, dataSz,
                            ASN_IP_TYPE) != WOLFSSL_SUCCESS) {
                    printf("error adding ip alt name %s\n", data);
                }
                wolfSSL_ASN1_STRING_free(str);
            }
        }
        i++;
    } while(current != NULL);

    return 0;
}


/* return 0 on success */
static int wolfCLU_setExtensions(WOLFSSL_X509* x509, WOLFSSL_CONF* conf,
            char* sect)
{
    WOLFSSL_X509_NAME *name;
    WOLFSSL_X509_NAME_ENTRY *entry;
    char *current;
    int  idx = 1;

    printf("extensions section : %s\n", sect);

    if (sect == NULL) {
        return 0; /* none set */
    }

    current = wolfSSL_NCONF_get_string(conf, sect, "basicConstraints");
    if (current != NULL) {
        wolfCLU_parseExtension(x509, current, NID_basic_constraints, &idx);
    }

    current = wolfSSL_NCONF_get_string(conf, sect, "subjectKeyIdentifier");
    if (current != NULL) {
        wolfCLU_parseExtension(x509, current, NID_subject_key_identifier, &idx);
    }

    current = wolfSSL_NCONF_get_string(conf, sect, "authorityKeyIdentifier");
    if (current != NULL) {
        wolfCLU_parseExtension(x509, current, NID_authority_key_identifier,
                &idx);
    }

    current = wolfSSL_NCONF_get_string(conf, sect, "keyUsage");
    if (current != NULL) {
        wolfCLU_parseExtension(x509, current, NID_key_usage, &idx);
    }


    current = wolfSSL_NCONF_get_string(conf, sect, "subjectAltName");
    printf("subject alt name location = %s\n", current);
    if (current != NULL && current[0] == '@') {
        current = current+1;
        wolfCLU_setAltNames(x509, conf, current);
    }
    return 0;
}
#else
static int wolfCLU_setExtensions(WOLFSSL_X509* x509, WOLFSSL_CONF* conf,
            char* sect)
{
    (void)x509;
    (void)conf;
    (void)sect;

    printf("wolfSSL not compiled with cert extensions\n");
    return -1;
}
#endif /* WOLFSSL_CERT_EXT */


/* return 0 on success */
static int wolfCLU_X509addEntry(WOLFSSL_X509_NAME* name, WOLFSSL_CONF* conf,
        int nid, int type, const char* sect, const char* str)
{
    const unsigned char *current;
    WOLFSSL_X509_NAME_ENTRY *entry;

    current = (const unsigned char*)wolfSSL_NCONF_get_string(conf, sect, str);
    if (current != NULL) {
        entry = wolfSSL_X509_NAME_ENTRY_create_by_NID(NULL, nid,
                type, current, strlen((const char*)current));
        wolfSSL_X509_NAME_add_entry(name, entry, -1, 0);
    }
    return 0;
}


/* extracts the distinguished names from the conf file and puts them into
 * the x509
 * returns 0 on success */
static int wolfCLU_setDisNames(WOLFSSL_X509* x509, WOLFSSL_CONF* conf,
        char* sect)
{
    WOLFSSL_X509_NAME *name;
    long countryName_min = 0;
    long countryName_max = 0;

    if (sect == NULL) {
        return 0; /* none set */
    }

    name = wolfSSL_X509_NAME_new();
    if (name == NULL) {
        return -1;
    }

    wolfCLU_X509addEntry(name, conf, NID_countryName, CTC_PRINTABLE, sect,
            "countryName_default");
    wolfCLU_X509addEntry(name, conf, NID_countryName, CTC_PRINTABLE, sect,
            "countryName");

    wolfSSL_NCONF_get_number(conf, sect, "countryName_min", &countryName_min);
    wolfSSL_NCONF_get_number(conf, sect, "countryName_max", &countryName_max);

    wolfCLU_X509addEntry(name, conf, NID_stateOrProvinceName, CTC_UTF8, sect,
            "stateOrProvinceName_default");
    wolfCLU_X509addEntry(name, conf, NID_stateOrProvinceName, CTC_UTF8, sect,
            "stateOrProvinceName");
    wolfCLU_X509addEntry(name, conf, NID_localityName, CTC_UTF8, sect,
            "localityName_default");
    wolfCLU_X509addEntry(name, conf, NID_localityName, CTC_UTF8, sect,
            "localityName");
    wolfCLU_X509addEntry(name, conf, NID_organizationName, CTC_UTF8, sect,
            "0.organizationName_default");
    wolfCLU_X509addEntry(name, conf, NID_organizationName, CTC_UTF8, sect,
            "0.organizationName");
    wolfCLU_X509addEntry(name, conf, NID_organizationalUnitName, CTC_UTF8, sect,
            "organizationalUnitName_default");
    wolfCLU_X509addEntry(name, conf, NID_organizationalUnitName, CTC_UTF8, sect,
            "organizationalUnitName");
    wolfCLU_X509addEntry(name, conf, NID_commonName, CTC_UTF8, sect,
            "commonName_default");
    wolfCLU_X509addEntry(name, conf, NID_commonName, CTC_UTF8, sect,
            "commonName");
    wolfCLU_X509addEntry(name, conf, NID_emailAddress, CTC_UTF8, sect,
            "emailAddress_default");
    wolfCLU_X509addEntry(name, conf, NID_emailAddress, CTC_UTF8, sect,
            "emailAddress");

    wolfSSL_X509_REQ_set_subject_name(x509, name);
    return 0;
}

/* Make a new WOLFSSL_X509 based off of the config file read */
int wolfCLU_readConfig(WOLFSSL_X509* x509, char* config, char* sect)
{
    WOLFSSL_CONF *conf = NULL;
    long line = 0;
    long defaultBits = 0;
    char *defaultKey = NULL;

    conf = wolfSSL_NCONF_new(NULL);
    wolfSSL_NCONF_load(conf, config, &line);

    wolfSSL_NCONF_get_number(conf, sect, "default_bits", &defaultBits);
    defaultKey = wolfSSL_NCONF_get_string(conf, sect, "default_keyfile");

    wolfCLU_setAttributes(x509, conf,
            wolfSSL_NCONF_get_string(conf, sect, "attributes"));
    wolfCLU_setExtensions(x509, conf,
            wolfSSL_NCONF_get_string(conf, sect, "x509_extensions"));
    wolfCLU_setDisNames(x509, conf,
            wolfSSL_NCONF_get_string(conf, sect, "distinguished_name"));

    (void)defaultKey;
    wolfSSL_NCONF_free(conf);
    return 0;
}

