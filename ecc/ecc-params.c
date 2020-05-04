/* ecc-params.c
 *
 * Copyright (C) 2006-2019 wolfSSL Inc.
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
#include <wolfssl/wolfcrypt/integer.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

#include <stdint.h>

#if defined(HAVE_ECC) && defined(WOLFSSL_PUBLIC_MP)

/*
STATIC:
./configure --enable-debug --disable-shared CFLAGS="-DWOLFSSL_PUBLIC_MP" && make && sudo make install
gcc -o ecc-params -I/usr/local/include ecc-params.c /usr/local/lib/libwolfssl.a
./ecc_params
*/


enum {
    ECC_CURVE_FIELD_PRIME = 1,
    ECC_CURVE_FIELD_AF,
    ECC_CURVE_FIELD_BF,
    ECC_CURVE_FIELD_ORDER,
    ECC_CURVE_FIELD_GX,
    ECC_CURVE_FIELD_GY,
};

int load_curve_param(int curve_id, int curve_param, unsigned char* buf, size_t buf_sz)
{
    int ret;
    int curve_idx = wc_ecc_get_curve_idx(curve_id);
    mp_int param;
    const char* param_hex_str = NULL;
    const ecc_set_type* ecc_set;
    int act_sz = 0;

    if (curve_idx < 0)
        return curve_idx;

    /* Get curve parameters */
    ecc_set = wc_ecc_get_curve_params(curve_idx);
    if (ecc_set == NULL)
        return BAD_FUNC_ARG;

    /* Get curve parameter hex string */
    switch (curve_param) {
        case ECC_CURVE_FIELD_PRIME:
            param_hex_str = ecc_set->prime;
            break;
        case ECC_CURVE_FIELD_AF:
            param_hex_str = ecc_set->Af;
            break;
        case ECC_CURVE_FIELD_BF:
            param_hex_str = ecc_set->Bf;
            break;
        case ECC_CURVE_FIELD_ORDER:
            param_hex_str = ecc_set->order;
            break;
        case ECC_CURVE_FIELD_GX:
            param_hex_str = ecc_set->Gx;
            break;
        case ECC_CURVE_FIELD_GY:
            param_hex_str = ecc_set->Gy;
            break;
    }

    ret = mp_init(&param);
    if (ret == MP_OKAY) {
        /* import hex string to multiple precision math */
        ret = mp_read_radix(&param, param_hex_str, MP_RADIX_HEX);
        if (ret == MP_OKAY) {
            /* check size */
            act_sz = mp_unsigned_bin_size(&param);
            if (buf_sz < act_sz) {
                ret = BUFFER_E;
            }
            /* export to unsigned bin */
            if (ret == MP_OKAY) {
                ret = mp_to_unsigned_bin(&param, buf);
            }
        }
        mp_clear(&param);
    }
    if (ret == 0)
        ret = act_sz;

    return ret;
}

#endif /* HAVE_ECC && WOLFSSL_PUBLIC_MP */

int main(void)
{
    int ret = 0;
#if defined(HAVE_ECC) && defined(WOLFSSL_PUBLIC_MP)
    const char* curve_str = "SECP256R1";
    int curve_id = ECC_SECP256R1;
    unsigned char param[MAX_ECC_BYTES];

    wolfSSL_Debugging_ON();

    printf("ECC Curve Parameters for %s\n", curve_str);

    ret = load_curve_param(curve_id, ECC_CURVE_FIELD_PRIME, param, sizeof(param));
    printf("Prime: %d\n", ret);
    WOLFSSL_BUFFER(param, ret);

    ret = load_curve_param(curve_id, ECC_CURVE_FIELD_AF, param, sizeof(param));
    printf("Af: %d\n", ret);
    WOLFSSL_BUFFER(param, ret);

    ret = load_curve_param(curve_id, ECC_CURVE_FIELD_BF, param, sizeof(param));
    printf("Bf: %d\n", ret);
    WOLFSSL_BUFFER(param, ret);

    ret = load_curve_param(curve_id, ECC_CURVE_FIELD_ORDER, param, sizeof(param));
    printf("Order: %d\n", ret);
    WOLFSSL_BUFFER(param, ret);

    ret = load_curve_param(curve_id, ECC_CURVE_FIELD_GX, param, sizeof(param));
    printf("Gx: %d\n", ret);
    WOLFSSL_BUFFER(param, ret);

    ret = load_curve_param(curve_id, ECC_CURVE_FIELD_GY, param, sizeof(param));
    printf("Gy: %d\n", ret);
    WOLFSSL_BUFFER(param, ret);
#else
    printf("Must build wolfSSL with ./configure CFLAGS=\"-DWOLFSSL_PUBLIC_MP\"\n");
#endif
    return ret;
}
