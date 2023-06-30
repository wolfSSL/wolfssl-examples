/* lms_sign_verify.c
 *
 * Copyright (C) 2022 wolfSSL Inc.
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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/asn.h>

#ifdef HAVE_LIBLMS

#include <wolfssl/wolfcrypt/lms.h>
#include <wolfssl/wolfcrypt/ext_lms.h>

static void print_usage_and_die(void) WC_NORETURN;
static bool write_key_file(unsigned char * priv, size_t priv_len,
                           void * context);
static bool read_key_file(unsigned char * priv, size_t priv_len,
                          void * context);
static int  lms_sign_verify(int levels, int height, int winternitz,
                            size_t sigs_to_do);
static void dump_hex(const char * what, const uint8_t * buf, size_t len);

static WC_RNG rng;

int
main(int    argc,
     char * argv[])
{
    int    levels = 0;
    int    height = 0;
    int    winternitz = 0;
    size_t sigs_to_do = 1;
    int    ret = 0;

    if (argc < 4 || argc > 5) {
        print_usage_and_die();
    }

    levels = atoi(argv[1]);
    height = atoi(argv[2]);
    winternitz = atoi(argv[3]);

    if (argc == 5) {
        sigs_to_do = atoll(argv[4]);
    }

    ret = wc_InitRng(&rng);
    if (ret) {
        fprintf(stderr, "error: wc_InitRng returned %d\n", ret);
        return EXIT_FAILURE;
    }

    ret = lms_sign_verify(levels, height, winternitz, sigs_to_do);

    wc_FreeRng(&rng);

    return (ret == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void
print_usage_and_die(void)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "  ./lms_sign_verify <levels> <height> <winternitz> [num signatures]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "examples:\n");
    fprintf(stderr, "  ./lms_sign_verify 1 5 1\n");
    fprintf(stderr, "  ./lms_sign_verify 3 5 4 100\n");
    fprintf(stderr, "  ./lms_sign_verify 2 10 2 0\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "description:\n");
    fprintf(stderr, "  Generates an LMS/HSS key pair with L=levels, H=height, and\n");
    fprintf(stderr, "  W=winternitz parameters, then signs and verifies a given\n");
    fprintf(stderr, "  number of signatures (1 by default).\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  If 0 is given for num signatures, it prints the private and\n");
    fprintf(stderr, "  public key as hex and exits early.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "notes:\n");
    fprintf(stderr, " - The acceptable parameter values are those in RFC8554:\n");
    fprintf(stderr, "     levels = {1..8}\n");
    fprintf(stderr, "     height = {5, 10, 15, 20, 25}\n");
    fprintf(stderr, "     winternitz = {1, 2, 4, 8}\n");
    fprintf(stderr, "\n");
    fprintf(stderr, " - The number of available signatures is\n");
    fprintf(stderr, "     n = 2 ** (levels * height)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, " - Larger winternitz values will reduce the signature size, at\n");
    fprintf(stderr, "   the expense of longer key generation and signature times.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, " - Key generation time is strongly determined by the height of\n");
    fprintf(stderr, "   the first level tree. A 3 level, 5 height tree is much faster\n");
    fprintf(stderr, "   than 1 level, 15 height at initial key gen, even if the number\n");
    fprintf(stderr, "   of available signatures is the same.\n");

    exit(EXIT_FAILURE);
}

static bool
write_key_file(unsigned char * priv,
               size_t          priv_len,
               void *          context)
{
    FILE *       file = NULL;
    const char * filename = NULL;
    size_t       n_write = 0;

    if (priv == NULL || context == NULL || priv_len == 0) {
        fprintf(stderr, "error: invalid args\n");
        return false;
    }

    filename = context;

    file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "error: fopen(%s, \"wb\") failed\n", filename);
        return false;
    }

    n_write = fwrite(priv, 1, priv_len, file);

    if (n_write != priv_len) {
        fprintf(stderr, "error: wrote %zu, expected %zu: %d\n", n_write, priv_len,
                ferror(file));
        return false;
    }

    fclose(file);

    return true;
}

static bool
read_key_file(unsigned char * priv,
              size_t          priv_len,
              void *          context)
{
    FILE *       file = NULL;
    const char * filename = NULL;
    size_t       n_read = 0;

    if (priv == NULL || context == NULL || priv_len == 0) {
        fprintf(stderr, "error: invalid args\n");
        return false;
    }

    filename = context;

    file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "error: fopen(%s, \"wb\") failed\n", filename);
        return false;
    }

    n_read = fread(priv, 1, priv_len, file);

    if (n_read != priv_len) {
        fprintf(stderr, "error: read %zu, expected %zu\n", n_read, priv_len);
        return false;
    }

    fclose(file);

    return true;
}

static int
lms_sign_verify(int    levels,
                int    height,
                int    winternitz,
                size_t sigs_to_do)
{
    LmsKey       signingKey;
    LmsKey       verifyKey;
    const char * msg = "wolfSSL LMS example message!";
    const char * filename = "lms_example.key";
    int          ret = 0;
    size_t       exp = 0;
    byte *       sig = NULL;
    word32       sigSz = 0;
    word32       privSz = 0;
    word32       pubSz = 0;

    printf("using parameters: levels=%d, height=%d, winternitz=%d\n",
           levels, height, winternitz);

    ret = wc_LmsKey_Init_ex(&signingKey, levels, height, winternitz, NULL, 0);
    if (ret) {
        fprintf(stderr, "error: wc_LmsKey_Init_ex(%d, %d, %d) returned %d\n",
                levels, height, winternitz, ret);
        goto exit_sign_verify;
    }

    ret = wc_LmsKey_SetWriteCb(&signingKey, write_key_file);
    if (ret) {
        fprintf(stderr, "error: wc_LmsKey_SetWriteCb failed: %d\n", ret);
        goto exit_sign_verify;
    }

    ret = wc_LmsKey_SetReadCb(&signingKey, read_key_file);
    if (ret) {
        fprintf(stderr, "error: wc_LmsKey_SetReadCb failed: %d\n", ret);
        goto exit_sign_verify;
    }

    ret = wc_LmsKey_SetContext(&signingKey, (void *) filename);
    if (ret) {
        fprintf(stderr, "error: wc_LmsKey_SetContext failed: %d\n", ret);
        goto exit_sign_verify;
    }

    ret = wc_LmsKey_GetSigLen(&signingKey, &sigSz);
    if (ret || sigSz == 0) {
        fprintf(stderr, "error: wc_LmsKey_GetSigLen returned: %d, %d\n",
                ret, sigSz);
        goto exit_sign_verify;
    }

    ret = wc_LmsKey_GetPubLen(&signingKey, &pubSz);
    if (ret || pubSz == 0) {
        fprintf(stderr, "error: wc_LmsKey_GetPubLen returned: %d, %d\n",
                ret, pubSz);
        goto exit_sign_verify;
    }

    ret = wc_LmsKey_GetPrivLen(&signingKey, &privSz);
    if (ret || pubSz == 0) {
        fprintf(stderr, "error: wc_LmsKey_GetPrivLen returned: %d, %d\n",
                ret, privSz);
        goto exit_sign_verify;
    }

    printf("signature length: %d\n", sigSz);
    printf("priv key length: %d\n", privSz);
    printf("pub key length: %d\n", pubSz);

    exp = (levels * height);
    if (exp >= 64) {
        printf("note: {levels = %d, height = %d}, limiting to 2**64 sigs\n",
              levels, height);
        exp = 63;
    }

    printf("generating key with %zu OTS signatures...\n", (size_t) 2 << (exp - 1));

    ret = wc_LmsKey_MakeKey(&signingKey, &rng);
    if (ret) {
        fprintf(stderr, "error: wc_LmsKey_MakeKey returned %d\n", ret);
        goto exit_sign_verify;
    }

    printf("...done!\n");

    if (sigs_to_do == 0) {
        /* If using callbacks the .priv member will not be filled. */
        read_key_file(signingKey.priv, privSz, (void *) filename);
        dump_hex("priv", signingKey.priv, privSz);
        dump_hex("pub", signingKey.pub, pubSz);
        goto exit_sign_verify;
    }

    sig = malloc(sigSz);
    if (sig == NULL) {
        fprintf(stderr, "error: malloc(%d) failed\n", sigSz);
        goto exit_sign_verify;
    }

    ret = wc_LmsKey_ExportPub(&verifyKey, &signingKey);
    if (ret) {
        fprintf(stderr, "error: wc_LmsKey_ExportPub returned %d\n", ret);
        goto exit_sign_verify;
    }

    printf("signing and verifying %zu signatures...\n", sigs_to_do);
    for (size_t i = 0; i < sigs_to_do; ++i) {
        if (wc_LmsKey_SigsLeft(&signingKey) <= 0) {
            fprintf(stderr, "error: no remaining signatures\n");
            goto exit_sign_verify;
        }

        ret = wc_LmsKey_Sign(&signingKey, sig, &sigSz,(byte *) msg,
                             strlen(msg));
        if (ret) {
            fprintf(stderr, "error: %zu: wc_LmsKey_Sign returned %d\n", i, ret);
            goto exit_sign_verify;
        }

        ret = wc_LmsKey_Verify(&verifyKey, sig, sigSz, (const byte *) msg,
                               strlen(msg));
        if (ret) {
            fprintf(stderr, "error: %zu: wc_LmsKey_Verify returned %d\n", i, ret);
            goto exit_sign_verify;
        }
    }

    printf("...done!\n");
    printf("finished\n");

exit_sign_verify:

    if (sig != NULL) {
        free(sig);
        sig = NULL;
    }

    wc_LmsKey_Free(&signingKey);
    wc_LmsKey_Free(&verifyKey);

    return ret;
}

static void
dump_hex(const char *    what,
         const uint8_t * buf,
         size_t          len)
{
    printf("%s\n", what);
    for (size_t i = 0; i < len; ++i) {
        printf("0x%02X ", buf[i]);

        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }

    if (len % 8) {
        printf("\n");
    }

    return;
}

#else

int main(int argc, char** argv) {
    printf("This requires the --with-liblms flag.\n");
    return 0;
}
#endif /* WITH_LILMS */

