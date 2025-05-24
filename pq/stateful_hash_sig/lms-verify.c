/* lms-verify.c
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

/* Example to demonstrate LMS Key Generation */
/* 
 * ./configure --enable-lms && make && sudo make install
 * gcc -lwolfssl -o lms-verify lms-verify.c
 */

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/lms.h>
#ifdef WOLFSSL_WC_LMS
#include <wolfssl/wolfcrypt/wc_lms.h>
#else
#include <wolfssl/wolfcrypt/ext_lms.h>
#endif
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>

#include <stdio.h>

#ifdef WOLFSSL_HAVE_LMS

void print_wolfssl_error(const char* msg, int err)
{
#ifndef NO_ERROR_STRINGS
    printf("%s: %s (%d)\n", msg, wc_GetErrorString(err), err);
#else
    printf("%s: %d\n", msg, err);
#endif
}

static int read_file(const char* filename, unsigned char* data, int* sz)
{
    FILE* f = NULL;
    int err = 1;
    int fileSz;

    printf("  Reading from file: %s\n", filename);

    /* Open file. */
    f = fopen(filename, "rb");
    if (f == NULL) {
        printf("    unable to open public key\n");
        goto load_end;
    }

    /* Get length of file. */
    fseek(f, 0, SEEK_END);
    fileSz = ftell(f);
    /* Check buffer is big enough. */
    if (fileSz > *sz) {
        printf("    File %s exceeds max size: %d > %d\n", filename, fileSz,
            *sz);
        goto load_end;
    }
    /* Go back to start of file. */
    fseek(f, 0, SEEK_SET);

    /* Read in all of file. */
    fileSz = fread(data, 1, fileSz, f);
    printf("    Read %d bytes\n", fileSz);

    *sz = fileSz;
    err = 0;
load_end:
    fclose(f);
    return err;
}

int lms_load_public_key(LmsKey* key, const char* filename)
{
    int ret;
    unsigned char* pub = NULL;
    word32 pubSz;
    int sz;

    /* Get public key length. */
    ret = wc_LmsKey_GetPubLen(key, &pubSz);
    if (ret != 0) {
        print_wolfssl_error("Failed to import publc key", ret);
        goto exit_lms_load_public_key;
    }

    /* Allocate memory for public key to be loaded into. */
    pub = malloc(pubSz);
    if (pub == NULL) {
        ret = MEMORY_E;
        printf("Failed to allocate memory: %d bytes\n", pubSz);
        goto exit_lms_load_public_key;
    }

    sz = (int)pubSz;
    /* Read the public key from file. */
    if (read_file(filename, pub, &sz) != 0) {
        ret = IO_FAILED_E;
        goto exit_lms_load_public_key;
    }

    /* Import public key into LMS key. */
    ret = wc_LmsKey_ImportPubRaw(key, pub, pubSz);
    if (ret != 0) {
        print_wolfssl_error("Failed to import publc key", ret);
        goto exit_lms_load_public_key;
    }

exit_lms_load_public_key:
    if (pub != NULL)
        free(pub);
    return ret;
}

int lms_verify_file(LmsKey* key, const char* filename)
{
    int ret; 
    unsigned char* sig = NULL;
    word32 sigSz;
    int sigLen;
    unsigned char msg[1024*1024];
    int msgSz;
    char sigFilename[128];

    /* Get the length of a signature. */
    ret = wc_LmsKey_GetSigLen(key, &sigSz);
    if (ret != 0) {
        print_wolfssl_error("Failed to get signature length", ret);
        goto exit_lms_verify_files;
    }

    /* Allocate memory for signature to be put into. */
    sig = malloc(sigSz);
    if (sig == NULL) {
        ret = MEMORY_E;
        printf("Failed to allocate me memory\n");
        goto exit_lms_verify_files;
    }

    sigLen = sigSz;
    msgSz = (int)sizeof(msg);

    /* Read file to verify. */
    ret = read_file(filename, msg, &msgSz);
    if (ret != 0) {
        goto exit_lms_verify_files;
    }

    /* Create signature filename. */
    strncpy(sigFilename, filename, sizeof(sigFilename) - 5);
    sigFilename[sizeof(sigFilename) - 5] = '\0';
    strcat(sigFilename, ".sig");
    /* Read in signature from file. */
    ret = read_file(sigFilename, sig, &sigLen);
    if (ret != 0) {
        goto exit_lms_verify_files;
    }
    /* Check signature is the expected size. */
    if (sigLen != (int)sigSz) {
        ret = IO_FAILED_E;
        printf("Signature size does not match expected: %d != %d\n", sigLen,
            sigSz);
        goto exit_lms_verify_files;
    }

    /* Verify the signature against the message. */
    ret = wc_LmsKey_Verify(key, sig, sigSz, msg, msgSz);
    if (ret != 0) {
        print_wolfssl_error("Failed to verify message", ret);
        goto exit_lms_verify_files;
    }
    printf("Verification succeeded\n");

exit_lms_verify_files:
    if (sig != NULL)
        free(sig);
    return ret;
}

int main(int argc, char* argv[])
{
    int ret;
    LmsKey key[1];
    const char* filename = "lms_pubkey.bin";
    int levels = 1;
    int height = 10;
    int winternitz = 4;
    int params = -1;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    XMEMSET(key, 0, sizeof(key));

    argc--;
    argv++;
    while (argc > 0) {
        /* Number of levels of trees. */
        if (strcmp(argv[0], "--levels") == 0) {
            argc--;
            argv++;
            if (argc == 0) {
                ret = -1;
                printf("No level value supplied\n");
                goto exit_lms_verify;
            }
            levels = atoi(argv[0]);
            if ((levels < 1) || (levels > 4)) {
                ret = -1;
                printf("Invalid levels (1-4): %d\n", levels);
            }
        }
        /* Height of each tree. */
        else if (strcmp(argv[0], "--height") == 0) {
            argc--;
            argv++;
            if (argc == 0) {
                ret = -1;
                printf("No height value supplied\n");
                goto exit_lms_verify;
            }
            height = atoi(argv[0]);
            if ((height != 5) && (height != 10) && (height != 15) &&
                     (height != 20)) {
                ret = -1;
                printf("Invalid height (5, 10, 15, 20): %d\n", height);
            }
        }
        /* Winternitz value. */
        else if (strcmp(argv[0], "--winternitz") == 0) {
            argc--;
            argv++;
            if (argc == 0) {
                ret = -1;
                printf("No winternitz value supplied\n");
                goto exit_lms_verify;
            }
            winternitz = atoi(argv[0]);
            if ((winternitz != 1) && (winternitz != 2) && (winternitz != 4) &&
                     (winternitz != 8)) {
                ret = -1;
                printf("Invalid height (1, 2, 4, 8): %d\n", winternitz);
            }
        }
        /* Parameters id. */
        else if (strcmp(argv[0], "--params") == 0) {
            argc--;
            argv++;
            if (argc == 0) {
                ret = -1;
                printf("No params value supplied\n");
                goto exit_lms_verify;
            }
            params = atoi(argv[0]);
            if ((params < 1) || (params > 60)) {
                ret = -1;
                printf("Invalid params (1-60): %d\n", params);
                goto exit_lms_verify;
            }
        }
        else if (argv[0][0] == '-') {
            printf("Unrecognized option: %s\n", argv[0]);
            goto exit_lms_verify;
        }
        else {
            /* Expecting filename. */
            break;
        }

        argc--;
        argv++;
    }

    /* Initialize the LMS key. */
    ret = wc_LmsKey_Init(key, NULL, INVALID_DEVID);
    if (ret != 0) {
        print_wolfssl_error("Failed to initialize LMS key", ret);
        goto exit_lms_verify;
    }

    /* Set parameters to use. */
    if (params == -1) {
        printf("Levels: %d, Height: %d, Winternitz: %d\n", levels, height,
            winternitz);
        ret = wc_LmsKey_SetParameters(key, levels, height, winternitz);
    }
    else {
        printf("Using parameters: %s\n", wc_LmsKey_ParmToStr(params));
        ret = wc_LmsKey_SetLmsParm(key, params);
    }
    if (ret != 0) {
        print_wolfssl_error("Parameters for LMS not valid", ret);
        goto exit_lms_verify;
    }

    /* Load the public key from file. */
    ret = lms_load_public_key(key, filename);
    if (ret != 0) {
        goto exit_lms_verify;
    }

    /* Verify the file with the LMS key. */
    ret = lms_verify_file(key, argv[0]);

exit_lms_verify:
    wc_LmsKey_Free(key);

    return (ret == 0) ? 0 : 1;
}

#else

int main()
{
    printf("wolfSSL requires LMS to be compiled in\n");
    return 1;
}

#endif

