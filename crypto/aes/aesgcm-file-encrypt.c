/* aesgcm-file-encrypt.c
 *
 * Copyright (C) 2006-2023 wolfSSL Inc.
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

/* aesgcm-file-encrypt */

#ifndef WOLFSSL_USER_SETTINGS
    #include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/types.h>

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef WOLFSSL_AESGCM_STREAM

#ifdef OPENSSL_EXTRA
    #include <wolfssl/ssl.h>
#endif
#ifndef WOLFCRYPT_MAGIC
    #define WOLFCRYPT_MAGIC "WOLFSSL"
#endif
/*
MIN_BUFFER_SIZE of less than
*/
#ifndef MIN_BUFFER_SIZE
    #define MIN_BUFFER_SIZE 1024
#endif

/*
For optimum performance, MAX_BUFFER_SIZE should be >= page_size and less than
system memory in order to prevent memory fragmentation,better work with MMU
memory alignments, leverage cache speedup, and minimize OS overhead.
*/
#ifndef MAX_BUFFER_SIZE
    /* Use upto 1 GByte of RAM */
    #define MAX_BUFFER_SIZE (1 << 30)
#endif

#ifndef AES_KEY_SIZE
    /* Use 32 Bytes for the key size */
    #define AES_KEY_SIZE AES_256_KEY_SIZE
#endif

#ifndef AESGCM_TAG_SIZE
    /* Use 16 for the authentication size */
    #define AESGCM_TAG_SIZE AES_BLOCK_SIZE
#endif

static size_t get_block_sz(int fd)
{
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }
    return st.st_blksize;
}
static size_t get_file_sz(int fd)
{
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }
    return st.st_size;
}
static size_t get_memory_sz()
{
    long page_size = 0;

    page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("sysconf");
        exit(EXIT_FAILURE);
    }
    return page_size * ((size_t)sysconf(_SC_PHYS_PAGES));
}

static size_t get_optimal_buffer_sz(int fd, size_t memory_size)
{
    size_t block_size;
    size_t file_size;
    size_t optimal_size;

    block_size = get_block_sz(fd);
    optimal_size = block_size;
    file_size = get_file_sz(fd);

    if (optimal_size > MAX_BUFFER_SIZE) {
        optimal_size = MAX_BUFFER_SIZE;
    }

    while (optimal_size * 2 <= memory_size &&
           optimal_size * 2 <= MAX_BUFFER_SIZE &&
           optimal_size <= file_size) {
        optimal_size *= 2;
    }
    return optimal_size;
}

static int check_file_permission(const char *fileName, uid_t owner, gid_t group)
{

    uid_t uid;
    gid_t gid;
    mode_t mode;
    struct stat st;

    if (stat(fileName, &st) == -1) {
       perror("stat Error: could not get file status");
       exit(EXIT_FAILURE);
    }

    mode = st.st_mode;
    uid = st.st_uid;
    gid = st.st_gid;

    if ((uid != owner || !(mode & S_IRUSR))
        && (gid != group || !(mode & S_IRGRP))
        && !(mode & S_IROTH)) {
        /* The owner and group don't have read access
         and the file is not readable by others */
        return -1;
    }
    else {
        /* The new owner and group have read access to the file */
        return 0;
    }
}
/*!
    \ingroup AES
    \brief This function encrypts the input file containing plain text
     and stores the resulting cipher text in the output file.
     A magic label, the computed authentication tag (TAG) and the supplied
     IV will be written in the beginning for the cipher file.

    \return 0 on successfully encrypting the file

    \param in_file filename with the plain text
    \param out_file file name to hold the cipher text
    \param key_str key must be 32 Bytes
    \param iv_str IV length must be 16 Bytes
*/
int encrypt_file_AesGCM(const char *in_file, const char *out_file,
                        const char *key_str, const char *iv_str)
{
    byte* in_buf;
    byte* out_buf;
    int in_fd;
    int out_fd;
    int read_size;
    int ret = 0;
    size_t buffer_size;
    size_t memory_size;
    byte iv[AES_IV_SIZE];
    byte key[AES_KEY_SIZE];
    byte tag_enc[AESGCM_TAG_SIZE];
    Aes gcm;

    if (!in_file || !out_file || !key_str || !iv_str) {
        return BAD_FUNC_ARG;
    }

    if (strlen(key_str) < AES_KEY_SIZE || strlen(iv_str) < AES_IV_SIZE) {
        return BAD_LENGTH_E;
    }

    if (check_file_permission(in_file, getuid(), getgid()) == -1) {
        return -1;
    }

    in_fd  = open(in_file, O_RDONLY);
    if (in_fd == -1) {
        perror("open");
        return -1;
    }

    out_fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (out_fd == -1) {
        perror("open");
        close(in_fd);
        return -1;
    }
    memory_size = get_memory_sz();
    buffer_size = get_optimal_buffer_sz(in_fd, memory_size);

    if (buffer_size < MIN_BUFFER_SIZE) {
        buffer_size = MIN_BUFFER_SIZE;
    }

    in_buf = malloc(buffer_size);
    if (in_buf == NULL) {
        perror("malloc");
        close(in_fd);
        close(out_fd);
        exit(EXIT_FAILURE);
    }
    out_buf = malloc(buffer_size);
    if (out_buf == NULL) {
        perror("malloc");
        close(in_fd);
        close(out_fd);
        free(in_buf);
        exit(EXIT_FAILURE);
    }

    memset(&gcm, 0, sizeof(Aes));
    memset(iv, 0, AES_IV_SIZE);
    memset(key, 0, AES_KEY_SIZE);
    memset(tag_enc, 0, AESGCM_TAG_SIZE);
    strncpy((char *)iv, iv_str, AES_IV_SIZE);
    strncpy((char *)key, key_str, AES_KEY_SIZE);

    ret = wc_AesGcmEncryptInit(&gcm, key, AES_KEY_SIZE, iv, AES_IV_SIZE);
    if (ret == 0) {
        /* Write magic label in the beginning of the cipher file */
        if (write(out_fd, WOLFCRYPT_MAGIC,
                  strlen(WOLFCRYPT_MAGIC)) != strlen(WOLFCRYPT_MAGIC)) {
            perror("write");
            ret = -1;
            goto exit;
        }
        /* Add a placeholder TAG and write IV in the cipher file */
        if (write(out_fd, tag_enc, AESGCM_TAG_SIZE) != AESGCM_TAG_SIZE) {
            perror("write");
            ret = -1;
            goto exit;
        }
        if (write(out_fd, iv, AES_IV_SIZE) != AES_IV_SIZE) {
            perror("write");
            ret = -1;
            goto exit;
        }
    }

    while (ret == 0) {
         read_size = read(in_fd, in_buf, buffer_size);
         if (read_size <= 0)
             break;

         ret = wc_AesGcmEncryptUpdate(&gcm, out_buf, in_buf, read_size, NULL, 0);
         if (ret == 0) {
             if (write(out_fd, out_buf, read_size) != read_size) {
                 perror("write");
                 ret = -1;
                 goto exit;
             }
         }
     }

    if (ret == 0) {
        ret = wc_AesGcmEncryptFinal(&gcm, tag_enc, AESGCM_TAG_SIZE);
        if (ret == 0) {
            /* move file pointer to beginning of file after the magic word */
            off_t offset = lseek(out_fd, strlen(WOLFCRYPT_MAGIC), SEEK_SET);
            if (offset == -1) {
                perror("lseek SEEK_SET");
                ret = -1;
                goto exit;
            }
            if (write(out_fd, tag_enc, AESGCM_TAG_SIZE) != AESGCM_TAG_SIZE) {
                 perror("write");
                 ret = -1;
                 goto exit;
            }
            if (lseek(out_fd, 0, SEEK_END) == -1) {
                perror("lseek SEEK_END");
                ret = -1;
                goto exit;
            }
        }
    }
    printf("File encryption with AES GCM complete.\n");
exit:
    free(in_buf);
    free(out_buf);
    close(in_fd);
    close(out_fd);


    return ret;
}

/*!
    \ingroup AES
    \brief This function decrypts the input file containing cipher text
     and stores the resulting plain text in the output file.
     The function expects a magic label, authentication tag (TAG) and
     the IV in the beginning for the cipher file.

    \return 0 on successfully decrypting the file
    \return negative number on error

    \param in_file filename with the cipher text
    \param out_file file name to hold plain text
    \param key_str key must be 32 Bytes
*/
int decrypt_file_AesGCM(const char *in_file, const char *out_file,
                        const char *key_str)
{
    byte* in_buf;
    byte* out_buf;
    int in_fd;
    int out_fd;
    int read_size;
    int ret = 0;
    size_t buffer_size;
    size_t memory_size;
    byte iv[AES_IV_SIZE];
    byte wolf_magic[strlen(WOLFCRYPT_MAGIC)];
    byte key[AES_KEY_SIZE];
    byte tag_dec[AESGCM_TAG_SIZE];
    byte tag_enc[AESGCM_TAG_SIZE];
    Aes gcm;

    if (!in_file || !out_file || !key_str) {
        return BAD_FUNC_ARG;
    }

    if (strlen(key_str) < AES_KEY_SIZE) {
        return BAD_LENGTH_E;
    }

    if (check_file_permission(in_file, getuid(), getgid()) == -1) {
        return -1;
    }

    in_fd  = open(in_file, O_RDONLY);

    if (in_fd == -1) {
        perror("open");
        return -1;
    }

    out_fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (out_fd == -1) {
        perror("open");
        close(in_fd);
        return -1;
    }
    memory_size = get_memory_sz();
    buffer_size = get_optimal_buffer_sz(in_fd, memory_size);

    if (buffer_size < MIN_BUFFER_SIZE) {
        buffer_size = MIN_BUFFER_SIZE;
    }

    in_buf = malloc(buffer_size);
    if (in_buf == NULL) {
        perror("malloc");
        close(in_fd);
        close(out_fd);
        exit(EXIT_FAILURE);
    }
    out_buf = malloc(buffer_size);
    if (out_buf == NULL) {
        perror("malloc");
        close(in_fd);
        close(out_fd);
        free(in_buf);
        exit(EXIT_FAILURE);
    }

    memset(&gcm, 0, sizeof(Aes));
    memset(iv, 0, AES_IV_SIZE);
    memset(key, 0, AES_KEY_SIZE);
    memset(tag_dec, 0, AESGCM_TAG_SIZE);
    memset(tag_enc, 0, AESGCM_TAG_SIZE);
    strncpy((char *)key, key_str, AES_KEY_SIZE);

    /* Extract a WOLFCRYPT MAGIC | TAG | IV  from the cipher file */
    if (read(in_fd, wolf_magic,
        strlen(WOLFCRYPT_MAGIC)) != strlen(WOLFCRYPT_MAGIC)) {
        perror("write");
        ret = -1;
        goto exit;
    }
    if (memcmp(wolf_magic, WOLFCRYPT_MAGIC, strlen(WOLFCRYPT_MAGIC)) != 0) {
        perror("WOLFCRYPT_MAGIC didn't match\n");
        ret = AES_GCM_AUTH_E;
        goto exit;
    }
    read_size = read(in_fd, tag_enc, AESGCM_TAG_SIZE);
    if (read_size != AESGCM_TAG_SIZE) {
        perror("read");
        ret = -1;
        goto exit;
    }
    read_size = read(in_fd, iv, AES_IV_SIZE);
    if (read_size != AES_IV_SIZE) {
        perror("read");
        ret = -1;
        goto exit;
    }

    ret = wc_AesGcmDecryptInit(&gcm, key, AES_KEY_SIZE, iv, AES_IV_SIZE);

    while (ret == 0) {
         read_size = read(in_fd, in_buf, buffer_size);
         if (read_size <= 0)
             break;

         ret = wc_AesGcmDecryptUpdate(&gcm, out_buf, in_buf, read_size, NULL, 0);
         if (ret == 0) {
             if (write(out_fd, out_buf, read_size) != read_size) {
                 perror("write");
                 ret = -1;
                 goto exit;
             }
         }
     }

    if (ret == 0) {
        ret = wc_AesGcmEncryptFinal(&gcm, tag_dec, AESGCM_TAG_SIZE);
        if (ret == 0 && (memcmp(tag_enc, tag_dec, AESGCM_TAG_SIZE) != 0)) {
            perror("TAG didn't match\n");
            ret = AES_GCM_AUTH_E;
            goto exit;
        }
    }
exit:
    free(in_buf);
    free(out_buf);
    close(in_fd);
    close(out_fd);

    printf("File decryption with AES GCM complete.\n");
    return ret;
}

#ifdef OPENSSL_EXTRA
what
int encrypt_file(const char *in_file, const char *out_file,
                 const char *key_str, const char *iv_str)
{
    int in_fd;
    int in_len;
    int out_fd;
    int out_len;
    int ret = WOLFSSL_SUCCESS;
    byte in_buf[AES_BLOCK_SIZE];
    byte iv[AES_IV_SIZE];
    byte key[AES_KEY_SIZE];
    byte out_buf[AES_BLOCK_SIZE];
    byte tag_enc[AESGCM_TAG_SIZE];
    EVP_CIPHER_CTX *ctx;

    if (!in_file || !out_file || !key_str || !iv_str) {
        return BAD_FUNC_ARG;
    }

    if (strlen(key_str) < AES_KEY_SIZE || strlen(iv_str) < AES_IV_SIZE) {
        return BAD_LENGTH_E;
    }

    if (check_file_permission(in_file, getuid(), getgid()) == -1) {
        return -1;
    }

    in_fd  = open(in_file, O_RDONLY);
    if (in_fd == -1) {
        perror("open");
        return -1;
    }
    out_fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd == -1) {
        perror("open");
        close(in_fd);
        return -1;
    }

    memset(iv, 0, AES_IV_SIZE);
    memset(key, 0, AES_KEY_SIZE);
    memset(tag_enc, 0, AESGCM_TAG_SIZE);
    strncpy((char *)iv, iv_str, AES_IV_SIZE);
    strncpy((char *)key, key_str, AES_KEY_SIZE);

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        perror("EVP_CIPHER_CTX_new");
        goto exit;
    }
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv) !=
        WOLFSSL_SUCCESS) {
        perror("EVP_EncryptInit_ex");
        goto exit;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_IV_SIZE, NULL) !=
        WOLFSSL_SUCCESS) {
        perror("EVP_CIPHER_CTX_ctrl");
        goto exit;
    }

    /* Write file identifier in the beginning of the cipher file */
    if (write(out_fd, WOLFCRYPT_MAGIC,
            strlen(WOLFCRYPT_MAGIC)) != strlen(WOLFCRYPT_MAGIC)) {
        perror("write");
        ret = -1;
        goto exit;
    }
    /* Add a placeholder TAG and write IV in the cipher file */
    if (write(out_fd, tag_enc, AESGCM_TAG_SIZE) != AESGCM_TAG_SIZE) {
        perror("write");
        ret = -1;
        goto exit;
    }
    if (write(out_fd, iv, AES_IV_SIZE) != AES_IV_SIZE) {
        perror("write");
        ret = -1;
        goto exit;
    }
    while (1) {
        in_len = read(in_fd, in_buf, AES_BLOCK_SIZE);
        if (in_len <= 0)
            break;

        if (EVP_EncryptUpdate(ctx, out_buf, &out_len, in_buf, in_len) !=
            WOLFSSL_SUCCESS) {
            perror("EVP_EncryptUpdate");
            ret = -1;
            goto exit;
        }
        if (write(out_fd, out_buf, out_len) != out_len) {
            perror("write");
            ret = -1;
            goto exit;
        }
    }

    if (EVP_EncryptFinal_ex(ctx, out_buf, &out_len) != WOLFSSL_SUCCESS) {
        perror("EVP_EncryptFinal_ex");
        ret = -1;
        goto exit;
    }

    if (write(out_fd, out_buf, out_len) != out_len) {
        perror("write");
        ret = -1;
        goto exit;
    }

    ret = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_IV_SIZE, tag_enc);

    if (ret == WOLFSSL_SUCCESS ) {
        /* move file pointer to beginning of file after the magic word */
        off_t offset = lseek(out_fd, strlen(WOLFCRYPT_MAGIC), SEEK_SET);
        if (offset == -1) {
            perror("lseek SEEK_SET");
            ret = -1;
            goto exit;
        }
        if (write(out_fd, tag_enc, AESGCM_TAG_SIZE) != AESGCM_TAG_SIZE) {
            perror("write");
            ret = -1;
            goto exit;
        }
        if (lseek(out_fd, 0, SEEK_END) == -1) {
            perror("lseek SEEK_END");
            ret = -1;
            goto exit;
        }
    }
    printf("File encryption with EVP GCM complete.\n");
exit:
    EVP_CIPHER_CTX_free(ctx);
    close(in_fd);
    close(out_fd);
    return ret;
}


int decrypt_file(const char *in_file, const char *out_file, const char *key_str)
{
    int in_fd;
    int in_len;
    int out_fd;
    int out_len;
    int ret = WOLFSSL_SUCCESS;
    int read_size;
    byte in_buf[AES_BLOCK_SIZE];
    byte iv[AES_IV_SIZE];
    byte wolf_magic[strlen(WOLFCRYPT_MAGIC)];
    byte key[AES_KEY_SIZE];
    byte out_buf[AES_BLOCK_SIZE];
    byte tag_dec[AESGCM_TAG_SIZE];
    byte tag_enc[AESGCM_TAG_SIZE];
    EVP_CIPHER_CTX *ctx;

    if (!in_file || !out_file || !key_str) {
        return BAD_FUNC_ARG;
    }

    if (strlen(key_str) < AES_KEY_SIZE) {
        return BAD_LENGTH_E;
    }

    if (check_file_permission(in_file, getuid(), getgid()) == -1) {
        return -1;
    }

    in_fd  = open(in_file, O_RDONLY);

    if (in_fd == -1) {
        perror("open");
        return -1;
    }

    out_fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (out_fd == -1) {
        perror("open");
        close(in_fd);
        return -1;
    }

    memset(iv, 0, AES_IV_SIZE);
    memset(key, 0, AES_KEY_SIZE);
    memset(tag_enc, 0, AESGCM_TAG_SIZE);
    memset(tag_dec, 0, AESGCM_TAG_SIZE);
    strncpy((char *)key, key_str, AES_KEY_SIZE);

    /* Extract a WOLFCRYPT MAGIC | TAG | IV  from the cipher file */
    if (read(in_fd, wolf_magic,
        strlen(WOLFCRYPT_MAGIC)) != strlen(WOLFCRYPT_MAGIC)) {
        perror("write");
        ret = -1;
        goto exit;
    }
    if (memcmp(wolf_magic, WOLFCRYPT_MAGIC, strlen(WOLFCRYPT_MAGIC)) != 0) {
        perror("WOLFCRYPT_MAGIC didn't match\n");
        ret = AES_GCM_AUTH_E;
        goto exit;
    }
    read_size = read(in_fd, tag_enc, AESGCM_TAG_SIZE);
    if (read_size != AESGCM_TAG_SIZE) {
        perror("read");
        ret = -1;
        goto exit;
    }
    read_size = read(in_fd, iv, AES_IV_SIZE);
    if (read_size != AES_IV_SIZE) {
        perror("read");
        ret = -1;
        goto exit;
    }

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        perror("EVP_CIPHER_CTX_new");
        goto exit;
    }
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv) !=
        WOLFSSL_SUCCESS) {
        perror("EVP_DecryptInit_ex");
        goto exit;
    }
    while (1) {
        in_len = read(in_fd, in_buf, AES_BLOCK_SIZE);
        if (in_len <= 0)
            break;

        if (EVP_DecryptUpdate(ctx, out_buf, &out_len, in_buf, in_len) !=
                WOLFSSL_SUCCESS) {
            perror("EVP_DecryptInit_ex");
            goto exit;
        }
        if (write(out_fd, out_buf, out_len) != out_len) {
            perror("write");
            ret = -1;
            goto exit;
        }
    }

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, AES_IV_SIZE, tag_enc)
            != WOLFSSL_SUCCESS) {
        perror("EVP_CIPHER_CTX_ctrl");
        goto exit;
    }
    if (EVP_DecryptFinal_ex(ctx, out_buf, &out_len) != WOLFSSL_SUCCESS) {
        perror("EVP_DecryptInit_ex");
        goto exit;
    }
    if (write(out_fd, out_buf, out_len) != out_len) {
        perror("write");
        ret = -1;
        goto exit;
    }

    if (ret == WOLFSSL_SUCCESS) {
        ret = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG,
                                  AES_IV_SIZE, tag_dec);
        if (ret == WOLFSSL_SUCCESS &&
            (memcmp(tag_enc, tag_dec, AESGCM_TAG_SIZE) != 0)) {
            perror("TAG didn't match\n");
            exit(EXIT_FAILURE);
        }
    }
    printf("File decryption with EVP GCM complete.\n");

exit:
    EVP_CIPHER_CTX_free(ctx);
    close(in_fd);
    close(out_fd);
    return ret;
}
#endif

#if defined(__linux__)

int sanityTest_default(int file_sz) {

    const char *cmd_enc ="./aesgcm-file-encrypt -e 256 -m 1 \
                   -k 77CF00EC060192530B5D06B6B426799B \
                   -v 77CF00EC060192530B5D06B6B426799B \
                   -i text.bin -o text2cipher.bin";
    const char *cmd_dec ="./aesgcm-file-encrypt -d 256 -m 1 \
                   -k 77CF00EC060192530B5D06B6B426799B \
                   -i text2cipher.bin -o text2cipher2text.bin";
    const char *cmd_diff = "diff -q text.bin text2cipher2text.bin";
    char buffer[1024];

    sprintf(buffer,"dd if=/dev/urandom bs=1024 count=%d | head -c %d > \
text.bin", (file_sz/1024)+1, file_sz);

    if (system(buffer) != 0 || system(cmd_enc) != 0 || system(cmd_dec) != 0 ) {
        perror("system command");
        return 1;
    }

    FILE* pipe = popen(cmd_diff, "r");
    if (!pipe) {
        perror("system command");
        return -1;
    }

    if (fgets(buffer, sizeof(buffer), pipe)) {
        printf("Error: The files are different.\n");
        return -1;
    }
    else {
        printf("Pass: The files are identical.\n");
    }
    pclose(pipe);

#ifdef OPENSSL_EXTRA
    const char *cmd_enc_evp ="./aesgcm-file-encrypt -e 256 -m 1 \
                              -k 77CF00EC060192530B5D06B6B426799B \
                              -v 77CF00EC060192530B5D06B6B426799B \
                              -i text.bin -o text2cipher.evp.bin";
    const char *cmd_dec_evp ="./aesgcm-file-encrypt -d 256 -m 1 \
                            -k 77CF00EC060192530B5D06B6B426799B \
                            -i text2cipher.evp.bin -o text2cipher2text.evp.bin";
    const char *cmd_diff_evp = "diff -q text.bin text2cipher2text.evp.bin";
    const char *cmd_diff_gcm_evp = "diff -q text2cipher2text.bin \
                                    text2cipher2text.evp.bin";

    if (system(cmd_enc_evp) != 0 || system(cmd_dec_evp) != 0 ) {
        perror("system command");
        return -1;
    }

    pipe = popen(cmd_diff_evp, "r");
    if (!pipe) {
        perror("system command");
        return -1;
    }
    if (fgets(buffer, sizeof(buffer), pipe)) {
        printf("Error: The files are different.\n");
    }
    else {
        printf("Pass: The files are identical.\n");
    }

    pclose(pipe);

    pipe = popen(cmd_diff_gcm_evp, "r");
    if (!pipe) {
        perror("Error opening pipe to command");
        return -1;
    }
    if (fgets(buffer, sizeof(buffer), pipe)) {
        printf("Error: The EVP and GCM files are different.\n");
    }
    else {
        printf("Pass: The EVP and GCM files are identical.\n");
    }

    pclose(pipe);
#endif

    return 0;
}

#endif /* __linux__ */

void usage(char *prog_name) {
    fprintf(stderr, "Usage: %s [-e | -d] [-m] [-i input filename] \
            [-o output filename]\n", prog_name);
    exit(EXIT_FAILURE);
}

void help(char *prog_name) {
    printf("This program accepts several switches:\n");
    printf("  -e <num>   encryption. 256, 192, 128 \n");
    printf("  -d <num>   decryption. 256, 192, 128\n");
    printf("  -m <num>   method to use.  GCM(1), EVP GCM (2), \n");
    printf("  -i <file>  Set the input filename to 'file'\n");
    printf("  -o <file>  Set the output filename to 'file'\n");
#if defined(__linux__)
    printf("  -t <num>   Sanity test with the given file size in Bytes. The \
test will create three files:text.bin, cipher, decrypted plain. \n");
    printf("   Example, ./aesgcm-file-encrypt -t 256");
#endif
    exit(EXIT_SUCCESS);
}


int main(int argc, char** argv)
{
    const char *inFile = NULL;
    const char *ivStr = NULL;
    const char *keyStr = NULL;
    const char *outFile = NULL;
    int    file_sz = 0;
    int    key_sz = 0;
    int    method = 0;
    int    option;    /* options of how to run the program */
    char   choice = 'n';

    while ((option = getopt(argc, argv, "e:d:i:o:m:t:k:v:h")) != -1 && choice != 't') {
        switch (option) {
            case 'e': /* encrypt */
                choice = 'e';
                key_sz = atoi(optarg);
                if (!(key_sz == 128 || key_sz == 192 || key_sz == 256 )) {
                    perror("Wrong key size: use 128, 192 or 256 \n");
                    usage(argv[0]);
                }
                break;
            case 'd': /* decrypt */
                key_sz = atoi(optarg);
                if (!(key_sz == 128 || key_sz == 192 || key_sz == 256 )) {
                    perror("Wrong key size: use 128, 192 or 256 \n");
                    usage(argv[0]);
                }
                choice = 'd';
                break;
            case 'i': /* input file */
                inFile = optarg;
                break;
            case 'o': /* output file */
                outFile = optarg;
                break;
            case 'm': /* options to do enc/dec */
                method = atoi(optarg);
                if (method < 1 || method > 2) {
                    perror("Wrong AES choice: use EVP (1), GCM(2)\n");
                    usage(argv[0]);
                }
                break;
            case 'k': /* key */
                keyStr = optarg;
                if (strlen(keyStr) < key_sz/8) {
                    perror("Wrong key string size\n");
                    usage(argv[0]);
                }
                break;
            case 'v': /* IV */
                ivStr = optarg;
                if (strlen(ivStr) < 12) {
                    perror("Wrong IV length\n");
                    usage(argv[0]);
                }
                break;
            case 't': /* sanity test */
                choice = 't';
                file_sz = atoi(optarg);
                break;
            case 'h': /* Get help */
            case '?':
                help(argv[0]);
                break;
            default:
                usage(argv[0]);
        }
    }
#if defined(__linux__)
    if (choice == 't') {
        if (sanityTest_default(file_sz) != 0) {
            perror("Error: sanityTest_default\n");
        }
        return 0;
    }
#endif
    if (inFile && outFile && choice != 'n') {

        switch (method) {
        case 1:
            if (choice == 'e') {
                if (encrypt_file_AesGCM(inFile, outFile, keyStr, ivStr) != 0) {
                    perror("Error: encrypt_file_AesGCM\n");
                }
            }
            else if (choice == 'd') {
                if (decrypt_file_AesGCM(inFile, outFile, keyStr) != 0) {
                    perror("Error: decrypt_file_AesGCM\n");
                }
                else
                    printf("Passed: decrypt_file_AesGCM\n");

            }
            break;
#ifdef OPENSSL_EXTRA
            case 2:
                if (choice == 'e') {
                    if (encrypt_file(inFile, outFile, keyStr, ivStr) !=
                        WOLFSSL_SUCCESS) {
                        perror("Error: encrypt_file \n");
                    }
                }
                else if (choice == 'd') {
                    if (decrypt_file(inFile, outFile, keyStr) !=
                        WOLFSSL_SUCCESS) {
                        perror("Error: decrypt_file\n");
                    }
                    else
                        printf("Passed: decrypt_file\n");
                }
                break;
#endif
            default:
                abort();
        }
    }
    return 0;
}

#else
int main()
{
    printf("This example requires AES GCM streaming. Build wolfSSL with "
        "./configure --enable-aesgcm-stream\n");
    return 0;
}
#endif
