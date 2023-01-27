/* wolfcrypt_generate_csr.c 
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

/* wolfCrypt example that generates a CSR using key generated in SE050. */

#ifdef __cplusplus
extern "C" {
#endif

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/port/nxp/se050_port.h>
#include <wolfssl/ssl.h>

#include <ex_sss_boot.h>
#include <fsl_sss_se05x_apis.h>
#include <nxLog_App.h>

#ifdef __cplusplus
}
#endif

#if defined(SIMW_DEMO_ENABLE__DEMO_WOLFCRYPT_GENERATE_CSR)

static ex_sss_boot_ctx_t gex_sss_boot_ctx;

#define EX_SSS_BOOT_PCONTEXT (&gex_sss_boot_ctx)
#define EX_SSS_BOOT_DO_ERASE 1
#define EX_SSS_BOOT_EXPOSE_ARGC_ARGV 0

#include <ex_sss_main_inc.h>

sss_status_t ex_sss_entry(ex_sss_boot_ctx_t *pCtx)
{
#if defined(WOLFSSL_CERT_GEN) && defined(WOLFSSL_CERT_REQ)
    int ret = 0;
    sss_status_t status = kStatus_SSS_Success;
    sss_session_t *pSession = (sss_session_t*)&pCtx->session;
    sss_key_store_t *pKeyStore = (sss_key_store_t*)&pCtx->ks;

    word32 keyId = 0;
    WC_RNG rng;
    ecc_key ecc;
    Cert req;
    byte der[1024];
    byte pem[1024];
    int derSz;
    int pemSz;

    LOG_I("Running wc_se050_set_config()");
    ret = wc_se050_set_config(pSession, NULL, pKeyStore);
    if (ret != 0) {
        LOG_E("wc_se050_set_config failed");
        return kStatus_SSS_Fail;
    }
    LOG_I("SE050 config successfully set in wolfSSL");

    /* Initialize wolfSSL library */
    wolfSSL_Init();

    LOG_I("wolfCrypt example of CSR generation\n");

    XMEMSET(der, 0, sizeof(der));
    XMEMSET(pem, 0, sizeof(pem));

    LOG_I("Initializing RNG\n");
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        LOG_I("wc_RngInit() error, ret = %d\n", ret);
    }

    /* Generate ECC key, stored in SE050 */
    if (ret == 0) {
        LOG_I("Generating ECC key in SE050\n");
        ret = wc_ecc_init(&ecc);
        if (ret != 0) {
            LOG_I("wc_ecc_init error, ret = %d\n", ret);
        }
    }

    if (ret == 0) {
        ret = wc_ecc_make_key(&rng, 32, &ecc);
        if (ret != 0) {
            LOG_I("wc_ecc_make_key error, ret = %d\n", ret);
        } else {
            ret = wc_ecc_get_key_id(&ecc, &keyId);
            if (ret != 0) {
                LOG_I("wc_ecc_get_key_id error, ret = %d\n", ret);
            } else {
                LOG_I("Generated ECC key in SE050 ID: 0x%08x\n", keyId);
            }
        }
    }

    if (ret == 0) {
        ret = wc_InitCert(&req);
        if (ret != 0) {
            LOG_I("wc_InitCert error, ret = %d\n", ret);
        }
    }

    if (ret == 0) {
        strncpy(req.subject.country, "US", CTC_NAME_SIZE);
        strncpy(req.subject.state, "MT", CTC_NAME_SIZE);
        strncpy(req.subject.locality, "Bozeman", CTC_NAME_SIZE);
        strncpy(req.subject.org, "Test Org", CTC_NAME_SIZE);
        strncpy(req.subject.unit, "Development", CTC_NAME_SIZE);
        strncpy(req.subject.commonName, "www.example.com", CTC_NAME_SIZE);
        strncpy(req.subject.email, "info@example.com", CTC_NAME_SIZE);

        ret = wc_MakeCertReq_ex(&req, der, sizeof(der), ECC_TYPE,
                                (void*)&ecc);
        if (ret <= 0) {
            LOG_I("wc_MakeCertReq_ex error, ret = %d\n", ret);
        } else {
            derSz = ret;
            ret = 0;
        }
    }

    if (ret == 0) {
        ret = wc_SignCert_ex(req.bodySz, req.sigType, der, sizeof(der),
                             ECC_TYPE, (void*)&ecc, &rng);
        if (ret <= 0) {
            LOG_I("wc_SignCert_ex error, ret = %d\n", ret);
        } else {
            derSz = ret;
            ret = 0;
        }
    }

    if (ret == 0) {
        XMEMSET(pem, 0, sizeof(pem));
        ret = wc_DerToPem(der, derSz, pem, sizeof(pem), CERTREQ_TYPE);
        if (ret <= 0) {
            LOG_I("wc_DerToPem error, ret = %d\n", ret);
        } else {
            pemSz = ret;
            LOG_I("Generated CSR (%d bytes)\n", pemSz);
            ret = 0;
            LOG_I("%s", pem);
        }
    }


    /* Delete generated key in SE050 */
    if (ret == 0) {
        LOG_I("Erasing ECC key stored in SE050 slot: 0x%08x\n", keyId);
        ret = wc_se050_erase_object(keyId);
        if (ret != 0) {
            LOG_I("Failed to erase ECC key in SE050\n");
        }
    }

    /* Free ECC key and RNG */
    if (ret == 0) {
        ret = wc_ecc_free(&ecc);
        if (ret != 0) {
            LOG_I("wc_ecc_free error, ret = %d\n", ret);
        }
    }
    if (ret == 0) {
        ret = wc_FreeRng(&rng);
        if (ret != 0) {
            LOG_I("wc_FreeRng error, ret = %d\n", ret);
        }
    }

    /* Cleanup wolfSSL library resources */
    wolfSSL_Cleanup();

    LOG_I("Done with sample app");
    return status;
#else
    LOG_E("Examples requires wolfSSL be compiled with WOLFSSL_CERT_GEN "
          "and WOLFSSL_CERT_REQ");
    return kStatus_SSS_Fail;
#endif /* WOLFSSL_CERT_GEN && WOLFSSL_CERT_REQ */
}

#endif /* SIMW_DEMO_ENABLE__DEMO_WOLFCRYPT_GENERATE_CSR */
