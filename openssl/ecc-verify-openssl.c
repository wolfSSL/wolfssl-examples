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
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

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

int verify_ecc_signature(void)
{
    int ret = 0;
    EC_KEY *eckey = NULL;
    EC_GROUP *group = NULL;
    EC_POINT *point = NULL;
    ECDSA_SIG *signature = NULL;
    BIGNUM *sig_r = NULL, *sig_s = NULL;
    unsigned char digest[SHA256_DIGEST_LENGTH];
    
    /* Initialize OpenSSL */
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    
    /* Create EC_KEY with the P-256 curve */
    eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (eckey == NULL) {
        printf("Error: Failed to create EC_KEY\n");
        ERR_print_errors_fp(stderr);
        ret = -1;
        goto cleanup;
    }
    
    /* Get the group from the key */
    group = (EC_GROUP*)EC_KEY_get0_group(eckey);
    if (group == NULL) {
        printf("Error: Failed to get group\n");
        ERR_print_errors_fp(stderr);
        ret = -1;
        goto cleanup;
    }
    
    /* Create a new point */
    point = EC_POINT_new(group);
    if (point == NULL) {
        printf("Error: Failed to create EC_POINT\n");
        ERR_print_errors_fp(stderr);
        ret = -1;
        goto cleanup;
    }
    
    /* Set the point from the encoded public key */
    if (EC_POINT_oct2point(group, point, fixed_pubkey, sizeof(fixed_pubkey), NULL) != 1) {
        printf("Error: Failed to decode public key point\n");
        ERR_print_errors_fp(stderr);
        ret = -1;
        goto cleanup;
    }
    
    /* Set the public key */
    if (EC_KEY_set_public_key(eckey, point) != 1) {
        printf("Error: Failed to set public key\n");
        ERR_print_errors_fp(stderr);
        ret = -1;
        goto cleanup;
    }
    
    /* Create SHA-256 hash of the message */
    SHA256(fixed_data, sizeof(fixed_data), digest);
    
    /* Create signature object */
    signature = ECDSA_SIG_new();
    if (signature == NULL) {
        printf("Error: Failed to create ECDSA_SIG\n");
        ERR_print_errors_fp(stderr);
        ret = -1;
        goto cleanup;
    }
    
    /* Convert signature components from binary */
    sig_r = BN_bin2bn(fixed_signature, 32, NULL);
    sig_s = BN_bin2bn(fixed_signature + 32, 32, NULL);
    if (sig_r == NULL || sig_s == NULL) {
        printf("Error: Failed to convert signature binary to BIGNUM\n");
        ERR_print_errors_fp(stderr);
        ret = -1;
        goto cleanup;
    }
    
    /* Set signature components */
    if (ECDSA_SIG_set0(signature, sig_r, sig_s) != 1) {
        printf("Error: Failed to set signature components\n");
        ERR_print_errors_fp(stderr);
        BN_free(sig_r);
        BN_free(sig_s);
        ret = -1;
        goto cleanup;
    }
    
    /* sig_r and sig_s are now owned by signature, don't free them separately */
    sig_r = sig_s = NULL;
    
    /* Verify the signature */
    ret = ECDSA_do_verify(digest, SHA256_DIGEST_LENGTH, signature, eckey);
    if (ret == 1) {
        printf("Signature verification successful!\n");
        ret = 0; /* Success */
    } else if (ret == 0) {
        printf("Signature verification failed - invalid signature\n");
        ret = -1;
    } else {
        printf("Signature verification error\n");
        ERR_print_errors_fp(stderr);
        ret = -1;
    }
    
cleanup:
    /* Clean up */
    if (signature) ECDSA_SIG_free(signature);
    if (point) EC_POINT_free(point);
    if (eckey) EC_KEY_free(eckey);
    
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
