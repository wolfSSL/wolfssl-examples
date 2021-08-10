/* clu_funcs.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as wolfSSL)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,USA
 */

#include <wolfclu/clu_header_main.h>
#include <wolfclu/clu_optargs.h>
#include <wolfclu/version.h>
#include <wolfclu/x509/clu_cert.h>        /* for PEM_FORM and DER_FORM */
#include <wolfclu/sign-verify/clu_sign.h> /* for RSA_SIG_VER, ECC_SIG_VER,
                                             ED25519_SIG_VER */

#define SALT_SIZE       8
#define DES3_BLOCK_SIZE 24

/*end type casting */

int     loop       =   1;       /* benchmarking loop */
int     i          =   0;       /* loop variable */

/*
 * generic help function
 */
 void wolfCLU_help()
 {  printf("\n");
    printf("-help           Help, print out this help menu\n");
    printf("\n");
    printf("Only set one of the following.\n\n");
    printf("-encrypt        Encrypt a file or some user input\n");
    printf("-decrypt        Decrypt an encrypted file\n");
    printf("-hash           Hash a file or input\n");
    printf("-bench          Benchmark one of the algorithms\n");
    printf("-x509           X509 certificate processing\n");
    printf("-req            Request for certificate generation\n");
    printf("-rsa            Rsa signing and signature verification\n");
    printf("-ecc            Ecc signing and signature verification\n");
    printf("-ed25519        Ed25519 signing and signature verification\n");
    printf("\n");
    /*optional flags*/
    printf("Optional Flags.\n\n");
    printf("-in             input file to manage\n");
    printf("-out            file to output as a result of option\n");
    printf("-pwd            user custom password\n");
    printf("-iv             user custom IV (hex input only)\n");
    printf("-key            user custom key(hex input only)\n");
    printf("-verify         when using -iv and -key this will print result of\n"
           "                encryption for user verification.\n"
           "                This flag takes no arguments.\n");
    printf("-time           used by Benchmark, set time in seconds to run.\n");
    printf("-verbose        display a more verbose help menu\n");
    printf("-inform         input format of the certificate file [PEM/DER]\n");
    printf("-outform        format to output [PEM/DER]\n");
    printf("-output         used with -genkey option to specify which keys to\n"
           "                output [PUB/PRIV/KEYPAIR]\n");

    printf("\nFor encryption: wolfssl -encrypt -help\n");
    printf("For decryption:   wolfssl -decrypt -help\n");
    printf("For hashing:      wolfssl -hash -help\n");
    printf("For benchmarking: wolfssl -bench -help\n");
    printf("For x509:         wolfssl -x509 -help\n");
    printf("For key creation: wolfssl -genkey -help\n");
    printf("For certificate creation: wolfssl -req -help\n");
    printf("For RSA sign/ver: wolfssl -rsa -help\n");
    printf("For ECC sign/ver: wolfssl -ecc -help\n");
    printf("For ED25519 sign/ver: wolfssl -ed25519 -help\n");
 }

/*
 * verbose help function
 */
void wolfCLU_verboseHelp()
{
    printf("\nwolfssl Command Line Utility version %3.1f\n\n", VERSION);

    /* hash options */
    const char* algsenc[] = {        /* list of acceptable algorithms */
    "Algorithms:"
#ifndef NO_MD5
        ,"md5"
#endif
#ifndef NO_SHA
        ,"sha"
#endif
#ifndef NO_SHA256
        ,"sha256"
#endif
#ifdef WOLFSSL_SHA384
        ,"sha384"
#endif
#ifdef WOLFSSL_SHA512
        ,"sha512"
#endif
#ifdef HAVE_BLAKE2
        ,"blake2b"
#endif
#ifndef NO_CODING
    #ifdef WOLFSSL_BASE64_ENCODE
        ,"base64enc"
    #endif
        ,"base64dec"
#endif
    };

    /* benchmark options */
    const char* algsother[] = {      /* list of acceptable algorithms */
        "ALGS: "
#ifndef NO_AES
        , "aes-cbc"
#endif
#ifdef WOLFSSL_AES_COUNTER
        , "aes-ctr"
#endif
#ifndef NO_DES3
        , "3des"
#endif
#ifdef HAVE_CAMELLIA
        , "camellia"
#endif
#ifndef NO_MD5
        , "md5"
#endif
#ifndef NO_SHA
        , "sha"
#endif
#ifndef NO_SHA256
        , "sha256"
#endif
#ifdef WOLFSSL_SHA384
        , "sha384"
#endif
#ifdef WOLFSSL_SHA512
        , "sha512"
#endif
#ifdef HAVE_BLAKE2
        , "blake2b"
#endif
    };

    wolfCLU_help();

    printf("Available En/De crypt Algorithms with current configure "
        "settings.\n\n");
#ifndef NO_AES
    printf("aes-cbc-128\t\taes-cbc-192\t\taes-cbc-256\n");
#endif
#ifdef WOLFSSL_AES_COUNTER
    printf("aes-ctr-128\t\taes-ctr-192\t\taes-ctr-256\n");
#endif
#ifndef NO_DES3
    printf("3des-cbc-56\t\t3des-cbc-112\t\t3des-cbc-168\n");
#endif
#ifdef HAVE_CAMELLIA
    printf("camellia-cbc-128\tcamellia-cbc-192\t"
            "camellia-cbc-256\n");
#endif
    printf("\n");
    printf("Available hashing algorithms with current configure settings:\n\n");

    for (i = 0; i < (int) sizeof(algsenc)/(int) sizeof(algsenc[0]); i++) {
            printf("%s\n", algsenc[i]);
    }
    printf("Available benchmark tests with current configure settings:\n");
    printf("(-a to test all)\n\n");

    for(i = 0; i < (int) sizeof(algsother)/(int) sizeof(algsother[0]); i++) {
        printf("%s\n", algsother[i]);
    }
}

/*
 * Encrypt Usage
 */
void wolfCLU_encryptHelp()
{
    printf("\nAvailable En/De crypt Algorithms with current configure "
            "settings.\n\n");
#ifndef NO_AES
    printf("aes-cbc-128\t\taes-cbc-192\t\taes-cbc-256\n");
#endif
#ifdef WOLFSSL_AES_COUNTER
    printf("aes-ctr-128\t\taes-ctr-192\t\taes-ctr-256\n");
#endif
#ifndef NO_DES3
    printf("3des-cbc-56\t\t3des-cbc-112\t\t3des-cbc-168\n");
#endif
#ifdef HAVE_CAMELLIA
    printf("camellia-cbc-128\tcamellia-cbc-192\t"
            "camellia-cbc-256\n\n");
#endif
    printf("***************************************************************\n");
    printf("\nENCRYPT USAGE: wolfssl -encrypt <-algorithm> -in <filename> "
           "-pwd <password> -out <output file name>\n\n");
    printf("***************************************************************\n");
    printf("\nEXAMPLE: \n\nwolfssl -encrypt aes-cbc-128 -pwd Thi$i$myPa$$w0rd"
           " -in somefile.txt -out encryptedfile.txt\n\n");
}

/*
 * Decrypt Usage
 */
void wolfCLU_decryptHelp()
{
    printf("\nAvailable En/De crypt Algorithms with current configure "
            "settings.\n\n");
#ifndef NO_AES
    printf("aes-cbc-128\t\taes-cbc-192\t\taes-cbc-256\n");
#endif
#ifdef WOLFSSL_AES_COUNTER
    printf("aes-ctr-128\t\taes-ctr-192\t\taes-ctr-256\n");
#endif
#ifndef NO_DES3
    printf("3des-cbc-56\t\t3des-cbc-112\t\t3des-cbc-168\n");
#endif
#ifdef HAVE_CAMELLIA
    printf("camellia-cbc-128\tcamellia-cbc-192\t"
            "camellia-cbc-256\n\n");
#endif
    printf("***************************************************************\n");
    printf("\nDECRYPT USAGE: wolfssl -decrypt <algorithm> -in <encrypted file> "
           "-pwd <password> -out <output file name>\n\n");
    printf("***************************************************************\n");
    printf("\nEXAMPLE: \n\nwolfssl -decrypt aes-cbc-128 -pwd Thi$i$myPa$$w0rd"
           " -in encryptedfile.txt -out decryptedfile.txt\n\n");
}

/*
 * Hash Usage
 */
void wolfCLU_hashHelp()
{
    printf("\n");
    /* hash options */
    const char* algsenc[] = {        /* list of acceptable algorithms */
    "Algorithms: "
#ifndef NO_MD5
        ,"md5"
#endif
#ifndef NO_SHA
        ,"sha"
#endif
#ifndef NO_SHA256
        ,"sha256"
#endif
#ifdef WOLFSSL_SHA384
        ,"sha384"
#endif
#ifdef WOLFSSL_SHA512
        ,"sha512"
#endif
#ifdef HAVE_BLAKE2
        ,"blake2b"
#endif
#ifndef NO_CODING
    #ifdef WOLFSSL_BASE64_ENCODE
        ,"base64enc"
    #endif
        ,"base64dec"
#endif
        };

    printf("\nAvailable algorithms with current configure settings:\n");
    for (i = 0; i < (int) sizeof(algsenc)/(int) sizeof(algsenc[0]); i++) {
        printf("%s\n", algsenc[i]);
    }
            /* encryption/decryption help lists options */
    printf("***************************************************************\n");
    printf("\nUSAGE: wolfssl -hash <-algorithm> -in <file to hash>\n");
    printf("***************************************************************\n");
    printf("\nEXAMPLE: \n\nwolfssl -hash sha -in <some file>\n\n");
}

/*
 * Benchmark Usage
 */
void wolfCLU_benchHelp()
{
    printf("\n");

    /* benchmark options */
    const char* algsother[] = {      /* list of acceptable algorithms */
        "ALGS: "
#ifndef NO_AES
        , "aes-cbc"
#endif
#ifdef WOLFSSL_AES_COUNTER
        , "aes-ctr"
#endif
#ifndef NO_DES3
        , "3des"
#endif
#ifdef HAVE_CAMELLIA
        , "camellia"
#endif
#ifndef NO_MD5
        , "md5"
#endif
#ifndef NO_SHA
        , "sha"
#endif
#ifndef NO_SHA256
        , "sha256"
#endif
#ifdef WOLFSSL_SHA384
        , "sha384"
#endif
#ifdef WOLFSSL_SHA512
        , "sha512"
#endif
#ifdef HAVE_BLAKE2
        , "blake2b"
#endif
    };

    printf("\nAvailable tests: (-a to test all)\n");
    printf("Available tests with current configure settings:\n");
    for(i = 0; i < (int) sizeof(algsother)/(int) sizeof(algsother[0]); i++) {
        printf("%s\n", algsother[i]);
    }
    printf("\n");
            /* encryption/decryption help lists options */
    printf("***************************************************************\n");
    printf("USAGE: wolfssl -bench [alg] -time [time in seconds [1-10]]\n"
           "       or\n       wolfssl -bench -time 10 -all (to test all)\n");
    printf("***************************************************************\n");
    printf("\nEXAMPLE: \n\nwolfssl -bench aes-cbc -time 10"
           " -in encryptedfile.txt -out decryptedfile.txt\n\n");
}

void wolfCLU_certHelp()
{
    printf("\n\n");
    printf("***************************************************************\n");
    printf("\nX509 USAGE: wolfssl -x509 -inform <PEM or DER> -in <filename> "
           "-outform <PEM or DER> -out <output file name> \n\n");
    printf("***************************************************************\n");
    printf("\nEXAMPLE: \n\nwolfssl -x509 -inform pem -in testing-certs/"
           "ca-cert.pem -outform der -out testing-certs/ca-cert-converted.der"
           "\n\n");
}

void wolfCLU_genKeyHelp() {

        const char* keysother[] = { /* list of acceptable key types */
        "KEYS: "
    #ifndef NO_RSA
        ,"rsa"
    #endif
    #ifdef HAVE_ED25519
        ,"ed25519"
    #endif
    #ifdef HAVE_ECC
        ,"ecc"
    #endif
        };

        printf("Available keys with current configure settings:\n");
        for(i = 0; i < (int) sizeof(keysother)/(int) sizeof(keysother[0]); i++) {
            printf("%s\n", keysother[i]);
        }
    printf("\n\n");
    printf("***************************************************************\n");
    printf("\ngenkey USAGE:\nwolfssl -genkey <keytype> -size(optional) <bits> "
           "-out <filename> -outform <PEM or DER> -output <PUB/PRIV/KEYPAIR> \n\n");
    printf("***************************************************************\n");
    printf("\nEXAMPLE: \n\nwolfssl -genkey rsa -size 2048 -out mykey -outform der "
           " -output KEYPAIR"
           "\n\nThe above command would output the files: mykey.priv "
           " and mykey.pub\nChanging the -output option to just PRIV would only"
           "\noutput the mykey.priv and using just PUB would only output"
           "\nmykey.pub\n\n");
}

void wolfCLU_signHelp(int keyType) {

    const char* keysother[] = { /* list of acceptable key types */
        "KEYS: "
        #ifndef NO_RSA
        ,"rsa"
        #endif
        #ifdef HAVE_ED25519
        ,"ed25519"
        #endif
        #ifdef HAVE_ECC
        ,"ecc"
        #endif
        };

        printf("\nAvailable keys with current configure settings:\n");
        for(i = 0; i < (int) sizeof(keysother)/(int) sizeof(keysother[0]); i++) {
            printf("%s\n", keysother[i]);
        }
        
        printf("\n***************************************************************\n");
        switch(keyType) {
            #ifndef NO_RSA
            case RSA_SIG_VER:
                printf("RSA Sign Usage: \nwolfssl -rsa -sign -inkey <priv_key>"
                       " -in <filename> -out <filename>\n\n");
                printf("***************************************************************\n");
                break;
            #endif
            #ifdef HAVE_ED25519
            case ED25519_SIG_VER:
                printf("ED25519 Sign Usage: \nwolfssl -ed25519 -sign -inkey "
                       "<priv_key> -in <filename> -out <filename>\n\n");
                printf("***************************************************************\n");
                break;
            #endif
            #ifdef HAVE_ECC
            case ECC_SIG_VER:
                printf("ECC Sign Usage: \nwolfssl -ecc -sign -inkey <priv_key>"
                       " -in <filename> -out <filename>\n\n");
                break;
            #endif
            default:
                printf("No valid key type defined.\n\n");
        }
}

void wolfCLU_verifyHelp(int keyType) {
    const char* keysother[] = { /* list of acceptable key types */
        "KEYS: "
        #ifndef NO_RSA
        ,"rsa"
        #endif
        #ifdef HAVE_ED25519
        ,"ed25519"
        #endif
        #ifdef HAVE_ECC
        ,"ecc"
        #endif
        };

        printf("\nAvailable keys with current configure settings:\n");
        for(i = 0; i < (int) sizeof(keysother)/(int) sizeof(keysother[0]); i++) {
            printf("%s\n", keysother[i]);
        }
        
        printf("\n***************************************************************\n");
        switch(keyType) {
            #ifndef NO_RSA
            case RSA_SIG_VER:
                printf("RSA Verify with Private Key:\n"
                        "wolfssl -rsa -verify -inkey <priv_key>"
                        " -sigfile <filename> -out <filename>\n\n");
                printf("***************************************************************\n");
                printf("RSA Verify with Public Key\n"
                       "wolfssl -rsa -verify -inkey <pub_key>"
                       " -sigfile <filename> -out <filename> -pubin\n\n");
                printf("***************************************************************\n");
                break;
            #endif
            #ifdef HAVE_ED25519
            case ED25519_SIG_VER:
                printf("ED25519 Verifiy with Private Key\n"
                       "wolfssl -ed25519 -verify -inkey "
                       "<priv_key> -sigfile <filename> -in <original>"
                       "\n\n");
                printf("***************************************************************\n");
                printf("ED25519 Verifiy with Public Key\n"
                       "wolfssl -ed25519 -verify -inkey "
                       "<pub_key> -sigfile <filename> -in <original> -pubin"
                       "\n\n");
                printf("***************************************************************\n");
                break;
            #endif
            #ifdef HAVE_ECC
            case ECC_SIG_VER:
                printf("ECC Verify with Public Key\n"
                       "wolfssl -ecc -verify -inkey <pub_key>"
                       " -sigfile <signature> -in <original>\n\n");
                break;
            #endif
            default:
                printf("No valid key type defined.\n\n");
        }
}

void wolfCLU_certgenHelp() {
    printf("\n\n");
    printf("***************************************************************\n");
    printf("\ncertgen USAGE:\nwolfssl -req -ecc/-rsa/-ed25519 -in <filename> -out"
           " <filename> -sha/sha224/sha256/sha384/sha512\n\n");
    printf("***************************************************************\n");
    printf("\nEXAMPLE: \n\nwolfssl -req ecc -in mykey -out cert.pem -sha256 "
           "\n\nThe above command would output the file: cert.pem\n");
}


/* return block size on success
 * alg and mode are null terminated strings that need free'd by the caller
 */
static int wolfCLU_parseAlgo(char* name, int* alg, char** mode, int* size)
{
    int     ret         = 0;        /* return variable */
    int     nameCheck   = 0;        /* check for acceptable name */
    int     modeCheck   = 0;        /* check for acceptable mode */
    char*   sz          = 0;        /* key size provided */
    char*   end         = 0;
    char*   tmpAlg      = NULL;
    char*   tmpMode     = NULL;

    const char* acceptAlgs[]  = {   /* list of acceptable algorithms */
        "Algorithms: "
#ifndef NO_AES
        , "aes"
#endif
#ifndef NO_DES3
        , "3des"
#endif
#ifdef HAVE_CAMELLIA
        , "camellia"
#endif
    };

    const char* acceptMode[] = {"cbc"
#ifdef WOLFSSL_AES_COUNTER
        , "ctr"
#endif
    };

    if (name == NULL) {
        return FATAL_ERROR;
    }

    if (alg == NULL || mode == NULL || size == NULL) {
        printf("null input to get algo function\n");
        return FATAL_ERROR;
    }

    /* gets name after first '-' and before the second */
    tmpAlg = strtok_r(name, "-", &end);
    if (tmpAlg == NULL) {
        return FATAL_ERROR;
    }


    for (i = 0; i < (int)(sizeof(acceptAlgs)/sizeof(acceptAlgs[0])); i++) {
        if (strcmp(tmpAlg, acceptAlgs[i]) == 0 )
            nameCheck = 1;
    }

    /* gets mode after second "-" and before the third */
    if (nameCheck != 0) {
        /* gets size after third "-" */
        sz = strtok_r(NULL, "-", &end);
        if (sz == NULL) {
            return FATAL_ERROR;
        }
        *size = atoi(sz);
    }

    tmpMode = strtok_r(NULL, "-", &end);
    if (tmpMode == NULL) {
        return FATAL_ERROR;
    }

    for (i = 0; i < (int) (sizeof(acceptMode)/sizeof(acceptMode[0])); i++) {
        if (strcmp(tmpMode, acceptMode[i]) == 0)
            modeCheck = 1;
    }

    /* if name or mode doesn't match acceptable options */
    if (nameCheck == 0 || modeCheck == 0) {
        printf("Invalid entry\n");
        return FATAL_ERROR;
    }

    /* checks key sizes for acceptability */
    if (XSTRNCMP(tmpAlg, "aes", 3) == 0) {
    #ifdef NO_AES
        printf("AES not compiled in.\n");
        return NOT_COMPILED_IN;
    #else
        ret = AES_BLOCK_SIZE;
        if (*size != 128 && *size != 192 && *size != 256) {
            printf("Invalid AES pwdKey size. Should be: %d\n", ret);
            ret = FATAL_ERROR;
        }

        if (XSTRNCMP(tmpMode, "cbc", 3) == 0) {
            switch (*size) {
                case 128:
                    *alg = WOLFCLU_AES128CBC;
                    break;
                case 192:
                    *alg = WOLFCLU_AES192CBC;
                    break;
                case 256:
                    *alg = WOLFCLU_AES256CBC;
                    break;
            }
        }

        if (XSTRNCMP(tmpMode, "ctr", 3) == 0) {
            switch (*size) {
                case 128:
                    *alg = WOLFCLU_AES128CTR;
                    break;
                case 192:
                    *alg = WOLFCLU_AES192CTR;
                    break;
                case 256:
                    *alg = WOLFCLU_AES256CTR;
                    break;
            }
        }
    #endif
    }

    else if (XSTRNCMP(tmpAlg, "3des", 4) == 0) {
    #ifdef NO_DES3
        printf("3DES not compiled in.\n");
        return NOT_COMPILED_IN;
    #else
        ret = DES3_BLOCK_SIZE;
        if (*size != 56 && *size != 112 && *size != 168) {
            printf("Invalid 3DES pwdKey size\n");
            ret = FATAL_ERROR;
        }
        *alg = WOLFCLU_DESCBC;
    #endif
    }

    else if (XSTRNCMP(tmpAlg, "camellia", 8) == 0) {
    #ifndef HAVE_CAMELLIA
        printf("CAMELIA not compiled in.\n");
        return NOT_COMPILED_IN;
    #else
        ret = CAMELLIA_BLOCK_SIZE;
        if (*size != 128 && *size != 192 && *size != 256) {
            printf("Invalid Camellia pwdKey size\n");
            ret = FATAL_ERROR;
        }

        if (XSTRNCMP(tmpMode, "cbc", 3) == 0) {
            switch (*size) {
                case 128:
                    *alg = WOLFCLU_CAMELLIA128CBC;
                    break;
                case 192:
                    *alg = WOLFCLU_CAMELLIA192CBC;
                    break;
                case 256:
                    *alg = WOLFCLU_CAMELLIA256CBC;
                    break;
            }
        }
    #endif
    }

    else {
        printf("Invalid algorithm: %s\n", tmpAlg);
        ret = FATAL_ERROR;
    }

    if (ret >= 0) {
        int s;

        /* free any existing mode buffers */
        if (*mode != NULL)
            XFREE(*mode, NULL, DYNAMIC_TYPE_TMP_BUFFER);

        if (ret >= 0) {
            s = XSTRLEN(tmpMode) + 1;
            *mode = (char*)XMALLOC(s, NULL, DYNAMIC_TYPE_TMP_BUFFER);
            if (*mode == NULL) {
                ret = MEMORY_E;
            }
        }

        if (ret >= 0) {
            XSTRNCPY(*mode, tmpMode, s);
        }
    }

    /* free up stuff in case of error */
    if (ret < 0) {
        if (*mode != NULL)
            XFREE(*mode, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        *mode = NULL;
    }

    return ret;
}

#define MAX_AES_IDX 6
static const char* algoName[] = {
    "aes-128-ctr",
    "aes-192-ctr",
    "aes-256-ctr",
    "aes-128-cbc",
    "aes-192-cbc",
    "aes-256-cbc",
    "camellia-128-cbc",
    "camellia-192-cbc",
    "camellia-256-cbc",
    "des-cbc",

};

/* support older name schemes */
static const char* oldAlgoName[] = {
    "aes-ctr-128",
    "aes-ctr-192",
    "aes-ctr-256",
    "aes-cbc-128",
    "aes-cbc-192",
    "aes-cbc-256",
};


/* convert an old algo name into one optargs can handle */
static void wolfCLU_oldAlgo(int argc, char* argv[], int maxIdx)
{
    int end;
    int i, j;

    end = (argc < maxIdx)? argc : maxIdx;
    for (i = 0; i < end; i++) {
        for (j = 0; j < MAX_AES_IDX; j++) {
            if (XSTRCMP(argv[i], oldAlgoName[j]) == 0) {
                argv[i] = (char*)algoName[j];
            }
        }
    }
}


/* get the WOLFSSL_EVP_CIPHER type from an algo enum value */
const WOLFSSL_EVP_CIPHER* wolfCLU_CipherTypeFromAlgo(int alg)
{
    switch (alg) {
        case WOLFCLU_AES128CTR:
            return wolfSSL_EVP_aes_128_ctr();
        case WOLFCLU_AES192CTR:
            return wolfSSL_EVP_aes_192_ctr();
        case WOLFCLU_AES256CTR:
            return wolfSSL_EVP_aes_256_ctr();
        case WOLFCLU_AES128CBC:
            return wolfSSL_EVP_aes_128_cbc();
        case WOLFCLU_AES192CBC:
            return wolfSSL_EVP_aes_192_cbc();
        case WOLFCLU_AES256CBC:
            return wolfSSL_EVP_aes_256_cbc();

        case WOLFCLU_DESCBC:
            return wolfSSL_EVP_des_cbc();

        default:
            return NULL;
    }
}


/*
 * finds algorithm for encryption/decryption
 * alg and mode are null terminated strings that need free'd by the caller
 */
int wolfCLU_getAlgo(int argc, char* argv[], int* alg, char** mode, int* size)
{
    int ret = 0;
    int long_index = 2;
    int option;
    char name[80];
    char *argvCopy[argc];
    int i;

    /* make a copy of args because getopt_long_only reorders them */
    for (i = 0; i < argc; i++) argvCopy[i] = argv[i];

    /* first just try the 3rd argument */
    if (argc < 3 || argvCopy[2] == NULL) {
        return FATAL_ERROR;
    }

    wolfCLU_oldAlgo(argc, argvCopy, 3);
    XMEMSET(name, 0, sizeof(name));
    XSTRNCPY(name, argvCopy[2], XSTRLEN(argv[2]));
    ret = wolfCLU_parseAlgo(name, alg, mode, size);

    /* next check for -cipher option passed through args */
    if (ret < 0) {
        opterr = 0; /* do not print out unknown options */
        XMEMSET(name, 0, sizeof(name));
        while ((option = getopt_long_only(argc, argvCopy, "",
                       crypt_algo_options, &long_index )) != -1) {
            switch (option) {
                /* AES */
                case WOLFCLU_AES128CTR:
                    XSTRNCPY(name, algoName[0], XSTRLEN(algoName[0]));
                    break;

                case WOLFCLU_AES192CTR:
                    XSTRNCPY(name, algoName[1], XSTRLEN(algoName[1]));
                    break;

                case WOLFCLU_AES256CTR:
                    XSTRNCPY(name, algoName[2], XSTRLEN(algoName[2]));
                    break;

                case WOLFCLU_AES128CBC:
                    XSTRNCPY(name, algoName[3], XSTRLEN(algoName[3]));
                    break;

                case WOLFCLU_AES192CBC:
                    XSTRNCPY(name, algoName[4], XSTRLEN(algoName[4]));
                    break;

                case WOLFCLU_AES256CBC:
                    XSTRNCPY(name, algoName[5], XSTRLEN(algoName[5]));
                    break;

                /* camellia */
                case WOLFCLU_CAMELLIA128CBC:
                    XSTRNCPY(name, algoName[6], XSTRLEN(algoName[6]));
                    break;

                case WOLFCLU_CAMELLIA192CBC:
                    XSTRNCPY(name, algoName[7], XSTRLEN(algoName[7]));
                    break;

                case WOLFCLU_CAMELLIA256CBC:
                    XSTRNCPY(name, algoName[8], XSTRLEN(algoName[8]));
                    break;

                /* 3des */
                case WOLFCLU_DESCBC:
                    XSTRNCPY(name, algoName[9], XSTRLEN(algoName[9]));
                    break;

                case '?':
                case ':':
                    break;
                default:
                    /* do nothing. */
                    (void)ret;
            };

            if (XSTRLEN(name) > 0) {
                ret = wolfCLU_parseAlgo(name, alg, mode, size);
                XMEMSET(name, 0, sizeof(name));
            }
        }
    }

    return ret;
}


/*
 * secure data entry by turning off key echoing in the terminal
 */
int wolfCLU_noEcho(char* pwdKey, int size)
{
    struct termios oflags, nflags;
    char* success;
    int ret;

    /* disabling echo */
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;

    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        printf("Error\n");
        return FATAL_ERROR;
    }

    printf("pwdKey: ");
    success = fgets(pwdKey, size, stdin);
    if (success == NULL) {
        /* User wants manual input to be encrypted
         * Do Nothing
         */
    }

    pwdKey[strlen(pwdKey) - 1] = 0;

    /* restore terminal */
    ret = tcsetattr(fileno(stdin), TCSANOW, &oflags);
    if (ret != 0) {
        printf("Error\n");
        return FATAL_ERROR;
    }
    return 0;
}

/*
 * adds character to end of string
 */
void wolfCLU_append(char* s, char c)
{
    int len = (int) strlen(s); /* length of string*/

    s[len] = c;
    s[len+1] = '\0';
}

/*
 * resets benchmarking loop
 */
void wolfCLU_stop(int signo)
{
    (void) signo; /* type cast to void for unused variable */
    loop = 0;
}

/*
 * gets current time durring program execution
 */
double wolfCLU_getTime(void)
{
    struct timeval tv;

    gettimeofday(&tv, 0);

    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
}

/*
 * prints out stats for benchmarking
 */
void wolfCLU_stats(double start, int blockSize, int64_t blocks)
{
    double mbs;
    double time_total = wolfCLU_getTime() - start;

    printf("took %6.3f seconds, blocks = %llu\n", time_total,
            (unsigned long long)blocks);

    mbs = ((blocks * blockSize) / MEGABYTE) / time_total;
    printf("Average MB/s = %8.1f\n", mbs);
    if (blockSize != MEGABYTE)
        printf("Block size of this algorithm is: %d.\n\n", blockSize);
    else
        printf("Benchmarked using 1 Megabyte at a time\n\n");
}


/* returns 0 on success */
int wolfCLU_version()
{
    printf("You are using version %s of the wolfssl Command Line Utility.\n"
        , CLUWOLFSSL_VERSION_STRING);
    printf("Linked to wolfSSL version %s\n", LIBWOLFSSL_VERSION_STRING);
    return 0;
}

int wolfCLU_checkForArg(char* searchTerm, int length, int argc, char** argv)
{
    int i;
    int ret = 0;
    int argFound = 0;

    if (searchTerm == NULL) {
        return 0;
    }

    for (i = 0; i < argc; i++) {
        if (argv[i] == NULL) {
            break; /* stop checking if no more args*/
        } else if (XSTRNCMP(searchTerm, "-help", length) == 0 &&
                   XSTRNCMP(argv[i], "-help", XSTRLEN(argv[i])) == 0 &&
                   (int) XSTRLEN(argv[i]) > 0) {
           return 1;

        } else if (XMEMCMP(argv[i], searchTerm, length) == 0 &&
                   XSTRLEN(argv[i]) == length) {

            ret = i;
            if (argFound == 1) {
                printf("ERROR: argument found twice: \"%s\"\n", searchTerm);
                return USER_INPUT_ERROR;
            }
            argFound = 1;
        }
    }

    return ret;
}

int wolfCLU_checkOutform(char* outform)
{
    if (outform == NULL) {
        printf("Usage: -outform [PEM/DER]\n");
        printf("missing outform required argument\n");
        return USER_INPUT_ERROR;
    }

    convert_to_lower(outform, XSTRLEN(outform));
    if (XSTRNCMP(outform, "pem", 3) == 0) {
        return PEM_FORM;
    } else if (XSTRNCMP(outform, "der", 3) == 0) {
        return DER_FORM;
    } else {
        printf("Usage: -outform [PEM/DER]\n");
        printf("\"%s\" is not a valid output format\n", outform);
    }
    return USER_INPUT_ERROR;
}

int wolfCLU_checkInform(char* inform)
{
    if (inform == NULL) {
        printf("Usage: -inform [PEM/DER]\n");
        printf("missing inform required argument\n");
        return USER_INPUT_ERROR;
    }

    convert_to_lower(inform, XSTRLEN(inform));
    if (XSTRNCMP(inform, "pem", 3) == 0) {
        return PEM_FORM;
    } else if (XSTRNCMP(inform, "der", 3) == 0) {
        return DER_FORM;
    } else {
        printf("Usage: -inform [PEM/DER]\n");
        printf("\"%s\" is not a valid input format\n", inform);
    }
    return USER_INPUT_ERROR;
}


static void wolfCLU_AddNameEntry(WOLFSSL_X509_NAME* name, int type, int nid,
        char* str)
{
    int i, sz;
    WOLFSSL_X509_NAME_ENTRY *entry;

    if (str != NULL) {
        /* strip off newline character if found at the end of str */
        i = XSTRLEN((const char*)str);
        while (i >= 0) {
            if (str[i] == '\n') {
                str[i] = '\0';
                break;
            }
            i--;
        }

        /* treats an empty space as 'do not add' */
        sz = (int)XSTRLEN((const char*)str);
        if (sz > 0) {
            entry = wolfSSL_X509_NAME_ENTRY_create_by_NID(NULL, nid,
                type, (const unsigned char*)str, sz);
            wolfSSL_X509_NAME_add_entry(name, entry, -1, 0);
        }
    }
}


/* returns 0 on success */
int wolfCLU_CreateX509Name(WOLFSSL_X509_NAME* name)
{
    char   *in = NULL;
    size_t  inSz;
    ssize_t ret;
    FILE *fin = stdin; /* defaulting to stdin but using a fd variable to make it
                        * easy for expanding to other inputs */

    printf("Enter without data will result in the feild being skipped\n");
    printf("Country [US] : ");
    ret = getline(&in, &inSz, fin);
    if (ret > 0) {
        wolfCLU_AddNameEntry(name, CTC_PRINTABLE, NID_countryName, in);
        free(in); in = NULL;
    }

    printf("State or Province [Montana] : ");
    ret = getline(&in, &inSz, fin);
    if (ret > 0) {
        wolfCLU_AddNameEntry(name, CTC_UTF8, NID_stateOrProvinceName, in);
        free(in); in = NULL;
    }

    printf("Locality [Bozeman] : ");
    ret = getline(&in, &inSz, fin);
    if (ret > 0) {
        wolfCLU_AddNameEntry(name, CTC_UTF8, NID_localityName, in);
        free(in); in = NULL;
    }

    printf("Organization Name [wolfSSL] : ");
    ret = getline(&in, &inSz, fin);
    if (ret > 0) {
        wolfCLU_AddNameEntry(name, CTC_UTF8, NID_organizationName, in);
        free(in); in = NULL;
    }

    printf("Organization Unit [engineering] : ");
    ret = getline(&in, &inSz, fin);
    if (ret > 0) {
        wolfCLU_AddNameEntry(name, CTC_UTF8, NID_organizationalUnitName, in);
        free(in); in = NULL;
    }

    printf("Common Name : ");
    ret = getline(&in, &inSz, fin);
    if (ret > 0) {
        wolfCLU_AddNameEntry(name, CTC_UTF8, NID_commonName, in);
        free(in); in = NULL;
    }

    printf("Email Address : ");
    ret = getline(&in, &inSz, fin);
    if (ret > 0) {
        wolfCLU_AddNameEntry(name, CTC_UTF8, NID_emailAddress, in);
        free(in); in = NULL;
    }

    return 0;
}
