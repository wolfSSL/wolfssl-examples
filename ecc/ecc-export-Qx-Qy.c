/* ecc-export-Qx-Qy.c
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

/* This example exports the public part of a given ECC key as a concatenation of
 * (Qx,Qy), in raw format.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/asn.h>

#define MAX_BUF 1024
#define POINT_SIZE 32

#if defined(HAVE_ECC_KEY_IMPORT) && defined(HAVE_ECC_KEY_EXPORT)

int main(int argc, char *argv[])
{
    int fd_in, fd_out, sz, ret;
    uint8_t der_buf[MAX_BUF];
    uint8_t Qx[POINT_SIZE], Qy[POINT_SIZE];
    uint32_t qxlen = POINT_SIZE, qylen = POINT_SIZE;
    word32 idx = 0;
    ecc_key ec;
    uint32_t len;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s der_key_file raw_key_file\n", argv[0]);
        exit(1);
    }
    fd_in = open(argv[1], O_RDONLY);
    if (fd_in < 0) {
        perror("opening input file");
        exit(2);
    }
    sz = read(fd_in, der_buf, MAX_BUF);
    if (sz < 0) {
        perror("read");
        exit(3);
    }
    close(fd_in);
    wc_ecc_init(&ec);
    ret = wc_EccPublicKeyDecode(der_buf, &idx, &ec, sz);
    if (ret != MP_OKAY) {
        fprintf(stderr, "wc_EccPublicKeyDecode: Error %d\n", ret);
        exit(4);
    }
    ret = wc_ecc_export_public_raw(&ec, Qx, &qxlen, Qy, &qylen);
    if (ret != MP_OKAY) {
        fprintf(stderr, "wc_ecc_export_public_raw: Error %d\n", ret);
        exit(4);
    }
    fd_out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0660);
    if (fd_out < 0) {
        perror("opening output file");
        exit(5);
    }
    len = (uint32_t)write(fd_out, Qx, qxlen);
    if (len != qxlen) {
        perror("write Qx - short");
        exit(6);
    }
    len = write(fd_out, Qy, qylen);
    if (len != qylen) {
        perror("write Qy - short");
        exit(7);
    }
    close(fd_out);
    return 0;
}

#else

int main(void)
{
    printf("Not compiled in: Build wolfSSL with `./configure --enable-ecc or `HAVE_ECC`, `HAVE_ECC_KEY_EXPORT` and `HAVE_ECC_KEY_IMPORT`\n");
    return 0;
}
#endif
