/* wolfcrypt_test.c
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

#ifdef __cplusplus
extern "C" {
#endif

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/types.h>
#include <wolfssl/wolfcrypt/port/nxp/se050_port.h>
#include <wolfssl/ssl.h>
#include "test.h"

#include <ex_sss_boot.h>
#include <fsl_sss_se05x_apis.h>
#include <nxLog_App.h>

#ifdef __cplusplus
}
#endif

#if defined(SIMW_DEMO_ENABLE__DEMO_WOLFCRYPTTEST)

static ex_sss_boot_ctx_t gex_sss_boot_ctx;

#define EX_SSS_BOOT_PCONTEXT (&gex_sss_boot_ctx)
#define EX_SSS_BOOT_DO_ERASE 1
#define EX_SSS_BOOT_EXPOSE_ARGC_ARGV 0

#include <ex_sss_main_inc.h>

sss_status_t ex_sss_entry(ex_sss_boot_ctx_t *pCtx)
{
    int ret = 0;
    sss_status_t status = kStatus_SSS_Success;
    sss_session_t *pSession = (sss_session_t*)&pCtx->session;
    sss_key_store_t *pKeyStore = (sss_key_store_t*)&pCtx->ks;

    LOG_I("running setconfig");
    ret = wc_se050_set_config(pSession, NULL, pKeyStore);
    if (ret != 0) {
        LOG_E("wc_se050_set_config failed");
        return kStatus_SSS_Fail;
    }
    LOG_I("Ran setconfig successfully");

    wolfSSL_Init();
    wolfcrypt_test(NULL);
    wolfSSL_Cleanup();

    LOG_I("Ran wolfCrypt test");
    return status;
}

#endif /* SIMW_DEMO_ENABLE__DEMO_WOLFCRYPTTEST */
