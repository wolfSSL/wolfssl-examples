/* clu_parse.c
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
 */

#include <stdio.h>
#include "clu_include/clu_header_main.h"
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/ssl.h> /* wolfSSL_CertPemToDer */
#include "clu_include/clu_error_codes.h"
#include "clu_include/x509/clu_parse.h"

#define MAX_CERT_SIZE 16384

enum {
    PEM = 0,
    DER = 1,
    TEXT = 2,
};

int wolfCLU_inpemOutpem(char* infile, char* outfile, int silent_flag)
{
    int ret;
    ret = wolfCLU_parseFile(infile, PEM, outfile, PEM, silent_flag);
    return ret;
}

int wolfCLU_inpemOutder(char* infile, char* outfile, int silent_flag)
{
    int ret;
    ret = wolfCLU_parseFile(infile, PEM, outfile, DER, silent_flag);
    return ret;
}

int wolfCLU_inderOutpem(char* infile, char* outfile, int silent_flag)
{
    int ret;
    ret = wolfCLU_parseFile(infile, DER, outfile, PEM, silent_flag);
    return ret;
}

int wolfCLU_inderOutder(char* infile, char* outfile, int silent_flag)
{
    int ret;
    ret = wolfCLU_parseFile(infile, DER, outfile, DER, silent_flag);
    return ret;
}

int wolfCLU_inpemOuttext(char* infile, char* outfile, int silent_flag) {
    int ret;
    ret = wolfCLU_parseFile(infile, PEM, outfile, TEXT, silent_flag);
    return ret;
}


/* returns alloc'd WOLFSSL_X509 structure on success */
static WOLFSSL_X509* wolfCLU_parseX509(char* infile, int inform)
{
    int type;

    type = (inform == DER)? WOLFSSL_FILETYPE_ASN1 : WOLFSSL_FILETYPE_PEM;

    return wolfSSL_X509_load_certificate_file(infile, type);
}


/* returns 0 on success */
int wolfCLU_printPubKey(char* infile, int inform, char* outfile,
        int silent_flag)
{
    int ret = 0;
    WOLFSSL_X509        *x509;
    WOLFSSL_BIO         *bio;

    unsigned char *der = NULL;
    int derSz;

    unsigned char *pem = NULL;
    int pemSz;

    //@TODO
    if (inform) {
        inform = PEM;
    }
    else {
        inform = DER;
    }

    x509 = wolfCLU_parseX509(infile, inform);
    if (x509 == NULL) {
        printf("unable to parse file %s\n", infile);
        ret = -1;
    }

    /* use stdout if outfile is null */
    if (ret == 0 && outfile == NULL) {
        bio = wolfSSL_BIO_new(wolfSSL_BIO_s_file());
        if (bio == NULL) {
            ret = -1;
        }
        else {
            if (wolfSSL_BIO_set_fp(bio, stdout, BIO_NOCLOSE)
                    != WOLFSSL_SUCCESS) {
                ret = -1;
            }
        }
    }

    if (ret == 0 && outfile != NULL) {
        bio = wolfSSL_BIO_new_file(outfile, "rb");
        if (bio == NULL) {
            printf("unable to read outfile %s\n", outfile);
            ret = -1;
        }
    }

    /* get the size of the pubkey der buffer and alloc it */
    if (ret == 0) {
        if (wolfSSL_X509_get_pubkey_buffer(x509, NULL, &derSz)
                == WOLFSSL_SUCCESS) {
            der = (unsigned char*)XMALLOC(derSz, NULL, DYNAMIC_TYPE_PUBLIC_KEY);
            if (der == NULL) {
                ret = -1;
            }
            else {
                if (wolfSSL_X509_get_pubkey_buffer(x509, der, &derSz)
                        != WOLFSSL_SUCCESS) {
                    ret = -1;
                }
            }
        }
        else {
            ret = -1;
        }
    }

    /* get pem size alloc buffer and convert to pem format */
    if (ret == 0) {
        pemSz = wc_DerToPemEx(der, derSz, NULL, 0, NULL, PUBLICKEY_TYPE);
        if (pemSz > 0) {
            pem = (unsigned char*)XMALLOC(pemSz, NULL, DYNAMIC_TYPE_PUBLIC_KEY);
            if (pem == NULL) {
                ret = -1;
            }
            else {
                if (wc_DerToPemEx(der, derSz, pem, pemSz, NULL, PUBLICKEY_TYPE)
                        <= 0) {
                    ret = -1;
                }
            }
        }
        else {
            ret = -1;
        }
    }

    if (ret == 0) {
        if (wolfSSL_BIO_write(bio, pem, pemSz) != pemSz) {
            ret = -1;
        }
    }

    wolfSSL_BIO_free(bio);
    wolfSSL_X509_free(x509);

    if (der != NULL)
        XFREE(der, NULL, DYNAMIC_TYPE_PUBLIC_KEY);
    if (pem != NULL)
        XFREE(pem, NULL, DYNAMIC_TYPE_PUBLIC_KEY);

    return ret;
}


int wolfCLU_parseFile(char* infile, int inform, char* outfile, int outform,
                                                                int silent_flag)
{
    int i, ret, inBufSz, outBufSz;
    FILE* instream;
    FILE* outstream;
    byte* inBuf = NULL;
    byte* outBuf = NULL;

    if (infile == NULL || outfile == NULL)
        return BAD_FUNC_ARG;

    /* MALLOC buffer for the certificate to be processed */
    inBuf = (byte*) XMALLOC(MAX_CERT_SIZE, HEAP_HINT,
                                                   DYNAMIC_TYPE_TMP_BUFFER);

    if (inBuf == NULL) return MEMORY_E;
    XMEMSET(inBuf, 0, MAX_CERT_SIZE);

    instream    = fopen(infile, "rb");
    if (XSTRNCMP(outfile, "stdout", 6) == 0) {
        outstream = stdout;
    }
    else {
        outstream  = fopen(outfile, "wb");
    }
    
/*----------------------------------------------------------------------------*/
/* read in der, output der */
/*----------------------------------------------------------------------------*/
    if ( (inform & outform) == 1) {
        printf("in parse: in = der, out = der\n");
    }
/*----------------------------------------------------------------------------*/
/* read in pem, output pem formatted human-readable-text */
/*----------------------------------------------------------------------------*/    
    else if ( inform == PEM && outform == TEXT ) {
        WOLFSSL_X509* x509;
        WOLFSSL_BIO* bio;
        
        x509 = wolfSSL_X509_load_certificate_file(infile, SSL_FILETYPE_PEM);
        bio = wolfSSL_BIO_new(wolfSSL_BIO_s_file());
        
        /* checking if output file was given, if not write to stdout */
        wolfSSL_BIO_set_fp(bio, outstream, BIO_NOCLOSE);
        
        if(x509 == NULL){
            printf("x509 Failure Still Null\n");
        }

        if(bio == NULL){
            printf("BIO Failure Still Null\n");
        }
        
        ret = wolfSSL_X509_print(bio, x509);
        if (ret == SSL_FAILURE) {
            printf("Failed to write x509 cert.\n");
            goto clu_parse_cleanup;
        }
        wolfSSL_BIO_free(bio);
        wolfSSL_X509_free(x509);
    }
/*----------------------------------------------------------------------------*/
/* read in der, output pem */
/*----------------------------------------------------------------------------*/
    else if ( (inform && !outform) ) {
        /* read in the certificate to be processed */
        inBufSz = fread(inBuf, 1, MAX_CERT_SIZE, instream);
        if (inBufSz <= 0) {
            ret = FREAD_ERROR;
            goto clu_parse_cleanup;
        }

        /* MALLOC buffer for the result of conversion from der to pem */
        outBuf = (byte*) XMALLOC(MAX_CERT_SIZE, HEAP_HINT,
                                                       DYNAMIC_TYPE_TMP_BUFFER);
        if (outBuf == NULL) {
            XFREE(inBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            ret = MEMORY_E;
            goto clu_parse_cleanup;
        }
        XMEMSET(outBuf, 0, MAX_CERT_SIZE);

        /* convert inBuf from der to pem, store result in outBuf  */
        outBufSz = wc_DerToPem(inBuf, inBufSz, outBuf, MAX_CERT_SIZE,
                                                                     CERT_TYPE);
        if (outBufSz < 0) {
            wolfCLU_freeBins(inBuf, outBuf, NULL, NULL, NULL);
            ret = DER_TO_PEM_ERROR;
            goto clu_parse_cleanup;
        }

        /* write the result of conversion to the outfile specified */
        ret = fwrite(outBuf, 1, outBufSz, outstream);
        if (ret <= 0) {
            wolfCLU_freeBins(inBuf, outBuf, NULL, NULL, NULL);
            ret = FWRITE_ERROR;
            goto clu_parse_cleanup;
        }

        if (!silent_flag) {
           for (i = 0; i < outBufSz; i++) {
                printf("%c", outBuf[i]);
            }
        }

        /* success cleanup */
        wolfCLU_freeBins(inBuf, outBuf, NULL, NULL, NULL);
   }
/*----------------------------------------------------------------------------*/
/* read in pem, output der */
/*----------------------------------------------------------------------------*/
    else if ( (!inform && outform) ) {
        inBufSz = fread(inBuf, 1, MAX_CERT_SIZE, instream);
        if (inBufSz <= 0) {
            ret = FREAD_ERROR;
            goto clu_parse_cleanup;
        }

        /* MALLOC buffer for the result of converstion from pem to der */
        outBuf = (byte*) XMALLOC(MAX_CERT_SIZE, HEAP_HINT,
                                                       DYNAMIC_TYPE_TMP_BUFFER);
        if (outBuf == NULL) {
            XFREE(inBuf, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
            ret = MEMORY_E;
            goto clu_parse_cleanup;
        }
        XMEMSET(outBuf, 0, MAX_CERT_SIZE);

        /* convert inBuf from pem to der, store result in outBuf */
        outBufSz = wolfSSL_CertPemToDer(inBuf, inBufSz, outBuf, MAX_CERT_SIZE,
                                                                     CERT_TYPE);
        if (outBufSz < 0) {
            wolfCLU_freeBins(inBuf, outBuf, NULL, NULL, NULL);
            ret = PEM_TO_DER_ERROR;
            goto clu_parse_cleanup;
        }

        /* write the result of conversion to the outfile specified */
        ret = fwrite(outBuf, 1, outBufSz, outstream);
        if (ret <= 0) {
            wolfCLU_freeBins(inBuf, outBuf, NULL, NULL, NULL);
            ret = FWRITE_ERROR;
            goto clu_parse_cleanup;
        }

        /* success cleanup */
        wolfCLU_freeBins(inBuf, outBuf, NULL, NULL, NULL);
    }
/*----------------------------------------------------------------------------*/
/* read in pem, output pem */
/*----------------------------------------------------------------------------*/
    else {
        printf("in parse: in = pem, out = pem\n");
    }
    ret = 0;

clu_parse_cleanup:
    
    fclose(outstream);
    fclose(instream);

    return ret;
}
