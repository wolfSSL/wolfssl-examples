/* srp.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/srp.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include "srp_params.h"
#include "srp_store.h"

#include <stdio.h>

#ifdef WOLFCRYPT_HAVE_SRP

/* Create a client side SRP object */
static int client_init(Srp** srp)
{
    int ret = 0;

    *srp = XMALLOC(sizeof(Srp), NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (*srp == NULL) {
        ret = MEMORY_E;
    }

    if (ret == 0) {
       ret = wc_SrpInit(*srp, SRP_TYPE_SHA, SRP_CLIENT_SIDE);
    }

    return ret;
}

/* Calculate the client's public key */
static int client_calc_public(Srp* srp, const char* username, char* password,
                              byte* salt, word32 saltSz, byte* pubKey,
                              word32* pubKeySz)
{
    int ret;

    ret = wc_SrpSetUsername(srp, (byte*)username, XSTRLEN(username));
    if (ret == 0) {
        ret = wc_SrpSetParams(srp, srp_n_640, sizeof(srp_n_640),
                              srp_g_640, sizeof(srp_g_640),
                              salt, saltSz);
    }
    if (ret == 0) {
        ret = wc_SrpSetPassword(srp, (byte*)password, XSTRLEN(password));
    }
    if (ret == 0) {
        ret = wc_SrpGetPublic(srp, pubKey, pubKeySz);
    }

    return ret;
}

/* Compute the client's proof */
static int client_compute_proof(Srp *srp, byte* clientPubKey,
                                word32 clientPubKeySz, byte* serverPubKey,
                                word32 serverPubKeySz, byte* proof,
                                word32* proofSz)
{
    int ret;

    ret = wc_SrpComputeKey(srp, clientPubKey, clientPubKeySz, serverPubKey,
                           serverPubKeySz);
    if (ret == 0) {
        ret = wc_SrpGetProof(srp, proof, proofSz);
    }

    return ret;
}

/* Verify the server's proof */
static int client_verify_proof(Srp *srp, byte* proof, word32 proofSz)
{
    return wc_SrpVerifyPeersProof(srp, proof, proofSz);
}


/* Create a server side SRP object */
static int server_init(Srp** srp)
{
    int ret = 0;

    *srp = XMALLOC(sizeof(Srp), NULL, DYNAMIC_TYPE_TMP_BUFFER);
    if (*srp == NULL) {
        ret = MEMORY_E;
    }

    if (ret == 0) {
       ret = wc_SrpInit(*srp, SRP_TYPE_SHA, SRP_SERVER_SIDE);
    }

    return ret;
}

/* Calculate the server's public key */
static int server_calc_public(Srp* srp, const char* username, byte* salt,
                              word32 saltSz, byte* verifier, word32 vSz,
                              byte* pubKey, word32* pubKeySz)
{
    int ret;

    ret = wc_SrpSetUsername(srp, (byte*)username, XSTRLEN(username));
    if (ret == 0) {
        ret = wc_SrpSetParams(srp, srp_n_640, sizeof(srp_n_640),
                              srp_g_640, sizeof(srp_g_640),
                              salt, saltSz);
    }
    if (ret == 0) {
        ret = wc_SrpSetVerifier(srp, verifier, vSz);
    }
    if (ret == 0) {
        ret = wc_SrpGetPublic(srp, pubKey, pubKeySz);
    }

    return ret;
}

/* Check the client's proof and generate the server's proof */
static int server_compute_proof(Srp *srp, byte* clientPubKey,
                                word32 clientPubKeySz, byte* serverPubKey,
                                word32 serverPubKeySz, byte* clientProof,
                                word32 clientProofSz, byte* proof,
                                word32* proofSz)
{
    int ret;

    ret = wc_SrpComputeKey(srp, clientPubKey, clientPubKeySz, serverPubKey,
                           serverPubKeySz);
    if (ret == 0) {
        ret = wc_SrpVerifyPeersProof(srp, clientProof, clientProofSz);
    }
    if (ret == 0) {
        ret = wc_SrpGetProof(srp, proof, proofSz);
    }

    return ret;
}

/* Dispose of the SRP object */
static void srp_free(Srp* srp)
{
    if (srp != NULL) {
        wc_SrpTerm(srp);
        XFREE(srp, NULL, DYNAMIC_TYPE_TMP_BUFFER);
    }
}

/* Main entry point. */
int main(int argc, char* argv[])
{
    int    ret = 0;
    Srp*   cli = NULL;
    Srp*   srv = NULL;
    char*  password = NULL;
    char*  username = NULL;
    byte   serverPubKey[80];
    word32 serverPubKeySz = (word32)sizeof(serverPubKey);
    byte   clientPubKey[80];
    word32 clientPubKeySz = (word32)sizeof(clientPubKey);
    byte   clientProof[SRP_MAX_DIGEST_SIZE];
    word32 clientProofSz = (word32)sizeof(clientProof);
    byte   serverProof[SRP_MAX_DIGEST_SIZE];
    word32 serverProofSz = (word32)sizeof(serverProof);

    if (argc < 3) {
        fprintf(stderr, "Usage: srp <username> <password>\n");
        return 1;
    }

    username = argv[1];
    password = argv[2];

    /* Initialize wolfSSL library. */
    wolfSSL_Init();

    if (XSTRNCMP(username, srp_username, XSTRLEN(srp_username)) != 0) {
        printf("Username does not match stored verfier\n");
        ret = -1;
    }

    if (ret == 0) {
        ret = client_init(&cli);
    }
    if (ret == 0) {
        ret = client_calc_public(cli, username, password, (byte*)salt,
                                 sizeof(salt), clientPubKey, &clientPubKeySz);
    }


    if (ret == 0) {
        ret = server_init(&srv);
    }
    if (ret == 0) {
        ret = server_calc_public(srv, username, (byte*)salt, sizeof(salt),
                                 (byte*)verifier, sizeof(verifier),
                                 serverPubKey, &serverPubKeySz);
    }


    if (ret == 0) {
        ret = client_compute_proof(cli, clientPubKey, clientPubKeySz,
                                   serverPubKey, serverPubKeySz, clientProof,
                                   &clientProofSz);
    }

    if (ret == 0) {
        ret = server_compute_proof(srv, clientPubKey, clientPubKeySz,
                                   serverPubKey, serverPubKeySz, clientProof,
                                   clientProofSz, serverProof, &serverProofSz);
        if (ret == 0) {
            printf("Server verified client proof\n");
        }
        else {
            printf("Server could not verify client proof\n");
        }
    }

    if (ret == 0) {
        ret = client_verify_proof(cli, serverProof, serverProofSz);
        if (ret == 0) {
            printf("Client verified server proof\n");
        }
        else {
            printf("Client could not verify server proof\n");
        }
    }

    srp_free(cli);
    srp_free(srv);

    /* Cleanup wolfSSL library. */
    wolfSSL_Cleanup();

    if (ret == 0)
        printf("Done\n");
    else {
        char buffer[80];
        printf("Error: %d, %s\n", ret, wolfSSL_ERR_error_string(ret, buffer));
    }

    return (ret == 0) ? 0 : 1;
}

#else

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    fprintf(stderr, "Must build wolfSSL with SRP enabled for this example\n");
    return 0;
}

#endif
