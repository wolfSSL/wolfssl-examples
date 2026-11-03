/* user_settings.h
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

/* wolfSSL configuration for an AUTOSAR ECU: no autotools, no CMake, no
 * filesystem, wolfCrypt only.
 *
 * Both the library and the application must be compiled with
 * -DWOLFSSL_USER_SETTINGS and with this file on the include path. Getting that
 * wrong on one side of the build is the classic wolfSSL integration bug: the
 * feature macros change struct layouts, so it links and then misbehaves.
 */

#ifndef WOLFSSL_USER_SETTINGS_AUTOSAR_H
#define WOLFSSL_USER_SETTINGS_AUTOSAR_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the AUTOSAR port ------------------------------------------------ */

#define WOLFSSL_AUTOSAR

/* MAC generate/verify services. Drop these three lines for a build with only
 * AES-CBC and random. WOLFSSL_AES_DIRECT is not optional alongside CMAC: the
 * WC_CMAC_AES case in cmac.c is compiled out without it, and wc_InitCmac()
 * then fails at runtime rather than at build time. */
#define WOLFSSL_AUTOSAR_CMAC
#define WOLFSSL_CMAC
#define WOLFSSL_AES_DIRECT

/* Point the whole port at a crypto callback device -- an HSM, SHE block or
 * accelerator -- without plumbing a Csm_ConfigType through. The application
 * still has to call wolfCrypt_Init() and wc_CryptoCb_RegisterDevice() before
 * Csm_Init(), and wolfSSL needs --enable-cryptocb / WOLF_CRYPTO_CB.
 * Left at the default here, which is INVALID_DEVID: software.
 *
 * #define WOLFSSL_AUTOSAR_DEVID 7
 */

/* Keystore slots and concurrent streaming jobs. Both default to larger values
 * (15 and 10); size them to the ECU. Every job streaming between START and
 * FINISH holds one slot. */
#define MAX_KEYSTORE 4
#define MAX_JOBS     2

/* Pin a job to a specific keystore slot instead of taking the first match.
 * See ../README.md -- these belong to the library build, not the application.
 *
 * Every REDIRECTION_*_KEYID must be < MAX_KEYSTORE above. The slots below fit
 * the 4 this file configures; the values in ../README.md and autosar/Makefile
 * are for the default MAX_KEYSTORE of 15, so do not copy them here unchanged
 * -- Csm_KeyElementSet() rejects an out-of-range slot and the driver then
 * reports "Bogus input key ID redirection (too large)", with every AES-CBC
 * check failing and nothing pointing at the slot number.
 *
 * #define REDIRECTION_CONFIG       0x03
 * #define REDIRECTION_IN1_KEYID    1
 * #define REDIRECTION_IN1_KEYELMID 0x01
 * #define REDIRECTION_IN2_KEYID    3
 * #define REDIRECTION_IN2_KEYELMID 0x05
 */

/* ---- what the port needs -------------------------------------------- */

/* No TLS: the CSM is a crypto API. An AUTOSAR stack gets TLS from TcpIp/Tls,
 * not from here. This alone removes most of the library. */
#define WOLFCRYPT_ONLY

#define HAVE_AES_CBC

/* The driver picks AES-128/192/256 from the length of the key in the keystore,
 * so enable the sizes the ECU will actually be provisioned with. A key length
 * that is not enabled here is accepted into the keystore and then rejected by
 * the job that tries to use it. */
#define WOLFSSL_AES_128
#define WOLFSSL_AES_192
#define WOLFSSL_AES_256

/* The Hash_DRBG behind Csm_RandomGenerate() is on by default: random.h
 * defines HAVE_HASHDRBG itself unless WC_NO_HASHDRBG or
 * CUSTOM_RAND_GENERATE_BLOCK is defined, so neither of those appears here.
 * Stated rather than defined, because defining it would suggest it is needed.
 * It is built on SHA-256, which is why NO_SHA256 is absent above. */

/* ---- trimming -------------------------------------------------------- */

#define NO_FILESYSTEM
#define NO_WRITEV
#define NO_MAIN_DRIVER
#define NO_ERROR_STRINGS

/* Public key, certificates and ASN.1 are all unreachable through the Csm_*
 * API this port implements. */
#define NO_ASN
#define NO_CERTS
#define NO_RSA
#define NO_DH
#define NO_DSA
#define NO_PWDBASED

/* Algorithms the port has no service for. */
#define NO_MD4
#define NO_MD5
#define NO_SHA       /* SHA-1; NOT NO_SHA256, the DRBG needs that */
#define NO_DES3
#define NO_RC4

/* ---- platform -------------------------------------------------------- */

/* One task drives the CSM. An RTOS build sets its own macro instead (FREERTOS,
 * WOLFSSL_ZEPHYR, THREADX, ...) so that the driver's keystore and job-table
 * mutexes become real locks -- see ../csm-threads.c for what they protect. */
#define SINGLE_THREADED

#define WOLFSSL_SMALL_STACK

/* Entropy.
 *
 * NO_FILESYSTEM above compiles out wolfSSL's own /dev/urandom reader
 * (random.c guards it with #ifndef NO_FILESYSTEM), so this build has to name
 * its own seed function. Leaving it out is NOT a build error: the DRBG simply
 * fails to seed and every Csm_RandomGenerate() returns E_NOT_OK at runtime.
 *
 * ecu-seed.c implements this. On a workstation it reads /dev/urandom so the
 * example runs; on a target it is where the on-chip TRNG, HSM or SHE goes.
 */
#define CUSTOM_RAND_GENERATE_SEED ecu_seed
extern int ecu_seed(unsigned char* output, unsigned int sz);

#ifdef __cplusplus
}
#endif

#endif /* WOLFSSL_USER_SETTINGS_AUTOSAR_H */
