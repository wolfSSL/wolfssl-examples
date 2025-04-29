#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

void print_buffer(const char* label, const unsigned char* buffer, size_t len) {
    printf("%s = {\n    ", label);
    for (size_t i = 0; i < len; i++) {
        printf("0x%02x", buffer[i]);
        if (i < len - 1) printf(", ");
        if ((i + 1) % 8 == 0 && i < len - 1) printf("\n    ");
    }
    printf("\n};\n");
}

int main(void) {
    int ret = 0;
    EC_KEY *key = NULL;
    ECDSA_SIG *signature = NULL;
    unsigned char *pubkey_buf = NULL;
    size_t pubkey_len = 0;
    const BIGNUM *sig_r = NULL, *sig_s = NULL;
    unsigned char r_buf[32], s_buf[32];
    unsigned char digest[SHA256_DIGEST_LENGTH];
    unsigned char message[] = "sample message for signing";
    
    /* Initialize OpenSSL */
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    
    /* Generate a new EC key pair */
    key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!key || !EC_KEY_generate_key(key)) {
        printf("Error generating EC key\n");
        ERR_print_errors_fp(stderr);
        ret = -1;
        goto cleanup;
    }
    
    /* Get the public key in uncompressed format */
    pubkey_len = EC_KEY_key2buf(key, POINT_CONVERSION_UNCOMPRESSED, &pubkey_buf, NULL);
    if (pubkey_len == 0) {
        printf("Error converting public key to buffer\n");
        ERR_print_errors_fp(stderr);
        ret = -1;
        goto cleanup;
    }
    
    /* Create SHA-256 hash of the message */
    SHA256(message, strlen((char*)message), digest);
    
    /* Sign the digest */
    signature = ECDSA_do_sign(digest, SHA256_DIGEST_LENGTH, key);
    if (!signature) {
        printf("Error creating signature\n");
        ERR_print_errors_fp(stderr);
        ret = -1;
        goto cleanup;
    }
    
    /* Get the signature components */
    ECDSA_SIG_get0(signature, &sig_r, &sig_s);
    
    /* Convert signature components to binary */
    if (BN_bn2binpad(sig_r, r_buf, sizeof(r_buf)) != sizeof(r_buf) ||
        BN_bn2binpad(sig_s, s_buf, sizeof(s_buf)) != sizeof(s_buf)) {
        printf("Error converting signature to binary\n");
        ERR_print_errors_fp(stderr);
        ret = -1;
        goto cleanup;
    }
    
    /* Print the test vectors */
    printf("/* Fixed test data in hex format */\n");
    print_buffer("static const unsigned char fixed_data[]", message, strlen((char*)message));
    printf("\n/* Fixed ECC public key in hex format (uncompressed format for P-256 curve) */\n");
    print_buffer("static const unsigned char fixed_pubkey[]", pubkey_buf, pubkey_len);
    printf("\n/* Fixed signature in hex format (R|S format) */\n");
    printf("static const unsigned char fixed_signature[] = {\n    /* R component */\n    ");
    for (size_t i = 0; i < sizeof(r_buf); i++) {
        printf("0x%02x", r_buf[i]);
        if (i < sizeof(r_buf) - 1) printf(", ");
        if ((i + 1) % 8 == 0 && i < sizeof(r_buf) - 1) printf("\n    ");
    }
    printf(",\n    /* S component */\n    ");
    for (size_t i = 0; i < sizeof(s_buf); i++) {
        printf("0x%02x", s_buf[i]);
        if (i < sizeof(s_buf) - 1) printf(", ");
        if ((i + 1) % 8 == 0 && i < sizeof(s_buf) - 1) printf("\n    ");
    }
    printf("\n};\n");
    
    /* Verify the signature to confirm it works */
    ret = ECDSA_do_verify(digest, SHA256_DIGEST_LENGTH, signature, key);
    printf("\nSignature verification %s (ret = %d)\n", 
           (ret == 1) ? "successful" : "failed", ret);
    
cleanup:
    if (pubkey_buf) OPENSSL_free(pubkey_buf);
    if (signature) ECDSA_SIG_free(signature);
    if (key) EC_KEY_free(key);
    
    /* Cleanup OpenSSL */
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();
    
    return (ret == 1) ? 0 : 1;
}
