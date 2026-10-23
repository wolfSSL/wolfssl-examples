/* mlkem-cryptocb.h
 *
 * Copyright (C) 2025 wolfSSL Inc.
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

#ifndef MLKEM_CRYPTOCB_H
#define MLKEM_CRYPTOCB_H

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/cryptocb.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#if defined(WOLF_CRYPTO_CB) && defined(WOLFSSL_HAVE_MLKEM)

/* Crypto callback context: counts the ML-KEM KEM operations the TLS 1.3
 * handshake routed to the device. */
typedef struct MlKemCryptoCbCtx {
    int keygenCount;
    int encapCount;
    int decapCount;
} MlKemCryptoCbCtx;

int myMlKemCryptoCb(int devId, wc_CryptoInfo* info, void* ctx);

#endif /* WOLF_CRYPTO_CB && WOLFSSL_HAVE_MLKEM */

#endif /* MLKEM_CRYPTOCB_H */
