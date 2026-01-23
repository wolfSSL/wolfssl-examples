/* stse_conf.h
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

#ifndef STSE_CONF_H
#define STSE_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stse_platform_generic.h"

/************************************************************
 *                STSELIB DEVICE SUPPORT
 ************************************************************/
#define STSE_CONF_STSAFE_A_SUPPORT

/************************************************************
 *                STSAFE-A API/SERVICE SETTINGS
 ************************************************************/
#ifdef STSE_CONF_STSAFE_A_SUPPORT

/* STSAFE-A ECC services configuration - enable P-256 and P-384 for wolfSSL */
#define STSE_CONF_ECC_NIST_P_256
#define STSE_CONF_ECC_NIST_P_384
//#define STSE_CONF_ECC_BRAINPOOL_P_256
//#define STSE_CONF_ECC_BRAINPOOL_P_384

/* STSAFE-A HASH services configuration */
#define STSE_CONF_HASH_SHA_256
#define STSE_CONF_HASH_SHA_384

/* STSAFE-A HOST KEY MANAGEMENT
 * Requires platform crypto implementation (stse_platform_crypto_wolfssl.c)
 * Enable this for encrypted host sessions with STSAFE
 */
#define STSE_CONF_USE_HOST_SESSION

/* Optional: Enable for additional key management features */
/* #define STSE_CONF_USE_HOST_KEY_ESTABLISHMENT */
/* #define STSE_CONF_USE_SYMMETRIC_KEY_ESTABLISHMENT */

#endif /* STSE_CONF_STSAFE_A_SUPPORT */

/*********************************************************
 *                COMMUNICATION SETTINGS
 *********************************************************/
#define STSE_USE_RSP_POLLING
#define STSE_MAX_POLLING_RETRY 100
#define STSE_FIRST_POLLING_INTERVAL 10
#define STSE_POLLING_RETRY_INTERVAL 10

/* Enable debug logging */
/* #define STSE_FRAME_DEBUG_LOG */

#ifdef __cplusplus
}
#endif

#endif /* STSE_CONF_H */
