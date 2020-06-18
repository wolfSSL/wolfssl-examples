/* eccsiglentest.c
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

#include <wolfssl/options.h>

#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/signature.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include <stdio.h>

/* utility to perform loop on each curve to determine max signature size */
#ifndef ECC_LOOP_COUNT
#define ECC_LOOP_COUNT 1000
#endif

/* optional debugging */
//#define DEBUG_SIG_TEST
//#define DEBUG_SIG_TEST_MAX

#if defined(DEBUG_SIG_TEST) || defined(DEBUG_SIG_TEST_MAX)
static void hexdump(const void *buffer, word32 len, byte cols)
{
   word32 i;

   for(i = 0; i < len + ((len % cols) ? (cols - len % cols) : 0); i++)
   {
      /* print hex data */
      if(i < len) {
         printf("%02X ", ((byte*)buffer)[i] & 0xFF);
      }

      if(i % cols == (cols - 1)) {
         printf("\n");
      }
   }
}
#endif

#ifdef HAVE_ECC
int ecc_sign_verify_test(enum wc_HashType hash_type,
    enum wc_SignatureType sig_type, const byte* fileBuf, int fileLen,
    byte* verifyFileBuf, int* verifyFileLen, int* pmaxSigSz, int* pmaxCurveSigSz,
    int curveId, int keySz)
{
    int ret;
    ecc_key eccKey;
    WC_RNG rng;
    byte* sigBuf = NULL;
    word32 sigLen;
    byte eccPubKeyBuf[ECC_BUFSIZE], eccPrivKeyBuf[ECC_BUFSIZE];
    word32 eccPubKeyLen, eccPrivKeyLen;
    word32 maxCurveSigSz;

#ifdef DEBUG_SIG_TEST
    printf("ECC Signature: Curve %s, Size %d\n", wc_ecc_get_name(curveId), keySz);
#endif

    /* Init */
    wc_InitRng(&rng);

    /* Generate key */
    wc_ecc_init(&eccKey);

    ret = wc_ecc_make_key_ex(&rng, keySz, &eccKey, curveId);
    if(ret != 0) {
        printf("ECC Make Key Failed! %d\n", ret);
        goto exit;
    }

    ret = wc_ecc_sig_size(&eccKey);
    if (ret < 0) {
        printf("ECC Sig SizeFailed! %d\n", ret);
        goto exit;
    }
    maxCurveSigSz = ret;

    /* Display public key data */
    eccPubKeyLen = ECC_BUFSIZE;
    ret = wc_ecc_export_x963(&eccKey, eccPubKeyBuf, &eccPubKeyLen);
    if (ret != 0) {
        printf("ECC public key x963 export failed! %d\n", ret);
        ret = EXIT_FAILURE;
        goto exit;
    }

#ifdef DEBUG_SIG_TEST
    printf("ECC Public Key: Len %d\n", eccPubKeyLen);
    hexdump(eccPubKeyBuf, eccPubKeyLen, 16);
#endif

    /* Display private key data */
    eccPrivKeyLen = ECC_BUFSIZE;
    ret = wc_ecc_export_private_only(&eccKey, eccPrivKeyBuf, &eccPrivKeyLen);
    if (ret != 0) {
        printf("ECC private key export failed! %d\n", ret);
        ret = EXIT_FAILURE;
        goto exit;
    }
#ifdef DEBUG_SIG_TEST
    printf("ECC Private Key: Len %d\n", eccPrivKeyLen);
    hexdump(eccPrivKeyBuf, eccPrivKeyLen, 16);
#endif

    if (verifyFileBuf) {
        sigLen = *verifyFileLen;
        sigBuf = verifyFileBuf;
    }
    else {
        /* Get signature length and allocate buffer */
        sigLen = wc_SignatureGetSize(sig_type, &eccKey, sizeof(eccKey));
        if(sigLen <= 0) {
            printf("ECC Signature type %d not supported!\n", sig_type);
            ret = EXIT_FAILURE;
            goto exit;
        }

        sigBuf = malloc(sigLen);
        if(!sigBuf) {
            printf("ECC Signature malloc failed!\n");
            ret = EXIT_FAILURE;
            goto exit;
        }
    #ifdef DEBUG_SIG_TEST
        printf("ECC Signature Len: %d\n", sigLen);
    #endif

        /* Perform hash and sign to create signature */
        ret = wc_SignatureGenerate(
            hash_type, sig_type,
            fileBuf, fileLen,
            sigBuf, &sigLen,
            &eccKey, sizeof(eccKey),
            &rng);
        *verifyFileLen = sigLen;
    #ifdef DEBUG_SIG_TEST
        printf("ECC Signature Generation: %s (%d)\n",
            (ret == 0) ? "Pass" : "Fail", ret);
    #endif
        if(ret < 0) {
            ret = EXIT_FAILURE;
            goto exit;
        }
    }

#ifdef DEBUG_SIG_TEST
    printf("Signature Data:\n");
    hexdump(sigBuf, sigLen, 16);
#endif

    /* Perform signature verification */
    /* Release and init new key */
    wc_ecc_free(&eccKey);
    wc_ecc_init(&eccKey);

    /* Import the public key */
    ret = wc_ecc_import_x963_ex(eccPubKeyBuf, eccPubKeyLen, &eccKey, curveId);
    if (ret != 0) {
        printf("ECC public key import failed! %d\n", ret);
        ret = EXIT_FAILURE;
        goto exit;
    }

    /* Perform signature verification using public key */
    ret = wc_SignatureVerify(
        hash_type, sig_type,
        fileBuf, fileLen,
        sigBuf, sigLen,
        &eccKey, sizeof(eccKey));
#ifdef DEBUG_SIG_TEST
    printf("ECC Signature Verification: %s (%d)\n",
        (ret == 0) ? "Pass" : "Fail", ret);
#endif
    if (ret < 0) {
        ret = EXIT_FAILURE;
    }

    if (pmaxSigSz && *pmaxSigSz < sigLen) {
    #ifdef DEBUG_SIG_TEST_MAX
        printf("Curve: Max %d->%d\n", *pmaxSigSz, sigLen);
        hexdump(sigBuf, sigLen, 16);
    #endif
        *pmaxSigSz = sigLen;
    }

    if (pmaxCurveSigSz && *pmaxCurveSigSz < maxCurveSigSz) {
        *pmaxCurveSigSz = maxCurveSigSz;
    }

exit:
    /* Free */
    if(sigBuf) {
        free(sigBuf);
    }
    wc_ecc_free(&eccKey);
    wc_FreeRng(&rng);

    return ret;
}
#endif /* HAVE_ECC */


static int load_file_to_buffer(const char* filename, byte** fileBuf, int* fileLen)
{
    int ret = 0;
    FILE* file = NULL;

    /* Open file */
    file = fopen(filename, "rb");
    if (file == NULL) {
        printf("File %s does not exist!\n", filename);
        ret = EXIT_FAILURE;
        goto exit;
    }

    /* Determine length of file */
    fseek(file, 0, SEEK_END);
    *fileLen = (int) ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("File %s is %d bytes\n", filename, *fileLen);

    /* Allocate buffer for image */
    *fileBuf = malloc(*fileLen);
    if(!*fileBuf) {
        printf("File buffer malloc failed!\n");
        ret = EXIT_FAILURE;
        goto exit;
    }

    /* Load file into buffer */
    ret = (int)fread(*fileBuf, 1, *fileLen, file);
    if(ret != *fileLen) {
        printf("Error reading file! %d", ret);
        ret = EXIT_FAILURE;
        goto exit;
    }

exit:

    if(file) {
        fclose(file);
    }

    return ret;
}

int main(int argc, char** argv)
{
    int ret = 0;
    int fileLen = 0;
    int verifyFileLen = 0;

    byte* fileBuf = NULL;
    byte* verifyFileBuf = NULL;

    const char* verify_file = NULL;

    enum wc_SignatureType sig_type = WC_SIGNATURE_TYPE_NONE;
    enum wc_HashType hash_type = WC_HASH_TYPE_NONE;

    sig_type = WC_SIGNATURE_TYPE_ECC;
    hash_type = WC_HASH_TYPE_SHA256;

#if 0
    wolfSSL_Debugging_ON();
#endif

    /* Check arguments */
    if (argc < 2) {
        printf("Usage: eccsiglentest <filename>\n");
        return 1;
    }

    /* Verify hash type is supported */
    if (wc_HashGetDigestSize(hash_type) <= 0) {
        printf("Hash type %d not supported!\n", hash_type);
        return 1;
    }

    printf("Signature Length Test: Loops %d\n", ECC_LOOP_COUNT);

    /* Load input file */
    ret = load_file_to_buffer(argv[1], &fileBuf, &fileLen);
    if (ret < 0) {
        goto exit;
    }

    /* Load verify signature file (optional) */
    if (verify_file) {
        ret = load_file_to_buffer(verify_file, &verifyFileBuf, &verifyFileLen);
        if (ret < 0) {
            goto exit;
        }
    }

    /* Perform sign and verify */
    switch(sig_type)
    {
#ifdef HAVE_ECC
    case WC_SIGNATURE_TYPE_ECC:
    {
        int curveId;
        int sigSz = verifyFileLen;
    #if 1
        for (curveId=ECC_SECP192R1; curveId<=ECC_BRAINPOOLP512R1; curveId++)
    #else
        curveId = ECC_SECP521R1;
    #endif
        {
            int keySz = wc_ecc_get_curve_size_from_id(curveId);
            if (keySz > 0) {
                int maxSigSz = 0;
                int maxCurveSigSz = 0;
                int tries = ECC_LOOP_COUNT;
                while (--tries > 0) {
                    ret = ecc_sign_verify_test(hash_type, sig_type, fileBuf, fileLen,
                        verifyFileBuf, &sigSz, &maxSigSz, &maxCurveSigSz, curveId, keySz);
                }

                /* print max */
                printf("ECC Curve %s, KeySz %d, Sig: CurveMax %d, ActMax %d, CalcMax %d\n",
                    wc_ecc_get_name(curveId), keySz, maxCurveSigSz, maxSigSz,
                    wc_ecc_sig_size_calc(keySz));
            }
        }
    }
        break;
#endif
        default:
            ret = EXIT_FAILURE;
            printf("Signature type %d, not supported!\n", sig_type);
    }

exit:
    /* Free */
    if (fileBuf) {
        free(fileBuf);
    }

    return ret;
}
