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

JNIEXPORT jint JNICALL
Java_com_wolfssl_wolfssljni_1ndk_1gradle_MainActivity_testWolfCrypt(JNIEnv *env, jobject thiz) {
    int ret;

    ret = wolfCrypt_Init();
#ifdef WC_RNG_SEED_CB
    wc_SetSeed_Cb(wc_GenerateSeed);
#endif
    if (ret == 0) {
        ret = wolfcrypt_test(NULL);
    }
    wolfCrypt_Cleanup();

    return ret;
}

JNIEXPORT jint JNICALL
Java_com_wolfssl_wolfssljni_1ndk_1gradle_MainActivity_benchWolfCrypt(JNIEnv *env, jobject thiz) {
    int ret = 0;

    ret = wolfCrypt_Init();
#ifdef WC_RNG_SEED_CB
    wc_SetSeed_Cb(wc_GenerateSeed);
#endif
    if (ret == 0) {
        ret = benchmark_test(NULL);
    }
    wolfCrypt_Cleanup();

    return ret;
}
