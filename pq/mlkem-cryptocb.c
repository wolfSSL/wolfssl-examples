/* mlkem-cryptocb.c
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

/* ML-KEM crypto callback shared by the wolfCrypt example (pq/ml_kem_cryptocb)
 * and the TLS 1.3 client and server (pq/tls_cryptocb). wolfSSL routes the KEM
 * operations here under WC_ALGO_TYPE_PK, selecting ML-KEM with
 * WC_PQC_KEM_TYPE_MLKEM: keygen, encapsulate and decapsulate. In TLS 1.3 a
 * client uses KEYGEN and DECAPS; a server uses ENCAPS.
 *
 * The callback delegates back to the wolfCrypt software implementation so the
 * examples run without hardware. A real secure element replaces each case body
 * with its own calls and keeps the private key on-chip. Recursion is avoided by
 * clearing key->devId before calling the public API and restoring it after. */

#include "mlkem-cryptocb.h"

#ifdef MLKEM_CRYPTOCB_ENABLED

#include <wolfssl/wolfcrypt/wc_mlkem.h>

int myMlKemCryptoCb(int devId, wc_CryptoInfo* info, void* ctx)
{
    int ret = CRYPTOCB_UNAVAILABLE;
    MlKemCryptoCbCtx* myCtx = (MlKemCryptoCbCtx*)ctx;
    MlKemKey* key = NULL;
    int savedId = INVALID_DEVID;

    (void)devId;

    if (info->algo_type != WC_ALGO_TYPE_PK)
        return CRYPTOCB_UNAVAILABLE;

    switch (info->pk.type) {
        case WC_PK_TYPE_PQC_KEM_KEYGEN:
            if (info->pk.pqc_kem_kg.type != WC_PQC_KEM_TYPE_MLKEM)
                break;
            key = (MlKemKey*)info->pk.pqc_kem_kg.key;
            savedId = key->devId;
            key->devId = INVALID_DEVID;
            ret = wc_MlKemKey_MakeKey(key, info->pk.pqc_kem_kg.rng);
            key->devId = savedId;
            if (ret == 0 && myCtx != NULL)
                myCtx->keygenCount++;
            break;

        case WC_PK_TYPE_PQC_KEM_ENCAPS:
            if (info->pk.pqc_encaps.type != WC_PQC_KEM_TYPE_MLKEM)
                break;
            key = (MlKemKey*)info->pk.pqc_encaps.key;
            savedId = key->devId;
            key->devId = INVALID_DEVID;
            ret = wc_MlKemKey_Encapsulate(key, info->pk.pqc_encaps.ciphertext,
                info->pk.pqc_encaps.sharedSecret, info->pk.pqc_encaps.rng);
            key->devId = savedId;
            if (ret == 0 && myCtx != NULL)
                myCtx->encapCount++;
            break;

        case WC_PK_TYPE_PQC_KEM_DECAPS:
            if (info->pk.pqc_decaps.type != WC_PQC_KEM_TYPE_MLKEM)
                break;
            key = (MlKemKey*)info->pk.pqc_decaps.key;
            savedId = key->devId;
            key->devId = INVALID_DEVID;
            ret = wc_MlKemKey_Decapsulate(key, info->pk.pqc_decaps.sharedSecret,
                info->pk.pqc_decaps.ciphertext, info->pk.pqc_decaps.ciphertextLen);
            key->devId = savedId;
            if (ret == 0 && myCtx != NULL)
                myCtx->decapCount++;
            break;

        default:
            break;
    }

    return ret;
}

#endif /* MLKEM_CRYPTOCB_ENABLED */
