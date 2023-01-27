/* wolfcrypt_key_cert_insert.c
 *
 * Copyright (C) 2006-2023 wolfSSL Inc.
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

/*
 * Example app inserts and uses keys and certificates into SE050.
 *
 * 1. Inserts sample RSA and ECC certs and keys into SE050 key IDs.
 *    Sample keys and certs used are from wolfSSL's <wolfssl/certs_test.h>
 *    header and are demo certs/keys that ship with wolfSSL:
 *         server_key_der_2048    RSA private key (2048-bit)
 *         server_cert_der_2048   RSA cert (2048-bit), matches above key
 *         public key from above  RSA public key extracted from above cert
 *         ecc_key_der_256        ECC private key (NIST P-256)
 *         serv_ecc_der_256       ECC cert (NIST P-256), matches above key
 *         public key from above  ECC public key extracted from above cert
 * 2. Does an ECDSA verify using one of the ECC public keys and a
 *    ECDSA signature in the form of ASCII R and S components.
 * 3. Reads back out the RSA and ECC certificate inserted, verifies
 *    they match the original inserted.
 * 4. Erases the keys and certs from the SE050.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/certs_test.h>
#include <wolfssl/wolfcrypt/types.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/port/nxp/se050_port.h>

#include <ex_sss_boot.h>
#include <fsl_sss_se05x_apis.h>
#include <nxLog_App.h>

#ifdef __cplusplus
}
#endif

#if defined(SIMW_DEMO_ENABLE__DEMO_WOLFCRYPT_KEY_CERT_INSERT)

static ex_sss_boot_ctx_t gex_sss_boot_ctx;

#define EX_SSS_BOOT_PCONTEXT (&gex_sss_boot_ctx)
#define EX_SSS_BOOT_DO_ERASE 1
#define EX_SSS_BOOT_EXPOSE_ARGC_ARGV 0

#define KEYID_RSA_PRIVATE  0x00005002 /* stores server_key_der_2048 */
#define KEYID_RSA_CERT     0x00005003 /* stores server_cert_der_2048 */
#define KEYID_RSA_PUBLIC   0x00005004 /* stores server_cert_der_2048 pub key */
#define KEYID_ECC_PRIVATE  0x00005005 /* stores ecc_key_der_256 */
#define KEYID_ECC_CERT     0x00005006 /* stores serv_ecc_der_256 */
#define KEYID_ECC_PUBLIC   0x00005007 /* stores serv_ecc_der_256 public key */

#include <ex_sss_main_inc.h>

static int se050_verify_ecc_sig_rs(const char* sigR, const char* sigS,
                                   byte* digest, word32 digestSz, word32 keyId)
{
    int ret = 0;
    int res = 0;
    byte eccSig[ECC_MAX_SIG_SIZE];
    word32 eccSigSz;
    ecc_key eccKey;

    if (sigR == NULL || sigS == NULL || digest == NULL || digestSz == 0) {
        return -1;
    }

    eccSigSz = sizeof(eccSig);
    memset(eccSig, 0, eccSigSz);

    ret = wc_ecc_rs_to_sig(sigR, sigS, eccSig, &eccSigSz);
    if (ret != 0) {
        LOG_I("wc_ecc_rs_to_sig() failed, ret = %d\n", ret);
    }

    /* Create ecc_key struct with SE050 key ID */
    if (ret == 0) {
        ret = wc_ecc_init(&eccKey);
        if (ret != 0) {
            LOG_I("wc_ecc_init error, ret = %d\n", ret);
        }
    }

    if (ret == 0) {
        LOG_I("Setting ecc_key to use ID from SE050\n");
        ret = wc_ecc_use_key_id(&eccKey, keyId, 0);
        if (ret != 0) {
            LOG_I("wc_ecc_use_key_id error, ret = %d\n", ret);
        }
    }

    if (ret == 0) {
        LOG_I("Verifying ECDSA signature with SE050 key ID: 0x%08x\n", keyId);
        ret = wc_ecc_verify_hash(eccSig, eccSigSz, digest, digestSz, &res,
                                 &eccKey);
        if (ret != 0 || res != 1) {
            LOG_I("wc_ecc_verify_hash failed, ret = %d\n", ret);
        }
    }

    if (ret == 0) {
        if (res != 1) {
            LOG_I("ECDSA verify failed!\n");
        } else {
            LOG_I("ECDSA verify success!\n");
        }
    }

    /* Free ECC key and RNG */
    if (ret == 0) {
        ret = wc_ecc_free(&eccKey);
        if (ret != 0) {
            LOG_I("wc_ecc_free error, ret = %d\n", ret);
        }
    }

    return ret;
}

/**
 * Insert sample certs/keys into SE050 key IDs:
 *
 * Key ID             Credential
 * ------             --------------------------------
 * KEYID_RSA_PRIVATE  RSA private key
 * KEYID_RSA_CERT     RSA certificate
 * KEYID_ECC_PRIVATE  ECC private key
 * KEYID_ECC_CERT     ECC certificate
 * KEYID_ECC_PUBLIC   ECC public key (from cert above)
 *
 * Return 0 on success, negative on error.
 */
static int write_certs_keys_to_se050(void)
{
    int ret = 0;

    DecodedCert cert;
    byte derKey[1024];
    word32 derKeySz = 0;
    word32 idx = 0;
    RsaKey rsaKey;

    /* Insert RSA private key */
    ret = wc_se050_rsa_insert_private_key(KEYID_RSA_PRIVATE,
                                          server_key_der_2048,
                                          sizeof_server_key_der_2048);
    if (ret != 0) {
        LOG_I("Error in se050_rsa_insert_private_key, ret = %d\n", ret);
    } else {
        LOG_I("Inserted RSA private key into SE050 key ID: 0x%08x\n",
              KEYID_RSA_PRIVATE);
    }

    /* Insert RSA certificate */
    ret = wc_se050_insert_binary_object(KEYID_RSA_CERT,
                                        server_cert_der_2048,
                                        sizeof_server_cert_der_2048);
    if (ret != 0) {
        LOG_I("Error in wc_se050_insert_binary_object, ret = %d\n", ret);
    } else {
        LOG_I("Inserted RSA certificate into SE050 key ID: 0x%08x\n",
              KEYID_RSA_CERT);
    }

    /* Extract RSA public key from cert DER */
    if (ret == 0) {
        wc_InitDecodedCert(&cert, server_cert_der_2048,
                           sizeof_server_cert_der_2048, NULL);

        ret = wc_ParseCert(&cert, CERT_TYPE, NO_VERIFY, NULL);
        if (ret != 0) {
            LOG_I("wc_ParseCert failed, ret = %d\n", ret);
        }
        else {
            derKeySz = sizeof(derKey);
            ret = wc_GetPubKeyDerFromCert(&cert, derKey, &derKeySz);
            if (ret != 0) {
                LOG_I("wc_GetPubKeyDerFromCert failed, ret = %d\n", ret);
            }
        }
        wc_FreeDecodedCert(&cert);
    }

    /* Convert RSA public key DER to format for SE050, needs header added */
    if (ret == 0) {
        ret = wc_InitRsaKey(&rsaKey, NULL);
        if (ret != 0) {
            LOG_I("wc_InitRsaKey failed, ret = %d\n", ret);
        }
        else {
            idx = 0;
            ret = wc_RsaPublicKeyDecode(derKey, &idx, &rsaKey, derKeySz);
            if (ret != 0) {
                LOG_I("wc_RsaPublicKeyDecode failed, ret = %d\n", ret);
            }
            else {
                derKeySz = sizeof(derKey);
                ret = wc_RsaKeyToPublicDer_ex(&rsaKey, derKey, derKeySz, 1);
                if (ret <= 0) {
                    LOG_I("wc_RsaKeyToDer failed, ret = %d\n", ret);
                } else {
                    LOG_I("RSA key written to DER, %d bytes\n", ret);
                    derKeySz = ret;
                    ret = 0;
                }
            }   
        }
        wc_FreeRsaKey(&rsaKey);
    }

    /* Insert RSA public key from cert into SE050 */
    if (ret == 0) {
        ret = wc_se050_rsa_insert_public_key(KEYID_RSA_PUBLIC, derKey, derKeySz);
        if (ret != 0) {
            LOG_I("Error in se050_rsa_insert_public_key, ret = %d\n", ret);
        } else {
            LOG_I("Inserted RSA public key into SE050 key ID: 0x%08x\n",
                  KEYID_RSA_PUBLIC);
        }
    }

    /* Insert ECC private key */
    ret = wc_se050_ecc_insert_private_key(KEYID_ECC_PRIVATE, ecc_key_der_256,
                                          sizeof_ecc_key_der_256);
    if (ret != 0) {
        LOG_I("Error in se050_ecc_insert_private_key, ret = %d\n", ret);
    } else {
        LOG_I("Inserted ECC private key into SE050 key ID: 0x%08x\n",
              KEYID_ECC_PRIVATE);
    }

    /* Insert ECC certificate */
    ret = wc_se050_insert_binary_object(KEYID_ECC_CERT, serv_ecc_der_256,
                                        sizeof_serv_ecc_der_256);
    if (ret != 0) {
        LOG_I("Error in wc_se050_insert_binary_object, ret = %d\n", ret);
    } else {
        LOG_I("Inserted ECC certificate into SE050 key ID: 0x%08x\n",
              KEYID_ECC_CERT);
    }

    /* Extract ECC public key from cert DER */
    if (ret == 0) {
        wc_InitDecodedCert(&cert, serv_ecc_der_256,
                           sizeof_serv_ecc_der_256, NULL);

        ret = wc_ParseCert(&cert, CERT_TYPE, NO_VERIFY, NULL);
        if (ret != 0) {
            LOG_I("wc_ParseCert failed, ret = %d\n", ret);
        }
        else {
            derKeySz = sizeof(derKey);
            ret = wc_GetPubKeyDerFromCert(&cert, derKey, &derKeySz);
            if (ret != 0) {
                LOG_I("wc_GetPubKeyDerFromCert failed, ret = %d\n", ret);
            }
        }
        wc_FreeDecodedCert(&cert);
    }

    /* Insert ECC public key from cert into SE050 */
    if (ret == 0) {
        ret = wc_se050_ecc_insert_public_key(KEYID_ECC_PUBLIC, derKey,
                                             derKeySz);
        if (ret != 0) {
            LOG_I("Error in se050_ecc_insert_public_key, ret = %d\n", ret);
        } else {
            LOG_I("Inserted ECC public key into SE050 key ID: 0x%08x\n",
                  KEYID_ECC_PUBLIC);
        }
    }

    return ret;
}

/**
 * Test reading certificates back out of SE050.
 *
 * Read sample certificates stored into the following key IDs:
 *   - KEYID_RSA_CERT
 *   - KEYID_ECC_CERT
 *
 * Return 0 on success, negative on error.
 */
static int read_certs_from_se050(void)
{
    int err = 0;
    int ret = -1;

    byte* tmp = NULL;
    word32 tmpSz = 0;

    /* get object size, to allocate memory */
    ret = wc_se050_get_binary_object(KEYID_RSA_CERT, tmp, &tmpSz);
    if (ret != LENGTH_ONLY_E) {
        LOG_I("wc_se050_get_binary_object did not return length only "
              "like expected\n");
        err = 1;
    }

    if (err == 0) {
        /* allocate memory to read RSA cert */
        tmp = (byte*)XMALLOC(tmpSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        if (tmp == NULL) {
            LOG_I("Failed to allocate memory to read RSA cert\n");
            err = 1;
        }
    }

    if (err == 0) {
        /* read RSA cert into tmp buffer */
        XMEMSET(tmp, 0, tmpSz);
        ret = wc_se050_get_binary_object(KEYID_RSA_CERT, tmp, &tmpSz);
        if (ret != 0) {
            LOG_I("wc_se050_get_binary_object failed, ret = %d\n", ret);
            err = 1;
        } else {
            LOG_I("Read RSA certificate (%d bytes)\n", tmpSz);
        }
    }

    if (err == 0) {
        /* make sure RSA cert is same as one we originally wrote */
        if ((tmpSz != sizeof_server_cert_der_2048) ||
            (memcmp(tmp, server_cert_der_2048, tmpSz) != 0)) {
            LOG_I("Error: RSA cert differs from original!\n");
            err = 1;
        } else {
            LOG_I("RSA cert matches original loaded\n");
        }
    }

    if (err == 0) {
        XFREE(tmp, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        tmp = NULL;
        tmpSz = 0;

        ret = wc_se050_get_binary_object(KEYID_ECC_CERT, tmp, &tmpSz);
        if (ret != LENGTH_ONLY_E) {
            LOG_I("wc_se050_get_binary_object did not return length only "
                  "like expected\n");
            err = 1;
        }
    }

    if (err == 0) {
        /* allocate memory to read ECC cert */
        tmp = (byte*)XMALLOC(tmpSz, NULL, DYNAMIC_TYPE_TMP_BUFFER);
        if (tmp == NULL) {
            LOG_I("Failed to allocate memory to read ECC cert\n");
            err = 1;
        }
    }

    if (err == 0) {
        /* read ECC cert into tmp buffer */
        XMEMSET(tmp, 0, tmpSz);
        ret = wc_se050_get_binary_object(KEYID_ECC_CERT, tmp, &tmpSz);
        if (ret != 0) {
            LOG_I("wc_se050_get_binary_object failed, ret = %d\n", ret);
            err = 1;
        } else {
            LOG_I("Read ECC certificate (%d bytes)\n", tmpSz);
        }
    }

    if (err == 0) {
        /* make sure ECC cert is same as one we originally wrote */
        if ((tmpSz != sizeof_serv_ecc_der_256) ||
            (memcmp(tmp, serv_ecc_der_256, tmpSz) != 0)) {
            LOG_I("Error: ECC cert differs from original!\n");
            err = 1;
        } else {
            LOG_I("ECC cert matches original loaded\n");
        }
    }

    if (tmp != NULL) {
        XFREE(tmp, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }

    if (err == 0) {
        ret = 0;
    }

    return ret;
}

/**
 * Erase sample certs/keys from SE050
 *
 * Returns 0 on success, negative on error.
 */
static int erase_certs_keys_from_se050(void)
{
    int ret = 0;

    ret = wc_se050_erase_object(KEYID_RSA_PRIVATE);
    if (ret != 0) {
        LOG_I("Failed to erase RSA private key, ret = %d\n", ret);
    } else {
        LOG_I("Erased RSA private key, key ID: 0x%08x\n", KEYID_RSA_PRIVATE);
    }
    ret = wc_se050_erase_object(KEYID_RSA_CERT);
    if (ret != 0) {
        LOG_I("Failed to erase RSA certificate, ret = %d\n", ret);
    } else {
        LOG_I("Erased RSA certificate, key ID: 0x%08x\n", KEYID_RSA_CERT);
    }
    ret = wc_se050_erase_object(KEYID_RSA_PUBLIC);
    if (ret != 0) {
        LOG_I("Failed to erase RSA public key, ret = %d\n", ret);
    } else {
        LOG_I("Erased RSA public key, key ID: 0x%08x\n", KEYID_RSA_PUBLIC);
    }
    ret = wc_se050_erase_object(KEYID_ECC_PRIVATE);
    if (ret != 0) {
        LOG_I("Failed to erase ECC private key, ret = %d\n", ret);
    } else {
        LOG_I("Erased ECC private key, key ID: 0x%08x\n", KEYID_ECC_PRIVATE);
    }
    ret = wc_se050_erase_object(KEYID_ECC_CERT);
    if (ret != 0) {
        LOG_I("Failed to erase ECC certificate, ret = %d\n", ret);
    } else {
        LOG_I("Erased ECC certificate, key ID: 0x%08x\n", KEYID_ECC_CERT);
    }
    ret = wc_se050_erase_object(KEYID_ECC_PUBLIC);
    if (ret != 0) {
        LOG_I("Failed to erase ECC public key, ret = %d\n", ret);
    } else {
        LOG_I("Erased ECC public key, key ID: 0x%08x\n", KEYID_ECC_PUBLIC);
    }

    return ret;
}

sss_status_t ex_sss_entry(ex_sss_boot_ctx_t *pCtx)
{
    int ret = 0;
    sss_status_t status = kStatus_SSS_Success;
    sss_session_t *pSession = (sss_session_t*)&pCtx->session;
    sss_key_store_t *pKeyStore = (sss_key_store_t*)&pCtx->ks;

    const char* sigR =
        "ccfaa12a0aa1b9aa9f1fd107865c8ab5b21f36ce6ca8fe5b2c8154280009f74e";
    const char* sigS =
        "c09defd7a7c67a61ce1099dceaf840054351e199bf1a405675bf3ccb1939b024";

    /* Test "hash" value, used to pre-generate the above R,S signature */
    const byte hashData[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    };

    LOG_I("Running wc_se050_set_config()");
    ret = wc_se050_set_config(pSession, NULL, pKeyStore);
    if (ret != 0) {
        LOG_E("wc_se050_set_config failed");
        return kStatus_SSS_Fail;
    }
    LOG_I("SE050 config successfully set in wolfSSL");

    /* Initialize wolfSSL library */
    wolfSSL_Init();

    /* Write sample keys/certs to SE050 */
    ret = write_certs_keys_to_se050();
    if (ret != 0) {
        LOG_E("Failed to write sample keys/certs to SE050\n");
    }

    /* Test verifying ECDSA {r,s} signature with KEYID_ECC_PUBLIC.
     * NOTE: The R and S values here were pre-generated using wolfSSL
     * and the private ECC key named "ecc_key_der_256" in the
     * <wolfssl/certs_test.h> header. The certificate which wraps the
     * public key for "ecc_key_der_256" is "serv_ecc_der_256". The public
     * key from that certificate has been extracted and stored into key ID
     * KEYID_ECC_PUBLIC. */
    if (ret == 0) {
        ret = se050_verify_ecc_sig_rs(sigR, sigS, (byte*)hashData,
                                      sizeof(hashData), KEYID_ECC_PUBLIC);
    }

    /* Test reading back certs from SE050 */
    if (ret == 0) {
        ret = read_certs_from_se050();
    }

    /* Erase sample keys/certs from SE050 */
    ret = erase_certs_keys_from_se050();
    if (ret != 0) {
        LOG_E("Failed to erase sample certs/keys from SE050\n");
    }

    /* Cleanup wolfSSL library resources */
    wolfSSL_Cleanup();

    LOG_I("Done with sample app");
    return status;
}

#endif /* SIMW_DEMO_ENABLE__DEMO_WOLFCRYPT_KEY_CERT_INSERT */
