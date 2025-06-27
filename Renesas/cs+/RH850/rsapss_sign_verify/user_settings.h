/* user_settings.h
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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

#define SIZEOF_LONG_LONG 8

#define WOLFSSL_NO_CURRDIR
#define WOLFSSL_LOG_PRINTF
#define NO_WOLFSSL_STUB
#define NO_DYNAMIC_ARRAY      /* for compilers not allowed dynamic size array */
#define WOLFSSL_SMALL_STACK
#define WOLFSSL_DH_CONST
#define TFM_TIMING_RESISTANT
#define ECC_TIMING_RESISTANT
#define WC_RSA_BLINDING
#define USER_TIME
#define XTIME time
#define USE_WOLF_SUSECONDS_T
#define USE_WOLF_TIMEVAL_T
#define WOLFSSL_USER_CURRTIME   /* for benchmark */
#define WOLFSSL_GENSEED_FORTEST /* Wardning: define your own seed gen */
#define SINGLE_THREADED         /* or define RTOS  option */

#define HAVE_RSA
#define WC_RSA_PSS
#define XSTRCASECMP(s1,s2) strcmp((s1),(s2))

#define WOLFSSL_GMTIME
#define WOLFCRYPT_ONLY
#define WOLFSSL_CMAC

#define NO_MAIN_DRIVER
#define NO_WRITEV
#define NO_DEV_RANDOM
#define NO_FILESYSTEM
#define NO_WOLFSSL_DIR
#define NO_DH
#define NO_DES3
#define NO_DSA
#define NO_RC4
#define NO_MD4
#define NO_MD5
#define NO_PSK
#define NO_PWDBASED
#define NO_ERROR_STRINGS
#define NO_WOLFSSL_SERVER
#define NO_WOLFSSL_CLIENT
#define NO_CRYPT_TEST
#define NO_CRYPT_BENCHMARK

#define XVSNPRINTF      snprintf
#define XSNPRINTF snprintf