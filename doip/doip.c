/* doip.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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

#include "doip.h"

void doip_write_header(unsigned char* buf, unsigned short payloadType,
        unsigned int payloadLen)
{
    buf[0] = (unsigned char)DOIP_PROTOCOL_VERSION;
    buf[1] = (unsigned char)(~DOIP_PROTOCOL_VERSION & 0xFF);
    buf[2] = (unsigned char)((payloadType >> 8) & 0xFF);
    buf[3] = (unsigned char)(payloadType & 0xFF);
    buf[4] = (unsigned char)((payloadLen >> 24) & 0xFF);
    buf[5] = (unsigned char)((payloadLen >> 16) & 0xFF);
    buf[6] = (unsigned char)((payloadLen >> 8) & 0xFF);
    buf[7] = (unsigned char)(payloadLen & 0xFF);
}

int doip_parse_header(const unsigned char* buf, unsigned short* payloadType,
        unsigned int* payloadLen, unsigned char* nackCode)
{
    unsigned int len;

    /* The synchronisation pattern is the first thing to check: it catches a
     * stream that is not DoIP at all, or one that has lost sync, before any
     * length is trusted.
     *
     * Both halves matter. The inverse byte must invert the version byte, and
     * the version itself must be the one this build speaks -- an 0x01 or 0x03
     * peer against an 0x02 gateway has a perfectly formed pattern for a
     * revision whose payload types and lengths differ. The standard treats an
     * unsupported version as the same "incorrect pattern format" condition. */
    if (buf[1] != (unsigned char)(~buf[0] & 0xFF) ||
            buf[0] != DOIP_PROTOCOL_VERSION) {
        *nackCode = DOIP_NACK_INCORRECT_PATTERN;
        return -1;
    }

    len = ((unsigned int)buf[4] << 24) | ((unsigned int)buf[5] << 16) |
          ((unsigned int)buf[6] << 8)  |  (unsigned int)buf[7];

    if (len > DOIP_MAX_PAYLOAD) {
        *nackCode = DOIP_NACK_MESSAGE_TOO_LARGE;
        return -1;
    }

    *payloadType = (unsigned short)(((unsigned short)buf[2] << 8) | buf[3]);
    *payloadLen  = len;
    return 0;
}

const char* doip_payload_name(unsigned short payloadType)
{
    switch (payloadType) {
        case DOIP_PT_GENERIC_NACK:
            return "generic header negative ack";
        case DOIP_PT_ROUTING_ACTIVATION_REQ:
            return "routing activation request";
        case DOIP_PT_ROUTING_ACTIVATION_RES:
            return "routing activation response";
        case DOIP_PT_ALIVE_CHECK_REQ:
            return "alive check request";
        case DOIP_PT_ALIVE_CHECK_RES:
            return "alive check response";
        case DOIP_PT_DIAG_MESSAGE:
            return "diagnostic message";
        case DOIP_PT_DIAG_MESSAGE_ACK:
            return "diagnostic message positive ack";
        case DOIP_PT_DIAG_MESSAGE_NACK:
            return "diagnostic message negative ack";
        default:
            return "unknown payload type";
    }
}

const char* doip_ra_reason(unsigned char code)
{
    switch (code) {
        case DOIP_RA_UNKNOWN_SOURCE:
            return "unknown source address";
        case DOIP_RA_ALL_SOCKETS_IN_USE:
            return "all sockets registered and active";
        case DOIP_RA_SA_MISMATCH:
            return "source address differs from the activated one";
        case DOIP_RA_SA_IN_USE:
            return "source address already active on another socket";
        case DOIP_RA_MISSING_AUTH:
            return "missing authentication";
        case DOIP_RA_REJECTED_CONFIRM:
            return "rejected confirmation";
        case DOIP_RA_UNSUPPORTED_TYPE:
            return "unsupported activation type";
        case DOIP_RA_SUCCESS:
            return "routing activated";
        case DOIP_RA_CONFIRMATION_NEEDED:
            return "activation pending, confirmation required";
        default:
            return "unknown response code";
    }
}
