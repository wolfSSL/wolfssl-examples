/*
 * ccb_vaultic.h
 *
 * Copyright (C) 2023 wolfSSL Inc.
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

#ifndef CCB_VAULTIC_H
#define CCB_VAULTIC_H

/* VaultIC DevID MSBs are ASCII "VI" */
#define CCBVAULTIC_DEVID (0x56490000ul)
#define CCBVAULTIC420_DEVID (CCBVAULTIC_DEVID + 0x0420)

#ifdef HAVE_CCBVAULTIC

/*
 * Implementation of wolfCrypt cryptocb callbacks
 *
 * The wolfSSL port of the Wisekey VaultIC provides a wrapper library to allow
 * the VaultIC to be used as an external crypto provider.  This library depends
 * on the Wisekey-provided VaultIC interface libraries that have been statically
 * compiled in the proper hardware configuration.
 */

typedef struct {
    int initialized;
    int vlt_rc;

    /* Buffer to store message during SHA with will_copy flag set */
    int hash_type;  /* enum wc_HashType */
    unsigned char *m;
    size_t m_len;

    /* Buffer to cache aes key between invocations */
    unsigned char *aescbc_key;
    size_t aescbc_keylen;
} ccbVaultIc_Context;

/* ccbVaultIc_Context static initializer */
#define CCBVAULTIC_CONTEXT_INITIALIZER   \
    {                                    \
        .initialized = 0                 \
    }

/* Initialize the Wisekey VaultIC library and clear the context.
 * Returns: 0 on success
 *          BAD_FUNC_ARGS with NULL context
 *          WC_INIT_E on error initializing the VaultIC.
 *                    c->vlt_rc will have error code
 */
int ccbVaultIc_Init(ccbVaultIc_Context *c);

/* Close the Wisekey VaultIC library. */
void ccbVaultIc_Cleanup(ccbVaultIc_Context *c);

#ifdef WOLF_CRYPTO_CB
#include "wolfssl/wolfcrypt/cryptocb.h"  /* For wc_CryptInfo */

/* Register this callback and associate with a context using:
 *      ccbVaultIc_Context ctx=CCBVAULTIC_CONTEXT_INITIALIZER;
 *      ccbVaultIc_Init(&ctx);
 *      wc_CryptoCb_RegisterDevice(
 *                      CCBVAULTIC420_DEVID,
 *                      ccbVaultIc_CryptoCb,
 *                      &ctx);
 *      wc_Aes aes={0};
 *      wc_AesInit(&aes, NULL, CCBVAULTIC420_DEVID);
 * Returns: 0 on success
 *          CRYPTOCB_UNAVAILABLE if not initialized or not implemented
 *          MEMORY_E if memory allocation fails
 */
int ccbVaultIc_CryptoCb(int devId,
                        wc_CryptoInfo* info,
                        void* ctx);
#endif  /* WOLF_CRYPTO_CB */

#endif  /* HAVE_CCBVAULTIC */

#endif /* CCB_VAULTIC_H_ */
