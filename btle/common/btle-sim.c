/* btle-sim.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
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

/* This is a BTLE simulator to demonstrate communications between devices.
 * The simulator uses IPC (pipes) to communicate between threads.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "btle-sim.h"

//#define DEBUG_BTLE_IO
#define BTLE_VER 1

typedef struct {
    unsigned char  ver;
    unsigned char  type;
    unsigned short len;
} __attribute__ ((packed)) BtleHeader_t;

typedef struct {
    int pos;
    int total;
    BtleHeader_t header;
} BtlePkt_t;

typedef struct {
    int role;
    int fdmiso;
    int fdmosi;
    BtlePkt_t recv;
    BtlePkt_t send;
} BtleDev_t;

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

static int btle_send_block(BtleDev_t* dev, const unsigned char* buf,int len)
{
    int ret;
    int fd = btle_get_write(dev);
    ret = write(fd, buf, len);
#ifdef DEBUG_BTLE_IO
    printf("Write: %d\n", ret);
#endif
    (void)dev;
    return ret;
}

static int btle_recv_block(BtleDev_t* dev, unsigned char* buf, int len,
    int non_block)
{
    fd_set set;
    int ret, pos = 0;
    int fd = btle_get_read(dev);

    FD_ZERO(&set);
    FD_SET(fd, &set);

    while (pos < len) {
        ret = select(fd+1, &set, NULL, NULL, NULL);
        if (ret == 0) {
            if (non_block)
                return 0;
            continue;
        }
        if (ret < 0)
            return ret;

        if (FD_ISSET(fd, &set)) {
            ret = read(fd, &buf[pos], len - pos);
        #ifdef DEBUG_BTLE_IO
            printf("Read: %d\n", ret);
        #endif
            if (ret > 0) {
                pos += ret;
            }
            else {
                if (errno == EWOULDBLOCK) {
                    if (non_block)
                        return 0;
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

    memset(&gBtleDev, 0, sizeof(gBtleDev));
    gBtleDev.role = role;
    gBtleDev.fdmiso = fdmiso;
    gBtleDev.fdmosi = fdmosi;

    if (dev) {
        *dev = &gBtleDev;
    }

    return 0;
}

int btle_send(const unsigned char* buf, int len, int type, void* context)
{
    int ret;
    BtleDev_t* dev = (BtleDev_t*)context;
    if (dev == NULL)
        return -1;

    memset(&dev->send.header, 0, sizeof(dev->send.header));
    dev->send.header.ver = BTLE_VER;
    dev->send.header.type = type;
    dev->send.header.len = len;
    ret = btle_send_block(dev, (unsigned char*)&dev->send.header,
        sizeof(dev->send.header));
    if (ret > 0) {
        ret = btle_send_block(dev, buf, len);
    }
    return ret;
}

int btle_recv_ex(unsigned char* buf, int len, int* type, void* context,
    int non_block)
{
    int ret;
    BtleDev_t* dev = (BtleDev_t*)context;
    if (dev == NULL)
        return -1;

    if (dev->recv.total == 0) {
        /* read header */
        memset(&dev->recv.header, 0, sizeof(dev->recv.header));
        ret = btle_recv_block(dev, (unsigned char*)&dev->recv.header,
            sizeof(dev->recv.header), non_block);
        if (ret <= 0)
            return ret;

        if (dev->recv.header.ver != BTLE_VER)
            return -1;

        dev->recv.total = dev->recv.header.len;
        dev->recv.pos = 0;
    }

    /* read data */
    if (len > dev->recv.total - dev->recv.pos) {
        len = dev->recv.total - dev->recv.pos;
    }
    ret = btle_recv_block(dev, buf, len, non_block);
    if (ret > 0) {
        dev->recv.pos += ret;
    }

    if (type) {
        *type = dev->recv.header.type;
    }
    if (ret < 0 || dev->recv.pos >= dev->recv.total) {
        /* reset recv state */
        memset(&dev->recv.header, 0, sizeof(dev->recv.header));
        dev->recv.total = 0;
        dev->recv.pos = 0;
    }

    return ret;
}

int btle_recv(unsigned char* buf, int len, int* type, void* context)
{
    return btle_recv_ex(buf, len, type, context, 0);
}

void btle_close(void* context)
{
    BtleDev_t* dev = (BtleDev_t*)context;
    if (dev == NULL)
        return;

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
