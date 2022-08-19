/* ecc-stack.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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


#define WOLFSSL_TRACK_MEMORY
#define HAVE_STACK_SIZE

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/random.h>

#include <wolfssl/ssl.h>
#include <wolfssl/test.h>

#define TEST_KEY_SZ 32

static WC_RNG mRng;
static ecc_key mGenKey;

static void* do_it(void* args)
{
    int ret;

    InitMemoryTracker();

    ret = wc_ecc_init(&mGenKey);
    if (ret == 0) {
        ret = wc_ecc_make_key(&mRng, TEST_KEY_SZ, &mGenKey);
    }
    if (ret != 0) {
        printf("ecc make key failed %d\n", ret);
    }

    ShowMemoryTracker();
    CleanupMemoryTracker();

    (void)args;

    return 0;
}


int main()
{
    int ret;

    wolfCrypt_Init();

    ret = wc_InitRng(&mRng);
    if (ret != 0) {
        printf("Init RNG failed %d\n", ret);
    }

    StackSizeCheck(NULL, do_it);
    printf("sizeof RNG = %lu\n", sizeof(WC_RNG));
    printf("sizeof ecc_key = %lu\n", sizeof(ecc_key));

    ret = wc_FreeRng(&mRng);
    if (ret != 0) {
        printf("Free RNG failed %d\n", ret);
    }
    wolfCrypt_Cleanup();

    return 0;
}
