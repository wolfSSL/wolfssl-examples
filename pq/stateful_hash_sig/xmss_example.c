/* xmss_example.c
 *
 * Copyright (C) 2023 wolfSSL Inc.
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
#include <stdio.h>
#include <stdlib.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#ifdef HAVE_LIBXMSS

#include <wolfssl/wolfcrypt/xmss.h>
#include <wolfssl/wolfcrypt/ext_xmss.h>

static void dump_hex(const char * what, const uint8_t * buf, size_t len);
static void print_usage(void);
#if !defined WOLFSSL_XMSS_VERIFY_ONLY
static int  write_key_file(const byte * priv, word32 privSz, void * context);
static int  read_key_file(byte * priv, word32 privSz, void * context);
static int  do_xmss_example(const char * params, size_t sigs_to_do);

static WC_RNG rng;
static byte * read_buf = NULL;

int
main(int    argc,
     char * argv[])
{
    const char * params = NULL;
    size_t sigs_to_do = 1;
    int    ret = 0;

    if (argc < 2 || argc > 3) {
        print_usage();
        return EXIT_FAILURE;
    }

    params = argv[1];

    if (argc == 3) {
        sigs_to_do = atoll(argv[2]);
    }

    ret = wc_InitRng(&rng);
    if (ret) {
        fprintf(stderr, "error: wc_InitRng returned %d\n", ret);
        return EXIT_FAILURE;
    }

    ret = do_xmss_example(params, sigs_to_do);

    wc_FreeRng(&rng);

    return (ret == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void
print_usage(void)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "  ./xmss_example <param string> [num signatures]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "examples:\n");
    fprintf(stderr, "  ./xmss_example XMSSMT-SHA2_20/4_256 5\n");
    fprintf(stderr, "  ./xmss_example XMSSMT-SHA2_60/6_256 100\n");
    fprintf(stderr, "  ./xmss_example XMSS-SHA2_10_256 1023\n");

    exit(EXIT_FAILURE);
}

static int
write_key_file(const byte * priv,
               word32       privSz,
               void *       context)
{
    FILE *       file = NULL;
    const char * filename = NULL;
    int          n_cmp = 0;
    size_t       n_read = 0;
    size_t       n_write = 0;
    int          err = 0;

    if (priv == NULL || context == NULL || privSz == 0) {
        fprintf(stderr, "error: invalid write args\n");
        return WC_XMSS_RC_BAD_ARG;
    }

    filename = context;

    /* Open file for read and write. */
    file = fopen(filename, "r+");
    if (!file) {
        /* Create the file if it didn't exist. */
        file = fopen(filename, "w+");
        if (!file) {
            fprintf(stderr, "error: fopen(%s, \"w+\") failed: %d\n", filename,
                    ferror(file));
            return WC_XMSS_RC_WRITE_FAIL;
        }
    }

    n_write = fwrite(priv, 1, privSz, file);

    if (n_write != privSz) {
        fprintf(stderr, "error: wrote %zu, expected %d: %d\n", n_write, privSz,
                ferror(file));
        return WC_XMSS_RC_WRITE_FAIL;
    }

    /* Verify data has actually been written to disk correctly. */
    rewind(file);

    XMEMSET(read_buf, 0, n_write);

    n_read = fread(read_buf, 1, n_write, file);

    if (n_read != n_write) {
        fprintf(stderr, "error: read %zu, expected %zu: %d\n", n_read, n_write,
                ferror(file));
        return WC_XMSS_RC_WRITE_FAIL;
    }

    n_cmp = XMEMCMP(read_buf, priv, n_write);
    if (n_cmp != 0) {
        fprintf(stderr, "error: write data was corrupted: %d\n", n_cmp);
        return WC_XMSS_RC_WRITE_FAIL;
    }

    err = fclose(file);
    if (err) {
        fprintf(stderr, "error: fclose returned %d\n", err);
        return WC_XMSS_RC_WRITE_FAIL;
    }

    return WC_XMSS_RC_SAVED_TO_NV_MEMORY;
}

static int
read_key_file(byte * priv,
              word32 privSz,
              void * context)
{
    FILE *       file = NULL;
    const char * filename = NULL;
    size_t       n_read = 0;

    if (priv == NULL || context == NULL || privSz == 0) {
        fprintf(stderr, "error: invalid read args\n");
        return WC_XMSS_RC_BAD_ARG;
    }

    filename = context;

    file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "error: fopen(%s, \"rb\") failed\n", filename);
        return WC_XMSS_RC_READ_FAIL;
    }

    n_read = fread(priv, 1, privSz, file);

    if (n_read != privSz) {
        fprintf(stderr, "error: read %zu, expected %d: %d\n", n_read, privSz,
                ferror(file));
        return WC_XMSS_RC_READ_FAIL;
    }

    fclose(file);

    return WC_XMSS_RC_READ_TO_MEMORY;
}

static int
do_xmss_example(const char * params,
                size_t sigs_to_do)
{
    XmssKey      signingKey;
    XmssKey      verifyKey;
    const char * msg = "wolfSSL XMSS example message!";
    const char * filename = "xmss_example.key";
    int          ret = 0;
    byte *       sig = NULL;
    word32       sigSz = 0;
    word32       privSz = 0;
    word32       pubSz = 0;

    printf("using parameters: %s\n", params);

    ret = wc_XmssKey_Init(&signingKey, NULL, 0);
    if (ret) {
        fprintf(stderr, "error: wc_XmssKey_Init returned %d\n", ret);
        goto exit_xmss_example;
    }

    ret = wc_XmssKey_Init(&verifyKey, NULL, 0);
    if (ret) {
        fprintf(stderr, "error: wc_XmssKey_Init returned %d\n", ret);
        goto exit_xmss_example;
    }

    ret = wc_XmssKey_SetParamStr(&signingKey, params);
    if (ret) {
        fprintf(stderr, "error: wc_XmssKey_SetParameters(%s) returned %d\n",
                params, ret);
        goto exit_xmss_example;
    }

    ret = wc_XmssKey_SetWriteCb(&signingKey, write_key_file);
    if (ret) {
        fprintf(stderr, "error: wc_XmssKey_SetWriteCb failed: %d\n", ret);
        goto exit_xmss_example;
    }

    ret = wc_XmssKey_SetReadCb(&signingKey, read_key_file);
    if (ret) {
        fprintf(stderr, "error: wc_XmssKey_SetReadCb failed: %d\n", ret);
        goto exit_xmss_example;
    }

    ret = wc_XmssKey_SetContext(&signingKey, (void *) filename);
    if (ret) {
        fprintf(stderr, "error: wc_XmssKey_SetContext failed: %d\n", ret);
        goto exit_xmss_example;
    }

    ret = wc_XmssKey_GetSigLen(&signingKey, &sigSz);
    if (ret || sigSz == 0) {
        fprintf(stderr, "error: wc_XmssKey_GetSigLen returned: %d, %d\n",
                ret, sigSz);
        goto exit_xmss_example;
    }

    ret = wc_XmssKey_GetPubLen(&signingKey, &pubSz);
    if (ret || pubSz == 0) {
        fprintf(stderr, "error: wc_XmssKey_GetPubLen returned: %d, %d\n",
                ret, pubSz);
        goto exit_xmss_example;
    }

    ret = wc_XmssKey_GetPrivLen(&signingKey, &privSz);
    if (ret || pubSz == 0) {
        fprintf(stderr, "error: wc_XmssKey_GetPrivLen returned: %d, %d\n",
                ret, privSz);
        goto exit_xmss_example;
    }

    printf("signature length: %d\n", sigSz);
    printf("priv key length: %d\n", privSz);
    printf("pub key length: %d\n", pubSz);

    read_buf = malloc(privSz);
    if (read_buf == NULL) {
        fprintf(stderr, "error: malloc read_buf failed\n");
        goto exit_xmss_example;
    }

    printf("making key with %s parameters...\n", params);

    ret = wc_XmssKey_MakeKey(&signingKey, &rng);
    if (ret) {
        fprintf(stderr, "error: wc_XmssKey_MakeKey returned %d\n", ret);
        goto exit_xmss_example;
    }

    printf("...done!\n");

    if (sigs_to_do == 0) {
        read_key_file(read_buf, privSz, (void *) filename);
        dump_hex("priv", read_buf, privSz);
        dump_hex("pub", signingKey.pk, pubSz);
        goto exit_xmss_example;
    }

    sig = malloc(sigSz);
    if (sig == NULL) {
        fprintf(stderr, "error: malloc(%d) failed\n", sigSz);
        goto exit_xmss_example;
    }

    ret = wc_XmssKey_ExportPub(&verifyKey, &signingKey);
    if (ret) {
        fprintf(stderr, "error: wc_XmssKey_ExportPub returned %d\n", ret);
        goto exit_xmss_example;
    }

    printf("signing and verifying %zu signatures...\n", sigs_to_do);
    for (size_t i = 0; i < sigs_to_do; ++i) {
        ret = wc_XmssKey_Sign(&signingKey, sig, &sigSz,(byte *) msg,
                             strlen(msg));
        if (ret) {
            fprintf(stderr, "error: %zu: wc_XmssKey_Sign returned %d\n", i, ret);
            goto exit_xmss_example;
        }

        ret = wc_XmssKey_Verify(&verifyKey, sig, sigSz, (const byte *) msg,
                               strlen(msg));
        if (ret) {
            fprintf(stderr, "error: %zu: wc_XmssKey_Verify returned %d\n", i, ret);
            goto exit_xmss_example;
        }
    }

    printf("...done!\n");
    printf("finished\n");

exit_xmss_example:

    if (sig != NULL) {
        free(sig);
        sig = NULL;
    }

    if (read_buf != NULL) {
        free(read_buf);
        read_buf = NULL;
    }

    wc_XmssKey_Free(&signingKey);
    wc_XmssKey_Free(&verifyKey);

    return ret;
}


#else /* if !defined WOLFSSL_XMSS_VERIFY_ONLY */
static int read_file(byte * data, word32 len, const char * filename);
static int do_xmss_example(const char * params, const char * pubfile,
                           const char * sigfile, const char * msgfile);

int
main(int    argc,
     char * argv[])
{
    const char * params = NULL;
    const char * pubfile = NULL;
    const char * sigfile = NULL;
    const char * msgfile = NULL;
    int          ret = 0;

    if (argc != 5) {
        print_usage();
    }

    params = argv[1];
    pubfile = argv[2];
    sigfile = argv[3];
    msgfile = argv[4];

    ret = do_xmss_example(params, pubfile, sigfile, msgfile);

    return ret;
}

static void
print_usage(void)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "  ./xmss_example_verifyonly <param string> <pub file> <sig file> <msg file>\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "For simplicity message is assumed to be 32 bytes in size.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "examples:\n");
    fprintf(stderr, "  ./xmss_example_verifyonly XMSSMT-SHA2_20/4_256 xmss_pub.key xmss_sig.bin msg.bin\n");
    fprintf(stderr, "  ./xmss_example_verifyonly XMSSMT-SHA2_60/6_256 xmss_pub.key xmss_sig.bin msg.bin\n");
    fprintf(stderr, "  ./xmss_example_verifyonly XMSS-SHA2_10_256 xmss_pub.key xmss_sig.bin msg.bin\n");

    exit(EXIT_FAILURE);
}


static int
do_xmss_example(const char * params,
                const char * pubfile,
                const char * sigfile,
                const char * msgfile)
{
    int     ret = 0;
    XmssKey verifyKey;
    byte *  sig = NULL;
    word32  sigSz = 0;
    word32  pubSz = 0;
    byte    pub[XMSS_SHA256_PUBLEN];
    byte    msg[32];

    printf("using parameters: %s\n", params);

    ret = wc_XmssKey_Init(&verifyKey, NULL, 0);
    if (ret) {
        fprintf(stderr, "error: wc_XmssKey_Init returned %d\n", ret);
        goto exit_xmss_example;
    }

    ret = wc_XmssKey_SetParamStr(&verifyKey, params);
    if (ret) {
        fprintf(stderr, "error: wc_XmssKey_SetParameters(%s) returned %d\n",
                params, ret);
        goto exit_xmss_example;
    }

    ret = wc_XmssKey_GetSigLen(&verifyKey, &sigSz);
    if (ret || sigSz == 0) {
        fprintf(stderr, "error: wc_XmssKey_GetSigLen returned: %d, %d\n",
                ret, sigSz);
        goto exit_xmss_example;
    }

    ret = wc_XmssKey_GetPubLen(&verifyKey, &pubSz);
    if (ret || pubSz == 0) {
        fprintf(stderr, "error: wc_XmssKey_GetPubLen returned: %d, %d\n",
                ret, pubSz);
        goto exit_xmss_example;
    }

    ret = read_file(msg, sizeof(msg), msgfile);
    if (ret) {
        fprintf(stderr, "error: read_file(%s) failed\n", msgfile);
        goto exit_xmss_example;
    }

    sig = malloc(sigSz);
    if (sig == NULL) {
        fprintf(stderr, "error: malloc(%d) failed\n", sigSz);
        goto exit_xmss_example;
    }

    ret = read_file(sig, sigSz, sigfile) ;
    if (ret) {
        fprintf(stderr, "error: read_file(%s) failed\n", sigfile);
        goto exit_xmss_example;
    }

    ret = read_file(pub, XMSS_SHA256_PUBLEN, pubfile);
    if (ret) {
        fprintf(stderr, "error: read_file(%s) failed\n", pubfile);
        goto exit_xmss_example;
    }

    ret = wc_XmssKey_ImportPubRaw(&verifyKey, pub, sizeof(pub));
    if (ret != 0) {
        /* Something is wrong with the pub key or LMS parameters. */
        fprintf(stderr, "error: wc_XmssKey_ImportPubRaw" \
                        " returned %d\n", ret);
        goto exit_xmss_example;
    }

    dump_hex("pub: ", pub, sizeof(pub));
    dump_hex("msg: ", msg, sizeof(msg));
    printf("signature length: %d\n", sigSz);
    printf("pub key length: %d\n", pubSz);

    ret = wc_XmssKey_Verify(&verifyKey, sig, sigSz, (const byte *) msg,
                            sizeof(msg));
    if (ret) {
        fprintf(stderr, "error: wc_XmssKey_Verify returned %d\n", ret);
        goto exit_xmss_example;
    }

    printf("Verify good!\n");
    printf("finished\n");

exit_xmss_example:

    if (sig != NULL) {
        free(sig);
        sig = NULL;
    }

    wc_XmssKey_Free(&verifyKey);

    return ret;
}

static int
read_file(byte *       data,
          word32       len,
          const char * filename)
{
    FILE *       file = NULL;
    size_t       n_read = 0;

    if (data == NULL || filename == NULL || len == 0) {
        fprintf(stderr, "error: invalid read_file args\n");
        return -1;
    }

    file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "error: fopen(%s, \"rb\") failed\n", filename);
        return -1;
    }

    n_read = fread(data, 1, len, file);

    if (n_read != len) {
        fprintf(stderr, "error: read %zu, expected %d: %d\n", n_read, len,
                ferror(file));
        return -1;
    }

    fclose(file);

    return 0;
}

#endif /* if !defined WOLFSSL_XMSS_VERIFY_ONLY */

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
    printf("This requires the --with-libxmss flag.\n");
    return 0;
}
#endif /* WITH_LIBXMSS */

