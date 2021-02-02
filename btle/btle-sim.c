/* btle-sim.c
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


#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/logging.h>
#include "btle-sim.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

//#define BTLE_DEBUG_IO
#define BTLE_VER 1

typedef struct {
    int role;
    int fdmiso;
    int fdmosi;
} BtleDev_t;

typedef struct {
    unsigned char  ver;
    unsigned char  type;
    unsigned short len;
} __attribute__ ((packed)) BtleHeader_t;

static BtleDev_t gBtleDev;
static const char* kBtleMisoFifo = "/tmp/btleMiso";
static const char* kBtleMosiFifo = "/tmp/btleMosi";


static int btle_get_write(BtleDev_t* dev)
{
    return (dev->role == BTLE_ROLE_SERVER) ? dev->fdmosi : dev->fdmiso;
}

static int btle_get_read(BtleDev_t* dev)
{
    return (dev->role == BTLE_ROLE_SERVER) ? dev->fdmiso : dev->fdmosi;
}

static int btle_send_block(BtleDev_t* dev, const unsigned char* buf, int len, int fd)
{
    int ret;
    ret = write(fd, buf, len);
#ifdef BTLE_DEBUG_IO
    printf("Write: %d\n", ret);
    WOLFSSL_BUFFER(buf, len);
#endif
    (void)dev;
    return ret;
}

static int btle_recv_block(BtleDev_t* dev, unsigned char* buf, int len, int fd)
{
    fd_set set;
    int ret, pos = 0;

    FD_ZERO(&set);
    FD_SET(fd, &set);

    while (pos < len) {
        ret = select(fd+1, &set, NULL, NULL, NULL);
        if (ret == 0)
            continue;
        if (ret < 0)
            return ret;

        if (FD_ISSET(fd, &set)) {
            ret = read(fd, &buf[pos], len - pos);
        #ifdef BTLE_DEBUG_IO
            printf("Read: %d\n", ret);
            WOLFSSL_BUFFER(&buf[pos], len-pos);
        #endif
            if (ret > 0) {
                pos += ret;
            }
            else {
                if (errno == EWOULDBLOCK) {
                    continue;
                }
                else {
                    return ret;
                }
            }
        }
    }
    (void)dev;

    return pos;
}

int btle_open(void** dev, int role)
{
    int fdmiso, fdmosi;

    mkfifo(kBtleMisoFifo, 0666);
    mkfifo(kBtleMosiFifo, 0666);

    if (role == BTLE_ROLE_SERVER) {
        fdmiso = open(kBtleMisoFifo, O_RDONLY | O_NONBLOCK);
        fdmosi = open(kBtleMosiFifo, O_WRONLY);
    }
    else {
        fdmosi = open(kBtleMosiFifo, O_RDONLY | O_NONBLOCK);
        fdmiso = open(kBtleMisoFifo, O_WRONLY);
    }

    if (fdmiso < 0) {
        printf("Open %s failed! %d\n", kBtleMisoFifo, errno);
        return -1;
    }
    if (fdmosi < 0) {
        printf("Open %s failed! %d\n", kBtleMosiFifo, errno);
        close(fdmiso);
        return -1;
    }

    gBtleDev.role = role;
    gBtleDev.fdmiso = fdmiso;
    gBtleDev.fdmosi = fdmosi;

    if (dev)
        *dev = &gBtleDev;

    return 0;
}

int btle_send(const unsigned char* buf, int len, int type, void* context)
{
    BtleDev_t* dev = (BtleDev_t*)context;
    BtleHeader_t header;
    int fd = btle_get_write(dev);
    int ret;
    header.ver = BTLE_VER;
    header.type = type;
    header.len = len;
    ret = btle_send_block(dev, (unsigned char*)&header, sizeof(header), fd);
    if (ret < 0)
        return ret;
    ret = btle_send_block(dev, buf, len, fd);
    if (ret < 0)
        return ret;
    return len;
}

int btle_recv(unsigned char* buf, int len, int* type, void* context)
{
    BtleDev_t* dev = (BtleDev_t*)context;
    BtleHeader_t header;
    int ret;
    int fd = btle_get_read(dev);


    ret = btle_recv_block(dev, (unsigned char*)&header, sizeof(header), fd);
    if (ret < 0)
        return ret;

    if (header.ver != BTLE_VER)
        return -1;

    if (type)
        *type = header.type;

    if (len > 0) {
        ret = header.len;
        if (ret > len)
            ret = len;

        ret = btle_recv_block(dev, buf, ret, fd);
        if (ret < 0)
            return ret;
    }

    return header.len;
}

void btle_close(void* context)
{
    BtleDev_t* dev = (BtleDev_t*)context;

    close(dev->fdmiso);
    close(dev->fdmosi);

    unlink(kBtleMisoFifo);
    unlink(kBtleMosiFifo);
}

int btle_msg_pad(unsigned char* buf, int* len, void* context)
{
    BtleDev_t* dev = (BtleDev_t*)context;
    int newLen = *len;
    int odd = (newLen % BTLE_BLOCK_SIZE);
    if (odd != 0) {
        int addLen = (BTLE_BLOCK_SIZE - odd);
        newLen += addLen;
        if (newLen > BTLE_MSG_MAX_SIZE)
            return -1;

        memset(&buf[*len], 0, addLen);
    }
    *len = newLen;
    (void)dev;
    return 0;
}
