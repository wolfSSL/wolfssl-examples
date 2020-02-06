/* ecc_sign_verify.c
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

#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/signature.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/wolfcrypt/logging.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

/* uncomment to show signatures */
/* #define SHOW_SIGS_IN_EXAMPLE */

#define HEAP_HINT NULL
#define ECC_KEY_SIZE_112 112
#define ECC_KEY_SIZE_128 128
#define ECC_KEY_SIZE_160 160
#define ECC_KEY_SIZE_192 192
#define ECC_KEY_SIZE_224 224
#define ECC_KEY_SIZE_239 239
#define ECC_KEY_SIZE_256 256
#define ECC_KEY_SIZE_320 320
#define ECC_KEY_SIZE_384 384
#define ECC_KEY_SIZE_512 512
#define ECC_KEY_SIZE_521 521
#define BYTE_SZ 8

#define CHECK_RET(a, b, eLabel, msg) { \
                                        if (a != b) {                    \
                                            printf("failed %s\n", msg);  \
                                            goto eLabel;                 \
                                        }                                \
                                     }

int do_sig_ver_test(int eccKeySz);

#ifdef SHOW_SIGS_IN_EXAMPLE
    static void hexdump(const void *buffer, word32 len, byte cols);
#endif

int main(void)
{
    int ret = 0;
    ret = do_sig_ver_test(ECC_KEY_SIZE_112);
    CHECK_RET(ret, 0, finished, "112 test");
    ret = do_sig_ver_test(ECC_KEY_SIZE_128);
    CHECK_RET(ret, 0, finished, "128 test");
    ret = do_sig_ver_test(ECC_KEY_SIZE_160);
    CHECK_RET(ret, 0, finished, "160 test");
    ret = do_sig_ver_test(ECC_KEY_SIZE_192);
    CHECK_RET(ret, 0, finished, "192 test");
    ret = do_sig_ver_test(ECC_KEY_SIZE_224);
    CHECK_RET(ret, 0, finished, "224 test");
    ret = do_sig_ver_test(ECC_KEY_SIZE_239);
    CHECK_RET(ret, 0, finished, "239 test");
    ret = do_sig_ver_test(ECC_KEY_SIZE_256);
    CHECK_RET(ret, 0, finished, "256 test");
    ret = do_sig_ver_test(ECC_KEY_SIZE_320);
    CHECK_RET(ret, 0, finished, "320 test");
    ret = do_sig_ver_test(ECC_KEY_SIZE_384);
    CHECK_RET(ret, 0, finished, "384 test");
    ret = do_sig_ver_test(ECC_KEY_SIZE_512);
    CHECK_RET(ret, 0, finished, "512 test");
    ret = do_sig_ver_test(ECC_KEY_SIZE_521);
    CHECK_RET(ret, 0, finished, "521 test");
finished:
    return ret;
}

int do_sig_ver_test(int eccKeySz)
{
    int ret;
    unsigned char hash[32] = "A 32-bit string to test signing";
    ecc_key key;
    byte* sig = NULL; // get rid of this magic number
    WC_RNG rng;
    int verified = 0;
    /* for odd curve sizes account for mod EG (256/8) + (256%8) == 32 + 0 = 32
     * alternatively (521/8 = 65.125 (rounds to 65) + (521%8) == 65 + 1 = 66
     */
    int byteField = (eccKeySz / BYTE_SZ) + (eccKeySz % BYTE_SZ);
    /* Calculate sig size based on byteField + padding + header */
    word32 maxSigSz = ((byteField * 2) + ECC_MAX_PAD_SZ + (SIG_HEADER_SZ * 2));

    printf("Key size is %d, byteField = %d, maxSigSz = %d\n", eccKeySz,
                                                              byteField,
                                                              maxSigSz);

    sig = (byte*) XMALLOC(maxSigSz * sizeof(byte), NULL,
                          DYNAMIC_TYPE_TMP_BUFFER);

    if (sig == NULL) {
        printf("Failed to allocate sig buff\n");
        return -1001;
    }

    wolfCrypt_Init();


    ret = wc_ecc_init(&key);
    CHECK_RET(ret, 0, sig_done, "wc_ecc_init()");

    ret = wc_InitRng(&rng);
    CHECK_RET(ret, 0, key_done, "wc_InitRng()");

    ret = wc_ecc_make_key(&rng, byteField, &key);
    CHECK_RET(ret, 0, rng_done, "wc_ecc_make_key()");

    ret = wc_ecc_sign_hash(hash, sizeof(hash), sig, &maxSigSz, &rng, &key);
    CHECK_RET(ret, 0, rng_done, "wc_ecc_sign_hash()");

#ifdef SHOW_SIGS_IN_EXAMPLE
    hexdump(sig, maxSigSz, 16);
#endif

    ret = wc_ecc_verify_hash(sig, maxSigSz, hash, sizeof(hash), &verified,
                             &key);
    CHECK_RET(ret, 0, rng_done, "wc_ecc_verify_hash()");
    CHECK_RET(verified, 1, rng_done, "verification check");

    printf("Successfully verified signature w/ ecc key size %d!\n", eccKeySz);

rng_done:
    wc_FreeRng(&rng);
key_done:
    wc_ecc_free(&key);
sig_done:
    XFREE(sig, NULL, DYNAMIC_TYPE_TMP_BUFFER);

    return ret;
}

#ifdef SHOW_SIGS_IN_EXAMPLE
static void hexdump(const void *buffer, word32 len, byte cols)
{
   word32 i;

   for (i = 0; i < len + ((len % cols) ? (cols - len % cols) : 0); i++) {
      /* print hex data */
      if (i < len)
         printf("%02X ", ((byte*)buffer)[i] & 0xFF);

      if (i % cols == (cols - 1))
         printf("\n");
   }
}
#endif

