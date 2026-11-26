/* csm-stream.c
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

/* Encrypting a payload that does not fit in a CAN frame, without buffering it.
 *
 * A classic CAN frame carries 8 bytes. An AES block is 16. So a single
 * block already spans two frames, and any message worth encrypting spans
 * many -- which is what segmentation protocols like ISO-TP exist for.
 *
 * The point of this example is the memory profile. Csm_Encrypt() and
 * Csm_Decrypt() in START/UPDATE/FINISH mode let both ends work a block at a
 * time, so an ECU needs one 16 byte block of scratch plus its CSM job --
 * not the whole message. A 1 KB diagnostic response costs the same RAM as
 * a 32 byte one.
 *
 * The second point is what that costs you: the stream carries no per-frame
 * integrity, so a lost frame is decrypted into rubbish rather than detected.
 * How much rubbish depends on what was lost, and the example shows both:
 *
 *   - A lost CAN frame is half an AES block here (8 bytes of 16), so every
 *     later block the receiver assembles is shifted by 8 bytes and comes out
 *     wrong. Nothing to do with CBC: losing block alignment would ruin ECB or
 *     CTR just the same.
 *   - A whole lost block is the CBC-specific case, and it is subtler. CBC
 *     decryption needs only a block and its predecessor, so the splice
 *     corrupts exactly one block and the chain recovers at once. But nothing
 *     in the stream numbers the blocks, so everything after it is written one
 *     block too early: the right plaintext in the wrong place.
 *
 * Either way nothing in the stream says so, which is the argument for putting
 * SecOC underneath (see csm-secoc.c): authenticate each frame, and a lost or
 * altered one is detected rather than quietly decrypted into rubbish.
 *
 * What this is NOT: an ISO-TP implementation. There are no FirstFrame /
 * ConsecutiveFrame headers and no flow control here -- that is a transport
 * protocol, and the can-bus example drives the real one through wolfSSL's
 * WOLFSSL_ISOTP support. This example segments into CAN-sized chunks so the
 * crypto side is visible on its own.
 *
 * Requires wolfSSL built with --enable-autosar (or -DWOLFSSL_AUTOSAR).
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
    printf("\ncsm-stream: SKIP\n");
    return 0;
}

#else /* WOLFSSL_AUTOSAR */

#include <wolfssl/wolfcrypt/port/autosar/Csm.h>

#define BLOCK_SIZE 16   /* AES block */
#define CAN_PAYLOAD 8   /* classic CAN frame data field */
#define FRAMES_PER_BLOCK (BLOCK_SIZE / CAN_PAYLOAD)

/* 8 blocks = 128 bytes = 16 CAN frames */
#define NUM_BLOCKS  8
#define MSG_LEN     (BLOCK_SIZE * NUM_BLOCKS)
#define NUM_FRAMES  (MSG_LEN / CAN_PAYLOAD)
/* the block the loss cases drop, whole or half: frames 4 and 5 */
#define LOST_BLOCK  2

#ifdef REDIRECTION_CONFIG
    #define KEY_SLOT ((uint32)REDIRECTION_IN1_KEYID)
    #define IV_SLOT  ((uint32)REDIRECTION_IN2_KEYID)
#else
    #define KEY_SLOT 0U
    #define IV_SLOT  1U
#endif

#define TX_JOB 1U
#define RX_JOB 2U

static const uint8 key[BLOCK_SIZE] = { /* "0123456789abcdef" */
    '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
};
static const uint8 iv[BLOCK_SIZE] = { /* "1234567890abcdef" */
    '1','2','3','4','5','6','7','8','9','0','a','b','c','d','e','f'
};

static int provision(void)
{
    if (Csm_KeyElementSet(KEY_SLOT, CRYPTO_KE_CIPHER_KEY,
                key, (uint32)sizeof(key)) != E_OK) {
        printf("Csm_KeyElementSet (key) failed\n");
        return -1;
    }
    if (Csm_KeyElementSet(IV_SLOT, CRYPTO_KE_CIPHER_IV,
                iv, (uint32)sizeof(iv)) != E_OK) {
        printf("Csm_KeyElementSet (IV) failed\n");
        return -1;
    }
    return 0;
}

/* The mode bits for block b of a transfer of NUM_BLOCKS. */
static Crypto_OperationModeType mode_for(int b)
{
    Crypto_OperationModeType mode = CRYPTO_OPERATIONMODE_UPDATE;

    if (b == 0) {
        mode |= CRYPTO_OPERATIONMODE_START;
    }
    if (b == NUM_BLOCKS - 1) {
        mode |= CRYPTO_OPERATIONMODE_FINISH;
    }
    return mode;
}

/* ---- sender --------------------------------------------------------- */

/* Encrypts the message a block at a time and emits the ciphertext as
 * CAN_PAYLOAD sized chunks into 'bus'. Its only scratch is one block.
 * Returns 0 on success. */
static int transmit(const uint8* msg, uint8* bus)
{
    uint8  block[BLOCK_SIZE];   /* the entire sender-side buffer */
    uint32 len;
    int    b, f;

    if (provision() != 0) {
        return -1;
    }

    for (b = 0; b < NUM_BLOCKS; b++) {
        len = BLOCK_SIZE;
        if (Csm_Encrypt(TX_JOB, mode_for(b),
                    msg + (b * BLOCK_SIZE), BLOCK_SIZE, block, &len) != E_OK) {
            printf("Csm_Encrypt block %d failed\n", b);
            return -1;
        }

        /* one AES block becomes two CAN frames */
        for (f = 0; f < FRAMES_PER_BLOCK; f++) {
            XMEMCPY(bus + (b * BLOCK_SIZE) + (f * CAN_PAYLOAD),
                    block + (f * CAN_PAYLOAD), CAN_PAYLOAD);
        }
    }
    return 0;
}

/* ---- receiver ------------------------------------------------------- */

/* Consumes CAN_PAYLOAD sized chunks, decrypting as soon as a whole block has
 * arrived. Its only scratch is one block plus a fill counter.
 *
 * 'dropFrame' and 'dropFrame2' are indices of frames to lose on the way, or
 * -1 for none. Two of them so a caller can drop a whole block (two
 * consecutive frames) as well as half of one.
 *
 * Returns the number of blocks delivered to 'out', or -1 if a Csm_* call
 * failed. */
static int receive(const uint8* bus, uint8* out, int dropFrame,
        int dropFrame2)
{
    uint8  block[BLOCK_SIZE];   /* the entire receiver-side buffer */
    uint32 fill = 0;
    uint32 len;
    int    f, b = 0;
    int    delivered = 0;

    if (provision() != 0) {
        return -1;
    }

    for (f = 0; f < NUM_FRAMES; f++) {
        if (f == dropFrame || f == dropFrame2) {
            continue; /* the frame never arrives */
        }

        XMEMCPY(block + fill, bus + (f * CAN_PAYLOAD), CAN_PAYLOAD);
        fill += CAN_PAYLOAD;

        if (fill < BLOCK_SIZE) {
            continue; /* still assembling */
        }
        fill = 0;

        /* a whole block is in hand, so it can be decrypted now and the
         * plaintext handed upward immediately */
        len = BLOCK_SIZE;
        if (Csm_Decrypt(RX_JOB, mode_for(b),
                    block, BLOCK_SIZE, out + (b * BLOCK_SIZE), &len) != E_OK) {
            printf("Csm_Decrypt block %d failed\n", b);
            return -1;
        }
        b++;
        delivered++;
        if (b == NUM_BLOCKS) {
            break;
        }
    }

    /* A dropped frame leaves the last block short, so the job never gets its
     * FINISH and its slot would leak. Close it out -- and check that it
     * closed: reclaiming the slot is the whole point of this call, and a run
     * that reported the blocks it delivered while the job stayed open would
     * claim a cleanup it did not do. */
    if (b < NUM_BLOCKS) {
        len = BLOCK_SIZE;
        if (Csm_Decrypt(RX_JOB, CRYPTO_OPERATIONMODE_FINISH,
                    block, 0, out + (b * BLOCK_SIZE), &len) != E_OK) {
            printf("Csm_Decrypt FINISH failed, the job slot is still open\n");
            return -1;
        }
    }

    return delivered;
}

/* Index of the first block that differs, or -1 if all match. */
static int first_bad_block(const uint8* a, const uint8* b)
{
    int i;

    for (i = 0; i < NUM_BLOCKS; i++) {
        if (XMEMCMP(a + (i * BLOCK_SIZE), b + (i * BLOCK_SIZE),
                    BLOCK_SIZE) != 0) {
            return i;
        }
    }
    return -1;
}

int main(void)
{
    uint8 msg[MSG_LEN];
    uint8 bus[MSG_LEN];
    uint8 out[MSG_LEN];
    int   i, bad, delivered;
    int   failures = 0;

    Csm_Init(NULL);

    /* a recognisable payload, e.g. a long diagnostic response */
    for (i = 0; i < MSG_LEN; i++) {
        msg[i] = (uint8)('A' + (i % 26));
    }

    printf("Streaming a payload larger than a CAN frame\n");
    printf("  message        %d bytes (%d AES blocks)\n", MSG_LEN, NUM_BLOCKS);
    printf("  CAN payload    %d bytes -> %d frames\n", CAN_PAYLOAD, NUM_FRAMES);
    printf("  sender RAM     %d bytes of scratch, not %d\n",
            BLOCK_SIZE, MSG_LEN);
    printf("  receiver RAM   %d bytes of scratch, not %d\n",
            BLOCK_SIZE, MSG_LEN);

    printf("\n== clean transfer ==\n");
    XMEMSET(bus, 0, sizeof(bus));
    XMEMSET(out, 0, sizeof(out));
    if (transmit(msg, bus) != 0) {
        return 1;
    }
    delivered = receive(bus, out, -1, -1);
    if (delivered < 0) {
        return 1;
    }
    printf("  %d of %d blocks delivered\n", delivered, NUM_BLOCKS);
    if (XMEMCMP(msg, out, MSG_LEN) == 0) {
        printf("  message recovered exactly, "
               "one block of scratch at each end\n");
    }
    else {
        printf("  FAILED: message did not survive the round trip\n");
        failures++;
    }

    printf("\n== half a block lost in the middle ==\n");
    XMEMSET(out, 0, sizeof(out));
    /* frame 5 is the second half of block 2 */
    delivered = receive(bus, out, 5, -1);
    if (delivered < 0) {
        return 1;
    }
    printf("  frame 5 dropped (second half of block 2)\n");
    printf("  %d of %d blocks delivered, %d never arrived\n", delivered,
            NUM_BLOCKS, NUM_BLOCKS - delivered);

    bad = first_bad_block(msg, out);
    if (bad < 0) {
        printf("  FAILED: expected corruption, got a clean message\n");
        failures++;
    }
    else if (bad != LOST_BLOCK) {
        /* Pin where it starts. Without this the loops below could run zero
         * times and the claim would hold vacuously. */
        printf("  FAILED: corruption starts at block %d, expected %d\n", bad,
                LOST_BLOCK);
        failures++;
    }
    else {
        int i;
        int allWrong = 1;

        /* Assert the claim rather than just printing it, and only over the
         * blocks that were actually delivered -- the rest were never
         * decrypted, so calling them "wrong" would be sloppy. */
        for (i = bad; i < delivered; i++) {
            if (XMEMCMP(msg + ((size_t)i * BLOCK_SIZE),
                        out + ((size_t)i * BLOCK_SIZE), BLOCK_SIZE) == 0) {
                allWrong = 0;
            }
        }

        printf("  first wrong block: %d\n", bad);
        if (allWrong) {
            printf("  every delivered block from %d to %d is wrong\n", bad,
                    delivered - 1);
        }
        else {
            printf("  FAILED: expected every delivered block from %d on to"
                   " be wrong\n", bad);
            failures++;
        }
        printf("  the frame was half a block, so everything after it is\n");
        printf("  shifted by 8 bytes -- block alignment is what was lost,\n");
        printf("  not CBC chaining, and nothing in the stream said so\n");
    }

    printf("\n== a whole block lost in the middle ==\n");
    XMEMSET(out, 0, sizeof(out));
    /* frames 4 and 5 together are block 2 */
    delivered = receive(bus, out, 4, 5);
    if (delivered < 0) {
        return 1;
    }
    printf("  frames 4 and 5 dropped (all of block 2)\n");
    printf("  %d of %d blocks delivered, %d never arrived\n", delivered,
            NUM_BLOCKS, NUM_BLOCKS - delivered);

    bad = first_bad_block(msg, out);
    if (bad < 0) {
        printf("  FAILED: expected corruption, got a clean message\n");
        failures++;
    }
    else if (bad != LOST_BLOCK || bad + 1 >= delivered) {
        /* As above, and the recovery claim needs at least one block after the
         * splice to be worth asserting. */
        printf("  FAILED: corruption starts at block %d of %d delivered,"
               " expected %d with blocks after it\n", bad, delivered,
                LOST_BLOCK);
        failures++;
    }
    else {
        int i;
        int shifted = 1;

        /* The CBC-specific result, and it is subtler than "everything after
         * is rubbish".
         *
         * Decrypting a block needs that block and the one before it. The
         * receiver splices C1 to C3, so the block it builds there is
         * D(C3) ^ C1 instead of D(C3) ^ C2 -- one wrong block. From the next
         * one on the chaining value is the true predecessor again, so the
         * plaintext is right.
         *
         * What is NOT right is where it lands. Nothing in the stream carries
         * a sequence number, so the receiver has no idea a block went
         * missing and writes what follows one block too early: everything
         * after the splice decrypts correctly but is displaced by 16 bytes.
         * A positional comparison still differs from the splice onwards --
         * for a different reason than in the case above, which is the point.
         */
        for (i = bad + 1; i < delivered; i++) {
            if (XMEMCMP(msg + ((size_t)(i + 1) * BLOCK_SIZE),
                        out + ((size_t)i * BLOCK_SIZE), BLOCK_SIZE) != 0) {
                shifted = 0;
            }
        }

        printf("  first wrong block: %d\n", bad);
        if (shifted) {
            printf("  the only one decrypted wrongly: CBC needs a block and\n");
            printf("  its predecessor, so the chain recovers at once. Every\n");
            printf("  block after it is the right plaintext, one block early\n");
            printf("  -- the stream carries no sequence number, so the gap\n");
            printf("  is invisible and the rest is silently displaced\n");
        }
        else {
            printf("  FAILED: expected block %d wrong and the rest shifted\n",
                    bad);
            failures++;
        }
        printf("  silent either way -- that is what SecOC adds; see\n");
        printf("  csm-secoc.c\n");
    }

    if (failures != 0) {
        printf("\ncsm-stream: FAIL\n");
        return 1;
    }

    printf("\ncsm-stream: PASS\n");
    return 0;
}

#endif /* WOLFSSL_AUTOSAR */
