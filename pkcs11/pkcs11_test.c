/* pkcs11_test.c
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
#include <wolfssl/wolfcrypt/wc_pkcs11.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

#ifndef NO_RSA
static const unsigned char client_key_der_2048[] =
{
        0x30, 0x82, 0x04, 0xA4, 0x02, 0x01, 0x00, 0x02, 0x82, 0x01,
        0x01, 0x00, 0xC3, 0x03, 0xD1, 0x2B, 0xFE, 0x39, 0xA4, 0x32,
        0x45, 0x3B, 0x53, 0xC8, 0x84, 0x2B, 0x2A, 0x7C, 0x74, 0x9A,
        0xBD, 0xAA, 0x2A, 0x52, 0x07, 0x47, 0xD6, 0xA6, 0x36, 0xB2,
        0x07, 0x32, 0x8E, 0xD0, 0xBA, 0x69, 0x7B, 0xC6, 0xC3, 0x44,
        0x9E, 0xD4, 0x81, 0x48, 0xFD, 0x2D, 0x68, 0xA2, 0x8B, 0x67,
        0xBB, 0xA1, 0x75, 0xC8, 0x36, 0x2C, 0x4A, 0xD2, 0x1B, 0xF7,
        0x8B, 0xBA, 0xCF, 0x0D, 0xF9, 0xEF, 0xEC, 0xF1, 0x81, 0x1E,
        0x7B, 0x9B, 0x03, 0x47, 0x9A, 0xBF, 0x65, 0xCC, 0x7F, 0x65,
        0x24, 0x69, 0xA6, 0xE8, 0x14, 0x89, 0x5B, 0xE4, 0x34, 0xF7,
        0xC5, 0xB0, 0x14, 0x93, 0xF5, 0x67, 0x7B, 0x3A, 0x7A, 0x78,
        0xE1, 0x01, 0x56, 0x56, 0x91, 0xA6, 0x13, 0x42, 0x8D, 0xD2,
        0x3C, 0x40, 0x9C, 0x4C, 0xEF, 0xD1, 0x86, 0xDF, 0x37, 0x51,
        0x1B, 0x0C, 0xA1, 0x3B, 0xF5, 0xF1, 0xA3, 0x4A, 0x35, 0xE4,
        0xE1, 0xCE, 0x96, 0xDF, 0x1B, 0x7E, 0xBF, 0x4E, 0x97, 0xD0,
        0x10, 0xE8, 0xA8, 0x08, 0x30, 0x81, 0xAF, 0x20, 0x0B, 0x43,
        0x14, 0xC5, 0x74, 0x67, 0xB4, 0x32, 0x82, 0x6F, 0x8D, 0x86,
        0xC2, 0x88, 0x40, 0x99, 0x36, 0x83, 0xBA, 0x1E, 0x40, 0x72,
        0x22, 0x17, 0xD7, 0x52, 0x65, 0x24, 0x73, 0xB0, 0xCE, 0xEF,
        0x19, 0xCD, 0xAE, 0xFF, 0x78, 0x6C, 0x7B, 0xC0, 0x12, 0x03,
        0xD4, 0x4E, 0x72, 0x0D, 0x50, 0x6D, 0x3B, 0xA3, 0x3B, 0xA3,
        0x99, 0x5E, 0x9D, 0xC8, 0xD9, 0x0C, 0x85, 0xB3, 0xD9, 0x8A,
        0xD9, 0x54, 0x26, 0xDB, 0x6D, 0xFA, 0xAC, 0xBB, 0xFF, 0x25,
        0x4C, 0xC4, 0xD1, 0x79, 0xF4, 0x71, 0xD3, 0x86, 0x40, 0x18,
        0x13, 0xB0, 0x63, 0xB5, 0x72, 0x4E, 0x30, 0xC4, 0x97, 0x84,
        0x86, 0x2D, 0x56, 0x2F, 0xD7, 0x15, 0xF7, 0x7F, 0xC0, 0xAE,
        0xF5, 0xFC, 0x5B, 0xE5, 0xFB, 0xA1, 0xBA, 0xD3, 0x02, 0x03,
        0x01, 0x00, 0x01, 0x02, 0x82, 0x01, 0x01, 0x00, 0xA2, 0xE6,
        0xD8, 0x5F, 0x10, 0x71, 0x64, 0x08, 0x9E, 0x2E, 0x6D, 0xD1,
        0x6D, 0x1E, 0x85, 0xD2, 0x0A, 0xB1, 0x8C, 0x47, 0xCE, 0x2C,
        0x51, 0x6A, 0xA0, 0x12, 0x9E, 0x53, 0xDE, 0x91, 0x4C, 0x1D,
        0x6D, 0xEA, 0x59, 0x7B, 0xF2, 0x77, 0xAA, 0xD9, 0xC6, 0xD9,
        0x8A, 0xAB, 0xD8, 0xE1, 0x16, 0xE4, 0x63, 0x26, 0xFF, 0xB5,
        0x6C, 0x13, 0x59, 0xB8, 0xE3, 0xA5, 0xC8, 0x72, 0x17, 0x2E,
        0x0C, 0x9F, 0x6F, 0xE5, 0x59, 0x3F, 0x76, 0x6F, 0x49, 0xB1,
        0x11, 0xC2, 0x5A, 0x2E, 0x16, 0x29, 0x0D, 0xDE, 0xB7, 0x8E,
        0xDC, 0x40, 0xD5, 0xA2, 0xEE, 0xE0, 0x1E, 0xA1, 0xF4, 0xBE,
        0x97, 0xDB, 0x86, 0x63, 0x96, 0x14, 0xCD, 0x98, 0x09, 0x60,
        0x2D, 0x30, 0x76, 0x9C, 0x3C, 0xCD, 0xE6, 0x88, 0xEE, 0x47,
        0x92, 0x79, 0x0B, 0x5A, 0x00, 0xE2, 0x5E, 0x5F, 0x11, 0x7C,
        0x7D, 0xF9, 0x08, 0xB7, 0x20, 0x06, 0x89, 0x2A, 0x5D, 0xFD,
        0x00, 0xAB, 0x22, 0xE1, 0xF0, 0xB3, 0xBC, 0x24, 0xA9, 0x5E,
        0x26, 0x0E, 0x1F, 0x00, 0x2D, 0xFE, 0x21, 0x9A, 0x53, 0x5B,
        0x6D, 0xD3, 0x2B, 0xAB, 0x94, 0x82, 0x68, 0x43, 0x36, 0xD8,
        0xF6, 0x2F, 0xC6, 0x22, 0xFC, 0xB5, 0x41, 0x5D, 0x0D, 0x33,
        0x60, 0xEA, 0xA4, 0x7D, 0x7E, 0xE8, 0x4B, 0x55, 0x91, 0x56,
        0xD3, 0x5C, 0x57, 0x8F, 0x1F, 0x94, 0x17, 0x2F, 0xAA, 0xDE,
        0xE9, 0x9E, 0xA8, 0xF4, 0xCF, 0x8A, 0x4C, 0x8E, 0xA0, 0xE4,
        0x56, 0x73, 0xB2, 0xCF, 0x4F, 0x86, 0xC5, 0x69, 0x3C, 0xF3,
        0x24, 0x20, 0x8B, 0x5C, 0x96, 0x0C, 0xFA, 0x6B, 0x12, 0x3B,
        0x9A, 0x67, 0xC1, 0xDF, 0xC6, 0x96, 0xB2, 0xA5, 0xD5, 0x92,
        0x0D, 0x9B, 0x09, 0x42, 0x68, 0x24, 0x10, 0x45, 0xD4, 0x50,
        0xE4, 0x17, 0x39, 0x48, 0xD0, 0x35, 0x8B, 0x94, 0x6D, 0x11,
        0xDE, 0x8F, 0xCA, 0x59, 0x02, 0x81, 0x81, 0x00, 0xEA, 0x24,
        0xA7, 0xF9, 0x69, 0x33, 0xE9, 0x71, 0xDC, 0x52, 0x7D, 0x88,
        0x21, 0x28, 0x2F, 0x49, 0xDE, 0xBA, 0x72, 0x16, 0xE9, 0xCC,
        0x47, 0x7A, 0x88, 0x0D, 0x94, 0x57, 0x84, 0x58, 0x16, 0x3A,
        0x81, 0xB0, 0x3F, 0xA2, 0xCF, 0xA6, 0x6C, 0x1E, 0xB0, 0x06,
        0x29, 0x00, 0x8F, 0xE7, 0x77, 0x76, 0xAC, 0xDB, 0xCA, 0xC7,
        0xD9, 0x5E, 0x9B, 0x3F, 0x26, 0x90, 0x52, 0xAE, 0xFC, 0x38,
        0x90, 0x00, 0x14, 0xBB, 0xB4, 0x0F, 0x58, 0x94, 0xE7, 0x2F,
        0x6A, 0x7E, 0x1C, 0x4F, 0x41, 0x21, 0xD4, 0x31, 0x59, 0x1F,
        0x4E, 0x8A, 0x1A, 0x8D, 0xA7, 0x57, 0x6C, 0x22, 0xD8, 0xE5,
        0xF4, 0x7E, 0x32, 0xA6, 0x10, 0xCB, 0x64, 0xA5, 0x55, 0x03,
        0x87, 0xA6, 0x27, 0x05, 0x8C, 0xC3, 0xD7, 0xB6, 0x27, 0xB2,
        0x4D, 0xBA, 0x30, 0xDA, 0x47, 0x8F, 0x54, 0xD3, 0x3D, 0x8B,
        0x84, 0x8D, 0x94, 0x98, 0x58, 0xA5, 0x02, 0x81, 0x81, 0x00,
        0xD5, 0x38, 0x1B, 0xC3, 0x8F, 0xC5, 0x93, 0x0C, 0x47, 0x0B,
        0x6F, 0x35, 0x92, 0xC5, 0xB0, 0x8D, 0x46, 0xC8, 0x92, 0x18,
        0x8F, 0xF5, 0x80, 0x0A, 0xF7, 0xEF, 0xA1, 0xFE, 0x80, 0xB9,
        0xB5, 0x2A, 0xBA, 0xCA, 0x18, 0xB0, 0x5D, 0xA5, 0x07, 0xD0,
        0x93, 0x8D, 0xD8, 0x9C, 0x04, 0x1C, 0xD4, 0x62, 0x8E, 0xA6,
        0x26, 0x81, 0x01, 0xFF, 0xCE, 0x8A, 0x2A, 0x63, 0x34, 0x35,
        0x40, 0xAA, 0x6D, 0x80, 0xDE, 0x89, 0x23, 0x6A, 0x57, 0x4D,
        0x9E, 0x6E, 0xAD, 0x93, 0x4E, 0x56, 0x90, 0x0B, 0x6D, 0x9D,
        0x73, 0x8B, 0x0C, 0xAE, 0x27, 0x3D, 0xDE, 0x4E, 0xF0, 0xAA,
        0xC5, 0x6C, 0x78, 0x67, 0x6C, 0x94, 0x52, 0x9C, 0x37, 0x67,
        0x6C, 0x2D, 0xEF, 0xBB, 0xAF, 0xDF, 0xA6, 0x90, 0x3C, 0xC4,
        0x47, 0xCF, 0x8D, 0x96, 0x9E, 0x98, 0xA9, 0xB4, 0x9F, 0xC5,
        0xA6, 0x50, 0xDC, 0xB3, 0xF0, 0xFB, 0x74, 0x17, 0x02, 0x81,
        0x80, 0x5E, 0x83, 0x09, 0x62, 0xBD, 0xBA, 0x7C, 0xA2, 0xBF,
        0x42, 0x74, 0xF5, 0x7C, 0x1C, 0xD2, 0x69, 0xC9, 0x04, 0x0D,
        0x85, 0x7E, 0x3E, 0x3D, 0x24, 0x12, 0xC3, 0x18, 0x7B, 0xF3,
        0x29, 0xF3, 0x5F, 0x0E, 0x76, 0x6C, 0x59, 0x75, 0xE4, 0x41,
        0x84, 0x69, 0x9D, 0x32, 0xF3, 0xCD, 0x22, 0xAB, 0xB0, 0x35,
        0xBA, 0x4A, 0xB2, 0x3C, 0xE5, 0xD9, 0x58, 0xB6, 0x62, 0x4F,
        0x5D, 0xDE, 0xE5, 0x9E, 0x0A, 0xCA, 0x53, 0xB2, 0x2C, 0xF7,
        0x9E, 0xB3, 0x6B, 0x0A, 0x5B, 0x79, 0x65, 0xEC, 0x6E, 0x91,
        0x4E, 0x92, 0x20, 0xF6, 0xFC, 0xFC, 0x16, 0xED, 0xD3, 0x76,
        0x0C, 0xE2, 0xEC, 0x7F, 0xB2, 0x69, 0x13, 0x6B, 0x78, 0x0E,
        0x5A, 0x46, 0x64, 0xB4, 0x5E, 0xB7, 0x25, 0xA0, 0x5A, 0x75,
        0x3A, 0x4B, 0xEF, 0xC7, 0x3C, 0x3E, 0xF7, 0xFD, 0x26, 0xB8,
        0x20, 0xC4, 0x99, 0x0A, 0x9A, 0x73, 0xBE, 0xC3, 0x19, 0x02,
        0x81, 0x81, 0x00, 0xBA, 0x44, 0x93, 0x14, 0xAC, 0x34, 0x19,
        0x3B, 0x5F, 0x91, 0x60, 0xAC, 0xF7, 0xB4, 0xD6, 0x81, 0x05,
        0x36, 0x51, 0x53, 0x3D, 0xE8, 0x65, 0xDC, 0xAF, 0x2E, 0xDC,
        0x61, 0x3E, 0xC9, 0x7D, 0xB8, 0x7F, 0x87, 0xF0, 0x3B, 0x9B,
        0x03, 0x82, 0x29, 0x37, 0xCE, 0x72, 0x4E, 0x11, 0xD5, 0xB1,
        0xC1, 0x0C, 0x07, 0xA0, 0x99, 0x91, 0x4A, 0x8D, 0x7F, 0xEC,
        0x79, 0xCF, 0xF1, 0x39, 0xB5, 0xE9, 0x85, 0xEC, 0x62, 0xF7,
        0xDA, 0x7D, 0xBC, 0x64, 0x4D, 0x22, 0x3C, 0x0E, 0xF2, 0xD6,
        0x51, 0xF5, 0x87, 0xD8, 0x99, 0xC0, 0x11, 0x20, 0x5D, 0x0F,
        0x29, 0xFD, 0x5B, 0xE2, 0xAE, 0xD9, 0x1C, 0xD9, 0x21, 0x56,
        0x6D, 0xFC, 0x84, 0xD0, 0x5F, 0xED, 0x10, 0x15, 0x1C, 0x18,
        0x21, 0xE7, 0xC4, 0x3D, 0x4B, 0xD7, 0xD0, 0x9E, 0x6A, 0x95,
        0xCF, 0x22, 0xC9, 0x03, 0x7B, 0x9E, 0xE3, 0x60, 0x01, 0xFC,
        0x2F, 0x02, 0x81, 0x80, 0x11, 0xD0, 0x4B, 0xCF, 0x1B, 0x67,
        0xB9, 0x9F, 0x10, 0x75, 0x47, 0x86, 0x65, 0xAE, 0x31, 0xC2,
        0xC6, 0x30, 0xAC, 0x59, 0x06, 0x50, 0xD9, 0x0F, 0xB5, 0x70,
        0x06, 0xF7, 0xF0, 0xD3, 0xC8, 0x62, 0x7C, 0xA8, 0xDA, 0x6E,
        0xF6, 0x21, 0x3F, 0xD3, 0x7F, 0x5F, 0xEA, 0x8A, 0xAB, 0x3F,
        0xD9, 0x2A, 0x5E, 0xF3, 0x51, 0xD2, 0xC2, 0x30, 0x37, 0xE3,
        0x2D, 0xA3, 0x75, 0x0D, 0x1E, 0x4D, 0x21, 0x34, 0xD5, 0x57,
        0x70, 0x5C, 0x89, 0xBF, 0x72, 0xEC, 0x4A, 0x6E, 0x68, 0xD5,
        0xCD, 0x18, 0x74, 0x33, 0x4E, 0x8C, 0x3A, 0x45, 0x8F, 0xE6,
        0x96, 0x40, 0xEB, 0x63, 0xF9, 0x19, 0x86, 0x3A, 0x51, 0xDD,
        0x89, 0x4B, 0xB0, 0xF3, 0xF9, 0x9F, 0x5D, 0x28, 0x95, 0x38,
        0xBE, 0x35, 0xAB, 0xCA, 0x5C, 0xE7, 0x93, 0x53, 0x34, 0xA1,
        0x45, 0x5D, 0x13, 0x39, 0x65, 0x42, 0x46, 0xA1, 0x9F, 0xCD,
        0xF5, 0xBF
};
static const int sizeof_client_key_der_2048 = sizeof(client_key_der_2048);

#if 0
static const unsigned char client_keypub_der_2048[] =
{
        0x30, 0x82, 0x01, 0x22, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86,
        0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03,
        0x82, 0x01, 0x0F, 0x00, 0x30, 0x82, 0x01, 0x0A, 0x02, 0x82,
        0x01, 0x01, 0x00, 0xC3, 0x03, 0xD1, 0x2B, 0xFE, 0x39, 0xA4,
        0x32, 0x45, 0x3B, 0x53, 0xC8, 0x84, 0x2B, 0x2A, 0x7C, 0x74,
        0x9A, 0xBD, 0xAA, 0x2A, 0x52, 0x07, 0x47, 0xD6, 0xA6, 0x36,
        0xB2, 0x07, 0x32, 0x8E, 0xD0, 0xBA, 0x69, 0x7B, 0xC6, 0xC3,
        0x44, 0x9E, 0xD4, 0x81, 0x48, 0xFD, 0x2D, 0x68, 0xA2, 0x8B,
        0x67, 0xBB, 0xA1, 0x75, 0xC8, 0x36, 0x2C, 0x4A, 0xD2, 0x1B,
        0xF7, 0x8B, 0xBA, 0xCF, 0x0D, 0xF9, 0xEF, 0xEC, 0xF1, 0x81,
        0x1E, 0x7B, 0x9B, 0x03, 0x47, 0x9A, 0xBF, 0x65, 0xCC, 0x7F,
        0x65, 0x24, 0x69, 0xA6, 0xE8, 0x14, 0x89, 0x5B, 0xE4, 0x34,
        0xF7, 0xC5, 0xB0, 0x14, 0x93, 0xF5, 0x67, 0x7B, 0x3A, 0x7A,
        0x78, 0xE1, 0x01, 0x56, 0x56, 0x91, 0xA6, 0x13, 0x42, 0x8D,
        0xD2, 0x3C, 0x40, 0x9C, 0x4C, 0xEF, 0xD1, 0x86, 0xDF, 0x37,
        0x51, 0x1B, 0x0C, 0xA1, 0x3B, 0xF5, 0xF1, 0xA3, 0x4A, 0x35,
        0xE4, 0xE1, 0xCE, 0x96, 0xDF, 0x1B, 0x7E, 0xBF, 0x4E, 0x97,
        0xD0, 0x10, 0xE8, 0xA8, 0x08, 0x30, 0x81, 0xAF, 0x20, 0x0B,
        0x43, 0x14, 0xC5, 0x74, 0x67, 0xB4, 0x32, 0x82, 0x6F, 0x8D,
        0x86, 0xC2, 0x88, 0x40, 0x99, 0x36, 0x83, 0xBA, 0x1E, 0x40,
        0x72, 0x22, 0x17, 0xD7, 0x52, 0x65, 0x24, 0x73, 0xB0, 0xCE,
        0xEF, 0x19, 0xCD, 0xAE, 0xFF, 0x78, 0x6C, 0x7B, 0xC0, 0x12,
        0x03, 0xD4, 0x4E, 0x72, 0x0D, 0x50, 0x6D, 0x3B, 0xA3, 0x3B,
        0xA3, 0x99, 0x5E, 0x9D, 0xC8, 0xD9, 0x0C, 0x85, 0xB3, 0xD9,
        0x8A, 0xD9, 0x54, 0x26, 0xDB, 0x6D, 0xFA, 0xAC, 0xBB, 0xFF,
        0x25, 0x4C, 0xC4, 0xD1, 0x79, 0xF4, 0x71, 0xD3, 0x86, 0x40,
        0x18, 0x13, 0xB0, 0x63, 0xB5, 0x72, 0x4E, 0x30, 0xC4, 0x97,
        0x84, 0x86, 0x2D, 0x56, 0x2F, 0xD7, 0x15, 0xF7, 0x7F, 0xC0,
        0xAE, 0xF5, 0xFC, 0x5B, 0xE5, 0xFB, 0xA1, 0xBA, 0xD3, 0x02,
        0x03, 0x01, 0x00, 0x01
};
static const int sizeof_client_keypub_der_2048 = sizeof(client_keypub_der_2048);
#endif
#endif

#ifdef HAVE_ECC
/* ./certs/ecc-client-key.der, ECC */
static const unsigned char ecc_clikey_der_256[] =
{
    0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0xF8,
    0xCF, 0x92, 0x6B, 0xBD, 0x1E, 0x28, 0xF1, 0xA8,
    0xAB, 0xA1, 0x23, 0x4F, 0x32, 0x74, 0x18, 0x88,
    0x50, 0xAD, 0x7E, 0xC7, 0xEC, 0x92, 0xF8, 0x8F,
    0x97, 0x4D, 0xAF, 0x56, 0x89, 0x65, 0xC7, 0xA0,
    0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
    0x03, 0x01, 0x07, 0xA1, 0x44, 0x03, 0x42, 0x00,
    0x04, 0x55, 0xBF, 0xF4, 0x0F, 0x44, 0x50, 0x9A,
    0x3D, 0xCE, 0x9B, 0xB7, 0xF0, 0xC5, 0x4D, 0xF5,
    0x70, 0x7B, 0xD4, 0xEC, 0x24, 0x8E, 0x19, 0x80,
    0xEC, 0x5A, 0x4C, 0xA2, 0x24, 0x03, 0x62, 0x2C,
    0x9B, 0xDA, 0xEF, 0xA2, 0x35, 0x12, 0x43, 0x84,
    0x76, 0x16, 0xC6, 0x56, 0x95, 0x06, 0xCC, 0x01,
    0xA9, 0xBD, 0xF6, 0x75, 0x1A, 0x42, 0xF7, 0xBD,
    0xA9, 0xB2, 0x36, 0x22, 0x5F, 0xC7, 0x5D, 0x7F,
    0xB4
};
static const int sizeof_ecc_clikey_der_256 = sizeof(ecc_clikey_der_256);

/* ./certs/ecc-client-keyPub.der, ECC */
static const unsigned char ecc_clikeypub_der_256[] =
{
        0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE,
        0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
        0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0x55, 0xBF, 0xF4,
        0x0F, 0x44, 0x50, 0x9A, 0x3D, 0xCE, 0x9B, 0xB7, 0xF0, 0xC5,
        0x4D, 0xF5, 0x70, 0x7B, 0xD4, 0xEC, 0x24, 0x8E, 0x19, 0x80,
        0xEC, 0x5A, 0x4C, 0xA2, 0x24, 0x03, 0x62, 0x2C, 0x9B, 0xDA,
        0xEF, 0xA2, 0x35, 0x12, 0x43, 0x84, 0x76, 0x16, 0xC6, 0x56,
        0x95, 0x06, 0xCC, 0x01, 0xA9, 0xBD, 0xF6, 0x75, 0x1A, 0x42,
        0xF7, 0xBD, 0xA9, 0xB2, 0x36, 0x22, 0x5F, 0xC7, 0x5D, 0x7F,
        0xB4
};
static const int sizeof_ecc_clikeypub_der_256 = sizeof(ecc_clikeypub_der_256);

static const unsigned char ecc_secret_256[] =
{
    0xcc, 0xfb, 0x45, 0xaf, 0xc3, 0x9c, 0xd7, 0x9e,
    0x1f, 0xd1, 0xc7, 0xb8, 0x0e, 0x63, 0xd2, 0xb0,
    0x09, 0xba, 0x5a, 0xbb, 0xcf, 0x9b, 0xe2, 0x9b,
    0x8b, 0x25, 0xe4, 0x6d, 0x5c, 0x9e, 0xdd, 0xf9
};
static const int sizeof_ecc_secret_256 = sizeof(ecc_secret_256);
#endif

static WC_RNG rng;

#ifndef NO_RSA
int decode_rsa_key(RsaKey* key, int devId)
{
    int    ret;
    word32 idx = 0;

    ret = wc_InitRsaKey_ex(key, NULL, devId);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize RSA key: %d\n", ret);
    }
    if (ret == 0) {
        ret = wc_RsaPrivateKeyDecode(client_key_der_2048, &idx, key,
                (word32)sizeof_client_key_der_2048);
        if (ret != 0)
            fprintf(stderr, "Failed to decode private key: %d\n", ret);
    }

    return ret;
}

#ifndef WOLFSSL_KEY_GEN
static CK_BBOOL        ckTrue = CK_TRUE;
static CK_BYTE         pub_exp[] = { 0x01, 0x00, 0x01 };

int get_public_key(RsaKey* key, Pkcs11Token* token, CK_SESSION_HANDLE session,
                   CK_OBJECT_HANDLE pubKey)
{
    int ret = 0;
    unsigned char* mod = NULL;
    unsigned char* exp = NULL;
    int modSz, expSz;
    CK_ATTRIBUTE template[] = {
      {CKA_MODULUS, NULL_PTR, 0},
      {CKA_PUBLIC_EXPONENT, NULL_PTR, 0}
    };
    CK_RV rv;

    rv = token->func->C_GetAttributeValue(session, pubKey, template, 2);
    if (rv == CKR_OK) {
        modSz = template[0].ulValueLen;
        expSz = template[1].ulValueLen;
        mod = (unsigned char *)malloc(modSz);
        template[0].pValue = mod;
        exp = (CK_BYTE_PTR) malloc(expSz);
        template[1].pValue = exp;

        rv = token->func->C_GetAttributeValue(session, pubKey, template, 2);
    }
    if (rv == CKR_OK)
        ret = wc_RsaPublicKeyDecodeRaw(mod, modSz, exp, expSz, key);

    if (exp != NULL)
      free(exp);
    if (mod != NULL)
      free(mod);

    return ret;
}

int gen_rsa_key(Pkcs11Token* token, RsaKey* key, unsigned char* id, int idLen,
                int devId)
{
    int               ret = 0;
    CK_RV             rv;
    CK_ULONG          bits = 2048;
    CK_OBJECT_HANDLE  pubKey = NULL_PTR, privKey = NULL_PTR;
    CK_MECHANISM      mech;
    CK_ATTRIBUTE      pubKeyTmpl[] = {
        { CKA_MODULUS_BITS,    &bits,    sizeof(bits)    },
        { CKA_ENCRYPT,         &ckTrue,  sizeof(ckTrue)  },
        { CKA_VERIFY,          &ckTrue,  sizeof(ckTrue)  },
        { CKA_PUBLIC_EXPONENT, &pub_exp, sizeof(pub_exp) }
    };
    CK_ATTRIBUTE      privKeyTmpl[] = {
        {CKA_DECRYPT,  &ckTrue, sizeof(ckTrue) },
        {CKA_SIGN,     &ckTrue, sizeof(ckTrue) },
        {CKA_ID,       id,      idLen          }
    };
    int               privTmplCnt = 2;

    if (idLen > 0)
        privTmplCnt++;
    if (ret == 0) {
        mech.mechanism      = CKM_RSA_PKCS_KEY_PAIR_GEN;
        mech.ulParameterLen = 0;
        mech.pParameter     = NULL;

        rv = token->func->C_GenerateKeyPair(token->handle, &mech, pubKeyTmpl, 4,
            privKeyTmpl, privTmplCnt, &pubKey, &privKey);
        if (rv != CKR_OK)
            ret = -1;
    }

    if (ret == 0)
        ret = get_public_key(key, token, token->handle, pubKey);

    return ret;
}

int gen_rsa_key_label(Pkcs11Token* token, RsaKey* key, char* label, int devId)
{
    int               ret = 0;
    CK_RV             rv;
    CK_ULONG          bits = 2048;
    CK_OBJECT_HANDLE  pubKey = NULL_PTR, privKey = NULL_PTR;
    CK_MECHANISM      mech;
    CK_ATTRIBUTE      pubKeyTmpl[] = {
        { CKA_MODULUS_BITS,    &bits,    sizeof(bits)    },
        { CKA_ENCRYPT,         &ckTrue,  sizeof(ckTrue)  },
        { CKA_VERIFY,          &ckTrue,  sizeof(ckTrue)  },
        { CKA_PUBLIC_EXPONENT, &pub_exp, sizeof(pub_exp) }
    };
    CK_ATTRIBUTE      privKeyTmpl[] = {
        {CKA_DECRYPT,  &ckTrue, sizeof(ckTrue) },
        {CKA_SIGN,     &ckTrue, sizeof(ckTrue) },
        {CKA_LABEL,    label,   XSTRLEN(label) }
    };
    int               privTmplCnt = 2;

    if (XSTRLEN(label) > 0)
        privTmplCnt++;
    if (ret == 0) {
        mech.mechanism      = CKM_RSA_PKCS_KEY_PAIR_GEN;
        mech.ulParameterLen = 0;
        mech.pParameter     = NULL;

        rv = token->func->C_GenerateKeyPair(token->handle, &mech, pubKeyTmpl, 4,
            privKeyTmpl, privTmplCnt, &pubKey, &privKey);
        if (rv != CKR_OK)
            ret = -1;
    }

    if (ret == 0)
        ret = get_public_key(key, token, token->handle, pubKey);

    return ret;
}
#else
int gen_rsa_key(Pkcs11Token* token, RsaKey* key, unsigned char* id, int idLen,
                int devId)
{
    int ret;

    ret = wc_InitRsaKey_Id(key, id, idLen, NULL, devId);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize RSA key: %d\n", ret);
    }

    if (ret == 0) {
        ret = wc_MakeRsaKey(key, 2048, 0x10001, &rng);
        if (ret != 0)
            fprintf(stderr, "Failed to generate RSA key: %d\n", ret);
    }

    return ret;
}

int gen_rsa_key_label(Pkcs11Token* token, RsaKey* key, char* label, int devId)
{
    int ret;

    ret = wc_InitRsaKey_Label(key, label, NULL, devId);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize RSA key: %d\n", ret);
    }

    if (ret == 0) {
        ret = wc_MakeRsaKey(key, 2048, 0x10001, &rng);
        if (ret != 0)
            fprintf(stderr, "Failed to generate RSA key: %d\n", ret);
    }

    return ret;
}
#endif

int rsaenc_test(RsaKey* key)
{
    int    ret = 0;
    byte   plain[128], out[2048/8], dec[2048/8];
    word32 plainSz, outSz, decSz;

    memset(plain, 9, sizeof(plain));
    plainSz = sizeof(plain);
    outSz = sizeof(out);
    decSz = sizeof(dec);

    if (ret == 0) {
        outSz = ret = wc_RsaPublicEncrypt_ex(plain, plainSz, out, (int)outSz,
            key, &rng, WC_RSA_PKCSV15_PAD, WC_HASH_TYPE_NONE, WC_MGF1NONE, NULL,
            0);
        if (ret < 0)
            fprintf(stderr, "Failed to perform public encrypt: %d\n", ret);
        else
            ret = 0;
    }
    if (ret == 0) {
        decSz = ret = wc_RsaPrivateDecrypt_ex(out, outSz, dec, (int)decSz, key,
            WC_RSA_PKCSV15_PAD, WC_HASH_TYPE_NONE, WC_MGF1NONE, NULL, 0);
        if (ret < 0)
            fprintf(stderr, "Failed to perform private decrypt: %d\n", ret);
        else
            ret = 0;
    }

    if (ret == 0) {
        if (decSz != plainSz || memcmp(plain, dec, decSz) != 0) {
            fprintf(stderr, "Decrypted data does not match plain text");
            ret = -1;
        }
    }

    return ret;
}

int rsasig_test(RsaKey* key)
{
    int    ret = 0;
    byte   plain[128], out[2048/8];
    word32 plainSz, outSz;

    memset(plain, 9, sizeof(plain));
    plainSz = sizeof(plain);
    outSz = sizeof(out);

    if (ret == 0) {
        outSz = ret = wc_RsaSSL_Sign(plain, plainSz, out, (int)outSz, key,
            NULL);
        if (ret < 0)
            fprintf(stderr, "Failed to sign: %d\n", ret);
        else
            ret = 0;
    }
    if (ret == 0) {
        ret = wc_RsaSSL_Verify(out, outSz, plain, (int)plainSz, key);
        if (ret < 0)
            fprintf(stderr, "Failed to verify: %d\n", ret);
        else
            ret = 0;
    }

    return ret;
}
#endif

#ifdef HAVE_ECC
int decode_ecc_keys(ecc_key* privKey, ecc_key* pubKey, int devId)
{
    int    ret;
    word32 idx;

    ret = wc_ecc_init_ex(privKey, NULL, devId);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize ECC key: %d\n", ret);
    }
    if (ret == 0) {
        ret = wc_ecc_init_ex(pubKey, NULL, devId);
        if (ret != 0) {
            fprintf(stderr, "Failed to initialize ECC key: %d\n", ret);
        }
    }
    if (ret == 0) {
        idx = 0;
        ret = wc_EccPrivateKeyDecode(ecc_clikey_der_256, &idx, privKey,
            (word32)sizeof_ecc_clikey_der_256);
        if (ret != 0)
            fprintf(stderr, "Failed to decode private key: %d\n", ret);
    }
    if (ret == 0) {
        idx = 0;
        ret = wc_EccPublicKeyDecode(ecc_clikeypub_der_256, &idx, pubKey,
                                          (word32)sizeof_ecc_clikeypub_der_256);
        if (ret != 0)
            fprintf(stderr, "Failed to decode public key: %d\n", ret);
    }

    return ret;
}

int gen_ec_keys_label(Pkcs11Token* token, ecc_key* key, char* label, int devId)
{
    int ret;

    ret = wc_ecc_init_label(key, label, NULL, devId);
    if (ret != 0)
        fprintf(stderr, "Failed to initialize EC key: %d\n", ret);
    if (ret == 0) {
        ret = wc_ecc_make_key_ex(&rng, 32, key, ECC_CURVE_DEF);
        if (ret != 0)
            fprintf(stderr, "Failed to generate EC key: %d\n", ret);
    }
    return ret;
}

int gen_ec_keys(Pkcs11Token* token, ecc_key* key, unsigned char* id, int idLen,
                int devId)
{
    int ret;

    ret = wc_ecc_init_id(key, id, idLen, NULL, devId);
    if (ret != 0)
        fprintf(stderr, "Failed to initialize EC key: %d\n", ret);
    if (ret == 0) {
        ret = wc_ecc_make_key_ex(&rng, 32, key, ECC_CURVE_DEF);
        if (ret != 0)
            fprintf(stderr, "Failed to generate EC key: %d\n", ret);
    }
    return ret;
}

int ecdh_test(ecc_key* privKey, ecc_key* pubKey, int check)
{
    int    ret = 0;
    byte   out[256/8];
    word32 outSz = sizeof(out);

#if defined(ECC_TIMING_RESISTANT) && (!defined(HAVE_FIPS) || \
    (!defined(HAVE_FIPS_VERSION) || (HAVE_FIPS_VERSION != 2))) && \
    !defined(HAVE_SELFTEST)
    ret = wc_ecc_set_rng(privKey, &rng);
#endif
    if (ret == 0) {
        ret = wc_ecc_shared_secret(privKey, pubKey, out, &outSz);
        if (ret < 0)
            fprintf(stderr, "Failed to perform EC key derivation: %d\n", ret);
        else
            ret = 0;
    }
    if (ret == 0 && check) {
        if (outSz != sizeof_ecc_secret_256 ||
                                      memcmp(out, ecc_secret_256, outSz) != 0) {
            fprintf(stderr, "Secert does not match expected");
            ret = -1;
        }
    }

    return ret;
}

int ecdsa_test(ecc_key* privKey, ecc_key* pubKey, ecc_key* privKeySoft,
               ecc_key* pubKeySoft)
{
    int    ret = 0;
    byte   hash[32], out[128];
    word32 hashSz, outSz;
    int    verify;

    memset(hash, 9, sizeof(hash));
    hashSz = sizeof(hash);
    outSz = sizeof(out);

    if (ret == 0) {
        ret = wc_ecc_sign_hash(hash, hashSz, out, &outSz, &rng, privKey);
        if (ret < 0)
            fprintf(stderr, "Failed to sign: %d\n", ret);
    }
    if (ret == 0) {
        ret = wc_ecc_verify_hash(out, outSz, hash, (int)hashSz, &verify,
                                                                        pubKey);
        if (ret < 0 || !verify)
            fprintf(stderr, "Failed to verify: %d (%d)\n", ret, verify);
        if (!verify)
            ret = -1;
    }
    if (ret == 0) {
        ret = wc_ecc_verify_hash(out, outSz, hash, (int)hashSz, &verify,
                                                                    pubKeySoft);
        if (ret < 0 || !verify)
            fprintf(stderr, "Failed to verify in software: %d (%d)\n", ret, verify);
        if (!verify)
            ret = -1;
    }
    if (ret == 0 && privKeySoft != NULL) {
        outSz = sizeof(out);
        ret = wc_ecc_sign_hash(hash, hashSz, out, &outSz, &rng, privKeySoft);
        if (ret < 0)
            fprintf(stderr, "Failed to sign: %d\n", ret);
    }
    if (ret == 0 && privKeySoft != NULL) {
        ret = wc_ecc_verify_hash(out, outSz, hash, (int)hashSz, &verify,
                                                                        pubKey);
        if (ret < 0 || !verify)
            fprintf(stderr, "Failed to verify software signature: %d (%d)\n", ret, verify);
        if (!verify)
            ret = -1;
    }

    return ret;
}
#endif

#if !defined(NO_AES) && defined(HAVE_AESGCM)
int aesgcm_test(int devId, Pkcs11Token* token)
{
    Aes           aes;
    unsigned char key[AES_256_KEY_SIZE];
    int           ret = 0;
    unsigned char data[33];
    unsigned char enc[33];
    unsigned char dec[33];
    unsigned char iv[GCM_NONCE_MID_SZ];
    unsigned char authTag[16];
    unsigned char exp[33] = {
        0x6c, 0xe8, 0x76, 0xda, 0x06, 0x52, 0xd9, 0xe0,
        0x2d, 0xdb, 0x34, 0x44, 0x29, 0xb2, 0x68, 0x34,
        0x2d, 0x1d, 0x30, 0x86, 0x85, 0x15, 0xeb, 0xbd,
        0xf1, 0x5e, 0xab, 0x20, 0x35, 0x9f, 0xb1, 0x0d,
        0x84
    };
    unsigned char expTag[16] = {
        0x62, 0x51, 0x91, 0xbe, 0x5c, 0x19, 0x00, 0xb3,
        0x89, 0xc5, 0x3c, 0x72, 0x0a, 0x92, 0x36, 0xc6
    };
    unsigned char exp256[33] = {
        0x64, 0x35, 0xa0, 0xcb, 0x9d, 0xad, 0x9a, 0xbc,
        0x09, 0xca, 0x71, 0xbb, 0xe0, 0x63, 0x12, 0x10,
        0x55, 0x87, 0x72, 0xe9, 0xd2, 0xbc, 0x09, 0x3d,
        0xde, 0x7e, 0xe7, 0x92, 0xc0, 0x28, 0xc4, 0x37,
        0x31
    };
    unsigned char expTag256[16] = {
        0xac, 0xe6, 0xd6, 0x1d, 0x53, 0x88, 0x3f, 0x85,
        0xb1, 0x86, 0x3e, 0xe3, 0xdd, 0x49, 0xc6, 0xc1
    };

    memset(key, 9, sizeof(key));
    memset(data, 9, sizeof(data));
    memset(iv, 9, sizeof(iv));

    /* AES128-GCM */
    ret = wc_AesInit_Id(&aes, NULL, 0, NULL, devId);
    if (ret == 0) {
        ret = wc_AesGcmSetKey(&aes, key, AES_128_KEY_SIZE);
        if (ret != 0)
            fprintf(stderr, "Set Key failed: %d\n", ret);
    }
    if (ret == 0) {
        ret = wc_AesGcmEncrypt(&aes, enc, data, sizeof(data), iv, sizeof(iv),
            authTag, sizeof(authTag), NULL, 0);
        if (ret != 0)
            fprintf(stderr, "Encrypt failed: %d\n", ret);
    }
    if (ret == 0) {
        if (memcmp(enc, exp, sizeof(exp)) != 0) {
            fprintf(stderr, "Encrypted data didn't match expected\n");
            ret = -1;
        }
    }
    if (ret == 0) {
        if (memcmp(authTag, expTag, sizeof(expTag)) != 0) {
            fprintf(stderr, "Auth tag didn't match expected\n");
            ret = -1;
        }
    }
    if (ret == 0) {
        ret = wc_AesGcmDecrypt(&aes, dec, enc, sizeof(enc), iv, sizeof(iv),
            authTag, sizeof(authTag), NULL, 0);
        if (ret != 0)
            fprintf(stderr, "Decrypt failed: %d\n", ret);
    }
    if (ret == 0) {
        if (memcmp(dec, data, ret) != 0) {
            fprintf(stderr, "Decrypted data didn't match plaintext\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        wc_Pkcs11Token_Open(token, 1);
        /* AES256-GCM */
        if (ret == 0)
            ret = wc_AesInit_Label(&aes, "myAesGcmKey", NULL, devId);
        if (ret == 0) {
            ret = wc_AesGcmSetKey(&aes, key, AES_256_KEY_SIZE);
            if (ret != 0)
                fprintf(stderr, "Set Key failed: %d\n", ret);
        }
        if (ret == 0) {
            ret = wc_Pkcs11StoreKey(token, PKCS11_KEY_TYPE_AES_GCM, 1,
                (void*)&aes);
            if (ret == NOT_COMPILED_IN)
                ret = 0;
            if (ret != 0)
                fprintf(stderr, "Store Key failed: %d\n", ret);
        }
        if (ret == 0) {
            ret = wc_AesGcmEncrypt(&aes, enc, data, sizeof(data), iv,
                sizeof(iv), authTag, sizeof(authTag), NULL, 0);
            if (ret != 0)
                fprintf(stderr, "Encrypt failed: %d\n", ret);
        }
        if (ret == 0) {
            if (memcmp(enc, exp256, sizeof(exp256)) != 0) {
                fprintf(stderr, "Encrypted data didn't match expected\n");
                ret = -1;
            }
        }
        if (ret == 0) {
            if (memcmp(authTag, expTag256, sizeof(expTag256)) != 0) {
                fprintf(stderr, "Auth tag didn't match expected\n");
                ret = -1;
            }
        }
        if (ret == 0) {
            ret = wc_AesGcmDecrypt(&aes, dec, enc, sizeof(enc), iv, sizeof(iv),
                authTag, sizeof(authTag), NULL, 0);
            if (ret != 0)
                fprintf(stderr, "Decrypt failed: %d\n", ret);
        }
        if (ret == 0) {
            if (memcmp(dec, data, ret) != 0) {
                fprintf(stderr, "Decrypted data didn't match plaintext\n");
                ret = -1;
            }
        }
        wc_Pkcs11Token_Close(token);
    }


    if (ret == 0) {
        wc_Pkcs11Token_Open(token, 1);
        /* AES256-GCM */
        if (ret == 0)
            ret = wc_AesInit_Id(&aes, (unsigned char*)"AES123", 6, NULL, devId);
        if (ret == 0) {
            ret = wc_AesGcmSetKey(&aes, key, AES_256_KEY_SIZE);
            if (ret != 0)
                fprintf(stderr, "Set Key failed: %d\n", ret);
        }
        if (ret == 0) {
            ret = wc_Pkcs11StoreKey(token, PKCS11_KEY_TYPE_AES_GCM, 1,
                (void*)&aes);
            if (ret == NOT_COMPILED_IN)
                ret = 0;
            if (ret != 0)
                fprintf(stderr, "Store Key failed: %d\n", ret);
        }
        if (ret == 0) {
            ret = wc_AesGcmEncrypt(&aes, enc, data, sizeof(data), iv,
                sizeof(iv), authTag, sizeof(authTag), NULL, 0);
            if (ret != 0)
                fprintf(stderr, "Encrypt failed: %d\n", ret);
        }
        if (ret == 0) {
            if (memcmp(enc, exp256, sizeof(exp256)) != 0) {
                fprintf(stderr, "Encrypted data didn't match expected\n");
                ret = -1;
            }
        }
        if (ret == 0) {
            if (memcmp(authTag, expTag256, sizeof(expTag256)) != 0) {
                fprintf(stderr, "Auth tag didn't match expected\n");
                ret = -1;
            }
        }
        if (ret == 0) {
            ret = wc_AesGcmDecrypt(&aes, dec, enc, sizeof(enc), iv, sizeof(iv),
                authTag, sizeof(authTag), NULL, 0);
            if (ret != 0)
                fprintf(stderr, "Decrypt failed: %d\n", ret);
        }
        if (ret == 0) {
            if (memcmp(dec, data, ret) != 0) {
                fprintf(stderr, "Decrypted data didn't match plaintext\n");
                ret = -1;
            }
        }
        wc_Pkcs11Token_Close(token);
    }

    return ret;
}
#endif

#if !defined(NO_AES) && defined(HAVE_AES_CBC)
int aescbc_test(int devId, Pkcs11Token* token)
{
    Aes           aes;
    unsigned char key[AES_256_KEY_SIZE];
    int           ret = 0;
    unsigned char data[32];
    unsigned char enc[32];
    unsigned char dec[32];
    unsigned char iv[AES_BLOCK_SIZE];
    unsigned char exp[32] = {
        0x84, 0xf9, 0xc2, 0x0e, 0x61, 0x4f, 0x86, 0x07,
        0xbc, 0x13, 0xef, 0xeb, 0x59, 0x4b, 0xdf, 0x5a,
        0x34, 0xa8, 0xbd, 0xc7, 0x29, 0x66, 0xa4, 0x03,
        0x5f, 0x8a, 0x7d, 0x85, 0xda, 0xc8, 0x9a, 0xc1
    };
    unsigned char exp256[32] = {
        0x3f, 0xb8, 0x65, 0xa2, 0xe2, 0x74, 0x04, 0x94,
        0xff, 0xff, 0x67, 0xa0, 0x3e, 0x83, 0x0e, 0xa3,
        0xa3, 0x9a, 0x4f, 0xd2, 0x33, 0x58, 0xf5, 0x90,
        0x04, 0x8c, 0xd8, 0x9a, 0xd6, 0x61, 0x19, 0x4a
    };

    memset(key, 9, sizeof(key));
    memset(data, 9, sizeof(data));
    memset(iv, 9, sizeof(iv));

    /* AES128-CBC */
    ret = wc_AesInit_Id(&aes, NULL, 0, NULL, devId);
    if (ret == 0) {
        ret = wc_AesSetKey(&aes, key, AES_128_KEY_SIZE, iv, AES_ENCRYPTION);
        if (ret != 0)
            fprintf(stderr, "Set Key failed: %d\n", ret);
    }
    if (ret == 0) {
        ret = wc_AesCbcEncrypt(&aes, enc, data, sizeof(data));
        if (ret != 0)
            fprintf(stderr, "Encrypt failed: %d\n", ret);
    }
    if (ret == 0) {
        if (memcmp(enc, exp, sizeof(exp)) != 0) {
            fprintf(stderr, "Encrypted data didn't match expected\n");
            ret = -1;
        }
    }
    if (ret == 0) {
        ret = wc_AesSetKey(&aes, key, AES_128_KEY_SIZE, iv, AES_DECRYPTION);
        if (ret != 0)
            fprintf(stderr, "Set Key failed: %d\n", ret);
    }
    if (ret == 0) {
        ret = wc_AesCbcDecrypt(&aes, dec, enc, sizeof(enc));
        if (ret != 0)
            fprintf(stderr, "Decrypt failed: %d\n", ret);
    }
    if (ret == 0) {
        if (memcmp(dec, data, ret) != 0) {
            fprintf(stderr, "Decrypted data didn't match plaintext\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        wc_Pkcs11Token_Open(token, 1);
        /* AES256-CBC */
        if (ret == 0)
            ret = wc_AesInit_Label(&aes, "myAesCbcKey", NULL, devId);
        if (ret == 0) {
            ret = wc_AesSetKey(&aes, key, AES_256_KEY_SIZE, iv, AES_ENCRYPTION);
            if (ret != 0)
                fprintf(stderr, "Set Key failed: %d\n", ret);
        }
        if (ret == 0) {
            ret = wc_Pkcs11StoreKey(token, PKCS11_KEY_TYPE_AES_CBC, 1,
                (void*)&aes);
            if (ret == NOT_COMPILED_IN)
                ret = 0;
            if (ret != 0)
                fprintf(stderr, "Store Key failed: %d\n", ret);
        }
        if (ret == 0) {
            ret = wc_AesCbcEncrypt(&aes, enc, data, sizeof(data));
            if (ret != 0)
                fprintf(stderr, "Encrypt failed: %d\n", ret);
        }
        if (ret == 0) {
            if (memcmp(enc, exp256, sizeof(exp256)) != 0) {
                fprintf(stderr, "Encrypted data didn't match expected\n");
                ret = -1;
            }
        }
        if (ret == 0) {
            ret = wc_AesSetKey(&aes, key, AES_256_KEY_SIZE, iv, AES_DECRYPTION);
            if (ret != 0)
                fprintf(stderr, "Set Key failed: %d\n", ret);
        }
        if (ret == 0) {
            ret = wc_AesCbcDecrypt(&aes, dec, enc, sizeof(enc));
            if (ret != 0)
                fprintf(stderr, "Decrypt failed: %d\n", ret);
        }
        if (ret == 0) {
            if (memcmp(dec, data, ret) != 0) {
                fprintf(stderr, "Decrypted data didn't match plaintext\n");
                ret = -1;
            }
        }
        wc_Pkcs11Token_Close(token);
    }


    if (ret == 0) {
        wc_Pkcs11Token_Open(token, 1);
        /* AES256-CBC */
        if (ret == 0)
            ret = wc_AesInit_Id(&aes, (unsigned char*)"AES123", 6, NULL, devId);
        if (ret == 0) {
            ret = wc_AesSetKey(&aes, key, AES_256_KEY_SIZE, iv, AES_ENCRYPTION);
            if (ret != 0)
                fprintf(stderr, "Set Key failed: %d\n", ret);
        }
        if (ret == 0) {
            ret = wc_Pkcs11StoreKey(token, PKCS11_KEY_TYPE_AES_CBC, 1,
                (void*)&aes);
            if (ret == NOT_COMPILED_IN)
                ret = 0;
            if (ret != 0)
                fprintf(stderr, "Store Key failed: %d\n", ret);
        }
        if (ret == 0) {
            ret = wc_AesCbcEncrypt(&aes, enc, data, sizeof(data));
            if (ret != 0)
                fprintf(stderr, "Encrypt failed: %d\n", ret);
        }
        if (ret == 0) {
            if (memcmp(enc, exp256, sizeof(exp256)) != 0) {
                fprintf(stderr, "Encrypted data didn't match expected\n");
                ret = -1;
            }
        }
        if (ret == 0) {
            ret = wc_AesSetKey(&aes, key, AES_256_KEY_SIZE, iv, AES_DECRYPTION);
            if (ret != 0)
                fprintf(stderr, "Set Key failed: %d\n", ret);
        }
        if (ret == 0) {
            ret = wc_AesCbcDecrypt(&aes, dec, enc, sizeof(enc));
            if (ret != 0)
                fprintf(stderr, "Decrypt failed: %d\n", ret);
        }
        if (ret == 0) {
            if (memcmp(dec, data, ret) != 0) {
                fprintf(stderr, "Decrypted data didn't match plaintext\n");
                ret = -1;
            }
        }
        wc_Pkcs11Token_Close(token);
    }

    return ret;
}
#endif

#ifndef NO_HMAC
int hmac_op(unsigned char* key, int keyLen, int hashAlg, unsigned char* data,
            int dataLen, unsigned char* exp, int expLen, int devId,
            Pkcs11Token* token)
{
    int           ret;
    Hmac          hmac;
    unsigned char res[WC_MAX_DIGEST_SIZE];

    wc_Pkcs11Token_Open(token, 1);
    /* HMAC */
    ret = wc_HmacInit_Label(&hmac, "myHmacKey", NULL, devId);
    if (ret == 0) {
        ret = wc_HmacSetKey(&hmac, hashAlg, key, keyLen);
        if (ret != 0)
            fprintf(stderr, "Set Key failed: %d\n", ret);
    }
    if (ret == 0) {
        ret = wc_Pkcs11StoreKey(token, PKCS11_KEY_TYPE_HMAC, 0,
            (void*)&hmac);
        if (ret == NOT_COMPILED_IN)
            ret = 0;
        if (ret != 0)
            fprintf(stderr, "Store Key failed: %d\n", ret);
    }
    if (ret == 0) {
        ret = wc_HmacUpdate(&hmac, data, dataLen);
        if (ret != 0)
            fprintf(stderr, "HMAC Update failed: %d\n", ret);
    }
    if (ret == 0) {
        ret = wc_HmacFinal(&hmac, res);
        if (ret != 0)
            fprintf(stderr, "HMAC Update failed: %d\n", ret);
    }
    if (ret == 0) {
        if (memcmp(res, exp, expLen) != 0) {
            fprintf(stderr, "HMAC result didn't match expected\n");
            ret = -1;
        }
    }
    wc_Pkcs11Token_Close(token);

    if (ret == 0) {
        wc_Pkcs11Token_Open(token, 1);
        /* HMAC */
        ret = wc_HmacInit_Id(&hmac, (unsigned char*)"HMAC123", 7, NULL, devId);
        if (ret == 0) {
            ret = wc_HmacSetKey(&hmac, hashAlg, key, keyLen);
            if (ret != 0)
                fprintf(stderr, "Set Key failed: %d\n", ret);
        }
        if (ret == 0) {
            ret = wc_Pkcs11StoreKey(token, PKCS11_KEY_TYPE_HMAC, 0,
                (void*)&hmac);
            if (ret == NOT_COMPILED_IN)
                ret = 0;
            if (ret != 0)
                fprintf(stderr, "Store Key failed: %d\n", ret);
        }
        if (ret == 0) {
            ret = wc_HmacUpdate(&hmac, data, dataLen);
            if (ret != 0)
                fprintf(stderr, "HMAC Update failed: %d\n", ret);
        }
        if (ret == 0) {
            ret = wc_HmacFinal(&hmac, res);
            if (ret != 0)
                fprintf(stderr, "HMAC Update failed: %d\n", ret);
        }
        if (ret == 0) {
            if (memcmp(res, exp, expLen) != 0) {
                fprintf(stderr, "HMAC result didn't match expected\n");
                ret = -1;
            }
        }
        wc_Pkcs11Token_Close(token);
    }

    return ret;
}

int hmac_test(int devId, Pkcs11Token* token)
{
    int           ret = 0;
    unsigned char key[WC_MAX_DIGEST_SIZE];
    unsigned char data[57];
#if !defined(NO_MD5)
    unsigned char exp_md5[MD5_DIGEST_SIZE] = {
        0x58, 0x8e, 0xd2, 0x4e, 0x04, 0x1f, 0xf4, 0xc6,
        0x98, 0x7c, 0x8e, 0xdc, 0xe5, 0xb1, 0xbc, 0x4b
    };
#endif
#if !defined(NO_SHA)
    unsigned char exp_sha[SHA_DIGEST_SIZE] = {
        0x2f, 0x69, 0xc1, 0xf9, 0xe1, 0x97, 0x04, 0xe4,
        0x75, 0x9f, 0x1c, 0x2a, 0x85, 0x87, 0x7e, 0x6b,
        0xa7, 0x9f, 0xe1, 0x13
    };
#endif
#if defined(WOLFSSL_SHA224)
    unsigned char exp_sha224[SHA224_DIGEST_SIZE] = {
        0x86, 0xa8, 0xfc, 0xfd, 0xd5, 0x95, 0xf2, 0xa6,
        0x45, 0x89, 0x3b, 0x8b, 0x4c, 0x0d, 0xd1, 0x81,
        0x20, 0x6b, 0x71, 0x2d, 0x7c, 0x88, 0x31, 0xa8,
        0x17, 0x9f, 0xc7, 0x66
    };
#endif
#if !defined(NO_SHA256)
    unsigned char exp_sha256[SHA256_DIGEST_SIZE] = {
        0x04, 0x9e, 0x43, 0x3c, 0x48, 0x7c, 0x31, 0x11,
        0x54, 0x90, 0x43, 0xf6, 0x2f, 0x97, 0x42, 0x80,
        0x3d, 0x22, 0x47, 0x1d, 0x4f, 0xc9, 0xb8, 0xa0,
        0x02, 0x13, 0x2f, 0x8a, 0x31, 0xc2, 0x6e, 0xbe
    };
#endif
#if defined(WOLFSSL_SHA384)
    unsigned char exp_sha384[SHA384_DIGEST_SIZE] = {
        0x0b, 0x4c, 0x32, 0x58, 0x7b, 0x00, 0xb7, 0xfa,
        0x82, 0x9f, 0xf0, 0x1d, 0x10, 0x85, 0xbc, 0x2e,
        0xe0, 0x4a, 0x71, 0x91, 0xd6, 0x9a, 0x93, 0xc2,
        0xd9, 0xd9, 0xca, 0xf6, 0xbd, 0x86, 0x2c, 0xc2,
        0xd8, 0xaf, 0x40, 0xf9, 0x47, 0x39, 0xf4, 0x09,
        0xee, 0x56, 0x8b, 0x1a, 0xa5, 0xaf, 0xb4, 0x93
    };
#endif
#if defined(WOLFSSL_SHA512)
    unsigned char exp_sha512[SHA512_DIGEST_SIZE] = {
        0x94, 0x7b, 0x97, 0x0f, 0x48, 0x68, 0xd1, 0x88,
        0x08, 0x09, 0xcf, 0xea, 0xae, 0x3e, 0xba, 0xa2,
        0x3f, 0xf4, 0x9d, 0x73, 0x78, 0x15, 0x34, 0x44,
        0xaf, 0x14, 0x96, 0x9d, 0x38, 0x82, 0x1f, 0x37,
        0x59, 0x3d, 0xbc, 0x66, 0x8d, 0xe3, 0x9f, 0x28,
        0x33, 0x4f, 0x13, 0x30, 0x4d, 0xe1, 0x5e, 0x9f,
        0x29, 0x1d, 0xac, 0x51, 0x65, 0x3b, 0xcd, 0x98,
        0x58, 0x42, 0xa2, 0xe6, 0xa9, 0x30, 0xed, 0xc3
    };
#endif

    memset(key, 9, sizeof(key));
    memset(data, 9, sizeof(data));

#ifndef NO_MD5
    if (ret == 0) {
        fprintf(stderr, "HMAC-MD5\n");
        ret = hmac_op(key, MD5_DIGEST_SIZE, WC_MD5, data, sizeof(data), exp_md5,
            sizeof(exp_md5), devId, token);
    }
#endif
#ifndef NO_SHA
    if (ret == 0) {
        fprintf(stderr, "HMAC-SHA\n");
        ret = hmac_op(key, SHA_DIGEST_SIZE, WC_SHA, data, sizeof(data), exp_sha,
            sizeof(exp_sha), devId, token);
    }
#endif
#ifdef WOLFSSL_SHA224
    if (ret == 0) {
        fprintf(stderr, "HMAC-SHA224\n");
        ret = hmac_op(key, SHA224_DIGEST_SIZE, WC_SHA224, data, sizeof(data),
            exp_sha224, sizeof(exp_sha224), devId, token);
    }
#endif
#ifndef NO_SHA256
    if (ret == 0) {
        fprintf(stderr, "HMAC-SHA256\n");
        ret = hmac_op(key, SHA256_DIGEST_SIZE, WC_SHA256, data, sizeof(data),
            exp_sha256, sizeof(exp_sha256), devId, token);
    }
#endif
#ifdef WOLFSSL_SHA384
    if (ret == 0) {
        fprintf(stderr, "HMAC-SHA384\n");
        ret = hmac_op(key, SHA384_DIGEST_SIZE, WC_SHA384, data, sizeof(data),
            exp_sha384, sizeof(exp_sha384), devId, token);
    }
#endif
#ifdef WOLFSSL_SHA512
    if (ret == 0) {
        fprintf(stderr, "HMAC-SHA512\n");
        ret = hmac_op(key, SHA512_DIGEST_SIZE, WC_SHA512, data, sizeof(data),
            exp_sha512, sizeof(exp_sha512), devId, token);
    }
#endif

    return ret;
}
#endif

int pkcs11_test(int devId, Pkcs11Token* token)
{
    int ret = 0;
#ifndef NO_RSA
    RsaKey key;
#endif
#ifdef HAVE_ECC
    ecc_key eccPriv;
    ecc_key eccPub;
    ecc_key eccPrivSoft;
    ecc_key eccPubSoft;
#endif

    memset(&key, 0, sizeof(key));

    if (ret != 0)
        fprintf(stderr, "Failed to register device\n");
#ifndef NO_RSA
    if (ret == 0) {
        fprintf(stderr, "Decode RSA Key\n");
        ret = decode_rsa_key(&key, devId);
    }
    if (ret == 0) {
        fprintf(stderr, "Encrypt/Decrypt with RSA Key\n");
        ret = rsaenc_test(&key);
    }
    if (ret == 0) {
        fprintf(stderr, "Sign/Verify with RSA Key\n");
        ret = rsasig_test(&key);
    }
    wc_FreeRsaKey(&key);

    if (ret == 0) {
        wc_Pkcs11Token_Open(token, 1);
        fprintf(stderr, "Generate RSA Key\n");
        ret = gen_rsa_key(token, &key, NULL, 0, devId);
    }
    if (ret == 0) {
        fprintf(stderr, "Encrypt/Decrypt with RSA Key\n");
        ret = rsaenc_test(&key);
    }
    if (ret == 0) {
        fprintf(stderr, "Sign/Verify with RSA Key\n");
        ret = rsasig_test(&key);
    }
    wc_Pkcs11Token_Close(token);
    wc_FreeRsaKey(&key);

    if (ret == 0) {
        wc_Pkcs11Token_Open(token, 1);
        fprintf(stderr, "Generate RSA Key - LABEL\n");
        ret = gen_rsa_key_label(token, &key, "myRsaKey", devId);
    }
    if (ret == 0) {
        fprintf(stderr, "Encrypt/Decrypt with RSA Key\n");
        ret = rsaenc_test(&key);
    }
    if (ret == 0) {
        fprintf(stderr, "Sign/Verify with RSA Key\n");
        ret = rsasig_test(&key);
    }
    wc_Pkcs11Token_Close(token);
    wc_FreeRsaKey(&key);

    if (ret == 0) {
        wc_Pkcs11Token_Open(token, 1);
        fprintf(stderr, "Generate RSA Key - ID\n");
        ret = gen_rsa_key(token, &key, (unsigned char*)"123", 3,
                          devId);
    }
    if (ret == 0) {
        fprintf(stderr, "Encrypt/Decrypt with RSA Key\n");
        ret = rsaenc_test(&key);
    }
    if (ret == 0) {
        fprintf(stderr, "Sign/Verify with RSA Key\n");
        ret = rsasig_test(&key);
    }
    wc_Pkcs11Token_Close(token);
    wc_FreeRsaKey(&key);
#endif

#ifdef HAVE_ECC
    if (ret == 0) {
        fprintf(stderr, "Decode ECC Keys\n");
        ret = decode_ecc_keys(&eccPrivSoft, &eccPubSoft, INVALID_DEVID);
    }

    if (ret == 0) {
        fprintf(stderr, "Decode ECC Keys\n");
        ret = decode_ecc_keys(&eccPriv, &eccPub, devId);
    }
    if (ret == 0) {
        fprintf(stderr, "Derive secret with ECC Keys\n");
        ret = ecdh_test(&eccPriv, &eccPub, 1);
    }
    if (ret == 0) {
        fprintf(stderr, "Sign/Verify with ECC Keys\n");
        ret = ecdsa_test(&eccPriv, &eccPub, &eccPrivSoft, &eccPubSoft);
    }
    wc_ecc_free(&eccPub);
    wc_ecc_free(&eccPriv);

    if (ret == 0) {
        wc_Pkcs11Token_Open(token, 1);
        fprintf(stderr, "Generate EC Keys\n");
        ret = gen_ec_keys(token, &eccPriv, NULL, 0, devId);
        memcpy(&eccPub, &eccPriv, sizeof(ecc_key));
        eccPub.devId = INVALID_DEVID;
    }
    if (ret == 0) {
        fprintf(stderr, "Derive secret with ECC Keys\n");
        ret = ecdh_test(&eccPriv, &eccPriv, 0);
    }
    if (ret == 0) {
        fprintf(stderr, "Sign/Verify with ECC Keys\n");
        ret = ecdsa_test(&eccPriv, &eccPriv, NULL, &eccPub);
    }
    wc_Pkcs11Token_Close(token);
    wc_ecc_free(&eccPriv);

    if (ret == 0) {
        wc_Pkcs11Token_Open(token, 1);
        fprintf(stderr, "Generate EC Keys - LABEL\n");
        ret = gen_ec_keys_label(token, &eccPriv, "myEccKey", devId);
        memcpy(&eccPub, &eccPriv, sizeof(ecc_key));
        eccPub.devId = INVALID_DEVID;
    }
    if (ret == 0) {
        fprintf(stderr, "Derive secret with ECC Keys\n");
        ret = ecdh_test(&eccPriv, &eccPriv, 0);
    }
    if (ret == 0) {
        fprintf(stderr, "Sign/Verify with ECC Keys\n");
        ret = ecdsa_test(&eccPriv, &eccPriv, NULL, &eccPub);
    }
    wc_Pkcs11Token_Close(token);
    wc_ecc_free(&eccPriv);

    if (ret == 0) {
        wc_Pkcs11Token_Open(token, 1);
        fprintf(stderr, "Generate EC Keys - ID\n");
        ret = gen_ec_keys(token, &eccPriv, (unsigned char*)"123ecc", 6,
                          devId);
        memcpy(&eccPub, &eccPriv, sizeof(ecc_key));
        eccPub.devId = INVALID_DEVID;
    }
    if (ret == 0) {
        fprintf(stderr, "Derive secret with ECC Keys\n");
        ret = ecdh_test(&eccPriv, &eccPriv, 0);
    }
    if (ret == 0) {
        fprintf(stderr, "Sign/Verify with ECC Keys\n");
        ret = ecdsa_test(&eccPriv, &eccPriv, NULL, &eccPub);
    }
    wc_Pkcs11Token_Close(token);
    wc_ecc_free(&eccPriv);

    wc_ecc_free(&eccPubSoft);
    wc_ecc_free(&eccPrivSoft);
#endif

#if !defined(NO_AES) && defined(HAVE_AESGCM)
    if (ret == 0) {
        fprintf(stderr, "AES-GCM encrypt/decrypt\n");
        ret = aesgcm_test(devId, token);
    }
#endif
#if !defined(NO_AES) && defined(HAVE_AES_CBC)
    if (ret == 0) {
        fprintf(stderr, "AES-CBC encrypt/decrypt\n");
        ret = aescbc_test(devId, token);
    }
#endif
#ifndef NO_HMAC
    if (ret == 0) {
        fprintf(stderr, "HMAC\n");
        ret = hmac_test(devId, token);
    }
#endif

    if (ret == 0)
        fprintf(stderr, "Success\n");

    return ret;
}

int main(int argc, char* argv[])
{
    int ret;
    const char* library;
    const char* slot;
    const char* tokenName;
    const char* userPin;
    Pkcs11Dev dev;
    Pkcs11Token token;
    int slotId;
    int devId = 1;

    if (argc != 4 && argc != 5) {
        fprintf(stderr,
                "Usage: pkcs11_test <libname> <slot> <tokenname> [userpin]\n");
        return 1;
    }

    library = argv[1];
    slot = argv[2];
    tokenName = argv[3];
    userPin = (argc == 4) ? NULL : argv[4];
    slotId = atoi(slot);

#if defined(DEBUG_WOLFSSL)
    wolfSSL_Debugging_ON();
#endif
    wolfCrypt_Init();

    ret = wc_Pkcs11_Initialize(&dev, library, NULL);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize PKCS#11 library\n");
        ret = 2;
    }
    if (ret == 0) {
        ret = wc_Pkcs11Token_Init(&token, &dev, slotId, tokenName,
            (byte*)userPin, userPin == NULL ? 0 : strlen(userPin));
        if (ret != 0) {
            fprintf(stderr, "Failed to initialize PKCS#11 token\n");
            ret = 2;
        }
        if (ret == 0) {
            ret = wc_CryptoDev_RegisterDevice(devId, wc_Pkcs11_CryptoDevCb,
                                              &token);
            if (ret != 0) {
                fprintf(stderr, "Failed to register PKCS#11 token\n");
                ret = 2;
            }
            if (ret == 0) {
                wc_InitRng_ex(&rng, NULL, devId);

                ret = pkcs11_test(devId, &token);
                if (ret != 0)
                    ret = 1;

                wc_FreeRng(&rng);
            }
            wc_Pkcs11Token_Final(&token);
        }
        wc_Pkcs11_Finalize(&dev);
    }

    wolfCrypt_Cleanup();

    return ret;
}

