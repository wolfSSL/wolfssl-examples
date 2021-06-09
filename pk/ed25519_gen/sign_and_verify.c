/* sign_and_verify.c
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

#include <stdio.h>

#include <wolfssl/options.h>
//#include <wolfssl/wolfcrypt/dsa.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/ssl.h>
#define USE_CERT_BUFFERS_ED
#include "test_keys.h"

#ifndef HAVE_ED25519
    #error "Please build wolfSSL with the --enable-ed25519 option"
#endif

static int err_sys(const char* msg, int es);

static int err_sys(const char* msg, int es)
{
    printf("%s error = %d\n", msg, es);

    exit(-1);
}

int
main(int argc, char* argv[])
{

    int ret = -1000;
    int verify;
    ed25519_key edPublicKey;
    ed25519_key edPrivateKey;

    byte message[] = "Hi how are you?\n";
    byte sigOut[ED25519_SIG_SIZE];
    word32 sigOutSz = sizeof(sigOut);

    wolfSSL_Debugging_ON();


/*--------------- INIT KEYS ---------------------*/
    ret = wc_ed25519_init(&edPublicKey);
    ret = wc_ed25519_init(&edPrivateKey);

    if (ret != 0) {
        printf("Error: wc_ed25519_init: %d\n", ret);
    }

/*--------------- IMPORT KEYS FROM HEADER ---------------------*/
    ret = wc_ed25519_import_public(ed_pub_key_der_32, sizeof_ed_pub_key_der_32,
                                                                  &edPublicKey);
    if (ret != 0) err_sys("Error: ED public key import failed: ", ret);

    ret = wc_ed25519_import_private_key(ed_priv_key_der_64,
                                        ED25519_KEY_SIZE,
                                        ed_priv_key_der_64 + ED25519_KEY_SIZE,
                                        ED25519_KEY_SIZE, &edPrivateKey);
    if (ret != 0) err_sys("Error: ED private key import failed: ", ret);

/*--------------- SIGN THE MESSAGE ---------------------*/
    ret = wc_ed25519_sign_msg(message, sizeof(message), sigOut, &sigOutSz,
                                                                 &edPrivateKey);
    if (ret != 0) err_sys("Error: ED sign failed: ", ret);

/*--------------- VERIFY THE MESSAGE ---------------------*/
    ret = wc_ed25519_verify_msg(sigOut, sigOutSz, message, sizeof(message),
                                                         &verify, &edPublicKey);
    if (ret != 0) err_sys("Error: Could not verify signature: ", ret);

    printf("Successfully validated signature\n");

    return 0;
}
