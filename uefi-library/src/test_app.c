/* test_app.c
 *
 * UEFI standalone application to test wolfCrypt
 * loaded as standalone bootloader driver
 *
 * Copyright (C) 2026 wolfSSL Inc.
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
#include <efi.h>
#include <efilib.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/curve25519.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/sha3.h>
#include <wolfssl/wolfcrypt/cmac.h>
#include <wolfssl/wolfcrypt/chacha20_poly1305.h>
#include <wolfssl/wolfcrypt/mlkem.h>
#include <wolfssl/wolfcrypt/wc_mlkem.h>
#include <wolfssl/wolfcrypt/dilithium.h>

#include "wolfcrypt_api.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

EFI_GUID g_wolfcrypt_protocol_guid = WOLFCRYPT_PROTOCOL_GUID;
WOLFCRYPT_PROTOCOL *Api = NULL;

/* ------------------------------------------------------------------ */
/* Test vectors */
/* ------------------------------------------------------------------ */

/* AES-128 key/IV (NIST SP 800-38A) */
static const UINT8 kAesKey128[16] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
};
static const UINT8 kAesIv128[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
};
static const UINT8 kAesPlain[16] = {
    0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
};
static const UINT8 kAesEcbCipher[16] = {
    0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
    0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97,
};
static const UINT8 kAesCbcCipher[16] = {
    0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
    0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d,
};
static const UINT8 kAesCfbCipher[16] = {
    0x3b, 0x3f, 0xd9, 0x2e, 0xb7, 0x2d, 0xad, 0x20,
    0x33, 0x34, 0x49, 0xf8, 0xe8, 0x3c, 0xfb, 0x4a,
};
/* AES-CTR (NIST SP 800-38A, F.5.1) */
static const UINT8 kAesCtrIv[16] = {
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
};
static const UINT8 kAesCtrCipher[16] = {
    0x87, 0x4d, 0x61, 0x91, 0xb6, 0x20, 0xe3, 0x26,
    0x1b, 0xef, 0x68, 0x64, 0x99, 0x0d, 0xb6, 0xce,
};

/* SHA digests of "abc" */
static const UINT8 kShaMsg[] = { 'a', 'b', 'c' };
static const UINT8 kSha1Digest[20] = {
    0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a,
    0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c,
    0x9c, 0xd0, 0xd8, 0x9d,
};
static const UINT8 kSha224Digest[28] = {
    0x23, 0x09, 0x7d, 0x22, 0x34, 0x05, 0xd8, 0x22,
    0x86, 0x42, 0xa4, 0x77, 0xbd, 0xa2, 0x55, 0xb3,
    0x2a, 0xad, 0xbc, 0xe4, 0xbd, 0xa0, 0xb3, 0xf7,
    0xe3, 0x6c, 0x9d, 0xa7,
};
static const UINT8 kSha256Digest[32] = {
    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
    0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
    0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
};
static const UINT8 kSha384Digest[48] = {
    0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b,
    0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
    0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
    0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
    0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23,
    0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7,
};
static const UINT8 kSha512Digest[64] = {
    0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba,
    0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
    0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
    0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
    0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8,
    0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
    0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e,
    0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f,
};
static const UINT8 kSha3_256Digest[32] = {
    0x3a, 0x98, 0x5d, 0xa7, 0x4f, 0xe2, 0x25, 0xb2,
    0x04, 0x5c, 0x17, 0x2d, 0x6b, 0xd3, 0x90, 0xbd,
    0x85, 0x5f, 0x08, 0x6e, 0x3e, 0x9d, 0x52, 0x5b,
    0x46, 0xbf, 0xe2, 0x45, 0x11, 0x43, 0x15, 0x32,
};
static const UINT8 kSha3_384Digest[48] = {
    0xec, 0x01, 0x49, 0x82, 0x88, 0x51, 0x6f, 0xc9,
    0x26, 0x45, 0x9f, 0x58, 0xe2, 0xc6, 0xad, 0x8d,
    0xf9, 0xb4, 0x73, 0xcb, 0x0f, 0xc0, 0x8c, 0x25,
    0x96, 0xda, 0x7c, 0xf0, 0xe4, 0x9b, 0xe4, 0xb2,
    0x98, 0xd8, 0x8c, 0xea, 0x92, 0x7a, 0xc7, 0xf5,
    0x39, 0xf1, 0xed, 0xf2, 0x28, 0x37, 0x6d, 0x25,
};
static const UINT8 kSha3_512Digest[64] = {
    0xb7, 0x51, 0x85, 0x0b, 0x1a, 0x57, 0x16, 0x8a,
    0x56, 0x93, 0xcd, 0x92, 0x4b, 0x6b, 0x09, 0x6e,
    0x08, 0xf6, 0x21, 0x82, 0x74, 0x44, 0xf7, 0x0d,
    0x88, 0x4f, 0x5d, 0x02, 0x40, 0xd2, 0x71, 0x2e,
    0x10, 0xe1, 0x16, 0xe9, 0x19, 0x2a, 0xf3, 0xc9,
    0x1a, 0x7e, 0xc5, 0x76, 0x47, 0xe3, 0x93, 0x40,
    0x57, 0x34, 0x0b, 0x4c, 0xf4, 0x08, 0xd5, 0xa5,
    0x65, 0x92, 0xf8, 0x27, 0x4e, 0xec, 0x53, 0xf0,
};

/* HMAC-SHA256 (RFC 4231 test case 1) */
static const UINT8 kHmacKey[20] = {
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b,
};
static const UINT8 kHmacMsg[] = "Hi There";
static const UINT8 kHmacSha256[32] = {
    0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53,
    0x5c, 0xa8, 0xaf, 0xce, 0xaf, 0x0b, 0xf1, 0x2b,
    0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7,
    0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7,
};
static const UINT8 kHmacSha384[48] = {
    0xaf, 0xd0, 0x39, 0x44, 0xd8, 0x48, 0x95, 0x62,
    0x6b, 0x08, 0x25, 0xf4, 0xab, 0x46, 0x90, 0x7f,
    0x15, 0xf9, 0xda, 0xdb, 0xe4, 0x10, 0x1e, 0xc6,
    0x82, 0xaa, 0x03, 0x4c, 0x7c, 0xeb, 0xc5, 0x9c,
    0xfa, 0xea, 0x9e, 0xa9, 0x07, 0x6e, 0xde, 0x7f,
    0x4a, 0xf1, 0x52, 0xe8, 0xb2, 0xfa, 0x9c, 0xb6,
};
static const UINT8 kHmacSha512[64] = {
    0x87, 0xaa, 0x7c, 0xde, 0xa5, 0xef, 0x61, 0x9d,
    0x4f, 0xf0, 0xb4, 0x24, 0x1a, 0x1d, 0x6c, 0xb0,
    0x23, 0x79, 0xf4, 0xe2, 0xce, 0x4e, 0xc2, 0x78,
    0x7a, 0xd0, 0xb3, 0x05, 0x45, 0xe1, 0x7c, 0xde,
    0xda, 0xa8, 0x33, 0xb7, 0xd6, 0xb8, 0xa7, 0x02,
    0x03, 0x8b, 0x27, 0x4e, 0xae, 0xa3, 0xf4, 0xe4,
    0xbe, 0x9d, 0x91, 0x4e, 0xeb, 0x61, 0xf1, 0x70,
    0x2e, 0x69, 0x6c, 0x20, 0x3a, 0x12, 0x68, 0x54,
};

/* CMAC-AES-128 (NIST SP 800-38B example 2: Mlen=128) */
static const UINT8 kCmacMsg[16] = {
    0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
};
static const UINT8 kCmacTag[16] = {
    0x07, 0x0a, 0x16, 0xb4, 0x6b, 0x4d, 0x41, 0x44,
    0xf7, 0x9b, 0xdd, 0x9d, 0xd0, 0x4a, 0x28, 0x7c,
};

/* PBKDF2 (wolfSSL test suite vector) */
static const UINT8 kPbkdf2Salt[8]    = { 0x78,0x57,0x8e,0x5a,0x5d,0x63,0xcb,0x06 };
static const UINT8 kPbkdf2Expected[24] = {
    0x43, 0x6d, 0xb5, 0xe8, 0xd0, 0xfb, 0x3f, 0x35,
    0x42, 0x48, 0x39, 0xbc, 0x2d, 0xd4, 0xf9, 0x37,
    0xd4, 0x95, 0x16, 0xa7, 0x2a, 0x9a, 0x21, 0xd1,
};
static const UINT8 kPkcs7Input[]    = { 0x61,0x62,0x63,0x64,0x65 };
static const UINT8 kPkcs7Expected[] = { 0x61,0x62,0x63,0x64,0x65,0x03,0x03,0x03 };

/* RSA-1024 private key DER (wolfSSL test asset) */
static const UINT8 kRsaKeyDer1024[] = {
    0x30,0x82,0x02,0x5D,0x02,0x01,0x00,0x02,0x81,0x81,
    0x00,0xBE,0x70,0x70,0xB8,0x04,0x18,0xE5,0x28,0xFE,
    0x66,0xD8,0x90,0x88,0xE0,0xF1,0xB7,0xC3,0xD0,0xD2,
    0x3E,0xE6,0x4B,0x94,0x74,0xB0,0xFF,0xB0,0xF7,0x63,
    0xA5,0xAB,0x7E,0xAF,0xB6,0x2B,0xB7,0x38,0x16,0x1A,
    0x50,0xBF,0xF1,0xCA,0x87,0x3A,0xD5,0xB0,0xDA,0xF8,
    0x43,0x7A,0x15,0xB9,0x7E,0xEA,0x2A,0x80,0xD2,0x51,
    0xB0,0x35,0xAF,0x07,0xF3,0xF2,0x5D,0x24,0x3A,0x4B,
    0x87,0x56,0x48,0x1B,0x3C,0x24,0x9A,0xDA,0x70,0x80,
    0xBD,0x3C,0x8B,0x03,0x4A,0x0C,0x83,0x71,0xDE,0xE3,
    0x03,0x70,0xA2,0xB7,0x60,0x09,0x1B,0x5E,0xC7,0x3D,
    0xA0,0x64,0x60,0xE3,0xA9,0x06,0x8D,0xD3,0xFF,0x42,
    0xBB,0x0A,0x94,0x27,0x2D,0x57,0x42,0x0D,0xB0,0x2D,
    0xE0,0xBA,0x18,0x25,0x60,0x92,0x11,0x92,0xF3,0x02,
    0x03,0x01,0x00,0x01,0x02,0x81,0x80,0x0E,0xEE,0x1D,
    0xC8,0x2F,0x7A,0x0C,0x2D,0x44,0x94,0xA7,0x91,0xDD,
    0x49,0x55,0x6A,0x04,0xCE,0x10,0x4D,0xA2,0x1C,0x76,
    0xCD,0x17,0x3B,0x54,0x92,0x70,0x9B,0x82,0x70,0x72,
    0x32,0x24,0x07,0x3F,0x3C,0x6C,0x5F,0xBC,0x4C,0xA6,
    0x86,0x27,0x94,0xAD,0x42,0xDD,0x87,0xDC,0xC0,0x6B,
    0x44,0x89,0xF3,0x3F,0x1A,0x3E,0x11,0x44,0x84,0x2E,
    0x69,0x4C,0xBB,0x4A,0x71,0x1A,0xBB,0x9A,0x52,0x3C,
    0x6B,0xDE,0xBC,0xB2,0x7C,0x51,0xEF,0x4F,0x8F,0x3A,
    0xDC,0x50,0x04,0x4E,0xB6,0x31,0x66,0xA8,0x8E,0x06,
    0x3B,0x51,0xA9,0xC1,0x8A,0xCB,0xC4,0x81,0xCA,0x2D,
    0x69,0xEC,0x88,0xFC,0x33,0x88,0xD1,0xD4,0x29,0x47,
    0x87,0x37,0xF9,0x6A,0x22,0x69,0xB9,0xC9,0xFE,0xEB,
    0x8C,0xC5,0x21,0x41,0x71,0x02,0x41,0x00,0xFD,0x17,
    0x98,0x42,0x54,0x1C,0x23,0xF8,0xD7,0x5D,0xEF,0x49,
    0x4F,0xAF,0xD9,0x35,0x6F,0x08,0xC6,0xC7,0x40,0x5C,
    0x7E,0x58,0x86,0xC2,0xB2,0x16,0x39,0x24,0xC5,0x06,
    0xB0,0x3D,0xAF,0x02,0xD2,0x87,0x77,0xD2,0x76,0xBA,
    0xE3,0x59,0x60,0x42,0xF1,0x16,0xEF,0x33,0x0B,0xF2,
    0x0B,0xBA,0x99,0xCC,0xB6,0x4C,0x46,0x3F,0x33,0xE4,
    0xD4,0x67,0x02,0x41,0x00,0xC0,0xA0,0x91,0x6D,0xFE,
    0x28,0xE0,0x81,0x5A,0x15,0xA7,0xC9,0xA8,0x98,0xC6,
    0x0A,0xAB,0x00,0xC5,0x40,0xC9,0x21,0xBB,0xB2,0x33,
    0x5A,0xA7,0xCB,0x6E,0xB8,0x08,0x56,0x4A,0x76,0x28,
    0xE8,0x6D,0xBD,0xF5,0x26,0x7B,0xBF,0xC5,0x46,0x45,
    0x0D,0xEC,0x7D,0xEE,0x82,0xD6,0xCA,0x5F,0x3D,0x6E,
    0xCC,0x94,0x73,0xCD,0xCE,0x86,0x6E,0x95,0x95,0x02,
    0x40,0x38,0xFD,0x28,0x1E,0xBF,0x5B,0xBA,0xC9,0xDC,
    0x8C,0xDD,0x45,0xAF,0xB8,0xD3,0xFB,0x11,0x2E,0x73,
    0xBC,0x08,0x05,0x0B,0xBA,0x19,0x56,0x1B,0xCD,0x9F,
    0x3E,0x65,0x53,0x15,0x3A,0x3E,0x7F,0x2F,0x32,0xAB,
    0xCB,0x6B,0x4A,0xB7,0xC8,0xB7,0x41,0x3B,0x92,0x43,
    0x78,0x46,0x17,0x51,0x86,0xC9,0xFC,0xEB,0x8B,0x8F,
    0x41,0xCA,0x08,0x9B,0xBF,0x02,0x41,0x00,0xAD,0x9B,
    0x89,0xB6,0xF2,0x8C,0x70,0xDA,0xE4,0x10,0x04,0x6B,
    0x11,0x92,0xAF,0x5A,0xCA,0x08,0x25,0xBF,0x60,0x07,
    0x11,0x1D,0x68,0x7F,0x5A,0x1F,0x55,0x28,0x74,0x0B,
    0x21,0x8D,0x21,0x0D,0x6A,0x6A,0xFB,0xD9,0xB5,0x4A,
    0x7F,0x47,0xF7,0xD0,0xB6,0xC6,0x41,0x02,0x97,0x07,
    0x49,0x93,0x1A,0x9B,0x33,0x68,0xB3,0xA2,0x61,0x32,
    0xA5,0x89,0x02,0x41,0x00,0x8F,0xEF,0xAD,0xB5,0xB0,
    0xB0,0x7E,0x86,0x03,0x43,0x93,0x6E,0xDD,0x3C,0x2D,
    0x9B,0x6A,0x55,0xFF,0x6F,0x3E,0x70,0x2A,0xD4,0xBF,
    0x1F,0x8C,0x93,0x60,0x9E,0x6D,0x2F,0x18,0x6C,0x11,
    0x36,0x98,0x3F,0x10,0x78,0xE8,0x3E,0x8F,0xFE,0x55,
    0xB9,0x9E,0xD5,0x5B,0x2E,0x87,0x1C,0x58,0xD0,0x37,
    0x89,0x96,0xEC,0x48,0x54,0xF5,0x9F,0x0F,0xB3,
};
static const UINT8 kRsaCiphertext[128] = {
    0x01,0x0c,0x1f,0xaf,0x1b,0x3c,0x5d,0xb6,0x6f,0x44,0xc5,0x6a,
    0xf6,0xbb,0x2c,0x52,0x36,0xd7,0x21,0xa2,0x25,0x41,0x28,0x36,
    0x0b,0xd8,0xa3,0xcd,0x74,0x43,0xe0,0xc3,0x76,0x73,0x3c,0x2b,
    0x96,0x93,0x68,0xfd,0x33,0x86,0xa6,0x09,0x39,0x14,0x6c,0x7b,
    0x9e,0xf5,0x68,0xf0,0x59,0x2d,0x0e,0x73,0xe3,0x12,0xf3,0xff,
    0xdb,0xce,0x31,0x37,0x54,0x30,0x36,0x57,0x7a,0x40,0xe4,0x34,
    0x43,0xa9,0x2f,0x33,0x57,0x38,0xc1,0x55,0x59,0x0b,0x68,0x4d,
    0x90,0x2a,0xd1,0x94,0x70,0x81,0x51,0x61,0xfb,0x85,0x96,0x1d,
    0x79,0x85,0x43,0xb1,0x4c,0x3b,0x57,0xff,0x57,0xa5,0x7d,0x58,
    0xaf,0xcd,0x61,0x74,0xae,0xe0,0xe8,0x22,0x52,0x12,0x91,0xf1,
    0x75,0xe4,0x16,0x47,0x40,0xd2,0x47,0xa1,
};
static const UINT8 kRsaPlain[] = { 'w','o','l','f','C','r','y','p','t' };

/* ML-KEM Kyber512 deterministic seeds */
#if defined(WOLFSSL_HAVE_MLKEM)
static const UINT8 kyber512_rand[] = {
    0x7c,0x99,0x35,0xa0,0xb0,0x76,0x94,0xaa,
    0x0c,0x6d,0x10,0xe4,0xdb,0x6b,0x1a,0xdd,
    0x2f,0xd8,0x1a,0x25,0xcc,0xb1,0x48,0x03,
    0x2d,0xcd,0x73,0x99,0x36,0x73,0x7f,0x2d,
    0x86,0x26,0xed,0x79,0xd4,0x51,0x14,0x08,
    0x00,0xe0,0x3b,0x59,0xb9,0x56,0xf8,0x21,
    0x0e,0x55,0x60,0x67,0x40,0x7d,0x13,0xdc,
    0x90,0xfa,0x9e,0x8b,0x87,0x2b,0xfb,0x8f,
};
static const UINT8 kyber512enc_rand[] = {
    0x14,0x7c,0x03,0xf7,0xa5,0xbe,0xbb,0xa4,
    0x06,0xc8,0xfa,0xe1,0x87,0x4d,0x7f,0x13,
    0xc8,0x0e,0xfe,0x79,0xa3,0xa9,0xa8,0x74,
    0xcc,0x09,0xfe,0x76,0xf6,0x99,0x76,0x15,
};
#endif

/* Deterministic RNG seed */
static const UINT8 kDeterministicSeed[32] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
};

/* ------------------------------------------------------------------ */
/* Helpers */
/* ------------------------------------------------------------------ */
static EFI_STATUS CheckWolfResult(int ret, const CHAR16 *label)
{
    if (ret == 0) return EFI_SUCCESS;
    const char *err = (Api && Api->wc_GetErrorString) ?
                      Api->wc_GetErrorString(ret) : NULL;
    if (err) Print(L"%s failed: %d (%a)\n", label, ret, err);
    else     Print(L"%s failed: %d\n", label, ret);
    return EFI_ABORTED;
}

static EFI_STATUS CheckCondition(BOOLEAN cond, const CHAR16 *label)
{
    if (cond) return EFI_SUCCESS;
    Print(L"%s check failed\n", label);
    return EFI_ABORTED;
}

static BOOLEAN BuffersEqual(const UINT8 *a, const UINT8 *b, UINTN len)
{
    return CompareMem(a, b, len) == 0;
}

static VOID CopyBytes(UINT8 *dst, const UINT8 *src, UINTN len)
{
    for (UINTN i = 0; i < len; ++i) dst[i] = src[i];
}

static UINTN NonZeroCount(const UINT8 *buf, UINTN len)
{
    UINTN n = 0;
    for (UINTN i = 0; i < len; ++i) if (buf[i]) ++n;
    return n;
}

static BOOLEAN IsOptionalRngError(int ret)
{
    return (ret == NOT_COMPILED_IN) || (ret == WC_HW_E) ||
           (ret == BAD_FUNC_ARG)   || (ret == RNG_FAILURE_E);
}

static int EFIAPI DeterministicSeedCb(OS_Seed *os, byte *seed, word32 sz)
{
    (void)os;
    for (word32 i = 0; i < sz; ++i) seed[i] = (byte)(i & 0xff);
    return 0;
}

/* Init RNG with deterministic seed callback; clears callback on return. */
static int InitRngDet(WC_RNG *rng)
{
    Api->wc_SetSeed_Cb(DeterministicSeedCb);
    int ret = Api->wc_InitRng(rng);
    Api->wc_SetSeed_Cb(NULL);
    return ret;
}

/* ------------------------------------------------------------------ */
/* AES: ECB, CBC, CFB, CTR, GCM, CCM, OFB */
/* ------------------------------------------------------------------ */
static EFI_STATUS TestAes(void)
{
    EFI_STATUS status;
    int ret;
    Aes aes;
    UINT8 buf[32];
    UINT8 tag[16];
    UINT8 plain[16];

    ZeroMem(&aes, sizeof(aes));

    ret = Api->wc_AesInit(&aes, NULL, INVALID_DEVID);
    status = CheckWolfResult(ret, L"wc_AesInit");
    if (EFI_ERROR(status)) return status;

    /* ECB */
    ret = Api->wc_AesSetKey(&aes, kAesKey128, 16, NULL, AES_ENCRYPTION);
    status = CheckWolfResult(ret, L"AesSetKey ECB enc");
    if (EFI_ERROR(status)) goto done;
    CopyBytes(buf, kAesPlain, 16);
    ret = Api->wc_AesEcbEncrypt(&aes, buf, buf, 16);
    status = CheckWolfResult(ret, L"AesEcbEncrypt");
    if (EFI_ERROR(status)) goto done;
    status = CheckCondition(BuffersEqual(buf, kAesEcbCipher, 16), L"AES-ECB enc KAT");
    if (EFI_ERROR(status)) goto done;

    ret = Api->wc_AesSetKey(&aes, kAesKey128, 16, NULL, AES_DECRYPTION);
    status = CheckWolfResult(ret, L"AesSetKey ECB dec");
    if (EFI_ERROR(status)) goto done;
    ret = Api->wc_AesEcbDecrypt(&aes, buf, buf, 16);
    status = CheckWolfResult(ret, L"AesEcbDecrypt");
    if (EFI_ERROR(status)) goto done;
    status = CheckCondition(BuffersEqual(buf, kAesPlain, 16), L"AES-ECB dec KAT");
    if (EFI_ERROR(status)) goto done;

    /* CBC */
    ret = Api->wc_AesSetKey(&aes, kAesKey128, 16, kAesIv128, AES_ENCRYPTION);
    status = CheckWolfResult(ret, L"AesSetKey CBC enc");
    if (EFI_ERROR(status)) goto done;
    ret = Api->wc_AesCbcEncrypt(&aes, buf, kAesPlain, 16);
    status = CheckWolfResult(ret, L"AesCbcEncrypt");
    if (EFI_ERROR(status)) goto done;
    status = CheckCondition(BuffersEqual(buf, kAesCbcCipher, 16), L"AES-CBC enc KAT");
    if (EFI_ERROR(status)) goto done;

    ret = Api->wc_AesSetKey(&aes, kAesKey128, 16, kAesIv128, AES_DECRYPTION);
    status = CheckWolfResult(ret, L"AesSetKey CBC dec");
    if (EFI_ERROR(status)) goto done;
    ret = Api->wc_AesCbcDecrypt(&aes, buf, buf, 16);
    status = CheckWolfResult(ret, L"AesCbcDecrypt");
    if (EFI_ERROR(status)) goto done;
    status = CheckCondition(BuffersEqual(buf, kAesPlain, 16), L"AES-CBC dec KAT");
    if (EFI_ERROR(status)) goto done;

    /* CFB */
    ret = Api->wc_AesSetKey(&aes, kAesKey128, 16, kAesIv128, AES_ENCRYPTION);
    status = CheckWolfResult(ret, L"AesSetKey CFB");
    if (EFI_ERROR(status)) goto done;
    CopyBytes(buf, kAesPlain, 16);
    ret = Api->wc_AesCfbEncrypt(&aes, buf, buf, 16);
    status = CheckWolfResult(ret, L"AesCfbEncrypt");
    if (EFI_ERROR(status)) goto done;
    status = CheckCondition(BuffersEqual(buf, kAesCfbCipher, 16), L"AES-CFB enc KAT");
    if (EFI_ERROR(status)) goto done;
    ret = Api->wc_AesSetIV(&aes, kAesIv128);
    status = CheckWolfResult(ret, L"AesSetIV");
    if (EFI_ERROR(status)) goto done;
    ret = Api->wc_AesCfbDecrypt(&aes, buf, buf, 16);
    status = CheckWolfResult(ret, L"AesCfbDecrypt");
    if (EFI_ERROR(status)) goto done;
    status = CheckCondition(BuffersEqual(buf, kAesPlain, 16), L"AES-CFB dec KAT");
    if (EFI_ERROR(status)) goto done;

    /* CTR */
    ret = Api->wc_AesSetKey(&aes, kAesKey128, 16, kAesCtrIv, AES_ENCRYPTION);
    status = CheckWolfResult(ret, L"AesSetKey CTR");
    if (EFI_ERROR(status)) goto done;
    ret = Api->wc_AesCtrEncrypt(&aes, buf, kAesPlain, 16);
    status = CheckWolfResult(ret, L"AesCtrEncrypt");
    if (EFI_ERROR(status)) goto done;
    status = CheckCondition(BuffersEqual(buf, kAesCtrCipher, 16), L"AES-CTR enc KAT");
    if (EFI_ERROR(status)) goto done;
    /* CTR decrypt == encrypt */
    ret = Api->wc_AesSetKey(&aes, kAesKey128, 16, kAesCtrIv, AES_ENCRYPTION);
    status = CheckWolfResult(ret, L"AesSetKey CTR dec");
    if (EFI_ERROR(status)) goto done;
    ret = Api->wc_AesCtrEncrypt(&aes, plain, buf, 16);
    status = CheckWolfResult(ret, L"AesCtrEncrypt dec");
    if (EFI_ERROR(status)) goto done;
    status = CheckCondition(BuffersEqual(plain, kAesPlain, 16), L"AES-CTR dec KAT");
    if (EFI_ERROR(status)) goto done;

    /* GCM — round-trip (12-byte IV, no AAD) */
    {
        UINT8 gcmIv[12];
        UINT8 gcmCt[16];
        UINT8 gcmPt[16];
        UINT8 gcmTag[16];
        ZeroMem(gcmIv, sizeof(gcmIv));
        ret = Api->wc_AesGcmSetKey(&aes, kAesKey128, 16);
        status = CheckWolfResult(ret, L"AesGcmSetKey");
        if (EFI_ERROR(status)) goto done;
        ret = Api->wc_AesGcmEncrypt(&aes, gcmCt, kAesPlain, 16,
                                    gcmIv, 12, gcmTag, 16, NULL, 0);
        status = CheckWolfResult(ret, L"AesGcmEncrypt");
        if (EFI_ERROR(status)) goto done;
        ret = Api->wc_AesGcmDecrypt(&aes, gcmPt, gcmCt, 16,
                                    gcmIv, 12, gcmTag, 16, NULL, 0);
        status = CheckWolfResult(ret, L"AesGcmDecrypt");
        if (EFI_ERROR(status)) goto done;
        status = CheckCondition(BuffersEqual(gcmPt, kAesPlain, 16), L"AES-GCM round-trip");
        if (EFI_ERROR(status)) goto done;
    }

    /* CCM — round-trip (12-byte nonce, no AAD) */
    {
        UINT8 ccmNonce[12];
        UINT8 ccmCt[16];
        UINT8 ccmPt[16];
        UINT8 ccmTag[8];
        ZeroMem(ccmNonce, sizeof(ccmNonce));
        ret = Api->wc_AesCcmSetKey(&aes, kAesKey128, 16);
        status = CheckWolfResult(ret, L"AesCcmSetKey");
        if (EFI_ERROR(status)) goto done;
        ret = Api->wc_AesCcmEncrypt(&aes, ccmCt, kAesPlain, 16,
                                    ccmNonce, 12, ccmTag, 8, NULL, 0);
        status = CheckWolfResult(ret, L"AesCcmEncrypt");
        if (EFI_ERROR(status)) goto done;
        ret = Api->wc_AesCcmDecrypt(&aes, ccmPt, ccmCt, 16,
                                    ccmNonce, 12, ccmTag, 8, NULL, 0);
        status = CheckWolfResult(ret, L"AesCcmDecrypt");
        if (EFI_ERROR(status)) goto done;
        status = CheckCondition(BuffersEqual(ccmPt, kAesPlain, 16), L"AES-CCM round-trip");
        if (EFI_ERROR(status)) goto done;
    }

    /* OFB — round-trip */
    {
        UINT8 ofbCt[16];
        UINT8 ofbPt[16];
        ret = Api->wc_AesSetKey(&aes, kAesKey128, 16, kAesIv128, AES_ENCRYPTION);
        status = CheckWolfResult(ret, L"AesSetKey OFB enc");
        if (EFI_ERROR(status)) goto done;
        ret = Api->wc_AesOfbEncrypt(&aes, ofbCt, kAesPlain, 16);
        status = CheckWolfResult(ret, L"AesOfbEncrypt");
        if (EFI_ERROR(status)) goto done;
        ret = Api->wc_AesSetKey(&aes, kAesKey128, 16, kAesIv128, AES_ENCRYPTION);
        status = CheckWolfResult(ret, L"AesSetKey OFB dec");
        if (EFI_ERROR(status)) goto done;
        ret = Api->wc_AesOfbEncrypt(&aes, ofbPt, ofbCt, 16);
        status = CheckWolfResult(ret, L"AesOfbEncrypt dec");
        if (EFI_ERROR(status)) goto done;
        status = CheckCondition(BuffersEqual(ofbPt, kAesPlain, 16), L"AES-OFB round-trip");
        if (EFI_ERROR(status)) goto done;
    }

    (void)tag; (void)plain;

done:
    Api->wc_AesFree(&aes);
    return status;
}

/* ------------------------------------------------------------------ */
/* SHA: 1, 224, 256, 384, 512, SHA-3 (256/384/512), SHAKE-128/256 */
/* ------------------------------------------------------------------ */
static EFI_STATUS TestSha(void)
{
    EFI_STATUS status;
    int ret;
    UINT8 digest[WC_SHA3_512_DIGEST_SIZE];
    wc_Sha   sha1;
    wc_Sha224 sha224;
    wc_Sha256 sha256;
    wc_Sha384 sha384;
    wc_Sha512 sha512;
    wc_Sha3  sha3;
    wc_Shake shake;

#define SHA_TEST(init_fn, update_fn, final_fn, free_fn, ctx, dgst, expect, sz, label) \
    ret = Api->init_fn(&ctx); \
    status = CheckWolfResult(ret, L ## label " Init"); \
    if (EFI_ERROR(status)) return status; \
    ret = Api->update_fn(&ctx, kShaMsg, (word32)sizeof(kShaMsg)); \
    status = CheckWolfResult(ret, L ## label " Update"); \
    if (EFI_ERROR(status)) { Api->free_fn(&ctx); return status; } \
    ret = Api->final_fn(&ctx, dgst); \
    Api->free_fn(&ctx); \
    status = CheckWolfResult(ret, L ## label " Final"); \
    if (EFI_ERROR(status)) return status; \
    status = CheckCondition(BuffersEqual(dgst, expect, sz), L ## label " KAT"); \
    if (EFI_ERROR(status)) return status;

    SHA_TEST(wc_InitSha,    wc_ShaUpdate,    wc_ShaFinal,    wc_ShaFree,    sha1,   digest, kSha1Digest,    20, "SHA-1")
    SHA_TEST(wc_InitSha224, wc_Sha224Update, wc_Sha224Final, wc_Sha224Free, sha224, digest, kSha224Digest,  28, "SHA-224")
    SHA_TEST(wc_InitSha256, wc_Sha256Update, wc_Sha256Final, wc_Sha256Free, sha256, digest, kSha256Digest,  32, "SHA-256")
    SHA_TEST(wc_InitSha384, wc_Sha384Update, wc_Sha384Final, wc_Sha384Free, sha384, digest, kSha384Digest,  48, "SHA-384")
    SHA_TEST(wc_InitSha512, wc_Sha512Update, wc_Sha512Final, wc_Sha512Free, sha512, digest, kSha512Digest,  64, "SHA-512")
#define SHA3_TEST(init_fn, update_fn, final_fn, free_fn, dgst, expect, sz, label) \
    ret = Api->init_fn(&sha3, NULL, INVALID_DEVID); \
    status = CheckWolfResult(ret, L ## label " Init"); \
    if (EFI_ERROR(status)) return status; \
    ret = Api->update_fn(&sha3, kShaMsg, (word32)sizeof(kShaMsg)); \
    status = CheckWolfResult(ret, L ## label " Update"); \
    if (EFI_ERROR(status)) { Api->free_fn(&sha3); return status; } \
    ret = Api->final_fn(&sha3, dgst); \
    Api->free_fn(&sha3); \
    status = CheckWolfResult(ret, L ## label " Final"); \
    if (EFI_ERROR(status)) return status; \
    status = CheckCondition(BuffersEqual(dgst, expect, sz), L ## label " KAT"); \
    if (EFI_ERROR(status)) return status;

    SHA3_TEST(wc_InitSha3_256, wc_Sha3_256_Update, wc_Sha3_256_Final, wc_Sha3_256_Free, digest, kSha3_256Digest, 32, "SHA3-256")
    SHA3_TEST(wc_InitSha3_384, wc_Sha3_384_Update, wc_Sha3_384_Final, wc_Sha3_384_Free, digest, kSha3_384Digest, 48, "SHA3-384")
    SHA3_TEST(wc_InitSha3_512, wc_Sha3_512_Update, wc_Sha3_512_Final, wc_Sha3_512_Free, digest, kSha3_512Digest, 64, "SHA3-512")
#undef SHA3_TEST
#undef SHA_TEST

    /* SHAKE-128: produce 32 bytes, check non-zero */
    ret = Api->wc_InitShake128(&shake, NULL, INVALID_DEVID);
    status = CheckWolfResult(ret, L"SHAKE128 Init");
    if (EFI_ERROR(status)) return status;
    ret = Api->wc_Shake128_Update(&shake, kShaMsg, (word32)sizeof(kShaMsg));
    status = CheckWolfResult(ret, L"SHAKE128 Update");
    if (EFI_ERROR(status)) { Api->wc_Shake128_Free(&shake); return status; }
    ret = Api->wc_Shake128_Final(&shake, digest, 32);
    Api->wc_Shake128_Free(&shake);
    status = CheckWolfResult(ret, L"SHAKE128 Final");
    if (EFI_ERROR(status)) return status;
    status = CheckCondition(NonZeroCount(digest, 32) > 0, L"SHAKE128 output");
    if (EFI_ERROR(status)) return status;

    /* SHAKE-256: produce 64 bytes, check non-zero */
    ret = Api->wc_InitShake256(&shake, NULL, INVALID_DEVID);
    status = CheckWolfResult(ret, L"SHAKE256 Init");
    if (EFI_ERROR(status)) return status;
    ret = Api->wc_Shake256_Update(&shake, kShaMsg, (word32)sizeof(kShaMsg));
    status = CheckWolfResult(ret, L"SHAKE256 Update");
    if (EFI_ERROR(status)) { Api->wc_Shake256_Free(&shake); return status; }
    ret = Api->wc_Shake256_Final(&shake, digest, 64);
    Api->wc_Shake256_Free(&shake);
    status = CheckWolfResult(ret, L"SHAKE256 Final");
    if (EFI_ERROR(status)) return status;
    status = CheckCondition(NonZeroCount(digest, 64) > 0, L"SHAKE256 output");

    return status;
}

/* ------------------------------------------------------------------ */
/* HMAC: SHA-256 (KAT), SHA-384 (KAT), SHA-512 (KAT) */
/* ------------------------------------------------------------------ */
static EFI_STATUS TestHmac(void)
{
    EFI_STATUS status;
    int ret;
    Hmac hmac;
    UINT8 digest[WC_SHA512_DIGEST_SIZE];

    ZeroMem(&hmac, sizeof(hmac));

#define HMAC_TEST(hashtype, expected, sz, label) \
    ret = Api->wc_HmacSetKey(&hmac, hashtype, kHmacKey, (word32)sizeof(kHmacKey)); \
    status = CheckWolfResult(ret, L ## label " HmacSetKey"); \
    if (EFI_ERROR(status)) return status; \
    ret = Api->wc_HmacUpdate(&hmac, kHmacMsg, (word32)sizeof(kHmacMsg) - 1); \
    status = CheckWolfResult(ret, L ## label " HmacUpdate"); \
    if (EFI_ERROR(status)) return status; \
    ret = Api->wc_HmacFinal(&hmac, digest); \
    status = CheckWolfResult(ret, L ## label " HmacFinal"); \
    if (EFI_ERROR(status)) return status; \
    status = CheckCondition(BuffersEqual(digest, expected, sz), L ## label " KAT"); \
    if (EFI_ERROR(status)) return status;

    HMAC_TEST(WC_SHA256, kHmacSha256, 32, "HMAC-SHA256")
    HMAC_TEST(WC_SHA384, kHmacSha384, 48, "HMAC-SHA384")
    HMAC_TEST(WC_SHA512, kHmacSha512, 64, "HMAC-SHA512")
#undef HMAC_TEST

    return EFI_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* CMAC-AES-128 (NIST SP 800-38B example 2) */
/* ------------------------------------------------------------------ */
static EFI_STATUS TestCmac(void)
{
    EFI_STATUS status;
    int ret;
    Cmac cmac;
    UINT8 tag[AES_BLOCK_SIZE];
    word32 tagSz = sizeof(tag);

    ZeroMem(&cmac, sizeof(cmac));

    ret = Api->wc_InitCmac(&cmac, kAesKey128, (word32)sizeof(kAesKey128),
                           WC_CMAC_AES, NULL);
    status = CheckWolfResult(ret, L"wc_InitCmac");
    if (EFI_ERROR(status)) return status;

    ret = Api->wc_CmacUpdate(&cmac, kCmacMsg, (word32)sizeof(kCmacMsg));
    status = CheckWolfResult(ret, L"wc_CmacUpdate");
    if (EFI_ERROR(status)) return status;

    ret = Api->wc_CmacFinal(&cmac, tag, &tagSz);
    status = CheckWolfResult(ret, L"wc_CmacFinal");
    if (EFI_ERROR(status)) return status;

    status = CheckCondition(tagSz == AES_BLOCK_SIZE &&
                            BuffersEqual(tag, kCmacTag, AES_BLOCK_SIZE),
                            L"CMAC-AES-128 KAT");
    return status;
}

/* ------------------------------------------------------------------ */
/* KDF: PBKDF2, PKCS12-PBKDF, PKCS7-PadData, HKDF */
/* ------------------------------------------------------------------ */
static EFI_STATUS TestKdf(void)
{
    EFI_STATUS status;
    int ret;
    UINT8 derived[64];
    UINT8 padded[16];
    const char passwd[] = "passwordpassword";

    /* PBKDF2-SHA256 */
    ret = Api->wc_PBKDF2(derived, (const byte *)passwd, (int)(sizeof(passwd)-1),
                         kPbkdf2Salt, (int)sizeof(kPbkdf2Salt), 2048,
                         (int)sizeof(kPbkdf2Expected), WC_SHA256);
    status = CheckWolfResult(ret, L"wc_PBKDF2");
    if (EFI_ERROR(status)) return status;
    status = CheckCondition(BuffersEqual(derived, kPbkdf2Expected,
                                        sizeof(kPbkdf2Expected)), L"PBKDF2 KAT");
    if (EFI_ERROR(status)) return status;

    /* PKCS7 padding */
    ret = Api->wc_PKCS7_PadData((byte *)kPkcs7Input, (word32)sizeof(kPkcs7Input),
                                padded, (word32)sizeof(padded), 8);
    if (ret < 0) return CheckWolfResult(ret, L"wc_PKCS7_PadData");
    status = CheckCondition((ret == (int)sizeof(kPkcs7Expected)) &&
                            BuffersEqual(padded, kPkcs7Expected, sizeof(kPkcs7Expected)),
                            L"PKCS7 pad KAT");
    if (EFI_ERROR(status)) return status;

    /* HKDF — RFC 5869 test case 1 (SHA-256) */
    {
        static const UINT8 hkdfIkm[22]  = { 0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b };
        static const UINT8 hkdfSalt[13] = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c };
        static const UINT8 hkdfInfo[10] = { 0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9 };
        static const UINT8 hkdfOkm[42]  = {
            0x3c,0xb2,0x5f,0x25,0xfa,0xac,0xd5,0x7a,0x90,0x43,0x4f,0x64,0xd0,0x36,0x2f,0x2a,
            0x2d,0x2d,0x0a,0x90,0xcf,0x1a,0x5a,0x4c,0x5d,0xb0,0x2d,0x56,0xec,0xc4,0xc5,0xbf,
            0x34,0x00,0x72,0x08,0xd5,0xb8,0x87,0x18,0x58,0x65,
        };
        ret = Api->wc_HKDF(WC_SHA256,
                           hkdfIkm,  (word32)sizeof(hkdfIkm),
                           hkdfSalt, (word32)sizeof(hkdfSalt),
                           hkdfInfo, (word32)sizeof(hkdfInfo),
                           derived,  (word32)sizeof(hkdfOkm));
        status = CheckWolfResult(ret, L"wc_HKDF");
        if (EFI_ERROR(status)) return status;
        status = CheckCondition(BuffersEqual(derived, hkdfOkm, sizeof(hkdfOkm)),
                                L"HKDF RFC5869 TC1 KAT");
        if (EFI_ERROR(status)) return status;
    }

    return EFI_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* RNG */
/* ------------------------------------------------------------------ */
static EFI_STATUS TestRng(void)
{
    EFI_STATUS status;
    int ret;
    WC_RNG rng;
    UINT8 block[32];
    byte single;
    OS_Seed os;

    ZeroMem(&rng, sizeof(rng));

    ret = InitRngDet(&rng);
    status = CheckWolfResult(ret, L"wc_InitRng");
    if (EFI_ERROR(status)) return status;

    ret = Api->wc_RNG_TestSeed(kDeterministicSeed, (word32)sizeof(kDeterministicSeed));
    status = CheckWolfResult(ret, L"wc_RNG_TestSeed");
    if (EFI_ERROR(status)) { Api->wc_FreeRng(&rng); return status; }

    ret = Api->wc_RNG_GenerateBlock(&rng, block, (word32)sizeof(block));
    status = CheckWolfResult(ret, L"wc_RNG_GenerateBlock");
    if (EFI_ERROR(status)) { Api->wc_FreeRng(&rng); return status; }
    status = CheckCondition(NonZeroCount(block, sizeof(block)) > 0, L"RNG entropy");
    if (EFI_ERROR(status)) { Api->wc_FreeRng(&rng); return status; }

    ret = Api->wc_RNG_GenerateByte(&rng, &single);
    status = CheckWolfResult(ret, L"wc_RNG_GenerateByte");
    Api->wc_FreeRng(&rng);
    if (EFI_ERROR(status)) return status;

    /* Optional platform seed paths */
    ZeroMem(&os, sizeof(os));
    ret = Api->wc_GenerateSeed(&os, block, 16);
    if (ret != 0 && !IsOptionalRngError(ret))
        return CheckWolfResult(ret, L"wc_GenerateSeed");

    ret = Api->wc_GenerateSeed_IntelRD(&os, block, 16);
    if (ret != 0 && !IsOptionalRngError(ret))
        return CheckWolfResult(ret, L"wc_GenerateSeed_IntelRD");

    return EFI_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* RSA: decode, decrypt KAT, encrypt/decrypt round-trip, keygen */
/* ------------------------------------------------------------------ */
static EFI_STATUS TestRsa(void)
{
    EFI_STATUS status = EFI_SUCCESS;
    int ret;
    WC_RNG rng;
    RsaKey key, pubKey, genKey;
    BOOLEAN rngInit = FALSE, keyInit = FALSE, pubInit = FALSE, genInit = FALSE;
    UINT8 buffer[sizeof(kRsaKeyDer1024)];
    UINT8 cipher[sizeof(kRsaCiphertext)];
    UINT8 plain[sizeof(kRsaCiphertext)];
    word32 idx;
    int cipherLen;
    const UINT8 sample[] = "wolfSSL RSA";

    ZeroMem(&rng, sizeof(rng)); ZeroMem(&key, sizeof(key));
    ZeroMem(&pubKey, sizeof(pubKey)); ZeroMem(&genKey, sizeof(genKey));

    ret = Api->wc_InitRsaKey(&key, NULL);
    status = CheckWolfResult(ret, L"wc_InitRsaKey"); if (EFI_ERROR(status)) goto cleanup;
    keyInit = TRUE;

    idx = 0;
    ret = Api->wc_RsaPrivateKeyDecode(kRsaKeyDer1024, &idx, &key, (word32)sizeof(kRsaKeyDer1024));
    status = CheckWolfResult(ret, L"wc_RsaPrivateKeyDecode"); if (EFI_ERROR(status)) goto cleanup;

    ret = InitRngDet(&rng);
    if (ret == 0) {
        rngInit = TRUE;
        ret = Api->wc_RNG_TestSeed(kDeterministicSeed, (word32)sizeof(kDeterministicSeed));
        status = CheckWolfResult(ret, L"wc_RNG_TestSeed RSA"); if (EFI_ERROR(status)) goto cleanup;

        ret = Api->wc_RsaSetRNG(&key, &rng);
        status = CheckWolfResult(ret, L"wc_RsaSetRNG"); if (EFI_ERROR(status)) goto cleanup;
    } else if (IsOptionalRngError(ret)) {
        status = EFI_UNSUPPORTED; goto cleanup;
    } else {
        status = CheckWolfResult(ret, L"wc_InitRng RSA"); goto cleanup;
    }

    ret = Api->wc_CheckRsaKey(&key);
    status = CheckWolfResult(ret, L"wc_CheckRsaKey"); if (EFI_ERROR(status)) goto cleanup;

    cipherLen = Api->wc_RsaEncryptSize(&key);
    status = CheckCondition(cipherLen == (int)sizeof(kRsaCiphertext), L"wc_RsaEncryptSize");
    if (EFI_ERROR(status)) goto cleanup;

    /* DER re-encode */
    ret = Api->wc_RsaKeyToDer(&key, buffer, (word32)sizeof(buffer));
    if (ret < 0) { status = CheckWolfResult(ret, L"wc_RsaKeyToDer"); goto cleanup; }
    status = CheckCondition((ret == (int)sizeof(kRsaKeyDer1024)) &&
                            BuffersEqual(buffer, kRsaKeyDer1024, sizeof(kRsaKeyDer1024)),
                            L"RSA DER re-encode");
    if (EFI_ERROR(status)) goto cleanup;

    /* Deterministic decrypt KAT */
    ret = Api->wc_RsaPrivateDecrypt(kRsaCiphertext, (word32)sizeof(kRsaCiphertext),
                                    plain, (word32)sizeof(plain), &key);
    if (ret < 0) { status = CheckWolfResult(ret, L"wc_RsaPrivateDecrypt KAT"); goto cleanup; }
    status = CheckCondition((ret == (int)sizeof(kRsaPlain)) &&
                            BuffersEqual(plain, kRsaPlain, sizeof(kRsaPlain)),
                            L"RSA decrypt KAT");
    if (EFI_ERROR(status)) goto cleanup;

    /* Encrypt / decrypt round-trip */
    ret = Api->wc_RsaPublicEncrypt(sample, (word32)(sizeof(sample)-1),
                                   cipher, (word32)sizeof(cipher), &key, &rng);
    if (ret < 0) { status = CheckWolfResult(ret, L"wc_RsaPublicEncrypt"); goto cleanup; }
    cipherLen = ret;
    ret = Api->wc_RsaPrivateDecrypt(cipher, (word32)cipherLen,
                                    plain, (word32)sizeof(plain), &key);
    if (ret < 0) { status = CheckWolfResult(ret, L"wc_RsaPrivateDecrypt round-trip"); goto cleanup; }
    status = CheckCondition((ret == (int)(sizeof(sample)-1)) &&
                            BuffersEqual(plain, sample, sizeof(sample)-1),
                            L"RSA round-trip");
    if (EFI_ERROR(status)) goto cleanup;

    /* Export public DER, re-import, encrypt with public */
    ret = Api->wc_RsaKeyToPublicDer(&key, buffer, (word32)sizeof(buffer));
    if (ret < 0) { status = CheckWolfResult(ret, L"wc_RsaKeyToPublicDer"); goto cleanup; }
    int pubDerSz = ret;

    ret = Api->wc_InitRsaKey(&pubKey, NULL);
    status = CheckWolfResult(ret, L"wc_InitRsaKey pub"); if (EFI_ERROR(status)) goto cleanup;
    pubInit = TRUE;

    idx = 0;
    ret = Api->wc_RsaPublicKeyDecode(buffer, &idx, &pubKey, (word32)pubDerSz);
    status = CheckWolfResult(ret, L"wc_RsaPublicKeyDecode"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_RsaPublicEncrypt(sample, (word32)(sizeof(sample)-1),
                                   cipher, (word32)sizeof(cipher), &pubKey, &rng);
    if (ret < 0) { status = CheckWolfResult(ret, L"wc_RsaPublicEncrypt pubKey"); goto cleanup; }
    cipherLen = ret;
    ret = Api->wc_RsaPrivateDecrypt(cipher, (word32)cipherLen,
                                    plain, (word32)sizeof(plain), &key);
    if (ret < 0) { status = CheckWolfResult(ret, L"wc_RsaPrivateDecrypt pubKey"); goto cleanup; }
    status = CheckCondition((ret == (int)(sizeof(sample)-1)) &&
                            BuffersEqual(plain, sample, sizeof(sample)-1),
                            L"RSA pub-import round-trip");
    if (EFI_ERROR(status)) goto cleanup;

    /* Export key components */
    {
        UINT8 e[4], n[128], d[128], p[64], q[64];
        word32 eSz=sizeof(e), nSz=sizeof(n), dSz=sizeof(d), pSz=sizeof(p), qSz=sizeof(q);
        ret = Api->wc_RsaExportKey(&key, e,&eSz, n,&nSz, d,&dSz, p,&pSz, q,&qSz);
        status = CheckWolfResult(ret, L"wc_RsaExportKey"); if (EFI_ERROR(status)) goto cleanup;
        status = CheckCondition(eSz==3 && e[0]==0x01 && e[1]==0x00 && e[2]==0x01,
                                L"RSA exponent");
        if (EFI_ERROR(status)) goto cleanup;
    }

    /* Key generation (2048-bit) */
    ret = Api->wc_InitRsaKey(&genKey, NULL);
    status = CheckWolfResult(ret, L"wc_InitRsaKey gen"); if (EFI_ERROR(status)) goto cleanup;
    genInit = TRUE;
    ret = Api->wc_MakeRsaKey(&genKey, 2048, 65537, &rng);
    status = CheckWolfResult(ret, L"wc_MakeRsaKey"); if (EFI_ERROR(status)) goto cleanup;
    ret = Api->wc_RsaSetRNG(&genKey, &rng);
    status = CheckWolfResult(ret, L"wc_RsaSetRNG gen"); if (EFI_ERROR(status)) goto cleanup;
    ret = Api->wc_CheckRsaKey(&genKey);
    status = CheckWolfResult(ret, L"wc_CheckRsaKey gen");

cleanup:
    if (genInit) { Api->wc_FreeRsaKey(&genKey); Api->wc_RsaCleanup(&genKey); }
    if (pubInit) { Api->wc_FreeRsaKey(&pubKey); Api->wc_RsaCleanup(&pubKey); }
    if (keyInit) { Api->wc_FreeRsaKey(&key);    Api->wc_RsaCleanup(&key); }
    if (rngInit)   Api->wc_FreeRng(&rng);
    return status;
}

/* ------------------------------------------------------------------ */
/* ECC: keygen, ECDH, sign/verify */
/* ------------------------------------------------------------------ */
static EFI_STATUS TestEcc(void)
{
    EFI_STATUS status = EFI_SUCCESS;
    int ret;
    WC_RNG rng;
    ecc_key keyA, keyB;
    BOOLEAN rngInit = FALSE, aInit = FALSE, bInit = FALSE;
    UINT8 sharedA[32], sharedB[32];
    word32 sharedASz = sizeof(sharedA), sharedBSz = sizeof(sharedB);
    UINT8 pub[65];
    word32 pubSz = sizeof(pub);
    UINT8 hash[WC_SHA256_DIGEST_SIZE];
    UINT8 sig[ECC_MAX_SIG_SIZE];
    word32 sigSz = sizeof(sig);
    int verified = 0;

    ZeroMem(&rng,  sizeof(rng));
    ZeroMem(&keyA, sizeof(keyA));
    ZeroMem(&keyB, sizeof(keyB));

    ret = InitRngDet(&rng);
    if (IsOptionalRngError(ret)) { return EFI_UNSUPPORTED; }
    status = CheckWolfResult(ret, L"wc_InitRng ECC");
    if (EFI_ERROR(status)) return status;
    rngInit = TRUE;

    ret = Api->wc_ecc_init(&keyA);
    status = CheckWolfResult(ret, L"wc_ecc_init A"); if (EFI_ERROR(status)) goto cleanup;
    aInit = TRUE;

    ret = Api->wc_ecc_init(&keyB);
    status = CheckWolfResult(ret, L"wc_ecc_init B"); if (EFI_ERROR(status)) goto cleanup;
    bInit = TRUE;

    ret = Api->wc_ecc_make_key(&rng, 32, &keyA); /* P-256 */
    status = CheckWolfResult(ret, L"wc_ecc_make_key A"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_ecc_make_key(&rng, 32, &keyB);
    status = CheckWolfResult(ret, L"wc_ecc_make_key B"); if (EFI_ERROR(status)) goto cleanup;

    /* Set RNG for timing-resistant ECDH */
    ret = Api->wc_ecc_set_rng(&keyA, &rng);
    status = CheckWolfResult(ret, L"wc_ecc_set_rng A"); if (EFI_ERROR(status)) goto cleanup;
    ret = Api->wc_ecc_set_rng(&keyB, &rng);
    status = CheckWolfResult(ret, L"wc_ecc_set_rng B"); if (EFI_ERROR(status)) goto cleanup;

    /* ECDH */
    ret = Api->wc_ecc_shared_secret(&keyA, &keyB, sharedA, &sharedASz);
    status = CheckWolfResult(ret, L"wc_ecc_shared_secret A"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_ecc_shared_secret(&keyB, &keyA, sharedB, &sharedBSz);
    status = CheckWolfResult(ret, L"wc_ecc_shared_secret B"); if (EFI_ERROR(status)) goto cleanup;

    status = CheckCondition(sharedASz == sharedBSz &&
                            BuffersEqual(sharedA, sharedB, sharedASz),
                            L"ECC ECDH shared secret matches");
    if (EFI_ERROR(status)) goto cleanup;

    /* Sign/verify (hash of "wolfCrypt ECC test") */
    {
        wc_Sha256 sha;
        const byte msg[] = "wolfCrypt ECC test";
        ret = Api->wc_InitSha256(&sha);
        status = CheckWolfResult(ret, L"ECC SHA256 init"); if (EFI_ERROR(status)) goto cleanup;
        ret = Api->wc_Sha256Update(&sha, msg, (word32)sizeof(msg)-1);
        status = CheckWolfResult(ret, L"ECC SHA256 update"); if (EFI_ERROR(status)) { Api->wc_Sha256Free(&sha); goto cleanup; }
        ret = Api->wc_Sha256Final(&sha, hash);
        Api->wc_Sha256Free(&sha);
        status = CheckWolfResult(ret, L"ECC SHA256 final"); if (EFI_ERROR(status)) goto cleanup;
    }

    ret = Api->wc_ecc_sign_hash(hash, sizeof(hash), sig, &sigSz, &rng, &keyA);
    status = CheckWolfResult(ret, L"wc_ecc_sign_hash"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_ecc_verify_hash(sig, sigSz, hash, sizeof(hash), &verified, &keyA);
    status = CheckWolfResult(ret, L"wc_ecc_verify_hash"); if (EFI_ERROR(status)) goto cleanup;
    status = CheckCondition(verified == 1, L"ECC signature verified");
    if (EFI_ERROR(status)) goto cleanup;

    /* Export / import public key X9.63 */
    ret = Api->wc_ecc_export_x963(&keyA, pub, &pubSz);
    status = CheckWolfResult(ret, L"wc_ecc_export_x963"); if (EFI_ERROR(status)) goto cleanup;

    Api->wc_ecc_free(&keyB); bInit = FALSE;
    ret = Api->wc_ecc_init(&keyB); bInit = TRUE;
    status = CheckWolfResult(ret, L"wc_ecc_init B2"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_ecc_import_x963(pub, pubSz, &keyB);
    status = CheckWolfResult(ret, L"wc_ecc_import_x963");
    if (EFI_ERROR(status)) goto cleanup;

    verified = 0;
    ret = Api->wc_ecc_verify_hash(sig, sigSz, hash, sizeof(hash), &verified, &keyB);
    status = CheckWolfResult(ret, L"wc_ecc_verify_hash imported"); if (EFI_ERROR(status)) goto cleanup;
    status = CheckCondition(verified == 1, L"ECC verify with imported key");

cleanup:
    if (bInit) Api->wc_ecc_free(&keyB);
    if (aInit) Api->wc_ecc_free(&keyA);
    if (rngInit) Api->wc_FreeRng(&rng);
    return status;
}

/* ------------------------------------------------------------------ */
/* ChaCha20-Poly1305 AEAD round-trip */
/* ------------------------------------------------------------------ */
static EFI_STATUS TestChaCha20Poly1305(void)
{
    EFI_STATUS status;
    int ret;
    static const UINT8 key[CHACHA20_POLY1305_AEAD_KEYSIZE] = {
        0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
        0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
        0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
        0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
    };
    static const UINT8 iv[CHACHA20_POLY1305_AEAD_IV_SIZE] = {
        0x07,0x00,0x00,0x00,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
    };
    static const UINT8 aad[12] = {
        0x50,0x51,0x52,0x53,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
    };
    static const byte plaintext[] = "wolfCrypt ChaCha20-Poly1305 AEAD test message";
    UINT8 ct[sizeof(plaintext)];
    UINT8 pt[sizeof(plaintext)];
    UINT8 authTag[CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE];

    ZeroMem(ct, sizeof(ct));
    ZeroMem(pt, sizeof(pt));

    ret = Api->wc_ChaCha20Poly1305_Encrypt(key, iv,
                                           aad, sizeof(aad),
                                           (const byte *)plaintext, sizeof(plaintext)-1,
                                           ct, authTag);
    status = CheckWolfResult(ret, L"ChaCha20Poly1305_Encrypt");
    if (EFI_ERROR(status)) return status;

    ret = Api->wc_ChaCha20Poly1305_Decrypt(key, iv,
                                           aad, sizeof(aad),
                                           ct, sizeof(plaintext)-1,
                                           authTag, pt);
    status = CheckWolfResult(ret, L"ChaCha20Poly1305_Decrypt");
    if (EFI_ERROR(status)) return status;

    return CheckCondition(BuffersEqual(pt, (const UINT8 *)plaintext, sizeof(plaintext)-1),
                          L"ChaCha20-Poly1305 round-trip");
}

/* ------------------------------------------------------------------ */
/* Curve25519 (X25519) ECDH round-trip */
/* ------------------------------------------------------------------ */
static EFI_STATUS TestCurve25519(void)
{
    EFI_STATUS status = EFI_SUCCESS;
    int ret;
    WC_RNG rng;
    curve25519_key alice, bob;
    BOOLEAN rngInit = FALSE, aliceInit = FALSE, bobInit = FALSE;
    UINT8 sharedA[CURVE25519_KEYSIZE], sharedB[CURVE25519_KEYSIZE];
    word32 sharedASz = sizeof(sharedA), sharedBSz = sizeof(sharedB);

    ZeroMem(&rng, sizeof(rng));

    ret = InitRngDet(&rng);
    if (IsOptionalRngError(ret)) return EFI_UNSUPPORTED;
    status = CheckWolfResult(ret, L"wc_InitRng Curve25519"); if (EFI_ERROR(status)) return status;
    rngInit = TRUE;

    ret = Api->wc_curve25519_init(&alice); status = CheckWolfResult(ret, L"curve25519_init alice"); if (EFI_ERROR(status)) goto cleanup; aliceInit = TRUE;
    ret = Api->wc_curve25519_init(&bob);   status = CheckWolfResult(ret, L"curve25519_init bob");   if (EFI_ERROR(status)) goto cleanup; bobInit   = TRUE;

    ret = Api->wc_curve25519_make_key(&rng, CURVE25519_KEYSIZE, &alice);
    status = CheckWolfResult(ret, L"curve25519_make_key alice"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_curve25519_make_key(&rng, CURVE25519_KEYSIZE, &bob);
    status = CheckWolfResult(ret, L"curve25519_make_key bob"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_curve25519_shared_secret(&alice, &bob, sharedA, &sharedASz);
    status = CheckWolfResult(ret, L"curve25519_shared_secret alice"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_curve25519_shared_secret(&bob, &alice, sharedB, &sharedBSz);
    status = CheckWolfResult(ret, L"curve25519_shared_secret bob"); if (EFI_ERROR(status)) goto cleanup;

    status = CheckCondition(sharedASz == sharedBSz &&
                            BuffersEqual(sharedA, sharedB, sharedASz),
                            L"Curve25519 shared secret matches");

cleanup:
    if (bobInit)   Api->wc_curve25519_free(&bob);
    if (aliceInit) Api->wc_curve25519_free(&alice);
    if (rngInit)   Api->wc_FreeRng(&rng);
    return status;
}

/* ------------------------------------------------------------------ */
/* Ed25519: keygen, sign, verify */
/* ------------------------------------------------------------------ */
static EFI_STATUS TestEd25519(void)
{
    EFI_STATUS status = EFI_SUCCESS;
    int ret;
    WC_RNG rng;
    ed25519_key key;
    BOOLEAN rngInit = FALSE, keyInit = FALSE;
    static const byte msg[] = "wolfCrypt UEFI Ed25519 test";
    UINT8 sig[ED25519_SIG_SIZE];
    word32 sigSz = sizeof(sig);
    int verified = 0;

    ZeroMem(&rng, sizeof(rng));
    ZeroMem(&key, sizeof(key));

    ret = InitRngDet(&rng);
    if (IsOptionalRngError(ret)) return EFI_UNSUPPORTED;
    status = CheckWolfResult(ret, L"wc_InitRng Ed25519"); if (EFI_ERROR(status)) return status;
    rngInit = TRUE;

    ret = Api->wc_ed25519_init(&key);
    status = CheckWolfResult(ret, L"wc_ed25519_init"); if (EFI_ERROR(status)) goto cleanup;
    keyInit = TRUE;

    ret = Api->wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &key);
    status = CheckWolfResult(ret, L"wc_ed25519_make_key"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_ed25519_sign_msg(msg, (word32)sizeof(msg)-1, sig, &sigSz, &key);
    status = CheckWolfResult(ret, L"wc_ed25519_sign_msg"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_ed25519_verify_msg(sig, sigSz, msg, (word32)sizeof(msg)-1, &verified, &key);
    status = CheckWolfResult(ret, L"wc_ed25519_verify_msg"); if (EFI_ERROR(status)) goto cleanup;
    status = CheckCondition(verified == 1, L"Ed25519 signature verified");

cleanup:
    if (keyInit) Api->wc_ed25519_free(&key);
    if (rngInit) Api->wc_FreeRng(&rng);
    return status;
}

/* ------------------------------------------------------------------ */
/* ML-KEM (Kyber512): full encode/decode round-trip */
/* ------------------------------------------------------------------ */
#if defined(WOLFSSL_HAVE_MLKEM)
static EFI_STATUS TestMlKem(void)
{
    EFI_STATUS status = EFI_SUCCESS;
    int ret;
    WC_RNG rng;
    MlKemKey key;
    BOOLEAN rngInit = FALSE, keyInit = FALSE, seedCbActive = FALSE;
    UINT8 priv[KYBER512_PRIVATE_KEY_SIZE];
    UINT8 privExpected[KYBER512_PRIVATE_KEY_SIZE];
    UINT8 pub[KYBER512_PUBLIC_KEY_SIZE];
    UINT8 pubExpected[KYBER512_PUBLIC_KEY_SIZE];
    UINT8 ct[KYBER512_CIPHER_TEXT_SIZE];
    UINT8 ss[KYBER_SS_SZ], ssDec[KYBER_SS_SZ];
    word32 len;

    ZeroMem(&rng, sizeof(rng)); ZeroMem(&key, sizeof(key));

    if (Api->wc_SetSeed_Cb(DeterministicSeedCb) == 0) seedCbActive = TRUE;
    ret = Api->wc_InitRng(&rng);
    status = CheckWolfResult(ret, L"wc_InitRng ML-KEM");
    if (EFI_ERROR(status)) { if (seedCbActive) Api->wc_SetSeed_Cb(NULL); return status; }
    rngInit = TRUE;

    ret = Api->wc_RNG_TestSeed(kyber512_rand, (word32)sizeof(kyber512_rand));
    status = CheckWolfResult(ret, L"wc_RNG_TestSeed ML-KEM key"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_MlKemKey_Init(&key, KYBER512, NULL, INVALID_DEVID);
    status = CheckWolfResult(ret, L"wc_MlKemKey_Init"); if (EFI_ERROR(status)) goto cleanup;
    keyInit = TRUE;

    ret = Api->wc_MlKemKey_MakeKey(&key, &rng);
    status = CheckWolfResult(ret, L"wc_MlKemKey_MakeKey"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_MlKemKey_EncodePublicKey(&key, pub, (word32)sizeof(pub));
    status = CheckWolfResult(ret, L"EncodePublicKey"); if (EFI_ERROR(status)) goto cleanup;
    CopyBytes(pubExpected, pub, sizeof(pub));

    ret = Api->wc_MlKemKey_EncodePrivateKey(&key, priv, (word32)sizeof(priv));
    status = CheckWolfResult(ret, L"EncodePrivateKey"); if (EFI_ERROR(status)) goto cleanup;
    CopyBytes(privExpected, priv, sizeof(priv));

    ret = Api->wc_MlKemKey_PublicKeySize(&key, &len);
    status = CheckWolfResult(ret, L"PublicKeySize"); if (EFI_ERROR(status)) goto cleanup;
    status = CheckCondition(len == KYBER512_PUBLIC_KEY_SIZE, L"ML-KEM pub size"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_MlKemKey_PrivateKeySize(&key, &len);
    status = CheckWolfResult(ret, L"PrivateKeySize"); if (EFI_ERROR(status)) goto cleanup;
    status = CheckCondition(len == KYBER512_PRIVATE_KEY_SIZE, L"ML-KEM priv size"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_MlKemKey_CipherTextSize(&key, &len);
    status = CheckWolfResult(ret, L"CipherTextSize"); if (EFI_ERROR(status)) goto cleanup;
    status = CheckCondition(len == KYBER512_CIPHER_TEXT_SIZE, L"ML-KEM ct size"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_MlKemKey_SharedSecretSize(&key, &len);
    status = CheckWolfResult(ret, L"SharedSecretSize"); if (EFI_ERROR(status)) goto cleanup;
    status = CheckCondition(len == KYBER_SS_SZ, L"ML-KEM ss size"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_RNG_TestSeed(kyber512enc_rand, (word32)sizeof(kyber512enc_rand));
    status = CheckWolfResult(ret, L"wc_RNG_TestSeed ML-KEM enc"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_MlKemKey_Encapsulate(&key, ct, ss, &rng);
    status = CheckWolfResult(ret, L"wc_MlKemKey_Encapsulate"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_MlKemKey_Decapsulate(&key, ssDec, ct, (word32)sizeof(ct));
    status = CheckWolfResult(ret, L"wc_MlKemKey_Decapsulate"); if (EFI_ERROR(status)) goto cleanup;
    status = CheckCondition(BuffersEqual(ss, ssDec, sizeof(ss)), L"ML-KEM shared secret"); if (EFI_ERROR(status)) goto cleanup;

    /* Decode round-trip */
    Api->wc_MlKemKey_Free(&key); keyInit = FALSE;
    ret = Api->wc_MlKemKey_Init(&key, KYBER512, NULL, INVALID_DEVID);
    status = CheckWolfResult(ret, L"wc_MlKemKey_Init decode"); if (EFI_ERROR(status)) goto cleanup;
    keyInit = TRUE;

    ret = Api->wc_MlKemKey_DecodePublicKey(&key, pubExpected, (word32)sizeof(pubExpected));
    status = CheckWolfResult(ret, L"DecodePublicKey"); if (EFI_ERROR(status)) goto cleanup;
    ret = Api->wc_MlKemKey_EncodePublicKey(&key, pub, (word32)sizeof(pub));
    status = CheckWolfResult(ret, L"EncodePublicKey decoded"); if (EFI_ERROR(status)) goto cleanup;
    status = CheckCondition(BuffersEqual(pub, pubExpected, sizeof(pub)), L"ML-KEM pub round-trip"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_MlKemKey_DecodePrivateKey(&key, privExpected, (word32)sizeof(privExpected));
    status = CheckWolfResult(ret, L"DecodePrivateKey"); if (EFI_ERROR(status)) goto cleanup;
    ret = Api->wc_MlKemKey_EncodePrivateKey(&key, priv, (word32)sizeof(priv));
    status = CheckWolfResult(ret, L"EncodePrivateKey decoded"); if (EFI_ERROR(status)) goto cleanup;
    status = CheckCondition(BuffersEqual(priv, privExpected, sizeof(priv)), L"ML-KEM priv round-trip");

cleanup:
    if (seedCbActive) Api->wc_SetSeed_Cb(NULL);
    if (keyInit)  Api->wc_MlKemKey_Free(&key);
    if (rngInit)  Api->wc_FreeRng(&rng);
    return status;
}
#endif /* WOLFSSL_HAVE_MLKEM */

/* ------------------------------------------------------------------ */
/* Dilithium (ML-DSA-44): keygen, sign, verify */
/* ------------------------------------------------------------------ */
#if defined(HAVE_DILITHIUM)
static EFI_STATUS TestDilithium(void)
{
    EFI_STATUS status = EFI_SUCCESS;
    int ret;
    WC_RNG rng;
    dilithium_key key;
    BOOLEAN rngInit = FALSE, keyInit = FALSE;
    static const byte msg[] = "wolfCrypt UEFI Dilithium test";
    byte sig[DILITHIUM_MAX_SIG_SIZE];
    word32 sigLen = sizeof(sig);
    int verify = 0;

    ZeroMem(&rng, sizeof(rng)); ZeroMem(&key, sizeof(key));

    ret = InitRngDet(&rng);
    status = CheckWolfResult(ret, L"wc_InitRng Dilithium"); if (EFI_ERROR(status)) return status;
    rngInit = TRUE;

    ret = Api->wc_dilithium_init(&key);
    status = CheckWolfResult(ret, L"wc_dilithium_init"); if (EFI_ERROR(status)) goto cleanup;
    keyInit = TRUE;

    ret = Api->wc_dilithium_set_level(&key, 2); /* ML-DSA-44 */
    status = CheckWolfResult(ret, L"wc_dilithium_set_level"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_dilithium_make_key(&key, &rng);
    status = CheckWolfResult(ret, L"wc_dilithium_make_key"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_dilithium_sign_msg(msg, (word32)sizeof(msg)-1, sig, &sigLen, &key, &rng);
    status = CheckWolfResult(ret, L"wc_dilithium_sign_msg"); if (EFI_ERROR(status)) goto cleanup;

    ret = Api->wc_dilithium_verify_msg(sig, sigLen, msg, (word32)sizeof(msg)-1, &verify, &key);
    status = CheckWolfResult(ret, L"wc_dilithium_verify_msg"); if (EFI_ERROR(status)) goto cleanup;
    status = CheckCondition(verify == 1, L"Dilithium signature verified");

cleanup:
    if (keyInit) Api->wc_dilithium_free(&key);
    if (rngInit) Api->wc_FreeRng(&rng);
    return status;
}
#endif /* HAVE_DILITHIUM */

/* ------------------------------------------------------------------ */
/* Test runner */
/* ------------------------------------------------------------------ */
static EFI_STATUS RunAllTests(void)
{
    static const struct {
        const CHAR16   *name;
        EFI_STATUS    (*func)(void);
    } tests[] = {
        { L"AES",               TestAes              },
        { L"SHA",               TestSha              },
        { L"HMAC",              TestHmac             },
        { L"CMAC",              TestCmac             },
        { L"KDF",               TestKdf              },
        { L"RNG",               TestRng              },
        { L"RSA",               TestRsa              },
        { L"ECC",               TestEcc              },
        { L"ChaCha20-Poly1305", TestChaCha20Poly1305 },
        { L"Curve25519",        TestCurve25519       },
        { L"Ed25519",           TestEd25519          },
#if defined(WOLFSSL_HAVE_MLKEM)
        { L"ML-KEM",            TestMlKem            },
#endif
#if defined(HAVE_DILITHIUM)
        { L"Dilithium",         TestDilithium        },
#endif
    };

    for (UINTN i = 0; i < ARRAY_SIZE(tests); ++i) {
        Print(L"Running %-20s ... ", tests[i].name);
        EFI_STATUS st = tests[i].func();
        if (st == EFI_UNSUPPORTED) { Print(L"skipped\n"); continue; }
        if (EFI_ERROR(st))         { Print(L"FAILED\n"); return st; }
        Print(L"passed\n");
    }
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;

    InitializeLib(ImageHandle, SystemTable);
    Print(L"test.efi loaded\n");

    status = uefi_call_wrapper(BS->LocateProtocol, 3,
                               &g_wolfcrypt_protocol_guid, NULL, (VOID **)&Api);
    if (EFI_ERROR(status) || Api == NULL) {
        Print(L"wolfCrypt protocol not found: %r\n", status);
        return status;
    }

    Print(L"wolfCrypt protocol found, version 0x%08x\n", Api->Version);
    Print(L"\n");

    status = RunAllTests();
    if (EFI_ERROR(status)) {
        Print(L"\nTest suite FAILED: %r\n", status);
        return status;
    }

    Print(L"\nAll tests passed!\n");
    return EFI_SUCCESS;
}
