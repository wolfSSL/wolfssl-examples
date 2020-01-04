/* ecc-verify.c
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

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/asn_public.h>

#if defined(WOLFSSL_CUSTOM_CURVES) && defined(HAVE_ECC_KOBLITZ)

#define MAX_BLOCK_SIZE 1024

int hash_firmware_verify(const byte* fwAddr, word32 fwLen, const byte* sigBuf, word32 sigLen)
{
    wc_Sha256 sha;
    byte hash[WC_SHA256_DIGEST_SIZE];
    int ret;
    ecc_key eccKey;
    word32 inOutIdx;
    const byte pubKey[] = {
        0x30, 0x56, 0x30, 0x10, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x05, 0x2b,
        0x81, 0x04, 0x00, 0x0a, 0x03, 0x42, 0x00, 0x04, 0xcf, 0x43, 0x31, 0x08, 0x86, 0xe1, 0xf5, 0xf3,
        0xd3, 0x73, 0x00, 0x40, 0x96, 0xaa, 0xa8, 0xe5, 0xb5, 0x39, 0x14, 0x64, 0xd6, 0xff, 0x65, 0x5b,
        0x29, 0x72, 0x66, 0x7a, 0x47, 0x90, 0x29, 0x16, 0x23, 0xad, 0xba, 0x90, 0x52, 0x56, 0x42, 0xe0,
        0x1c, 0x8a, 0x80, 0x0d, 0x79, 0x57, 0xe6, 0x6e, 0x3d, 0x73, 0x4a, 0xf1, 0xe1, 0xd2, 0x6f, 0x2b,
        0x51, 0xd0, 0xa8, 0x89, 0xa0, 0x58, 0xff, 0xbd
    };
    int pos, verify;

    ret = wc_InitSha256(&sha);
    if (ret != 0)
        return ret;

    pos = 0;
    while (fwLen > 0) {
        word32 len = fwLen;

        if (len > MAX_BLOCK_SIZE)
            len = MAX_BLOCK_SIZE;

        ret = wc_Sha256Update(&sha, fwAddr + pos, len);
        if (ret < 0)
            goto exit;

        pos += len;
        fwLen -= len;
    }

    ret = wc_Sha256Final(&sha, hash);
    if (ret < 0)
        goto exit;

    inOutIdx = 0;
    ret = wc_EccPublicKeyDecode(pubKey, &inOutIdx, &eccKey, sizeof(pubKey));
    if (ret < 0)
        goto exit;

    ret = wc_ecc_set_curve(&eccKey, 32, ECC_SECP256K1);
    if (ret < 0)
        goto exit;

    ret = wc_ecc_verify_hash((byte*)sigBuf, sigLen, hash, sizeof(hash), &verify, &eccKey);
    if (ret < 0)
        goto exit;

exit:
    wc_Sha256Free(&sha);

    return ret;
}

int main(void)
{
    int ret;
    const byte data[] = {'t', 'e', 's', 't', '\n'};
    const byte sigBuf[] = {
        0x30, 0x43, 0x02, 0x1f, 0x11, 0x80, 0x77, 0xa8, 0xfe, 0x05, 0x70, 0x3a, 0x62, 0x9f, 0x2a, 0xe3,
        0x10, 0x67, 0x8b, 0x58, 0xb4, 0x87, 0x81, 0x51, 0x21, 0x7e, 0x8b, 0x8a, 0x15, 0xd2, 0xa0, 0xc6,
        0x58, 0xcc, 0x95, 0x02, 0x20, 0x00, 0xd6, 0x07, 0xdc, 0x50, 0xcb, 0xbb, 0x48, 0x89, 0xc1, 0x89,
        0x34, 0xff, 0xcc, 0x75, 0x33, 0x08, 0x82, 0x12, 0x3f, 0x8d, 0x32, 0x16, 0xf4, 0xf3, 0xa4, 0x18,
        0xb7, 0x0e, 0x26, 0x55, 0x81
    };
    word32 sigLen = (word32)sizeof(sigBuf);

    ret = hash_firmware_verify(data, sizeof(data), sigBuf, sigLen);
    printf("hash_firmware_verify: %d\n", ret);

    return 0;
}

#else

int main(void)
{
    printf("Not compiled in: Build wolfSSL with `./configure --enable-ecccustcurves CFLAGS=-DHAVE_ECC_KOBLITZ` or `WOLFSSL_CUSTOM_CURVES` and `HAVE_ECC_KOBLITZ`\n");
    return 0;
}

#endif
