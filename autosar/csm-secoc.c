/* csm-secoc.c
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

/* AUTOSAR Secure Onboard Communication (SecOC) over classic CAN, built on the
 * wolfSSL AUTOSAR port's MAC services.
 *
 * SecOC authenticates an I-PDU by appending a freshness value and a MAC:
 *
 *   MAC input     = Data ID | Authentic I-PDU | complete Freshness Value
 *   Secured I-PDU = Authentic I-PDU | truncated FV | truncated Authenticator
 *
 * Both halves matter and neither works alone. The MAC stops an attacker
 * forging a payload; the freshness value stops one replaying a payload that
 * was genuine an hour ago. A verifier that checks the MAC and ignores
 * freshness accepts every replay, which is the classic way to get this wrong.
 *
 * The configuration below is sized so a secured PDU is exactly one classic CAN
 * frame -- 8 bytes, no CAN FD, no ISO-TP segmentation:
 *
 *   4 byte payload + 1 byte truncated FV + 3 byte truncated MAC = 8
 *
 * That 3 byte (24 bit) authenticator is a real SecOC profile length, and it is
 * why truncation exists at all: a full 16 byte CMAC tag does not fit in a CAN
 * frame alongside any payload.
 *
 * Usage:
 *   csm-secoc                  run the scenarios in process (default)
 *   csm-secoc --can <iface>    same scenarios, but every frame crosses a real
 *                              SocketCAN interface (Linux)
 *   csm-secoc --send <iface>   transmit genuine secured frames and exit
 *   csm-secoc --recv <iface>   receive and verify until idle
 *
 * Requires wolfSSL built with --enable-autosar --enable-autosar-cmac.
 */

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>

#include <stdio.h>
#include <string.h>

#ifndef WOLFSSL_AUTOSAR

int main(void)
{
    printf("wolfSSL was not built with AUTOSAR support.\n");
    printf("Rebuild wolfSSL with: ./configure --enable-autosar\n");
    /* SKIP and exit 0, like the missing-option paths, so `make
     * check` reports a configuration this example cannot run on
     * rather than failing on it. 77 is automake's skip code, but
     * this directory's check target reads the marker, not the
     * status. */
    printf("\ncsm-secoc: SKIP\n");
    return 0;
}

#elif !defined(WOLFSSL_AUTOSAR_CMAC)

int main(void)
{
    printf("SecOC needs the AUTOSAR MAC services.\n");
    printf("Rebuild wolfSSL with: "
           "./configure --enable-autosar --enable-autosar-cmac\n");
    printf("\ncsm-secoc: SKIP\n");
    return 0;
}

#else /* WOLFSSL_AUTOSAR && WOLFSSL_AUTOSAR_CMAC */

#include <wolfssl/wolfcrypt/port/autosar/Csm.h>

/* SocketCAN is Linux only. Everything else in this file is portable, and the
 * default run needs no CAN interface at all. */
#if defined(__linux__) && !defined(NO_SOCKETCAN)
    #define HAVE_SOCKETCAN
#endif

#ifdef HAVE_SOCKETCAN
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <linux/can.h>
    #include <linux/can/raw.h>
    #include <net/if.h>
    #include <unistd.h>
    #include <poll.h>
    #include <errno.h>
#endif

/* ---- SecOC configuration -------------------------------------------- */

/* Unique per-PDU identifier, mixed into the MAC so a frame authenticated for
 * one PDU cannot be replayed as another. Often derived from the CAN ID, which
 * is what this example does. */
#define SECOC_DATA_ID 0x0042

/* CAN arbitration ID carrying the secured PDU */
#define SECOC_CAN_ID  0x042

#define AUTHENTIC_PDU_LEN 4  /* application payload */
#define TRUNC_FV_LEN      1  /* freshness bytes on the wire */
#define TRUNC_AUTH_LEN    3  /* authenticator bytes on the wire, 24 bit */
/* 3 bytes is what the port's WOLFSSL_AUTOSAR_MAC_MIN_SZ defaults to, profile 1
 * being the reason for that default. A library built with the floor raised --
 * to wolfCrypt's WC_CMAC_TAG_MIN_SZ, or to 16 to refuse truncation -- refuses
 * this verify, which is a configuration choice and not a bug here. */

/* one classic CAN frame */
#define SECURED_PDU_LEN \
    (AUTHENTIC_PDU_LEN + TRUNC_FV_LEN + TRUNC_AUTH_LEN)

/* Data ID | payload | complete 32 bit FV */
#define MAC_INPUT_LEN (2 + AUTHENTIC_PDU_LEN + 4)

/* How far ahead of its own counter the verifier will search for a matching
 * freshness value. This is what tolerates frames lost on the bus; it also
 * bounds how far an attacker can push the counter forward. */
#define SECOC_FV_WINDOW 16

/* keystore slot and job IDs */
#ifdef REDIRECTION_CONFIG
    #define MAC_KEY_SLOT ((uint32)REDIRECTION_IN1_KEYID)
#else
    #define MAC_KEY_SLOT 0U
#endif
#define SECOC_TX_JOB 1U
#define SECOC_RX_JOB 2U

/* Shared between the two ECUs in this example. On a real vehicle this is
 * provisioned per key slot, not compiled in. */
static const uint8 secocKey[16] = { /* "SecOC demo key!!" */
    'S','e','c','O','C',' ','d','e','m','o',' ','k','e','y','!','!'
};

/* ---- the SecOC layer ------------------------------------------------ */
/* These two functions are the whole SecOC layer, and they are transport
 * agnostic: they produce and consume the 8 bytes that go in a CAN frame's
 * data field. The front-ends below just move those bytes around. */

/* Data ID and freshness value go in big endian, as on the wire. */
static void build_mac_input(uint8* out, const uint8* pdu, uint32 fv)
{
    out[0] = (uint8)((SECOC_DATA_ID >> 8) & 0xFF);
    out[1] = (uint8)(SECOC_DATA_ID & 0xFF);
    XMEMCPY(out + 2, pdu, AUTHENTIC_PDU_LEN);
    out[2 + AUTHENTIC_PDU_LEN + 0] = (uint8)((fv >> 24) & 0xFF);
    out[2 + AUTHENTIC_PDU_LEN + 1] = (uint8)((fv >> 16) & 0xFF);
    out[2 + AUTHENTIC_PDU_LEN + 2] = (uint8)((fv >>  8) & 0xFF);
    out[2 + AUTHENTIC_PDU_LEN + 3] = (uint8)(fv & 0xFF);
}

/* Builds a secured I-PDU for the given payload and freshness value.
 * Returns 0 on success. */
static int secoc_authenticate(const uint8* pdu, uint32 fv, uint8* frame)
{
    uint8  macIn[MAC_INPUT_LEN];
    uint8  mac[16];
    uint32 macLen = (uint32)sizeof(mac);

    build_mac_input(macIn, pdu, fv);

    if (Csm_MacGenerate(SECOC_TX_JOB, CRYPTO_OPERATIONMODE_SINGLECALL,
                macIn, MAC_INPUT_LEN, mac, &macLen) != E_OK) {
        printf("Csm_MacGenerate failed\n");
        return -1;
    }

    XMEMCPY(frame, pdu, AUTHENTIC_PDU_LEN);
    /* only the low byte of the counter is transmitted */
    frame[AUTHENTIC_PDU_LEN] = (uint8)(fv & 0xFF);
    XMEMCPY(frame + AUTHENTIC_PDU_LEN + TRUNC_FV_LEN, mac, TRUNC_AUTH_LEN);

    return 0;
}

/* Verifies a secured I-PDU against the receiver's own freshness counter.
 *
 * The transmitted freshness value is one byte, so the verifier has to
 * reconstruct the complete value: it walks forward from the last value it
 * accepted and tries each candidate whose low byte matches. Starting at
 * lastFv + 1 is what makes a replay fail -- the old value is behind the
 * window, so no candidate ever reproduces its MAC.
 *
 * On success *acceptedFv holds the reconstructed value, which the caller
 * stores as its new low water mark.
 *
 * Returns 0 if accepted, -1 if rejected (with *why set). */
static int secoc_verify(const uint8* frame, uint32 lastFv, uint32* acceptedFv,
        const char** why)
{
    uint8  macIn[MAC_INPUT_LEN];
    uint32 candidate;
    uint8  truncFv = frame[AUTHENTIC_PDU_LEN];
    int    tried = 0;

    /* The window is bounded by the DIFFERENCE from lastFv, not by a sum that
     * can wrap. With `candidate <= lastFv + SECOC_FV_WINDOW`, a lastFv near
     * UINT32_MAX makes the sum wrap to a small number, the loop starts at 0,
     * and low counter values become acceptable again -- the freshness low
     * water mark rolls backwards and every old frame is replayable. Stopping
     * at the wrap means a counter that reaches UINT32_MAX needs an explicit
     * freshness reset and rekey, which is what SecOC expects of it anyway. */
    for (candidate = lastFv + 1;
            candidate > lastFv && candidate - lastFv <= SECOC_FV_WINDOW;
            candidate++) {
        Crypto_VerifyResultType verify = CRYPTO_E_VER_NOT_OK;

        if ((candidate & 0xFF) != truncFv) {
            continue; /* cannot be the value that was sent */
        }
        tried++;

        build_mac_input(macIn, frame, candidate);

        /* macLength shorter than the tag compares the leading bytes, which is
         * exactly SecOC's truncated authenticator. */
        if (Csm_MacVerify(SECOC_RX_JOB, CRYPTO_OPERATIONMODE_SINGLECALL,
                    macIn, MAC_INPUT_LEN,
                    frame + AUTHENTIC_PDU_LEN + TRUNC_FV_LEN,
                    TRUNC_AUTH_LEN * 8,
                    &verify) != E_OK) {
            *why = "Csm_MacVerify job failed";
            return -1;
        }

        if (verify == CRYPTO_E_VER_OK) {
            *acceptedFv = candidate;
            return 0;
        }
    }

    *why = (tried == 0) ?
        "no freshness value in the window matches the one transmitted" :
        "authenticator did not verify for any candidate freshness value";
    return -1;
}

/* ---- SocketCAN front-end -------------------------------------------- */

#ifdef HAVE_SOCKETCAN

/* Opens a CAN_RAW socket bound to iface. When filter is non-zero the kernel
 * drops everything but that arbitration ID, which is cheaper than filtering in
 * user space.
 * Returns the socket, or -1. */
static int can_open(const char* iface, canid_t filter)
{
    struct sockaddr_can addr;
    struct ifreq ifr;
    int sock;

    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        perror("socket(PF_CAN)");
        return -1;
    }

    if (filter != 0) {
        struct can_filter rfilter[1];

        /* Match the 11 bit ID, and require the format and remote flags to be
         * clear. CAN_SFF_MASK alone covers only the ID bits, so an
         * extended-format or remote-transmission frame whose low 11 bits
         * happen to equal the filter would reach us too -- an RTR frame with
         * no payload at all. Masking the flags in makes the kernel drop them
         * before they ever arrive. */
        rfilter[0].can_id   = filter;
        rfilter[0].can_mask = CAN_SFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG;
        if (setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter,
                    sizeof(rfilter)) < 0) {
            perror("setsockopt(CAN_RAW_FILTER)");
            close(sock);
            return -1;
        }
    }

    XMEMSET(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        fprintf(stderr, "no such CAN interface '%s': %s\n", iface,
                strerror(errno));
        close(sock);
        return -1;
    }

    XMEMSET(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return -1;
    }

    return sock;
}

/* Puts one secured PDU on the bus. Returns 0 on success. */
static int can_put(int sock, const uint8* frame)
{
    struct can_frame cf;
    ssize_t n;

    XMEMSET(&cf, 0, sizeof(cf));
    cf.can_id  = SECOC_CAN_ID;
    cf.can_dlc = SECURED_PDU_LEN;
    XMEMCPY(cf.data, frame, SECURED_PDU_LEN);

    n = write(sock, &cf, sizeof(cf));
    if (n != (ssize_t)sizeof(cf)) {
        perror("write(can_frame)");
        return -1;
    }
    return 0;
}

/* Waits up to timeoutMs for one secured PDU.
 * Returns 0 on success, 1 on timeout, -1 on error. */
static int can_get(int sock, uint8* frame, int timeoutMs)
{
    struct can_frame cf;
    struct pollfd p[1];
    ssize_t n;
    int ret;

    p[0].fd = sock;
    p[0].events = POLLIN;

    /* Loops so a frame this example does not speak can be skipped and the
     * next one waited for. Each skip restarts the idle timeout, which is fine
     * for a demo bus. */
    for (;;) {
        ret = poll(p, 1, timeoutMs);
        if (ret < 0) {
            perror("poll");
            return -1;
        }
        if (ret == 0) {
            return 1; /* idle */
        }

        n = read(sock, &cf, sizeof(cf));
        if (n < (ssize_t)sizeof(cf)) {
            perror("read(can_frame)");
            return -1;
        }

        /* Backstop for the filter above: a classic 11 bit data frame is
         * the only thing this example speaks. Skipping goes back to poll()
         * for the next frame -- reporting the idle code instead would end
         * the receive session on the first stray frame, and count as a lost
         * frame in --can mode. */
        if ((cf.can_id & CAN_EFF_FLAG) != 0 ||
                (cf.can_id & CAN_RTR_FLAG) != 0) {
            fprintf(stderr, "ignoring an extended or RTR frame (id 0x%08X)\n",
                    (unsigned int)cf.can_id);
            continue;
        }

        if (cf.can_dlc != SECURED_PDU_LEN) {
            fprintf(stderr, "frame carries %u bytes, expected %d\n",
                    (unsigned int)cf.can_dlc, SECURED_PDU_LEN);
            return -1;
        }

        XMEMCPY(frame, cf.data, SECURED_PDU_LEN);
        return 0;
    }
}

#endif /* HAVE_SOCKETCAN */

/* ---- the scenarios -------------------------------------------------- */

static int failures = 0;

#ifdef HAVE_SOCKETCAN
/* Set when running with --can: every frame then makes a real round trip
 * through the interface before it is verified. */
static int busTx = -1;
static int busRx = -1;
static int onCan = 0;
#endif

static void print_frame(const char* label, const uint8* frame)
{
    int i;

    printf("  %-22s", label);
    for (i = 0; i < SECURED_PDU_LEN; i++) {
        printf("%02x", frame[i]);
        if (i == AUTHENTIC_PDU_LEN - 1 ||
                i == AUTHENTIC_PDU_LEN + TRUNC_FV_LEN - 1) {
            printf(" ");
        }
    }
    printf("   (payload fv mac)\n");
}

/* Moves a frame from the authenticator to the verifier.
 *
 * In the default run this is a no-op: the bytes are already where the verifier
 * will read them. With --can they are written to the interface and read back
 * from a second socket, so the verifier works on bytes that actually crossed
 * the bus. Returns 0 on success. */
static int bus_carry(uint8* frame)
{
#ifdef HAVE_SOCKETCAN
    if (onCan) {
        uint8 received[SECURED_PDU_LEN];

        if (can_put(busTx, frame) != 0) {
            return -1;
        }
        if (can_get(busRx, received, 1000) != 0) {
            fprintf(stderr, "frame did not come back off the bus\n");
            return -1;
        }
        /* what the verifier sees is what the wire delivered */
        XMEMCPY(frame, received, SECURED_PDU_LEN);
        return 0;
    }
#endif
    (void)frame;
    return 0;
}

/* Sender state */
static uint32 txFv = 0;
/* Receiver state: the highest freshness value it has accepted */
static uint32 rxFv = 0;

static void expect_accepted(const char* what, uint8* frame)
{
    uint32 fv = 0;
    const char* why = "";

    if (bus_carry(frame) != 0) {
        failures++;
        return;
    }

    if (secoc_verify(frame, rxFv, &fv, &why) == 0) {
        printf("  accepted  %s (fv=%u)\n", what, (unsigned int)fv);
        rxFv = fv;
    }
    else {
        printf("  REJECTED  %s  <-- expected accept: %s\n", what, why);
        failures++;
    }
}

static void expect_rejected(const char* what, uint8* frame)
{
    uint32 fv = 0;
    const char* why = "";

    if (bus_carry(frame) != 0) {
        failures++;
        return;
    }

    if (secoc_verify(frame, rxFv, &fv, &why) != 0) {
        printf("  rejected  %s\n", what);
        printf("            reason: %s\n", why);
    }
    else {
        printf("  ACCEPTED  %s  <-- expected reject\n", what);
        failures++;
    }
}

/* Sends one message: the sender bumps its counter, authenticates, and the
 * frame that would go on the bus is handed to the caller. */
static int send_message(const uint8* pdu, uint8* frame)
{
    txFv++;
    return secoc_authenticate(pdu, txFv, frame);
}

static int provision_key(void)
{
    /* Both ECUs hold the same key. CRYPTO_KE_MAC_KEY shares element ID 0x01
     * with CRYPTO_KE_CIPHER_KEY, so an ECU that also does cipher jobs cannot
     * let the driver resolve its own key -- it would take whichever slot came
     * first. Such an ECU names the slot instead, with
     * wolfSSL_Csm_MacGenerateWithKey() and wolfSSL_Csm_EncryptWithKey(). Key
     * input redirection does not help: it maps an element ID to one slot for
     * every service. This example only does MAC jobs, so the plain call is
     * unambiguous here. */
    if (Csm_KeyElementSet(MAC_KEY_SLOT, CRYPTO_KE_MAC_KEY,
                secocKey, (uint32)sizeof(secocKey)) != E_OK) {
        printf("Csm_KeyElementSet failed\n");
        return -1;
    }
    return 0;
}

static void print_config(void)
{
    printf("  data ID           0x%04x\n", (unsigned int)SECOC_DATA_ID);
    printf("  payload           %d bytes\n", AUTHENTIC_PDU_LEN);
    printf("  truncated FV      %d byte\n", TRUNC_FV_LEN);
    printf("  truncated MAC     %d bytes (%d bit)\n", TRUNC_AUTH_LEN,
            TRUNC_AUTH_LEN * 8);
    printf("  secured PDU       %d bytes = one CAN frame\n", SECURED_PDU_LEN);
}

/* The scenario suite. Runs identically in process and over a real bus.
 * Returns 0 if every case behaved as expected. */
static int run_scenarios(void)
{
    uint8 frame[SECURED_PDU_LEN];
    uint8 replay[SECURED_PDU_LEN];
    uint8 tampered[SECURED_PDU_LEN];
    /* a 4 byte payload, e.g. a signal pair in a real CAN matrix */
    uint8 pdu[AUTHENTIC_PDU_LEN] = { 0x01, 0x02, 0x03, 0x04 };
    int i;

    printf("\n== normal traffic ==\n");
    for (i = 0; i < 3; i++) {
        pdu[3] = (uint8)(0x04 + i);
        if (send_message(pdu, frame) != 0) {
            return -1;
        }
        print_frame("on the bus:", frame);

        if (i == 0) {
            /* keep this one to replay later */
            XMEMCPY(replay, frame, sizeof(frame));
        }
        expect_accepted("fresh frame", frame);
    }

    printf("\n== frames lost on the bus ==\n");
    /* The sender transmits three more, the bus drops the first two. The
     * verifier never sees them, so its counter is behind -- the window is
     * what lets it catch up. */
    for (i = 0; i < 3; i++) {
        pdu[3] = (uint8)(0x10 + i);
        if (send_message(pdu, frame) != 0) {
            return -1;
        }
        if (i < 2) {
            printf("  (dropped fv=%u)\n", (unsigned int)txFv);
            continue;
        }
    }
    print_frame("on the bus:", frame);
    expect_accepted("frame after two losses", frame);

    printf("\n== attacks ==\n");

    /* A frame that really was authentic, resent. The MAC is still valid for
     * its original freshness value -- and that value is now behind the
     * receiver's low water mark, so it can never be reconstructed again. */
    print_frame("replayed frame:", replay);
    expect_rejected("replay of an earlier genuine frame", replay);
    printf("            note: the authenticator on this frame is still "
           "valid;\n");
    printf("                  freshness is what rejects it\n");

    /* Payload edited, authenticator left alone.
     *
     * Each tamper case below starts from a frame the verifier has NOT seen
     * yet, so its freshness value is still inside the window and the
     * rejection really does come from the MAC. Reusing an already-accepted
     * frame would be rejected on freshness alone and prove nothing about the
     * authenticator. */
    if (send_message(pdu, frame) != 0) {
        return -1;
    }
    XMEMCPY(tampered, frame, sizeof(tampered));
    tampered[0] ^= 0xFF;
    print_frame("payload flipped:", tampered);
    expect_rejected("modified payload", tampered);

    /* Authenticator edited, payload left alone. */
    if (send_message(pdu, frame) != 0) {
        return -1;
    }
    XMEMCPY(tampered, frame, sizeof(tampered));
    tampered[AUTHENTIC_PDU_LEN + TRUNC_FV_LEN] ^= 0xFF;
    print_frame("mac flipped:", tampered);
    expect_rejected("modified authenticator", tampered);

    /* A genuine frame whose freshness value is far beyond the window. A
     * verifier with an unbounded search would accept this and let an attacker
     * drag the counter forward, locking out the real sender. */
    if (secoc_authenticate(pdu, rxFv + SECOC_FV_WINDOW + 50, tampered) != 0) {
        return -1;
    }
    print_frame("fv beyond window:", tampered);
    expect_rejected("valid MAC, freshness far beyond the window", tampered);

    /* Same payload and freshness, but authenticated for a different Data ID,
     * i.e. lifted from another CAN message. */
    {
        uint8 macIn[MAC_INPUT_LEN];
        uint8 mac[16];
        uint32 macLen = (uint32)sizeof(mac);
        uint32 fv = rxFv + 1;

        build_mac_input(macIn, pdu, fv);
        macIn[1] ^= 0xFF; /* pretend a different Data ID */
        if (Csm_MacGenerate(SECOC_TX_JOB, CRYPTO_OPERATIONMODE_SINGLECALL,
                    macIn, MAC_INPUT_LEN, mac, &macLen) != E_OK) {
            printf("Csm_MacGenerate failed\n");
            return -1;
        }
        XMEMCPY(tampered, pdu, AUTHENTIC_PDU_LEN);
        tampered[AUTHENTIC_PDU_LEN] = (uint8)(fv & 0xFF);
        XMEMCPY(tampered + AUTHENTIC_PDU_LEN + TRUNC_FV_LEN, mac,
                TRUNC_AUTH_LEN);

        print_frame("other data ID:", tampered);
        expect_rejected("frame authenticated for a different Data ID",
                tampered);
    }

    /* The receiver still works after all that: its state was never advanced
     * by a rejected frame. */
    printf("\n== the bus recovers ==\n");
    pdu[3] = 0x77;
    if (send_message(pdu, frame) != 0) {
        return -1;
    }
    print_frame("on the bus:", frame);
    expect_accepted("next genuine frame", frame);

    return 0;
}

/* ---- single-role front-ends ----------------------------------------- */

#ifdef HAVE_SOCKETCAN

#define SEND_COUNT 10
#define RECV_IDLE_MS 3000

/* Transmits SEND_COUNT genuine secured frames. */
static int role_send(const char* iface)
{
    uint8 frame[SECURED_PDU_LEN];
    uint8 pdu[AUTHENTIC_PDU_LEN] = { 0x01, 0x02, 0x03, 0x00 };
    int sock, i, ret = 0;

    sock = can_open(iface, 0);
    if (sock < 0) {
        return -1;
    }

    printf("sending %d secured frames on %s (CAN ID 0x%03x)\n",
            SEND_COUNT, iface, (unsigned int)SECOC_CAN_ID);

    for (i = 0; i < SEND_COUNT; i++) {
        pdu[3] = (uint8)i;
        if (send_message(pdu, frame) != 0) {
            ret = -1;
            break;
        }
        if (can_put(sock, frame) != 0) {
            ret = -1;
            break;
        }
        print_frame("sent:", frame);
    }

    close(sock);
    return ret;
}

/* Verifies frames until the bus goes idle. */
static int role_recv(const char* iface)
{
    uint8 frame[SECURED_PDU_LEN];
    int sock;
    int accepted = 0, rejected = 0;

    sock = can_open(iface, SECOC_CAN_ID);
    if (sock < 0) {
        return -1;
    }

    printf("verifying frames on %s (CAN ID 0x%03x), "
           "stops after %d ms idle\n",
            iface, (unsigned int)SECOC_CAN_ID, RECV_IDLE_MS);

    for (;;) {
        uint32 fv = 0;
        const char* why = "";
        int ret = can_get(sock, frame, RECV_IDLE_MS);

        if (ret < 0) {
            close(sock);
            return -1;
        }
        if (ret == 1) {
            break; /* idle */
        }

        print_frame("received:", frame);
        if (secoc_verify(frame, rxFv, &fv, &why) == 0) {
            printf("  accepted (fv=%u)\n", (unsigned int)fv);
            rxFv = fv;
            accepted++;
        }
        else {
            printf("  rejected: %s\n", why);
            rejected++;
        }
    }

    close(sock);
    printf("\n%d accepted, %d rejected\n", accepted, rejected);
    return 0;
}

#endif /* HAVE_SOCKETCAN */

static void usage(const char* argv0)
{
    printf("usage: %s [--can <iface> | --send <iface> | --recv <iface>]\n",
            argv0);
    printf("  no arguments   run the scenarios in process\n");
    printf("  --can <iface>  same scenarios, every frame crosses a real "
           "CAN interface\n");
    printf("  --send <iface> transmit genuine secured frames and exit\n");
    printf("  --recv <iface> receive and verify until the bus goes idle\n");
}

int main(int argc, char** argv)
{
    const char* mode  = (argc > 1) ? argv[1] : NULL;
    const char* iface = (argc > 2) ? argv[2] : NULL;

    if (mode != NULL && (XSTRCMP(mode, "-h") == 0 ||
                XSTRCMP(mode, "--help") == 0)) {
        usage(argv[0]);
        return 0;
    }

    if (mode != NULL && iface == NULL) {
        printf("%s needs an interface name\n", mode);
        usage(argv[0]);
        return 1;
    }

#ifndef HAVE_SOCKETCAN
    if (mode != NULL) {
        printf("SocketCAN is Linux only; this build has no CAN support.\n");
        printf("Run without arguments for the in-process scenarios.\n");
        return 77; /* skip */
    }
#endif

    Csm_Init(NULL);

    printf("SecOC over classic CAN\n");
    print_config();

    if (provision_key() != 0) {
        return 1;
    }

#ifdef HAVE_SOCKETCAN
    if (mode != NULL && XSTRCMP(mode, "--send") == 0) {
        return (role_send(iface) == 0) ? 0 : 1;
    }
    if (mode != NULL && XSTRCMP(mode, "--recv") == 0) {
        return (role_recv(iface) == 0) ? 0 : 1;
    }
    if (mode != NULL && XSTRCMP(mode, "--can") == 0) {
        /* Two sockets on one interface: the frames written to busTx are
         * delivered to busRx by SocketCAN's local loopback, so the scenarios
         * run over the real bus inside one process. */
        busTx = can_open(iface, 0);
        busRx = can_open(iface, SECOC_CAN_ID);
        if (busTx < 0 || busRx < 0) {
            if (busTx >= 0) {
                close(busTx);
            }
            if (busRx >= 0) {
                close(busRx);
            }
            return 1;
        }
        onCan = 1;
        printf("  transport         %s (CAN ID 0x%03x)\n", iface,
                (unsigned int)SECOC_CAN_ID);
    }
    else
#endif
    if (mode != NULL) {
        printf("unknown option '%s'\n", mode);
        usage(argv[0]);
        return 1;
    }
    else {
        printf("  transport         in process "
               "(pass --can <iface> for a real bus)\n");
    }

    if (run_scenarios() != 0) {
        failures++;
    }

#ifdef HAVE_SOCKETCAN
    if (onCan) {
        close(busTx);
        close(busRx);
    }
#endif

    if (failures != 0) {
        printf("\n%d case(s) behaved unexpectedly\n", failures);
        printf("\ncsm-secoc: FAIL\n");
        return 1;
    }

    printf("\ncsm-secoc: PASS\n");
    return 0;
}

#endif /* WOLFSSL_AUTOSAR && WOLFSSL_AUTOSAR_CMAC */
