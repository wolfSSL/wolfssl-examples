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
    ecEncCtx* cliCtx = NULL;
    const byte* mySalt;
    void* devCtx = NULL;
    byte peerSalt[EXCHANGE_SALT_SZ];
    byte buffer[MAX_BTLE_MSG_SIZE];
    word32 bufferSz;
    byte plain[MAX_BTLE_MSG_SIZE];
    word32 plainSz;
    ecc_key myKey, peerKey;

    wolfSSL_Init();

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    /* make my session key */
    wc_ecc_init(&myKey);
    wc_ecc_init(&peerKey);
    wc_ecc_make_key(&rng, 32, &myKey);

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

    cliCtx = wc_ecc_ctx_new(REQ_RESP_CLIENT, &rng);
    if (cliCtx == NULL) {
        printf("wc_ecc_ctx_new failed!\n");
        ret = -1; goto cleanup;
    }

    /* exchange public keys */
    /* send my public key */
    /* export my public key */
    bufferSz = sizeof(buffer);
    wc_ecc_export_x963(&myKey, buffer, &bufferSz);
    ret = btle_send(buffer, bufferSz, devCtx);

    /* Get peer key */
    ret = btle_recv(buffer, sizeof(buffer), devCtx);
    bufferSz = ret;
    ret = wc_ecc_import_x963(buffer, bufferSz, &peerKey);

    /* Collect Message to send and get echo */
    while (1) {
        /* get my salt */
        mySalt = wc_ecc_ctx_get_own_salt(cliCtx);
        if (mySalt == NULL) {
            printf("wc_ecc_ctx_get_own_salt failed!\n");
            ret = -1; goto cleanup;
        }

        /* Send my salt */
        ret = btle_send(mySalt, EXCHANGE_SALT_SZ, devCtx);

        /* Get peer salt */
        ret = btle_recv(peerSalt, EXCHANGE_SALT_SZ, devCtx);
        wc_ecc_ctx_set_peer_salt(cliCtx, peerSalt);

        /* get message to send */
        bufferSz = sizeof(buffer);
        fgets((char*)buffer, bufferSz, stdin);
        bufferSz = strlen((char*)buffer);

        /* Encrypt message */
        bufferSz = sizeof(buffer);
        ret = wc_ecc_encrypt(&myKey, &peerKey, plain, plainSz, buffer, &bufferSz, cliCtx);

        /* Send message */
        btle_send(buffer, bufferSz, devCtx);

        /* get message until null termination found */
        bufferSz = sizeof(bufferSz);
        ret = btle_recv(buffer, bufferSz, devCtx);

        /* decrypt message */
        bufferSz = ret;
        ret = wc_ecc_decrypt(&myKey, &peerKey, buffer, bufferSz, plain, &plainSz, cliCtx);

        printf("Recv %d: %s\n", plainSz, plain);

        /* check for exit flag */
        if (strstr((char*)plain, "EXIT"))
            break;

        /* reset context (reset my salt) */
        ret = wc_ecc_ctx_reset(cliCtx, &rng);
    }

cleanup:

    if (devCtx != NULL)
        btle_close(devCtx);

    wolfSSL_Cleanup();

    return ret;
}
