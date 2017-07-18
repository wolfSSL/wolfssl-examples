/* btle-sim.c
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.
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


#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    int fd;
} BtleDev_t;

static BtleDev_t gBtleDev;
static const char* myfifo = "/tmp/myfifo";

int btle_open(void** dev)
{
    int fd;

    mkfifo(myfifo, 0666);

    fd = open(myfifo, O_WRONLY);
    if (fd < 0) {
        unlink(myfifo);
        return -1;
    }
    gBtleDev.fd = fd;

    if (dev)
        *dev = &gBtleDev;

    return 0;
}

int btle_send(const unsigned char* buf, int len, void* context)
{
    BtleDev_t* dev = (BtleDev_t*)context;
    write(dev->fd, buf, len);

    return len;
}

int btle_recv(unsigned char* buf, int len, void* context)
{
    BtleDev_t* dev = (BtleDev_t*)context;
    return read(dev->fd, buf, len);
}

void btle_close(void* context)
{
    BtleDev_t* dev = (BtleDev_t*)context;

    close(dev->fd);
    unlink(myfifo);
}
