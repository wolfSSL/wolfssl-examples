/* ocsp-load-certs.h
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

/* Shared file/cert loading helpers for the OCSP responder examples. */

#ifndef OCSP_LOAD_CERTS_H
#define OCSP_LOAD_CERTS_H

#include <wolfssl/wolfcrypt/wc_port.h> /* WC_MAYBE_UNUSED */
#include <wolfssl/wolfcrypt/asn.h>     /* wc_CertPemToDer, wc_KeyPemToDer */
#include <wolfssl/wolfcrypt/memory.h>  /* wc_ForceZero */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Load file into malloc'd buffer. Caller must free(*buf). */
static WC_MAYBE_UNUSED byte* LoadFile(const char* path, int* outSz)
{
    FILE* f;
    long sz;
    byte* buf;

    f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    buf = (byte*)malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }
    *outSz = (int)fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (*outSz != (int)sz) { free(buf); return NULL; }
    return buf;
}

/* Load PEM certificate, convert to DER. Caller must free() result. */
static WC_MAYBE_UNUSED byte* LoadCertDer(const char* path, int* derSz)
{
    byte *pem, *der;
    int pemSz = 0, ret;

    pem = LoadFile(path, &pemSz);
    if (!pem) return NULL;
    der = (byte*)malloc((size_t)pemSz);
    if (!der) { free(pem); return NULL; }
    ret = wc_CertPemToDer(pem, pemSz, der, pemSz, CERT_TYPE);
    free(pem);
    if (ret <= 0) { free(der); return NULL; }
    *derSz = ret;
    return der;
}

/* Load PEM private key, convert to DER. Caller must free() result. */
static WC_MAYBE_UNUSED byte* LoadKeyDer(const char* path, int* derSz)
{
    byte *pem, *der;
    int pemSz = 0, ret;

    pem = LoadFile(path, &pemSz);
    if (!pem) return NULL;
    der = (byte*)malloc((size_t)pemSz);
    if (!der) {
        /* Zero the PEM buffer before releasing it, it may hold the
         * unencrypted private key material. */
        wc_ForceZero(pem, (word32)pemSz);
        free(pem);
        return NULL;
    }
    ret = wc_KeyPemToDer(pem, pemSz, der, pemSz, NULL);
    /* Zero the PEM buffer before releasing it since it may contain the
     * unencrypted private key material. */
    wc_ForceZero(pem, (word32)pemSz);
    free(pem);
    if (ret <= 0) {
        /* der may hold partially-written key bytes on failure. */
        wc_ForceZero(der, (word32)pemSz);
        free(der);
        return NULL;
    }
    *derSz = ret;
    return der;
}

#endif /* OCSP_LOAD_CERTS_H */
