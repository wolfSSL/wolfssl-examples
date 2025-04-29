/* ecc-verify-openssl.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/asn1.h>
#include <openssl/params.h>
#include <openssl/core_names.h>
#include <openssl/param_build.h>

/* Helper function to convert hex string to binary */
static int hex2bin(const char *hex, unsigned char *bin, int bin_size)
{
    int i;
    int hex_len = strlen(hex);

    if (hex_len % 2 != 0) {
        fprintf(stderr, "Error: Hex string must have even length\n");
        return -1;
    }

    if (bin_size < hex_len / 2) {
        fprintf(stderr, "Error: Binary buffer too small\n");
        return -1;
    }

    for (i = 0; i < hex_len / 2; i++) {
        unsigned char byte = 0;
        int j;

        for (j = 0; j < 2; j++) {
            char c = hex[i * 2 + j];
            byte <<= 4;

            if (c >= '0' && c <= '9')
                byte |= c - '0';
            else if (c >= 'a' && c <= 'f')
                byte |= c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                byte |= c - 'A' + 10;
            else {
                fprintf(stderr, "Error: Invalid hex character: %c\n", c);
                return -1;
            }
        }

        bin[i] = byte;
    }

    return hex_len / 2;
}

/* Fixed test data in hex format (pre-computed hash) */
static const char *fixed_data_hex =
    "E7BD43AEE750B0DA4B3F537E152C4A7407764037A57447CACD2096A5D60994BA";

/* Fixed ECC public key in hex format (uncompressed format for P-256 curve) */
static const char *fixed_pubkey_hex =
    "042FBFB32C9E2890FD742D991FD43B889191791E18337AD408CB12CA79325E88DD"
    "7D20A5182318EC9BD696DDF41128D8BF1A137D2446C7A5AF6AA60AFEF3E8610E";

/* Fixed signature in hex format (DER format) */
static const char *fixed_signature_hex =
    "30450220763B61459CF2FC3E821053702CC24C0E378C976FC6F83F0F0FF05BD85203B958"
    "022100A40A0A3AF50769EE740CBA8F9B5F4530F5E5A93EDEBBAB14A1F3BECE93FB5A47";

/* Sample message that was signed (for display purposes) */
static const char *sample_message = "sample message for signing";

/* Helper function to print OpenSSL errors */
static void print_openssl_errors(void)
{
    unsigned long err;
    while ((err = ERR_get_error()) != 0) {
        char *err_str = ERR_error_string(err, NULL);
        fprintf(stderr, "OpenSSL error: %s\n", err_str);
    }
}

/* Helper function to print binary data in hex format */
static void print_hex(const char *label, const unsigned char *data, size_t len)
{
    size_t i;

    printf("%s (length=%zu):\n", label, len);
    for (i = 0; i < len; i++) {
        printf("%02X", data[i]);
        if ((i + 1) % 16 == 0 || i == len - 1)
            printf("\n");
        else if ((i + 1) % 8 == 0)
            printf("  ");
        else
            printf(" ");
    }
    printf("\n");
}

int verify_ecc_signature(void)
{
    int ret = 0;
    int err = 0;
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX *ctx = NULL;
    EVP_PKEY_CTX *vctx = NULL;
    OSSL_PARAM_BLD *bld = NULL;
    OSSL_PARAM *params = NULL;
    unsigned char fixed_data[128];
    unsigned char fixed_pubkey[128];
    unsigned char fixed_signature[128];
    int data_len, pubkey_len, sig_len;
    
    /* Convert hex strings to binary */
    data_len = hex2bin(fixed_data_hex, fixed_data, sizeof(fixed_data));
    if (data_len < 0) {
        printf("Error: Failed to convert data hex string\n");
        ret = -1;
        goto cleanup;
    }
    
    pubkey_len = hex2bin(fixed_pubkey_hex, fixed_pubkey, sizeof(fixed_pubkey));
    if (pubkey_len < 0) {
        printf("Error: Failed to convert pubkey hex string\n");
        ret = -1;
        goto cleanup;
    }
    
    sig_len = hex2bin(fixed_signature_hex, fixed_signature, sizeof(fixed_signature));
    if (sig_len < 0) {
        printf("Error: Failed to convert signature hex string\n");
        ret = -1;
        goto cleanup;
    }
    
    /* Print the binary data for verification */
    printf("Data to verify: \"%s\"\n\n", sample_message);
    print_hex("Data (pre-computed hash)", fixed_data, data_len);
    print_hex("Public Key (uncompressed format)", fixed_pubkey, pubkey_len);
    print_hex("Signature (DER format)", fixed_signature, sig_len);
    
    /* Initialize OpenSSL */
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    
    /* Create parameter builder for the key */
    err = (bld = OSSL_PARAM_BLD_new()) == NULL;
    if (err) {
        printf("Error: Failed to create parameter builder\n");
        print_openssl_errors();
        ret = -1;
        goto cleanup;
    }
    
    /* Add curve name parameter */
    if (err == 0) {
        err = OSSL_PARAM_BLD_push_utf8_string(bld, OSSL_PKEY_PARAM_GROUP_NAME,
                                           "prime256v1", 0) != 1;
        if (err) {
            printf("Error: Failed to add curve name parameter\n");
            print_openssl_errors();
            ret = -1;
            goto cleanup;
        }
    }
    
    /* Add public key parameter */
    if (err == 0) {
        err = OSSL_PARAM_BLD_push_octet_string(bld, OSSL_PKEY_PARAM_PUB_KEY,
                                            fixed_pubkey, pubkey_len) != 1;
        if (err) {
            printf("Error: Failed to add public key parameter\n");
            print_openssl_errors();
            ret = -1;
            goto cleanup;
        }
    }
    
    /* Build parameters */
    if (err == 0) {
        err = (params = OSSL_PARAM_BLD_to_param(bld)) == NULL;
        if (err) {
            printf("Error: Failed to build parameters\n");
            print_openssl_errors();
            ret = -1;
            goto cleanup;
        }
    }
    
    /* Create a context for key creation */
    if (err == 0) {
        err = (ctx = EVP_PKEY_CTX_new_from_name(NULL, "EC", NULL)) == NULL;
        if (err) {
            printf("Error: Failed to create key context\n");
            print_openssl_errors();
            ret = -1;
            goto cleanup;
        }
    }
    
    /* Initialize the context for fromdata operation */
    if (err == 0) {
        err = EVP_PKEY_fromdata_init(ctx) != 1;
        if (err) {
            printf("Error: Failed to initialize fromdata operation\n");
            print_openssl_errors();
            ret = -1;
            goto cleanup;
        }
    }
    
    /* Create the key from parameters */
    if (err == 0) {
        err = EVP_PKEY_fromdata(ctx, &pkey, EVP_PKEY_KEYPAIR, params) != 1;
        if (err) {
            printf("Error: Failed to create key from data\n");
            print_openssl_errors();
            ret = -1;
            goto cleanup;
        }
    }
    
    /* Create a verification context */
    vctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (vctx == NULL) {
        printf("Error: Failed to create verification context\n");
        print_openssl_errors();
        ret = -1;
        goto cleanup;
    }
    
    /* Initialize for verification */
    if (EVP_PKEY_verify_init(vctx) <= 0) {
        printf("Error: Failed to initialize verification\n");
        print_openssl_errors();
        ret = -1;
        goto cleanup;
    }
    
    /* Set the digest algorithm */
    if (EVP_PKEY_CTX_set_signature_md(vctx, EVP_sha256()) <= 0) {
        printf("Error: Failed to set digest algorithm\n");
        print_openssl_errors();
        ret = -1;
        goto cleanup;
    }
    
    /* Verify the signature using the pre-computed hash directly */
    printf("\nVerifying signature using EVP_PKEY_verify with pre-computed hash...\n");
    ret = EVP_PKEY_verify(vctx, fixed_signature, sig_len, fixed_data, data_len);
    if (ret == 1) {
        printf("Signature verification successful!\n");
        ret = 0; /* Success */
    } else if (ret == 0) {
        printf("Signature verification failed - invalid signature\n");
        print_openssl_errors();
        ret = -1;
    } else {
        printf("Signature verification error\n");
        print_openssl_errors();
        ret = -1;
    }
    
cleanup:
    /* Clean up */
    if (vctx) EVP_PKEY_CTX_free(vctx);
    if (ctx) EVP_PKEY_CTX_free(ctx);
    if (pkey) EVP_PKEY_free(pkey);
    if (params) OSSL_PARAM_free(params);
    if (bld) OSSL_PARAM_BLD_free(bld);
    
    /* Cleanup OpenSSL */
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();
    
    return ret;
}

int main(void)
{
    int ret;

    printf("OpenSSL ECC Signature Verification Example\n");
    printf("Using fixed test values for public key, signature, and data\n\n");

    ret = verify_ecc_signature();

    printf("\nVerification result: %s (ret = %d)\n",
           (ret == 0) ? "SUCCESS" : "FAILURE", ret);

    return (ret == 0) ? 0 : 1;
}
