/*
 * dtls-common.h
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
 *
 *=============================================================================
 *
 * Bare-bones example of a DTLS 1.3 client for instructional/learning purposes.
 */

#ifndef DTLS_COMMON_H_
#define DTLS_COMMON_H_

#define INVALID_SOCKET (-1)

#define SFD_TIMEOUT_E (-1)
#define SFD_SOCKET_E  (-2)
#define SFD_SELECT_E  (-3)

int wait_sfd(SOCKET_T socketfd, int to, int rx) {
    fd_set fds, errfds;
    fd_set* recvfds = NULL;
    fd_set* sendfds = NULL;
    SOCKET_T nfds = socketfd + 1;
    struct timeval timeout;
    int result;

    memset(&timeout, 0, sizeof(timeout));
    timeout.tv_sec = to;

    FD_ZERO(&fds);
    FD_SET(socketfd, &fds);
    FD_ZERO(&errfds);
    FD_SET(socketfd, &errfds);

    if (rx)
        recvfds = &fds;
    else
        sendfds = &fds;

    result = select(nfds, recvfds, sendfds, &errfds, &timeout);

    if (result == 0)
        return SFD_TIMEOUT_E;
    else if (result > 0) {
        if (FD_ISSET(socketfd, &fds)) {
            return 0;
        }
        else if(FD_ISSET(socketfd, &errfds))
            return SFD_SOCKET_E;
    }
    perror("select()");

    return SFD_SELECT_E;
}


#endif /* DTLS_COMMON_H_ */
