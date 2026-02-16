/* lms-kg.c
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
 * gcc -lwolfssl -o lms-kg lms-kg.c
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

static int write_file(const char* filename, const unsigned char* data,
    word32 sz)
{
    FILE* f;
    int err = 1;

    printf("  Writing to file: %s\n", filename);

    /* Open file to put LMS private into. */
    f = fopen(filename, "wb");
    if (f == NULL) {
        printf("      unable to write to file\n");
        goto write_file_end;
    }

    /* Write out LMS private key. */
    fwrite(data, 1, sz, f);
    fclose(f);

    err = 0;
write_file_end:
    return err;
}

static int lms_write_key_file(const byte* priv, word32 privSz, void *context)
{
    int ret = WC_LMS_RC_WRITE_FAIL;
    const char* filename = (const char*)context;

    /* Write the private key to file. */
    if (write_file(filename, priv, privSz) == 0)
        ret = WC_LMS_RC_SAVED_TO_NV_MEMORY;

    return ret;
}

static int lms_export_public_key(LmsKey* key, const char* filename)
{
    int ret;
    unsigned char* pub;
    word32 pubSz;

    /* Get the public key length. */
    ret = wc_LmsKey_GetPubLen(key, &pubSz);
    if (ret != 0) {
        print_wolfssl_error("Failed to get public key size", ret);
        goto exit_lms_export_public_key;
    }

    /* Allocate memory for public key to be exported into. */
    pub = malloc(pubSz);
    if (pub == NULL) {
        ret = MEMORY_E;
        printf("Failed to allocate memory: %d bytes\n", pubSz);
        goto exit_lms_export_public_key;
    }

    /* Export public key. */
    ret = wc_LmsKey_ExportPubRaw(key, pub, &pubSz);
    if (ret != 0) {
        print_wolfssl_error("Failed to export public key", ret);
        goto exit_lms_export_public_key;
    }

    /* Write public key to file. */
    ret = write_file(filename, pub, pubSz);

exit_lms_export_public_key:
    if (pub != NULL)
        free(pub);

    return ret;
}

int main(int argc, char* argv[])
{
    int ret;
    WC_RNG rng[1];
    LmsKey key[1];
    const char* filename = "lms_key.bin";
    const char* pubKeyFilename = "lms_pubkey.bin";
    int levels = 1;
    int height = 10;
    int winternitz = 4;
    int params = -1;

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
#endif

    XMEMSET(rng, 0, sizeof(rng));
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
                goto exit_lms_kg;
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
                goto exit_lms_kg;
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
                goto exit_lms_kg;
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
                goto exit_lms_kg;
            }
            params = atoi(argv[0]);
            if ((params < 1) || (params > 60)) {
                ret = -1;
                printf("Invalid params (1-60): %d\n", params);
                goto exit_lms_kg;
            }
        }
        else {
            printf("Unrecognized option: %s\n", argv[0]);
            goto exit_lms_kg;
        }

        argc--;
        argv++;
    }

    /* Initialize random number generator for use in making a key. */
    ret = wc_InitRng(rng);
    if (ret != 0) {
        print_wolfssl_error("Failed to initialize random", ret);
        goto exit_lms_kg;
    }

    /* Initialize the LMS key. */
    ret = wc_LmsKey_Init(key, NULL, INVALID_DEVID);
    if (ret != 0) {
        print_wolfssl_error("Failed to initialize LMS key", ret);
        goto exit_lms_kg;
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
        goto exit_lms_kg;
    }

    /* Set writing callback for generated private key. */
    ret = wc_LmsKey_SetWriteCb(key, lms_write_key_file);
    if (ret != 0) {
        print_wolfssl_error("Could not set write callback", ret);
        goto exit_lms_kg;
    }

    /* Set write callback context - the filename to write private key to. */
    ret = wc_LmsKey_SetContext(key, (void*)filename);
    if (ret != 0) {
        print_wolfssl_error("Could not set context for write callback", ret);
        goto exit_lms_kg;
    }

    /* Make the private/public key pair and store private key. */
    ret = wc_LmsKey_MakeKey(key, rng);
    if (ret != 0) {
        print_wolfssl_error("Failed to generate key", ret);
        goto exit_lms_kg;
    }

    /* Store public key to file. */
    ret = lms_export_public_key(key, pubKeyFilename);

exit_lms_kg:
    /* Cleanup. */
    wc_LmsKey_Free(key);
    wc_FreeRng(rng);

    return (ret == 0) ? 0 : 1;
}

#else

int main()
{
    printf("wolfSSL requires LMS to be compiled in\n");
    return 1;
}

#endif

