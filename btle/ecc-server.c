/* ecc-server.c
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include "btle-sim.h"

int main(int argc, char** argv)
{
    int ret;
    WC_RNG rng;
    ecEncCtx* srvCtx = NULL;
    const byte* mySalt;
    void* devCtx = NULL;
    byte peerSalt[EXCHANGE_SALT_SZ];
    byte buffer[MAX_BTLE_MSG_SIZE];
    size_t bufLen;

    wolfSSL_Init();

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    /* open BTLE */
    ret = btle_open(&devCtx);
    if (ret != 0) {
        goto cleanup;
    }

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("wc_InitRng failed! %d\n", ret);
        goto cleanup;
    }

    srvCtx = wc_ecc_ctx_new(REQ_RESP_SERVER, &rng);
    if (srvCtx == NULL) {
        printf("wc_ecc_ctx_new failed!\n");
        ret = -1; goto cleanup;
    }

    while (1) {
        mySalt = wc_ecc_ctx_get_own_salt(srvCtx);
        if (mySalt == NULL) {
            printf("wc_ecc_ctx_get_own_salt failed!\n");
            ret = -1; goto cleanup;
        }

        /* Get peer salt */
        ret = btle_recv(peerSalt, EXCHANGE_SALT_SZ, devCtx);

        /* Send my salt */
        ret = btle_send(mySalt, EXCHANGE_SALT_SZ, devCtx);

        /* get message until null termination found */
        btle_recv(buffer, sizeof(buffer), devCtx);

        bufLen = strlen((char*)buffer);

        btle_send(buffer, bufLen, devCtx);

        if (strstr((char*)buffer, "EXIT"))
            break;

        /* reset context (reset my salt) */
        ret = wc_ecc_ctx_reset(srvCtx, &rng);
    }

cleanup:

    if (devCtx != NULL)
        btle_close(devCtx);

    wolfSSL_Cleanup();

    return ret;
}
