/* sign_request.c
 *
 * Example: Sign an HTTP request using RFC 9421 HTTP Message Signatures
 * with Ed25519 via wolfCrypt.
 *
 * Build wolfSSL with:
 *   ./configure --enable-ed25519 --enable-coding && make && sudo make install
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include <stdio.h>
#include <string.h>

#include "common/wc_http_sig.h"

#if defined(HAVE_ED25519) && defined(HAVE_ED25519_SIGN)

int main(void)
{
    int ret;
    ed25519_key key;
    WC_RNG rng;
    char sigBuf[512];
    word32 sigBufSz = sizeof(sigBuf);
    char inputBuf[1024];
    word32 inputBufSz = sizeof(inputBuf);

    /* Request to sign */
    const char* method    = "POST";
    const char* authority = "example.com";
    const char* path      = "/api/resource";
    const char* query     = "?action=update";
    const char* keyId     = "my-agent-key-1";

    /* Additional headers to include in the signature */
    wc_HttpHeader headers[2];
    headers[0].name  = "content-type";
    headers[0].value = "application/json";
    headers[1].name  = "date";
    headers[1].value = "Thu, 19 Mar 2026 12:00:00 GMT";

    printf("=== RFC 9421 HTTP Message Signature - Sign Example ===\n\n");

    /* Initialize RNG and generate Ed25519 key */
    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("RNG init failed: %d\n", ret);
        return 1;
    }

    ret = wc_ed25519_init(&key);
    if (ret != 0) {
        printf("Ed25519 init failed: %d\n", ret);
        wc_FreeRng(&rng);
        return 1;
    }

    ret = wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &key);
    if (ret != 0) {
        printf("Key generation failed: %d\n", ret);
        goto cleanup;
    }

    printf("Request:\n");
    printf("  %s %s%s HTTP/1.1\n", method, path, query);
    printf("  Host: %s\n", authority);
    printf("  Content-Type: %s\n", headers[0].value);
    printf("  Date: %s\n\n", headers[1].value);

    /* Sign the request */
    ret = wc_HttpSig_Sign(
        method, authority, path, query,
        headers, 2,
        &key, keyId,
        0,  /* use current time */
        sigBuf, &sigBufSz,
        inputBuf, &inputBufSz
    );

    if (ret != 0) {
        printf("Signing failed: %d (%s)\n", ret, wc_GetErrorString(ret));
        goto cleanup;
    }

    printf("Signed headers to add to request:\n\n");
    printf("  Signature-Input: %.*s\n", (int)inputBufSz, inputBuf);
    printf("  Signature: %.*s\n\n", (int)sigBufSz, sigBuf);

    /* Export public key so verifier can use it */
    {
        byte pubKeyBuf[ED25519_PUB_KEY_SIZE];
        word32 pubKeySz = sizeof(pubKeyBuf);
        int i;

        ret = wc_ed25519_export_public(&key, pubKeyBuf, &pubKeySz);
        if (ret == 0) {
            printf("Public key (hex, %d bytes):\n  ", pubKeySz);
            for (i = 0; i < (int)pubKeySz; i++)
                printf("%02x", pubKeyBuf[i]);
            printf("\n\n");
        }
    }

    printf("Success\n");

cleanup:
    wc_ed25519_free(&key);
    wc_FreeRng(&rng);

    if (ret != 0) {
        printf("Error: %d (%s)\n", ret, wc_GetErrorString(ret));
        return 1;
    }
    return 0;
}

#else

int main(void)
{
    printf("This example requires wolfSSL compiled with --enable-ed25519\n");
    return 1;
}

#endif /* HAVE_ED25519 */
