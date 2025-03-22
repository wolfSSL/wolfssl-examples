/* tap_interface.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <netinet/in.h>

static int tap_fd = -1;

/* Initialize TAP interface */
int tap_init(const char *ifname, const char *ipaddr, const char *netmask)
{
    struct ifreq ifr;
    int fd, ret;
    
    /* Open TUN/TAP device */
    fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("Opening /dev/net/tun");
        return -1;
    }

    /* Set up interface request */
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    /* Create TAP interface */
    ret = ioctl(fd, TUNSETIFF, (void *)&ifr);
    if (ret < 0) {
        /* If the interface already exists, try to use it */
        if (errno == EBUSY) {
            printf("TAP interface %s already exists, trying to use it\n", ifname);
            close(fd);
            
            /* Reopen with the existing interface name */
            fd = open("/dev/net/tun", O_RDWR);
            if (fd < 0) {
                perror("Reopening /dev/net/tun");
                return -1;
            }
            
            /* Set up interface request again */
            memset(&ifr, 0, sizeof(ifr));
            ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
            strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
            
            /* Try to attach to the existing interface */
            ret = ioctl(fd, TUNSETIFF, (void *)&ifr);
            if (ret < 0) {
                perror("ioctl(TUNSETIFF) on existing interface");
                close(fd);
                return -1;
            }
        } else {
            perror("ioctl(TUNSETIFF)");
            close(fd);
            return -1;
        }
    }

    printf("Using TAP interface %s\n", ifname);
    printf("Expected FreeRTOS IP: %s/%s\n", ipaddr, netmask);
    printf("Host IP should be 10.10.0.1/24\n");

    tap_fd = fd;
    return fd;
}

/* Read from TAP interface */
int tap_read(void *buf, size_t len)
{
    if (tap_fd < 0) {
        return -1;
    }
    return read(tap_fd, buf, len);
}

/* Write to TAP interface */
int tap_write(const void *buf, size_t len)
{
    if (tap_fd < 0) {
        return -1;
    }
    return write(tap_fd, buf, len);
}

/* Close TAP interface */
void tap_close(void)
{
    if (tap_fd >= 0) {
        close(tap_fd);
        tap_fd = -1;
    }
}
