/* NativeHelper.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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

#include <jni.h>
#include <wolfssl/ssl.h>
#include <wolfcrypt/test/test.h>
#include <wolfcrypt/benchmark/benchmark.h>

#if defined(WOLF_CRYPTO_CB)
    #if defined(HAVE_CCBVAULTIC) && defined(WOLF_CRYPTO_CB_CMD)
        #include "ccb_vaultic.h"
    #endif
#endif

static int nativeStartup(void)
{
    int ret = wolfCrypt_Init();

#ifdef WOLF_CRYPTO_CB
    {
    #ifdef WC_USE_DEVID
        int devId = WC_USE_DEVID;
    #else
        int devId = INVALID_DEVID;
    #endif

    #if defined(HAVE_CCBVAULTIC) && defined(WOLF_CRYPTO_CB_CMD)
        if((ret == 0) && (devId == CCBVAULTIC420_DEVID)) {
            ret = wc_CryptoCb_RegisterDevice((int) devId,
                                       ccbVaultIc_CryptoDevCb, NULL);
        }
    #endif

    }
#endif /* WOLF_CRYPTO_CB */

#ifdef WC_RNG_SEED_CB
    wc_SetSeed_Cb(wc_GenerateSeed);
#endif
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_wolfssl_wolfssljni_1ndk_1gradle_MainActivity_testWolfCrypt(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;

    int ret = nativeStartup();

    if (ret == 0) {
        /* test uses WC_USE_DEVID for hardware offload tests */
        ret = wolfcrypt_test(NULL);
    }
    wolfCrypt_Cleanup();

    return ret;
}

JNIEXPORT jint JNICALL
Java_com_wolfssl_wolfssljni_1ndk_1gradle_MainActivity_benchWolfCrypt(JNIEnv *env, jobject thiz) {
    (void)env;
    (void)thiz;

    int ret = nativeStartup();

    if (ret == 0) {
        /* benchmark uses WC_USE_DEVID for hardware offload tests */
        ret = benchmark_test(NULL);
    }
    wolfCrypt_Cleanup();

    return ret;
}
