/* wolfCrypt test application for Wisekey VaultIC
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
#include "wolfcrypt/test/test.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    int rc = 0;

    /* Initialize wolfCrypt */
    rc = wolfCrypt_Init();
    if (rc == 0) {

#ifdef WOLF_CRYPTO_CB
        /* Allocate/initialize context */
        void* ctx = NULL;
    #ifndef WOLF_CRYPTO_CB_CMD
        /* External allocation/initialization of hardware context */
        #ifdef HAVE_CCBVAULTIC
        static ccbVaultIc_Context ctx_storage = CCBVAULTIC_CONTEXT_INITIALIZER;
        ctx = &ctx_storage;
        rc = ccbVaultIc_Init(ctx);
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
                    ctx);
#endif  /* WOLF_CRYPTO_CB */

            if (rc == 0) {
                /* Run tests */
                rc = wolfcrypt_test(NULL);

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
            ccbVaultIc_Cleanup(ctx);
        #else
            /* Add other hardware cleanup here */
        #endif
    #endif
#endif
        } else {
            printf(" Failed to initialize hardware: %d (%x)\n", rc, rc);
        }
        wolfCrypt_Cleanup();
    }else {
        printf("Failed to initialize wolfCrypt: %d (%x)\n", rc, rc);
    }
    return rc;
}
