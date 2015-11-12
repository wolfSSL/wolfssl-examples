/* signature.c
 *
 * Copyright (C) 2006-2015 wolfSSL Inc.
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

#include <stdio.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/signature.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

void hexdump(const void *buffer, word32 len, byte cols)
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


int main(int argc, char** argv)
{
    int ret = 0;
    ecc_key eccKey;
    RNG rng;
    int fileLen;
    byte* fileBuf = NULL;
    FILE* file;
    byte* sigBuf = NULL;
    word32 sigLen;
    enum wc_HashType hash_type = WC_HASH_TYPE_SHA512;
    enum wc_SignatureType sig_type = WC_SIGNATURE_TYPE_ECC;
    byte eccPubKeyBuf[ECC_BUFSIZE], eccPrivKeyBuf[ECC_BUFSIZE];
    word32 eccPubKeyLen, eccPrivKeyLen;

    /* Check arguments */
    if (argc < 2) {
        printf("Usage: signature <filename> <hash>\n");
        printf("  <hash>: 1=MD2, 2=MD4, 3=MD5, 4=SHA, 5=SHA256, 6=SHA384, 7=SHA512 (def)\n");
        return 1;
    }
    if(argc >= 3) {
        hash_type = atoi(argv[2]);
    }
    
    /* Verify hash type is supported */
    if (wc_HashGetDigestSize(hash_type) <= 0) {
        printf("Hash type %d not supported!\n", hash_type);
        return 1;
    }
    
    printf("Signature Example: Hash=%d\n", hash_type);
    
    /* Init */
    wc_InitRng(&rng);

    /* Open file */
    file = fopen(argv[1], "rb");
    if (file == NULL) {
        printf("File %s does not exist!\n", argv[1]);
        ret = EXIT_FAILURE;
        goto exit;
    }

    /* Determine length of file */
    fseek(file, 0, SEEK_END);
    fileLen = (int) ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("File %s is %d bytes\n", argv[1], fileLen);

    /* Allocate buffer for image */
    fileBuf = malloc(fileLen);
    if(!fileBuf) {
        printf("File buffer malloc failed!\n");
        ret = EXIT_FAILURE;
        goto exit;
    }

    /* Load file into buffer */
    ret = (int)fread(fileBuf, 1, fileLen, file);
    if(ret != fileLen) {
        printf("Error reading file! %d", ret);
        ret = EXIT_FAILURE;
        goto exit;
    }

    /* Generate key */
    wc_ecc_init(&eccKey);
    ret = wc_ecc_make_key(&rng, 32, &eccKey);
    if(ret != 0) {
        printf("Make ECC Key Failed! %d\n", ret);
    }

    /* Display public key data */
    eccPubKeyLen = ECC_BUFSIZE;
    ret = wc_ecc_export_x963(&eccKey, eccPubKeyBuf, &eccPubKeyLen);
    if (ret != 0) {
        printf("ECC public key x963 export failed! %d\n", ret);
        ret = EXIT_FAILURE;
        goto exit;
    }
    printf("ECC Public Key: Len %d\n", eccPubKeyLen);
    hexdump(eccPubKeyBuf, eccPubKeyLen, 16);

    /* Display private key data */
    eccPrivKeyLen = ECC_BUFSIZE;
    ret = wc_ecc_export_private_only(&eccKey, eccPrivKeyBuf, &eccPrivKeyLen);
    if (ret != 0) {
        printf("ECC private key export failed! %d\n", ret);
        ret = EXIT_FAILURE;
        goto exit;
    }
    printf("ECC Private Key: Len %d\n", eccPrivKeyLen);
    hexdump(eccPrivKeyBuf, eccPrivKeyLen, 16);

    /* Get signature length and allocate buffer */
    sigLen = wc_SignatureGetSize(sig_type, &eccKey, sizeof(eccKey));
    if(sigLen <= 0) {
        printf("Signature type %d not supported!\n", sig_type);
        ret = EXIT_FAILURE;
        goto exit;
    }
    sigBuf = malloc(sigLen);
    if(!sigBuf) {
        printf("Signature malloc failed!\n");
        ret = EXIT_FAILURE;
        goto exit;
    }
    printf("Signature Len: %d\n", sigLen);

    /* Perform hash and sign to create signature */
    ret = wc_SignatureGenerate(
        hash_type, sig_type,
        fileBuf, fileLen,
        sigBuf, &sigLen,
        &eccKey, sizeof(eccKey),
        &rng);
    printf("Signature Generation: %s (%d)\n", (ret == 0) ? "Pass" : "Fail", ret);
    if(ret != 0) {
        ret = EXIT_FAILURE;
        goto exit;
    }

    printf("Signature Data:\n");
    hexdump(sigBuf, sigLen, 16);

    /* Perform signature verification */
    /* Release and init new key */
    wc_ecc_free(&eccKey);
    wc_ecc_init(&eccKey);

    /* Import the public key */
    ret = wc_ecc_import_x963(eccPubKeyBuf, eccPubKeyLen, &eccKey);
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
    printf("Signature Verification: %s (%d)\n", (ret == 0) ? "Pass" : "Fail", ret);
    if(ret != 0) {
        ret = EXIT_FAILURE;
        goto exit;
    }

exit:
    /* Free */
    if(fileBuf) {
        free(fileBuf);
    }
    if(sigBuf) {
        free(sigBuf);
    }
    wc_ecc_free(&eccKey);
    wc_FreeRng(&rng);

    return ret;
}
