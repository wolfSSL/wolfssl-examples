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
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/obj_mac.h>

/* Fixed test data in hex format */
static const unsigned char fixed_data[] = {
    0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x6d, 
    0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 0x20, 0x66, 
    0x6f, 0x72, 0x20, 0x73, 0x69, 0x67, 0x6e, 0x69, 
    0x6e, 0x67
}; /* "sample message for signing" */

/* Fixed ECC public key in hex format (uncompressed format for P-256 curve) */
static const unsigned char fixed_pubkey[] = {
    0x04, 0x2a, 0x27, 0x06, 0x8b, 0x67, 0xb1, 0xfb, 
    0x7f, 0x87, 0xa0, 0x1b, 0x0a, 0xe8, 0xbe, 0x76, 
    0x04, 0x1e, 0x4b, 0xa2, 0x79, 0x77, 0x69, 0x73, 
    0x22, 0x72, 0x26, 0xe1, 0x14, 0xb5, 0x27, 0xf0, 
    0xb6, 0x25, 0x1a, 0xbd, 0x37, 0x85, 0x4c, 0x03, 
    0xae, 0xab, 0x72, 0x8d, 0x60, 0x87, 0x1c, 0xed, 
    0x3e, 0x3f, 0x7a, 0x66, 0xce, 0x0f, 0xbe, 0x28, 
    0x23, 0x7a, 0x6f, 0xf0, 0xf7, 0x6b, 0x36, 0xd5, 
    0x31
};

/* Fixed signature in hex format (R|S format) */
static const unsigned char fixed_signature[] = {
    /* R component */
    0xe6, 0xcf, 0x7a, 0x89, 0x9b, 0x7a, 0xdb, 0xec, 
    0x93, 0x9b, 0x0c, 0x19, 0xd5, 0x89, 0x16, 0x3d, 
    0x80, 0x1a, 0x03, 0x11, 0x59, 0x2a, 0xb0, 0x17, 
    0x3b, 0x9a, 0x3e, 0xbc, 0xd3, 0xe0, 0xd5, 0xb5,
    /* S component */
    0x19, 0xe8, 0x57, 0x95, 0x4b, 0xa2, 0xf7, 0xe1, 
    0xcb, 0x7f, 0x33, 0x8d, 0xdf, 0x27, 0xcf, 0x18, 
    0xa0, 0x6a, 0x19, 0x4d, 0x1d, 0xb6, 0x1f, 0x65, 
    0x6c, 0xde, 0x6c, 0x2c, 0x6c, 0xb4, 0x57, 0xbc
};

/* Helper function to print OpenSSL errors */
static void print_openssl_errors(void)
{
    unsigned long err;
    while ((err = ERR_get_error()) != 0) {
        char *err_str = ERR_error_string(err, NULL);
        fprintf(stderr, "OpenSSL error: %s\n", err_str);
    }
}

/* Convert raw signature (R|S format) to DER format for EVP API */
static int convert_rs_to_der(const unsigned char *rs_sig, size_t rs_len, 
                            unsigned char **der_sig, size_t *der_len)
{
    int ret = 0;
    ECDSA_SIG *sig = NULL;
    BIGNUM *r = NULL, *s = NULL;
    
    /* Create ECDSA_SIG structure */
    sig = ECDSA_SIG_new();
    if (sig == NULL) {
        fprintf(stderr, "Error: Failed to create ECDSA_SIG\n");
        print_openssl_errors();
        ret = -1;
        goto cleanup;
    }
    
    /* Convert R and S components to BIGNUMs */
    r = BN_bin2bn(rs_sig, rs_len/2, NULL);
    s = BN_bin2bn(rs_sig + rs_len/2, rs_len/2, NULL);
    if (r == NULL || s == NULL) {
        fprintf(stderr, "Error: Failed to convert signature to BIGNUMs\n");
        print_openssl_errors();
        ret = -1;
        goto cleanup;
    }
    
    /* Set R and S in the ECDSA_SIG structure */
    if (ECDSA_SIG_set0(sig, r, s) != 1) {
        fprintf(stderr, "Error: Failed to set R and S components\n");
        print_openssl_errors();
        BN_free(r);
        BN_free(s);
        ret = -1;
        goto cleanup;
    }
    
    /* r and s are now owned by sig, don't free them separately */
    r = s = NULL;
    
    /* Convert to DER format */
    *der_len = i2d_ECDSA_SIG(sig, der_sig);
    if (*der_len <= 0) {
        fprintf(stderr, "Error: Failed to convert signature to DER format\n");
        print_openssl_errors();
        ret = -1;
        goto cleanup;
    }
    
    ret = 0;
    
cleanup:
    if (sig) ECDSA_SIG_free(sig);
    if (r) BN_free(r);
    if (s) BN_free(s);
    
    return ret;
}

int verify_ecc_signature(void)
{
    int ret = 0;
    EVP_PKEY_CTX *ctx = NULL;
    EVP_PKEY *pkey = NULL;
    EVP_MD_CTX *md_ctx = NULL;
    unsigned char *der_sig = NULL;
    size_t der_sig_len = 0;
    
    /* Initialize OpenSSL */
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    
    /* Create a public key from the raw bytes */
    pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_EC, NULL, 
                                      fixed_pubkey, sizeof(fixed_pubkey));
    if (pkey == NULL) {
        /* If direct raw key import fails, try creating it from parameters */
        EVP_PKEY_CTX *param_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
        if (param_ctx == NULL) {
            printf("Error: Failed to create parameter context\n");
            print_openssl_errors();
            ret = -1;
            goto cleanup;
        }
        
        if (EVP_PKEY_paramgen_init(param_ctx) <= 0) {
            printf("Error: Failed to initialize parameter generation\n");
            print_openssl_errors();
            EVP_PKEY_CTX_free(param_ctx);
            ret = -1;
            goto cleanup;
        }
        
        if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(param_ctx, NID_X9_62_prime256v1) <= 0) {
            printf("Error: Failed to set curve parameters\n");
            print_openssl_errors();
            EVP_PKEY_CTX_free(param_ctx);
            ret = -1;
            goto cleanup;
        }
        
        EVP_PKEY *params = NULL;
        if (EVP_PKEY_paramgen(param_ctx, &params) <= 0) {
            printf("Error: Failed to generate parameters\n");
            print_openssl_errors();
            EVP_PKEY_CTX_free(param_ctx);
            ret = -1;
            goto cleanup;
        }
        
        EVP_PKEY_CTX_free(param_ctx);
        
        /* Create key context using the parameters */
        ctx = EVP_PKEY_CTX_new(params, NULL);
        if (ctx == NULL) {
            printf("Error: Failed to create key context\n");
            print_openssl_errors();
            EVP_PKEY_free(params);
            ret = -1;
            goto cleanup;
        }
        
        /* Import the public key using the EC point */
        EC_KEY *ec_key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        if (ec_key == NULL) {
            printf("Error: Failed to create EC_KEY\n");
            print_openssl_errors();
            EVP_PKEY_free(params);
            ret = -1;
            goto cleanup;
        }
        
        const EC_GROUP *group = EC_KEY_get0_group(ec_key);
        EC_POINT *point = EC_POINT_new(group);
        if (point == NULL) {
            printf("Error: Failed to create EC_POINT\n");
            print_openssl_errors();
            EC_KEY_free(ec_key);
            EVP_PKEY_free(params);
            ret = -1;
            goto cleanup;
        }
        
        if (EC_POINT_oct2point(group, point, fixed_pubkey, sizeof(fixed_pubkey), NULL) != 1) {
            printf("Error: Failed to decode public key point\n");
            print_openssl_errors();
            EC_POINT_free(point);
            EC_KEY_free(ec_key);
            EVP_PKEY_free(params);
            ret = -1;
            goto cleanup;
        }
        
        if (EC_KEY_set_public_key(ec_key, point) != 1) {
            printf("Error: Failed to set public key\n");
            print_openssl_errors();
            EC_POINT_free(point);
            EC_KEY_free(ec_key);
            EVP_PKEY_free(params);
            ret = -1;
            goto cleanup;
        }
        
        pkey = EVP_PKEY_new();
        if (pkey == NULL || EVP_PKEY_set1_EC_KEY(pkey, ec_key) != 1) {
            printf("Error: Failed to create EVP_PKEY from EC_KEY\n");
            print_openssl_errors();
            if (pkey) EVP_PKEY_free(pkey);
            EC_POINT_free(point);
            EC_KEY_free(ec_key);
            EVP_PKEY_free(params);
            ret = -1;
            goto cleanup;
        }
        
        EC_POINT_free(point);
        EC_KEY_free(ec_key);
        EVP_PKEY_free(params);
    }
    
    /* Convert the R|S signature format to DER format for EVP API */
    if (convert_rs_to_der(fixed_signature, sizeof(fixed_signature), 
                          &der_sig, &der_sig_len) != 0) {
        printf("Error: Failed to convert signature format\n");
        ret = -1;
        goto cleanup;
    }
    
    /* Create message digest context */
    md_ctx = EVP_MD_CTX_new();
    if (md_ctx == NULL) {
        printf("Error: Failed to create message digest context\n");
        print_openssl_errors();
        ret = -1;
        goto cleanup;
    }
    
    /* Initialize verification operation */
    if (EVP_DigestVerifyInit(md_ctx, NULL, EVP_sha256(), NULL, pkey) != 1) {
        printf("Error: Failed to initialize verification\n");
        print_openssl_errors();
        ret = -1;
        goto cleanup;
    }
    
    /* Update with the message data */
    if (EVP_DigestVerifyUpdate(md_ctx, fixed_data, sizeof(fixed_data)) != 1) {
        printf("Error: Failed to update verification\n");
        print_openssl_errors();
        ret = -1;
        goto cleanup;
    }
    
    /* Verify the signature */
    ret = EVP_DigestVerifyFinal(md_ctx, der_sig, der_sig_len);
    if (ret == 1) {
        printf("Signature verification successful!\n");
        ret = 0; /* Success */
    } else if (ret == 0) {
        printf("Signature verification failed - invalid signature\n");
        ret = -1;
    } else {
        printf("Signature verification error\n");
        print_openssl_errors();
        ret = -1;
    }
    
cleanup:
    /* Clean up */
    if (der_sig) OPENSSL_free(der_sig);
    if (md_ctx) EVP_MD_CTX_free(md_ctx);
    if (ctx) EVP_PKEY_CTX_free(ctx);
    if (pkey) EVP_PKEY_free(pkey);
    
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
    
    printf("Data to verify: \"sample message for signing\"\n\n");
    
    ret = verify_ecc_signature();
    
    printf("\nVerification result: %s (ret = %d)\n", 
           (ret == 0) ? "SUCCESS" : "FAILURE", ret);
    
    return (ret == 0) ? 0 : 1;
}
