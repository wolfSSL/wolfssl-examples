/*
 * wisekey_vaultic.h
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

#ifndef WISEKEY_VAULTIC_H
#define WISEKEY_VAULTIC_H

#include "wisekey_vaultic_defs.h"
#include <wolfssl/wolfcrypt/cryptocb.h>

/*
 * Implementation of wolfCrypt devcrypto callbacks
 *
 * The wolfSSL port of the Wisekey VaultIC provides a wrapper library to allow
 * the VaultIC to be used as an external crypto provider.  This library depends
 * on the Wisekey-provided VaultIC interface libraries that have been statically
 * compiled in the proper hardware configuration.
 */

typedef struct {
    int invoked;
} wkvicContext;

/* Initialize the Wisekey VaultIC library and clear the context. */
int WisekeyVaultIC_Init(wkvicContext *c);

/* Register this callback using:
 * wisekeyvaultic_context_t ctx={0};
 * int rc = wc_CryptoCb_RegisterDevice(WISEKEY_VAULTIC420_DEVID,
 *                      WisekeyVaultIC_CryptoDevCb, &ctx);
 */
int WisekeyVaultIC_CryptoDevCb(int devId,
                               wc_CryptoInfo* info,
                               void* ctx);

/* Close the Wisekey VaultIC library. */
int WisekeyVaultIC_Cleanup(wkvicContext *c);

#endif /* _WOLFPORT_WISEKEY_VAULTIC_H_ */
