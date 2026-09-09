/* nuvoton_nsc.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

/* Non-secure callable entry points for the M2354 port.
 *
 * The CRPT accelerator, the TRNG and the Key Store are secure only in the
 * default SCU partition, and M2354.h has no KS_NS alias at all, so a
 * non-secure wolfCrypt cannot reach any of them directly. This file is the
 * secure half: one veneer per wc_nuvoton_hw_* call, each checking that the
 * buffers the non-secure side passed really belong to the non-secure world
 * before handing them to the driver.
 *
 * Build it into the secure image together with wolfcrypt/src/port/nuvoton/
 * nuvoton_hw.c, with -mcmse and -DWOLFSSL_NUVOTON_NSC_IMPL. That macro renames
 * the implementations to wc_nuvoton_hw_*_s (see nuvoton_hw.h) so the veneers
 * below can take the plain names, which is what the non-secure image links
 * against through the import library the linker emits.
 *
 * The non-secure side builds the same port sources with
 * -DWOLFSSL_NUVOTON_NSC, which leaves nuvoton_hw.c empty.
 */

#include "NuMicro.h"

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/types.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#ifndef NO_AES
    #include <wolfssl/wolfcrypt/aes.h>
#endif

#include "wolfcrypt/src/port/nuvoton/nuvoton_hw.h"

#include <arm_cmse.h>

#ifndef WOLFSSL_NUVOTON_NSC_IMPL
    #error "Build the secure side with -DWOLFSSL_NUVOTON_NSC_IMPL"
#endif

/* nuvoton_hw.h has just declared the implementations under their _s names.
 * Drop the macros so the veneers below define the plain names the non-secure
 * image links against, while still being able to call the _s ones. */
#undef wc_nuvoton_hw_init
#undef wc_nuvoton_hw_cleanup
#undef wc_nuvoton_hw_trng
#undef wc_nuvoton_hw_sha
#undef wc_nuvoton_hw_aes
#undef wc_nuvoton_hw_ecc_sign
#undef wc_nuvoton_hw_ecc_verify
#undef wc_nuvoton_hw_ecc_shared
#undef wc_nuvoton_hw_ecc_pubkey
#undef wc_nuvoton_hw_rsa
#undef wc_nuvoton_hw_ks_write
#undef wc_nuvoton_hw_ks_read
#undef wc_nuvoton_hw_ks_erase
#undef wc_nuvoton_hw_ks_revoke

#define NSC_ENTRY __attribute__((cmse_nonsecure_entry))

/* Refuse a buffer that is not entirely non-secure, so a non-secure caller
 * cannot talk the secure world into reading or overwriting secure memory.
 *
 * A NULL or zero-length buffer reports failure, so callers must NULL-test an
 * optional buffer before calling this rather than relying on a zero length to
 * pass through. */
static int nsc_check(void* p, word32 sz, int write)
{
    int flags = CMSE_NONSECURE | CMSE_MPU_READ;

    if (p == NULL || sz == 0) {
        return 0;
    }
    if (write) {
        flags |= CMSE_MPU_READWRITE;
    }

    return (cmse_check_address_range(p, sz, flags) != NULL);
}

/* Same, for a buffer whose length comes from the request. Nothing is read or
 * written when that length is zero, so there is no span to check and no reason
 * to reject the call: an AES-GCM operation over AAD alone carries a
 * zero-length payload. Every other length still goes through nsc_check(). */
static int nsc_check_len(void* p, word32 sz, int write)
{
    if (sz == 0) {
        return 1;
    }

    return nsc_check(p, sz, write);
}

/* Same, for the NUL terminated hex strings the public key requests carry. The
 * length is not known up front, so the string is measured inside a bound and
 * then the whole span is checked in one go. */
static int nsc_check_str(char* s, word32 maxSz, word32* lenOut)
{
    word32 i;

    if (s == NULL) {
        return 0;
    }
    if (!nsc_check(s, 1, 0)) {
        return 0;
    }

    for (i = 0; i < maxSz; i++) {
        if (!nsc_check(s + i, 1, 0)) {
            return 0;
        }
        if (s[i] == '\0') {
            if (lenOut != NULL) {
                *lenOut = i + 1;
            }
            return 1;
        }
    }

    return 0;
}

NSC_ENTRY int wc_nuvoton_hw_init(void)
{
    return wc_nuvoton_hw_init_s();
}

NSC_ENTRY void wc_nuvoton_hw_cleanup(void)
{
    wc_nuvoton_hw_cleanup_s();
}

NSC_ENTRY int wc_nuvoton_hw_trng(byte* out, word32 sz)
{
    if (!nsc_check(out, sz, 1)) {
        return BAD_FUNC_ARG;
    }

    return wc_nuvoton_hw_trng_s(out, sz);
}

NSC_ENTRY int wc_nuvoton_hw_sha(wc_NuvotonShaReq* req)
{
    wc_NuvotonShaReq local;

    if (!nsc_check(req, (word32)sizeof(*req), 1)) {
        return BAD_FUNC_ARG;
    }
    XMEMCPY(&local, req, sizeof(local));

    /* digest is set only on the final chunk; an update passes NULL. Checking
     * it unconditionally rejects every intermediate chunk, and the hash
     * callback result is not remapped to CRYPTOCB_UNAVAILABLE, so that fails
     * the caller outright rather than falling back. fdbck is read and written
     * by the engine over DMA, so it needs checking too. */
    if (!nsc_check_len((void*)local.in, local.inSz, 0)) {
        return BAD_FUNC_ARG;
    }
    if (!nsc_check(local.fdbck,
            WC_NUVOTON_SHA_FDBCK_WORDS * (word32)sizeof(word32), 1)) {
        return BAD_FUNC_ARG;
    }
    if (local.digest != NULL &&
        !nsc_check(local.digest, local.digestSz, 1)) {
        return BAD_FUNC_ARG;
    }

    return wc_nuvoton_hw_sha_s(&local);
}

#ifndef NO_AES
NSC_ENTRY int wc_nuvoton_hw_aes(wc_NuvotonAesReq* req)
{
    wc_NuvotonAesReq local;

    /* Copy the request into secure memory first, so the non-secure side
     * cannot change a length or a pointer after it has been checked. */
    if (!nsc_check(req, (word32)sizeof(*req), 1)) {
        return BAD_FUNC_ARG;
    }
    XMEMCPY(&local, req, sizeof(local));

    /* Only in and out are always present. iv is absent for ECB, key is absent
     * when a Key Store slot supplies it, and aad and tag belong to the
     * authenticated modes. nsc_check() reports a NULL or zero-length buffer as
     * a failure, so each optional one is guarded by its own NULL test first -
     * checking them unconditionally rejects every request the port actually
     * makes, and the AES path maps that to a decline, so the accelerator would
     * silently never be used. */
    if (!nsc_check_len((void*)local.in, local.sz, 0) ||
        !nsc_check_len(local.out, local.sz, 1)) {
        return BAD_FUNC_ARG;
    }
    if (local.key != NULL &&
        !nsc_check((void*)local.key, local.keySz, 0)) {
        return BAD_FUNC_ARG;
    }
    /* GCM and CCM carry a nonce of their own length - 12 bytes for GCM, 7 to
     * 13 for CCM - and the engine only reads it. The block modes pass a whole
     * block which is read and written back as the chaining state. Probing a
     * full block of a 7 byte CCM nonce reaches past the caller's buffer, and
     * demanding write access rejects a nonce that legitimately sits in
     * read-only memory. */
    if (local.iv != NULL) {
        int ok;

        if (local.mode == WC_NUVOTON_AES_GCM ||
                local.mode == WC_NUVOTON_AES_CCM) {
            ok = nsc_check_len(local.iv, local.ivSz, 0);
        }
        else {
            ok = nsc_check_len(local.iv, (word32)WC_AES_BLOCK_SIZE, 1);
        }
        if (!ok) {
            return BAD_FUNC_ARG;
        }
    }
    if (local.aad != NULL &&
        !nsc_check_len((void*)local.aad, local.aadSz, 0)) {
        return BAD_FUNC_ARG;
    }
    if (local.tag != NULL && !nsc_check(local.tag, local.tagSz, 1)) {
        return BAD_FUNC_ARG;
    }

    return wc_nuvoton_hw_aes_s(&local);
}
#endif /* !NO_AES */

#ifdef WOLFSSL_NUVOTON_ECC
/* Longest string the ECC requests carry: P-521 is 132 hex characters. */
/* The width of one ECC hex field in the port, NUVOTON_ECC_HEX_SZ in
 * nuvoton_cb_pk.c. They are packed contiguously in one allocation, so probing
 * a wider span than this runs past the object on the last field, and
 * cmse_check_address_range() judges exactly the span it is given. Keep the two
 * in step. */
#define NSC_ECC_STR_MAX 144

/* Which fields the engine writes depends on the operation, and the checks have
 * to follow: an output needs write access across the width the engine will
 * fill, an input needs to be a readable NUL terminated string inside the
 * bound. Getting it backwards either fails a valid request or, worse, lets the
 * secure driver write a span nobody validated. */
#define NSC_ECC_POINT_OUT 0x1   /* qx, qy written (key generation) */
#define NSC_ECC_SIG_OUT   0x2   /* r, s written (sign) */

static int nsc_ecc_strings_ok(wc_NuvotonEccReq* r, int dir)
{
    char*  in[3];
    word32 i;

    in[0] = r->msg;
    in[1] = r->d;
    in[2] = r->k;

    for (i = 0; i < 3; i++) {
        if (in[i] != NULL && !nsc_check_str(in[i], NSC_ECC_STR_MAX, NULL)) {
            return 0;
        }
    }

    /* The public point is an input to verify and to ECDH, and an output of key
     * generation, where the engine writes it into buffers that start zeroed.
     * Checking those as strings passes on the first NUL without ever probing
     * the span the engine is about to write. */
    if ((dir & NSC_ECC_POINT_OUT) != 0) {
        if ((r->qx != NULL && !nsc_check(r->qx, NSC_ECC_STR_MAX, 1)) ||
            (r->qy != NULL && !nsc_check(r->qy, NSC_ECC_STR_MAX, 1))) {
            return 0;
        }
    }
    else {
        if ((r->qx != NULL && !nsc_check_str(r->qx, NSC_ECC_STR_MAX, NULL)) ||
            (r->qy != NULL && !nsc_check_str(r->qy, NSC_ECC_STR_MAX, NULL))) {
            return 0;
        }
    }

    /* r and s are the signature: written by sign, read by verify. */
    if ((dir & NSC_ECC_SIG_OUT) != 0) {
        if ((r->r != NULL && !nsc_check(r->r, NSC_ECC_STR_MAX, 1)) ||
            (r->s != NULL && !nsc_check(r->s, NSC_ECC_STR_MAX, 1))) {
            return 0;
        }
    }
    else {
        if ((r->r != NULL && !nsc_check_str(r->r, NSC_ECC_STR_MAX, NULL)) ||
            (r->s != NULL && !nsc_check_str(r->s, NSC_ECC_STR_MAX, NULL))) {
            return 0;
        }
    }

    /* The shared secret is always an output. */
    if (r->out != NULL && !nsc_check(r->out, NSC_ECC_STR_MAX, 1)) {
        return 0;
    }


    return 1;
}

NSC_ENTRY int wc_nuvoton_hw_ecc_sign(wc_NuvotonEccReq* req)
{
    wc_NuvotonEccReq local;

    if (!nsc_check(req, (word32)sizeof(*req), 1)) {
        return BAD_FUNC_ARG;
    }
    XMEMCPY(&local, req, sizeof(local));

    if (!nsc_ecc_strings_ok(&local, NSC_ECC_SIG_OUT)) {
        return BAD_FUNC_ARG;
    }

    return wc_nuvoton_hw_ecc_sign_s(&local);
}

NSC_ENTRY int wc_nuvoton_hw_ecc_verify(wc_NuvotonEccReq* req)
{
    wc_NuvotonEccReq local;

    if (!nsc_check(req, (word32)sizeof(*req), 1)) {
        return BAD_FUNC_ARG;
    }
    XMEMCPY(&local, req, sizeof(local));

    if (!nsc_ecc_strings_ok(&local, 0)) {
        return BAD_FUNC_ARG;
    }

    return wc_nuvoton_hw_ecc_verify_s(&local);
}

NSC_ENTRY int wc_nuvoton_hw_ecc_shared(wc_NuvotonEccReq* req)
{
    wc_NuvotonEccReq local;

    if (!nsc_check(req, (word32)sizeof(*req), 1)) {
        return BAD_FUNC_ARG;
    }
    XMEMCPY(&local, req, sizeof(local));

    if (!nsc_ecc_strings_ok(&local, 0)) {
        return BAD_FUNC_ARG;
    }

    return wc_nuvoton_hw_ecc_shared_s(&local);
}

NSC_ENTRY int wc_nuvoton_hw_ecc_pubkey(wc_NuvotonEccReq* req)
{
    wc_NuvotonEccReq local;

    if (!nsc_check(req, (word32)sizeof(*req), 1)) {
        return BAD_FUNC_ARG;
    }
    XMEMCPY(&local, req, sizeof(local));

    if (!nsc_ecc_strings_ok(&local, NSC_ECC_POINT_OUT)) {
        return BAD_FUNC_ARG;
    }

    return wc_nuvoton_hw_ecc_pubkey_s(&local);
}
#endif /* WOLFSSL_NUVOTON_ECC */

#ifdef WOLFSSL_NUVOTON_RSA
NSC_ENTRY int wc_nuvoton_hw_rsa(wc_NuvotonRsaReq* req)
{
    wc_NuvotonRsaReq local;
    /* 4096 bits is 1024 hex characters plus the NUL. */
    const word32     strMax = 1088;

    if (!nsc_check(req, (word32)sizeof(*req), 1)) {
        return BAD_FUNC_ARG;
    }
    XMEMCPY(&local, req, sizeof(local));

    if (!nsc_check_str(local.in, strMax, NULL) ||
        !nsc_check_str(local.n, strMax, NULL) ||
        !nsc_check_str(local.e, strMax, NULL)) {
        return BAD_FUNC_ARG;
    }
    if (local.p != NULL && !nsc_check_str(local.p, strMax, NULL)) {
        return BAD_FUNC_ARG;
    }
    if (local.q != NULL && !nsc_check_str(local.q, strMax, NULL)) {
        return BAD_FUNC_ARG;
    }
    if (local.outSz > strMax || !nsc_check(local.out, local.outSz, 1)) {
        return BAD_FUNC_ARG;
    }

    return wc_nuvoton_hw_rsa_s(&local);
}
#endif /* WOLFSSL_NUVOTON_RSA */

#ifdef WOLFSSL_NUVOTON_KS
NSC_ENTRY int wc_nuvoton_hw_ks_write(wc_NuvotonKsWriteReq* req)
{
    wc_NuvotonKsWriteReq local;

    if (!nsc_check(req, (word32)sizeof(*req), 1)) {
        return BAD_FUNC_ARG;
    }
    XMEMCPY(&local, req, sizeof(local));

    if (!nsc_check((void*)local.key, local.keySz, 0)) {
        return BAD_FUNC_ARG;
    }

    return wc_nuvoton_hw_ks_write_s(&local);
}

NSC_ENTRY int wc_nuvoton_hw_ks_read(int keyMem, int keySlot, byte* out,
    word32 outSz)
{
    if (!nsc_check(out, outSz, 1)) {
        return BAD_FUNC_ARG;
    }

    return wc_nuvoton_hw_ks_read_s(keyMem, keySlot, out, outSz);
}

/* Key Store handles are bare indices, so a veneer that forwards whatever index
 * it is handed lets the non-secure world manage keys that belong to the secure
 * world. Erase is bounded here to volatile slots in a window the non-secure
 * side owns.
 *
 * Partitioning by index is the weakest form of this: it is a demonstration,
 * not an ownership model. A product should record which slots it issued to the
 * non-secure world and check membership, because nothing stops non-secure code
 * from asking about a slot inside the window that the secure world also uses. */
#ifndef NSC_KS_NS_SLOT_FIRST
    #define NSC_KS_NS_SLOT_FIRST 0
#endif
#ifndef NSC_KS_NS_SLOT_LAST
    #define NSC_KS_NS_SLOT_LAST  31
#endif

NSC_ENTRY int wc_nuvoton_hw_ks_erase(int keyMem, int keySlot)
{
    /* Only the volatile store, and only the non-secure world's own window. A
     * Flash or OTP slot has no per-key erase anyway. */
    if (keyMem != WC_NUVOTON_KS_SRAM) {
        return BAD_FUNC_ARG;
    }
    if (keySlot < NSC_KS_NS_SLOT_FIRST || keySlot > NSC_KS_NS_SLOT_LAST) {
        return BAD_FUNC_ARG;
    }

    return wc_nuvoton_hw_ks_erase_s(keyMem, keySlot);
}

NSC_ENTRY int wc_nuvoton_hw_ks_revoke(int keyMem, int keySlot)
{
    /* Revoking is permanent and there is no undo, so the non-secure world does
     * not get to do it through this veneer. A design that needs it should
     * revoke from the secure side, where the policy for which keys may be
     * retired lives. */
    (void)keyMem;
    (void)keySlot;

    return BAD_FUNC_ARG;
}
#endif /* WOLFSSL_NUVOTON_KS */
