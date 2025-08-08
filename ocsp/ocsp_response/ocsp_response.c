/* ocsp_response.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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
 *=============================================================================
 *
 * This example shows how to manually parse an OCSP response. 
 * Note: This requires access to local wolfSSL functions, which is why a static library is used.
*/

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/ocsp.h>

/* 
./configure --enable-ocsp --enable-debug --disable-shared CFLAGS="-DWOLFSSL_NO_OCSP_DATE_CHECK -DWOLFSSL_NO_ASN_STRICT"
make
gcc -g -I. -o ocsp_response src/.libs/libwolfssl.a ocsp_response.c
./ocsp_response ca.der cert.der ocsp_resp.bin
*/

/* reads file size, allocates buffer, reads into buffer, returns buffer */
static int load_file(const char* fname, byte** buf, size_t* bufLen)
{
    int ret;
    long int fileSz;
    XFILE file;

    if (fname == NULL || buf == NULL || bufLen == NULL)
        return BAD_FUNC_ARG;

    /* set defaults */
    *buf = NULL;
    *bufLen = 0;

    /* open file (read-only binary) */
    file = XFOPEN(fname, "rb");
    if (!file) {
        printf("Error loading %s\n", fname);
        return BAD_PATH_ERROR;
    }

    fseek(file, 0, SEEK_END);
    fileSz = (int)ftell(file);
    rewind(file);
    if (fileSz  > 0) {
        *bufLen = (size_t)fileSz;
        *buf = (byte*)malloc(*bufLen);
        if (*buf == NULL) {
            ret = MEMORY_E;
            printf("Error allocating %lu bytes\n", (unsigned long)*bufLen);
        }
        else {
            size_t readLen = fread(*buf, *bufLen, 1, file);

            /* check response code */
            ret = (readLen > 0) ? 0 : -1;
        }
    }
    else {
        ret = BUFFER_E;
    }
    fclose(file);

    return ret;
}

int main(int argc, char* argv[])
{
    int ret;
    WOLFSSL_CERT_MANAGER* cm;
    OcspRequest ocspRequest;
    CertStatus newStatus;
    OcspResponse ocspResponse;
    DecodedCert cert;
    byte *certBuf = NULL, *buf = NULL;
    size_t certBufSz, bufSz;

    if (argc != 4) {
        printf("Usage ocsp_test [ca.der] [cert.der] [ocsp_resp.bin]\n");
        return -1;
    }

    XMEMSET(&cert, 0, sizeof(cert));
    XMEMSET(&ocspResponse, 0, sizeof(ocspResponse));
    XMEMSET(&ocspRequest, 0, sizeof(ocspRequest));

    cm = wolfSSL_CertManagerNew();
    if (cm == NULL) {
        printf("Cert manager error new\n");
        return -1;
    }

    //wolfSSL_Debugging_ON();
    wolfSSL_Init();

    /* Load CA into certificate manager */
    ret = load_file(argv[1], &buf, &bufSz);
    if (ret != 0)
        goto exit;
    ret = wolfSSL_CertManagerLoadCABuffer(cm, buf, bufSz, WOLFSSL_FILETYPE_ASN1);
    free(buf);
    if (ret != WOLFSSL_SUCCESS) {
        printf("wolfSSL_CertManagerLoadCABuffer Error: %d (%s)\n", 
            ret, wolfSSL_ERR_reason_error_string(ret));
    }

    /* Load certificate */
    ret = load_file(argv[2], &certBuf, &certBufSz);
    if (ret != 0)
        goto exit;
    InitDecodedCert(&cert, certBuf, certBufSz, NULL);
    ret = ParseCert(&cert, CERT_TYPE, VERIFY, cm);
    if (ret != 0) {
        printf("ParseCertRelative Error: %d (%s)\n", 
            ret, wolfSSL_ERR_reason_error_string(ret));
    }

    /* Setup OCSP request structure */
    ret = InitOcspRequest(&ocspRequest, &cert, 0, NULL);
    if (ret != 0) {
        printf("InitOcspRequest Error: %d (%s)\n", 
            ret, wolfSSL_ERR_reason_error_string(ret));
    }

    /* Load OCSP response binary data to parse */
    ret = load_file(argv[3], &buf, &bufSz);
    if (ret != 0)
        goto exit;
    InitOcspResponse(&ocspResponse, &newStatus, buf, bufSz, NULL);
    ret = OcspResponseDecode(&ocspResponse, cm, NULL, 0);
    if (ret != 0) {
        printf("OcspResponseDecode Error: %d (%s)\n", 
            ret, wolfSSL_ERR_reason_error_string(ret));
    }

    ret = CompareOcspReqResp(&ocspRequest, &ocspResponse);
    printf("CompareOcspReqResp Result: %d (%s)\n",
        ret, ret == 0 ? "success" : "fail");

exit:
    if (certBuf) free(certBuf);
    if (buf) free(buf);
    FreeOcspResponse(&ocspResponse);
    FreeOcspRequest(&ocspRequest);
    FreeDecodedCert(&cert);

    wolfSSL_CertManagerFree(cm);
    wolfSSL_Cleanup();
    return ret;
}
