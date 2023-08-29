/* wolfCrypt load application for Wisekey VaultIC
 *
 * Copyright (C) 2023 wolfSSL Inc.
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

#ifndef WOLFSSL_USER_SETTINGS
    #include "wolfssl/options.h"
#endif

#include "wolfssl/ssl.h"

#include "wolfssl/wolfcrypt/settings.h"    /* For X-defines */

/* wolfCrypt includes */
#include "wolfssl/wolfcrypt/wc_port.h"     /* For Init/Cleanup */

#ifdef WOLF_CRYPTO_CB
#include "wolfssl/wolfcrypt/cryptocb.h"    /* For Register/Unregister */
#endif

#ifdef HAVE_CCBVAULTIC
#include "ccb_vaultic.h"                   /* For devId and cryptocb */
#endif

/* Local include */
#include "ccb_vaultic_config.h"    /* For authentication and file data */

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    int rc = 0;

    /* Initialize wolfSSL */
    rc = wolfSSL_Init();
    if (rc == WOLFSSL_SUCCESS) {

#ifdef WOLF_CRYPTO_CB
        /* Allocate/initialize context */
        void* c = NULL;
    #if 1
        /* External allocation/initialization of hardware context */
        #ifdef HAVE_CCBVAULTIC
        static ccbVaultIc_Context ctx_storage = CCBVAULTIC_CONTEXT_INITIALIZER;
        c = &ctx_storage;

        ctx_storage.config = &gUserConfig;
        rc = ccbVaultIc_Init(c);
        #else
        /* Add other elif hardware here */
        #endif
    #endif   /* WOLF_CRYPTO_CB_CMD */

        if (rc == 0) {
            /* Setup callback and devId */
            int devId = INVALID_DEVID;
            CryptoDevCallbackFunc ccb = NULL;
    #ifdef HAVE_CCBVAULTIC
            devId = CCBVAULTIC420_DEVID;
            ccb = ccbVaultIc_CryptoCb;
    #else
            /* Add other elif hardware here */
    #endif

            /* Register cryptocb */
            rc = wc_CryptoCb_RegisterDevice(
                    devId,
                    ccb,
                    c);
#endif  /* WOLF_CRYPTO_CB */

            if (rc == 0) {
                /* Perform Load action */
                rc = ccbVaultIc_LoadAction(c, &gLoad);

                if (rc == 0) {
                    WOLFSSL_CTX* ctx = NULL;
                    int i;
                    for(i=0; i < gLoad.file_count; i++) {
                        printf("File %d: \n Name:%.*s\n Len:%d\n Data:%.*s\n---\n",
                                i,
                                gLoad.file[i].name_len,
                                gLoad.file[i].name,
                                gLoad.file[i].data_len,
                                gLoad.file[i].data_len,
                                gLoad.file[i].data
                                );
                    }
                    ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
                    if (ctx != NULL) {
                        rc = wolfSSL_CTX_use_PrivateKey_buffer(
                                ctx,
                                (const byte*)gLoad.file[0].data,
                                gLoad.file[0].data_len,
                                WOLFSSL_FILETYPE_PEM);
                        printf("Use PrivateKey with file %d returned %d\n",0,rc);
                        rc = wolfSSL_CTX_use_certificate_chain_buffer_format(
                                ctx,
                                (const byte*)gLoad.file[1].data,
                                gLoad.file[1].data_len,
                                WOLFSSL_FILETYPE_PEM);
                        printf("Use Certificate Chain with file %d returned %d\n",1,rc);
                        rc = wolfSSL_CTX_load_verify_chain_buffer_format(
                                ctx,
                                (const byte*)gLoad.file[2].data,
                                gLoad.file[2].data_len,
                                WOLFSSL_FILETYPE_PEM);
                        printf("Use Load Verify Chain with file %d returned %d\n",2,rc);
                        wolfSSL_CTX_free(ctx);
                    }
                }
                else
                {
                    printf("Load failed with %d vlt_rc:%x\n", rc, ctx_storage.vlt_rc);
                }

#ifdef WOLF_CRYPTO_CB
            /* Unregister the callback */
            wc_CryptoCb_UnRegisterDevice(devId);
#endif
            } else {
                printf("Failed to register cryptocb:%d (%x) "\
                        " with devId:%x ccb:%p\n",
                        rc, rc, devId, ccb);
            }

#ifdef WOLF_CRYPTO_CB
    #ifndef WOLF_CRYPTO_CB_CMD
            /* External deallocation of hardware context */
        #ifdef HAVE_CCBVAULTIC
            ccbVaultIc_Cleanup(c);
        #else
            /* Add other hardware cleanup here */
        #endif
    #endif
#endif
        } else {
            printf(" Failed to initialize hardware: %d (%x)\n", rc, rc);
        }
        wolfSSL_Cleanup();
    }else {
        printf("Failed to initialize wolfSSL: %d (%x)\n", rc, rc);
    }
    return rc;
}
