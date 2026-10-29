/* doip.h
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

/* DoIP (Diagnostics over IP, ISO 13400-2) message framing.
 *
 * Every DoIP message is an 8 byte generic header followed by a payload:
 *
 *   byte 0     protocol version
 *   byte 1     inverse protocol version (~version), a cheap sanity check
 *   bytes 2-3  payload type, big endian
 *   bytes 4-7  payload length, big endian
 *
 * The constants below are the subset these examples use. CHECK THEM AGAINST
 * THE REVISION YOU TARGET before building anything on top: payload type
 * numbering and response codes are stable across 13400-2:2012 and :2019, but
 * the version byte is not, and neither is the set of activation types.
 */

#ifndef WOLFSSL_EXAMPLE_DOIP_H
#define WOLFSSL_EXAMPLE_DOIP_H

#include <stddef.h>

/* Protocol version byte. 0x02 is ISO 13400-2:2012, 0x03 is :2019.
 *
 * 0x03 by default, because the TLS port below arrived with the 2019 edition:
 * announcing 0x02 on it would pair a version with a port its own edition does
 * not define. A 2012-era entity is reached with -DDOIP_PROTOCOL_VERSION=0x02,
 * which the parser then requires on the wire in both directions -- gateway and
 * tester have to agree, since the version is part of the synchronisation
 * pattern. */
#ifndef DOIP_PROTOCOL_VERSION
    #define DOIP_PROTOCOL_VERSION 0x03
#endif

#define DOIP_HEADER_LEN 8

/* Registered ports. TLS moves diagnostics off the plaintext port. */
#define DOIP_PORT_TCP_DATA      13400
#define DOIP_PORT_TCP_DATA_TLS  3496

/* Payload types used here */
#define DOIP_PT_GENERIC_NACK            0x0000
#define DOIP_PT_ROUTING_ACTIVATION_REQ  0x0005
#define DOIP_PT_ROUTING_ACTIVATION_RES  0x0006
#define DOIP_PT_ALIVE_CHECK_REQ         0x0007
#define DOIP_PT_ALIVE_CHECK_RES         0x0008
#define DOIP_PT_DIAG_MESSAGE            0x8001
#define DOIP_PT_DIAG_MESSAGE_ACK        0x8002
#define DOIP_PT_DIAG_MESSAGE_NACK       0x8003

/* Routing activation types */
#define DOIP_ACTIVATION_DEFAULT   0x00
#define DOIP_ACTIVATION_WWH_OBD   0x01

/* Routing activation response codes. 0x10 is the success case; everything
 * else is a refusal, and the tester must not send diagnostics after one. */
#define DOIP_RA_UNKNOWN_SOURCE      0x00
#define DOIP_RA_ALL_SOCKETS_IN_USE  0x01
#define DOIP_RA_SA_MISMATCH         0x02
#define DOIP_RA_SA_IN_USE           0x03
#define DOIP_RA_MISSING_AUTH        0x04
#define DOIP_RA_REJECTED_CONFIRM    0x05
#define DOIP_RA_UNSUPPORTED_TYPE    0x06
#define DOIP_RA_SUCCESS             0x10
#define DOIP_RA_CONFIRMATION_NEEDED 0x11

/* Diagnostic message ack / nack codes */
#define DOIP_DIAG_ACK_OK            0x00
#define DOIP_DIAG_NACK_INVALID_SA   0x02
#define DOIP_DIAG_NACK_UNKNOWN_TA   0x03
#define DOIP_DIAG_NACK_TOO_LARGE    0x04
#define DOIP_DIAG_NACK_OUT_OF_MEM   0x05
#define DOIP_DIAG_NACK_UNREACHABLE  0x06

/* Generic header nack codes */
#define DOIP_NACK_INCORRECT_PATTERN 0x00
#define DOIP_NACK_UNKNOWN_TYPE      0x01
#define DOIP_NACK_MESSAGE_TOO_LARGE 0x02
#define DOIP_NACK_OUT_OF_MEMORY     0x03
#define DOIP_NACK_INVALID_LENGTH    0x04

/* Logical addresses for these examples. A real vehicle gets these from its
 * diagnostic database; testers conventionally sit in 0x0E00-0x0EFF. */
#define DOIP_TESTER_ADDRESS  0x0E00
#define DOIP_GATEWAY_ADDRESS 0x1000

/* Refuse anything larger rather than trying to allocate it. The standard
 * permits far more; an example does not need to. */
#define DOIP_MAX_PAYLOAD 4096

/* ---- header ---- */

/* Writes the 8 byte generic header into buf, which must hold at least
 * DOIP_HEADER_LEN bytes. */
void doip_write_header(unsigned char* buf, unsigned short payloadType,
        unsigned int payloadLen);

/* Parses a generic header.
 *
 * Returns 0 on success, with *payloadType and *payloadLen filled in, or -1 on
 * a malformed header, with *nackCode set to the DOIP_NACK_* value to send
 * back and the two out-parameters left alone.
 *
 * The NACK code cannot be the return value: ISO 13400-2 assigns 0x00 to
 * "incorrect pattern format", so returning it would be indistinguishable from
 * success and a caller would go on to use a length it never received. */
int doip_parse_header(const unsigned char* buf, unsigned short* payloadType,
        unsigned int* payloadLen, unsigned char* nackCode);

/* Printable name for a payload type, for logging. */
const char* doip_payload_name(unsigned short payloadType);

/* Printable reason for a routing activation response code. */
const char* doip_ra_reason(unsigned char code);

#endif /* WOLFSSL_EXAMPLE_DOIP_H */
