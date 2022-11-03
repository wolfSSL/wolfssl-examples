/* sphincs_sign_verify.c
 *
 * Copyright (C) 2022 wolfSSL Inc.
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

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/sphincs.h>

#ifdef HAVE_LIBOQS

#define MAX_PEM_CERT_SIZE 70000
#define MAX_KEY_KEY_SIZE  256

/* You can use any of the following combinations, but make sure to use the
 * correct file names:
 * LEVEL = 1 | 3 | 5
 * VARIANT = FAST_VARIANT | SMALL_VARIANT */
#define LEVEL 5
#define VARIANT FAST_VARIANT
#define CERT_FILE "../certs/sphincs_fast_level5_entity_cert.pem"
#define KEY_FILE  "../certs/sphincs_fast_level5_entity_key.pem"

#define MESSAGE   "This message is protected with post-quantum cryptography!"

static void check_ret(char *func_name, int ret) {
    if (ret != 0) {
        fprintf(stderr, "ERROR: %s() returned %d\n", func_name, ret);
    }
}

int main(int argc, char** argv)
{
    int ret = 0;
    int verify_result = -1;
    FILE *file = NULL;

    byte pem_buf[MAX_PEM_CERT_SIZE];
    word32 pem_len = sizeof(pem_buf);

    byte priv_der_buf[MAX_KEY_KEY_SIZE];
    word32 priv_der_len = sizeof(priv_der_buf);

    byte cert_der_buf[MAX_PEM_CERT_SIZE];
    word32 cert_der_len = sizeof(cert_der_buf);

    byte pub_der_buf[MAX_KEY_KEY_SIZE];
    word32 pub_der_len = sizeof(pub_der_buf);

    byte signature[SPHINCS_MAX_SIG_SIZE];
    word32 signature_len  = sizeof(signature);

    WC_RNG rng;
    sphincs_key priv_key;
    sphincs_key pub_key;
    DecodedCert decodedCert;

    wc_InitRng(&rng);

    if (ret == 0) {
        ret = wc_sphincs_init(&priv_key);
        check_ret("wc_sphincs_init", ret);
    }

    if (ret == 0) {
        ret = wc_sphincs_init(&pub_key);
        check_ret("wc_sphincs_init", ret);
    }

    /* Get private key from key PEM file. */

    if (ret == 0) {
        ret = wc_sphincs_set_level_and_optim(&priv_key, LEVEL, VARIANT);
        check_ret("wc_sphincs_set_level_and_optim", ret);
    }

    if (ret == 0) {
        file = fopen(KEY_FILE, "rb");
        ret = fread(pem_buf, 1, sizeof(pem_buf), file);
        fclose(file);
        file = NULL;
        if (ret > 0) {
            pem_len = ret;
            ret = 0;
        } else {
            check_ret("fread", ret);
            ret = -1;
        }
    }

    if (ret == 0) {
        ret = wc_KeyPemToDer((const byte*)pem_buf, pem_len, 
                  priv_der_buf, priv_der_len, NULL);
        if (ret > 0) {
            priv_der_len = ret;
            ret = 0;
        } else {
            check_ret("wc_KeyPemToDer", ret);
            /* In case ret = 0. */
            ret = -1;
        }
    }

    if (ret == 0) {
        ret = wc_sphincs_import_private_only(priv_der_buf, priv_der_len,
                  &priv_key);
        check_ret("wc_sphincs_import_private_only", ret);
    }

    /* Get public key from certificate PEM file. */

    if (ret == 0) {
        ret = wc_sphincs_set_level_and_optim(&pub_key, LEVEL, VARIANT);
        check_ret("wc_sphincs_set_level_and_optim", ret);
    }

    if (ret == 0) {
        file = fopen(CERT_FILE, "rb");
        ret = fread(pem_buf, 1, sizeof(pem_buf), file);
        fclose(file);
        file = NULL;
        if (ret > 0) {
            pem_len = ret;
            ret = 0;
        } else {
            check_ret("fread", ret);
            ret = -1;
        }
    }

    if (ret == 0) {
        ret = wc_CertPemToDer((const byte*)pem_buf, pem_len, cert_der_buf,
                  cert_der_len, CERT_TYPE);
        if (ret > 0) {
            cert_der_len = ret;
            ret = 0;
        } else {
            check_ret("wc_CertPemToDer", ret);
            /* In case ret = 0. */
            ret = -1;
        }
    }

    if (ret == 0) {
        wc_InitDecodedCert(&decodedCert, cert_der_buf, cert_der_len, 0);
        ret = wc_ParseCert(&decodedCert, CERT_TYPE, NO_VERIFY, NULL);
        check_ret("ParseCert", ret);
    }

    if (ret == 0) {
        ret = wc_GetPubKeyDerFromCert(&decodedCert, pub_der_buf,
                  &pub_der_len);
        check_ret("wc_GetPubKeyDerFromCert", ret);
    }

    if (ret == 0) {
        ret = wc_sphincs_import_public(pub_der_buf, pub_der_len, &pub_key);
        check_ret("wc_sphincs_import_public", ret);
    }

    /* We now have the public and private key. Time to sign and verify the
     * message. */

    if (ret == 0) {
        ret = wc_sphincs_sign_msg((const byte *)MESSAGE, sizeof(MESSAGE),
                  signature, &signature_len, &priv_key);
        check_ret("wc_sphincs_sign_msg", ret);
    }

    if (ret == 0) {
       ret = wc_sphincs_verify_msg(signature, signature_len,
                 (const byte *)MESSAGE, sizeof(MESSAGE), &verify_result,
                 &pub_key);
       check_ret("wc_sphincs_verify_msg", ret);
    }

    printf("verify result: %s\n", verify_result == 1 ? "SUCCESS" : "FAILURE");

    wc_FreeDecodedCert(&decodedCert);
    wc_sphincs_free(&priv_key);
    wc_sphincs_free(&pub_key);
    wc_FreeRng(&rng);
    wolfCrypt_Cleanup();

    return ret;
}

#else

int main(int argc, char** argv) {
    printf("This requires the --with-liboqs flag.\n");
    return 0;
}
#endif /* WITH_LIBOQS */

