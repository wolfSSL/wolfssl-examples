/* wolfCrypt AmebaPro2 (RTL8735B) HUK example -- built inside the RealTek
 * FreeRTOS SDK. Registers the wolfCrypt HUK crypto-callback device and runs
 * AES-GCM (full payload) / AES-ECB / AES-CBC / AES-CTR, HMAC-SHA256, and ECDSA
 * P-256 under a key derived from the silicon Hardware Unique Key; the working
 * key never enters software. The 256-bit "seed" passed as the key is HKDF input
 * diversifying the HUK. It also runs regression checks: unaligned-buffer GCM
 * (port bounce path), in-place multi-call CBC, and a non-12-byte GCM IV that
 * must hard-fail (no silent software fallback).
 *
 * Build: configure with -DEXAMPLE=wolfcrypt_huk (see wolfcrypt_huk.cmake).
 */

#include <string.h>

#include "platform_stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "device_lock.h"
#include "hal_trng_sec.h"

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/hmac.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/port/realtek/rtl8735b.h>

/* ForceZero (used to scrub key material below). Inline the misc helpers the same
 * way wolfCrypt's own test.c/benchmark.c do; misc.c provides ForceZero only as an
 * inline, so a plain extern reference would not link. */
#ifdef NO_INLINE
    #include <wolfssl/wolfcrypt/misc.h>
#else
    #define WOLFSSL_MISC_INCLUDED
    #include <wolfcrypt/src/misc.c>
#endif

#define STACKSIZE 8192

#define CHECK(label, cond) \
    dbg_printf("[%s] %s\r\n", (cond) ? "PASS" : "FAIL", (label))

/* wolfCrypt RNG seed hook (user_settings: CUSTOM_RAND_GENERATE_SEED). Fills from
 * the AmebaPro2 secure (self-tested) hardware TRNG -- the same source the HUK
 * port exposes as a crypto-callback SEED. (Using the secure TRNG directly also
 * avoids the non-secure hal_trng wrapper, whose C-cut path references a ROM
 * symbol not resolvable in a from-source SDK build.) */
int rtl8735b_rand_seed(unsigned char* output, unsigned int sz)
{
    static int inited = 0;
    unsigned int i, n;
    u32 r;

    if (inited == 0) {
        if (hal_trng_sec_init() != 0) {
            return -1;
        }
        inited = 1;
    }
    for (i = 0; i < sz; ) {
        r = hal_trng_sec_get_rand();
        n = (sz - i) < 4u ? (sz - i) : 4u;
        memcpy(output + i, &r, n);
        i += n;
    }
    return 0;
}

static void huk_gcm_test(void)
{
    Aes  aes;
    /* HAL crypto engine requires 32-byte-aligned key/iv/aad/tag buffers. */
    byte seed[32] __attribute__((aligned(32)));
    byte iv[12]   __attribute__((aligned(32)));
    byte aad[16]  __attribute__((aligned(32)));
    byte pt[32]   __attribute__((aligned(32)));
    byte ct[32]   __attribute__((aligned(32)));
    byte dec[32]  __attribute__((aligned(32)));
    byte tag[16]  __attribute__((aligned(32)));
    byte tag2[16] __attribute__((aligned(32)));
    int  ret;

    memset(seed, 0xA5, sizeof(seed)); memset(iv, 0x11, sizeof(iv));
    memset(aad, 0x22, sizeof(aad));   memset(pt, 0x33, sizeof(pt));

    dbg_printf("\r\n== AES-GCM (full payload) under HUK-derived key ==\r\n");
    ret = wc_AesInit(&aes, NULL, WC_HUK_DEVID);
    CHECK("AesInit(devId=WC_HUK_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_AesGcmSetKey(&aes, seed, sizeof(seed));
    CHECK("AesGcmSetKey(seed,32)", ret == 0);
    if (ret != 0) {
        wc_AesFree(&aes);
        return;
    }

    ret = wc_AesGcmEncrypt(&aes, ct, pt, sizeof(pt), iv, sizeof(iv),
                           tag, sizeof(tag), aad, sizeof(aad));
    CHECK("AesGcmEncrypt", ret == 0);
    if (ret != 0) {
        wc_AesFree(&aes);
        return;
    }

    memset(tag2, 0, sizeof(tag2));
    ret = wc_AesGcmEncrypt(&aes, ct, pt, sizeof(pt), iv, sizeof(iv),
                           tag2, sizeof(tag2), aad, sizeof(aad));
    CHECK("deterministic tag", ret == 0 && memcmp(tag, tag2, 16) == 0);

    ret = wc_AesGcmDecrypt(&aes, dec, ct, sizeof(ct), iv, sizeof(iv),
                           tag, sizeof(tag), aad, sizeof(aad));
    CHECK("AesGcmDecrypt verifies", ret == 0);
    CHECK("plaintext round-trips", memcmp(dec, pt, sizeof(pt)) == 0);

    seed[0] ^= 0xFF;
    ret = wc_AesGcmSetKey(&aes, seed, sizeof(seed));
    CHECK("AesGcmSetKey(wrong seed)", ret == 0);
    ret = wc_AesGcmDecrypt(&aes, dec, ct, sizeof(ct), iv, sizeof(iv),
                           tag, sizeof(tag), aad, sizeof(aad));
    CHECK("wrong seed -> AES_GCM_AUTH_E", ret == AES_GCM_AUTH_E);
    wc_AesFree(&aes);
    ForceZero(seed, sizeof(seed));   /* scrub key material before return */
}

static void huk_ecb_cbc_test(void)
{
    Aes  aes;
    byte seed[32] __attribute__((aligned(32)));
    byte iv[16]   __attribute__((aligned(32)));
    byte pt[32]   __attribute__((aligned(32)));
    byte ct[32]   __attribute__((aligned(32)));
    byte dec[32]  __attribute__((aligned(32)));
    int  ret;

    memset(seed, 0x5A, sizeof(seed)); memset(iv, 0x44, sizeof(iv));
    memset(pt, 0x77, sizeof(pt));

    dbg_printf("\r\n== AES-ECB under HUK-derived key ==\r\n");
    ret = wc_AesInit(&aes, NULL, WC_HUK_DEVID);
    CHECK("AesInit(devId=WC_HUK_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_AesSetKey(&aes, seed, sizeof(seed), NULL, AES_ENCRYPTION);
    CHECK("AesSetKey(ECB enc)", ret == 0);
    if (ret != 0) {
        wc_AesFree(&aes);
        return;
    }
    ret = wc_AesEcbEncrypt(&aes, ct, pt, sizeof(pt));
    CHECK("AesEcbEncrypt", ret == 0);
    ret = wc_AesSetKey(&aes, seed, sizeof(seed), NULL, AES_DECRYPTION);
    CHECK("AesSetKey(ECB dec)", ret == 0);
    ret = wc_AesEcbDecrypt(&aes, dec, ct, sizeof(ct));
    CHECK("AesEcb round-trip", ret == 0 && memcmp(dec, pt, sizeof(pt)) == 0);
    wc_AesFree(&aes);

    dbg_printf("\r\n== AES-CBC under HUK-derived key ==\r\n");
    ret = wc_AesInit(&aes, NULL, WC_HUK_DEVID);
    CHECK("AesInit(devId=WC_HUK_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_AesSetKey(&aes, seed, sizeof(seed), iv, AES_ENCRYPTION);
    CHECK("AesSetKey(CBC enc)", ret == 0);
    ret = wc_AesCbcEncrypt(&aes, ct, pt, sizeof(pt));
    CHECK("AesCbcEncrypt", ret == 0);
    ret = wc_AesSetKey(&aes, seed, sizeof(seed), iv, AES_DECRYPTION);
    CHECK("AesSetKey(CBC dec)", ret == 0);
    ret = wc_AesCbcDecrypt(&aes, dec, ct, sizeof(ct));
    CHECK("AesCbc round-trip", ret == 0 && memcmp(dec, pt, sizeof(pt)) == 0);
    wc_AesFree(&aes);
    ForceZero(seed, sizeof(seed));   /* scrub key material before return */
}

static void huk_ctr_test(void)
{
    Aes  aes;
    byte seed[32] __attribute__((aligned(32)));
    byte iv[16]   __attribute__((aligned(32)));
    byte pt[20]   __attribute__((aligned(32))); /* non-block-multiple: partial */
    byte ct[20]   __attribute__((aligned(32)));
    byte dec[20]  __attribute__((aligned(32)));
    int  ret;

    memset(seed, 0x5A, sizeof(seed)); memset(iv, 0x66, sizeof(iv));
    memset(pt, 0x99, sizeof(pt));

    dbg_printf("\r\n== AES-CTR under HUK-derived key ==\r\n");
    ret = wc_AesInit(&aes, NULL, WC_HUK_DEVID);
    CHECK("AesInit(devId=WC_HUK_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_AesSetKeyDirect(&aes, seed, sizeof(seed), iv, AES_ENCRYPTION);
    CHECK("AesSetKeyDirect(CTR)", ret == 0);
    if (ret != 0) {
        wc_AesFree(&aes);
        return;
    }
    ret = wc_AesCtrEncrypt(&aes, ct, pt, sizeof(pt));
    CHECK("AesCtrEncrypt", ret == 0);
    CHECK("CTR ciphertext != plaintext", memcmp(ct, pt, sizeof(pt)) != 0);
    wc_AesFree(&aes);

    /* CTR is its own inverse with the same key+IV */
    ret = wc_AesInit(&aes, NULL, WC_HUK_DEVID);
    CHECK("AesInit(devId=WC_HUK_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_AesSetKeyDirect(&aes, seed, sizeof(seed), iv, AES_ENCRYPTION);
    CHECK("AesSetKeyDirect(CTR round-trip)", ret == 0);
    ret = wc_AesCtrEncrypt(&aes, dec, ct, sizeof(ct));
    CHECK("AesCtr round-trip", ret == 0 && memcmp(dec, pt, sizeof(pt)) == 0);
    wc_AesFree(&aes);
    ForceZero(seed, sizeof(seed));   /* scrub key material before return */
}

static void huk_gcm_unaligned_test(void)
{
    Aes   aes;
    /* 32-byte-aligned backing; the +1 views below are deliberately unaligned so
     * the port's bounce-to-aligned path is exercised. */
    byte  buf[7][48] __attribute__((aligned(32)));
    byte* seed = buf[0] + 1;
    byte* iv   = buf[1] + 1;
    byte* aad  = buf[2] + 1;
    byte* pt   = buf[3] + 1;
    byte* ct   = buf[4] + 1;
    byte* dec  = buf[5] + 1;
    byte* tag  = buf[6] + 1;
    int   ret;

    memset(seed, 0xA5, 32); memset(iv, 0x11, 12);
    memset(aad, 0x22, 16);  memset(pt, 0x33, 32);

    dbg_printf("\r\n== AES-GCM with UNALIGNED buffers (port bounces) ==\r\n");
    ret = wc_AesInit(&aes, NULL, WC_HUK_DEVID);
    CHECK("AesInit(devId=WC_HUK_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_AesGcmSetKey(&aes, seed, 32);
    CHECK("AesGcmSetKey (unaligned seed)", ret == 0);
    if (ret != 0) {
        wc_AesFree(&aes);
        return;
    }
    ret = wc_AesGcmEncrypt(&aes, ct, pt, 32, iv, 12, tag, 16, aad, 16);
    CHECK("AesGcmEncrypt (unaligned)", ret == 0);
    ret = wc_AesGcmDecrypt(&aes, dec, ct, 32, iv, 12, tag, 16, aad, 16);
    CHECK("AesGcmDecrypt (unaligned) verifies", ret == 0);
    CHECK("unaligned round-trip", memcmp(dec, pt, 32) == 0);
    wc_AesFree(&aes);
    ForceZero(seed, 32);   /* scrub key material before return */
}

/* In-place CBC decrypt (in == out) across two chained calls -- regression for
 * the in-place chaining / IV-advance fix. */
static void huk_cbc_inplace_test(void)
{
    Aes  aes;
    byte seed[32] __attribute__((aligned(32)));
    byte iv[16]   __attribute__((aligned(32)));
    byte pt[48]   __attribute__((aligned(32)));   /* 3 blocks */
    byte buf[48]  __attribute__((aligned(32)));    /* encrypted then decrypted in place */
    int  ret, i;

    memset(seed, 0x5A, sizeof(seed)); memset(iv, 0x44, sizeof(iv));
    for (i = 0; i < (int)sizeof(pt); i++) {
        pt[i] = (byte)i;
    }
    memcpy(buf, pt, sizeof(pt));

    dbg_printf("\r\n== AES-CBC in-place + multi-call under HUK-derived key ==\r\n");
    ret = wc_AesInit(&aes, NULL, WC_HUK_DEVID);
    CHECK("AesInit(devId=WC_HUK_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_AesSetKey(&aes, seed, sizeof(seed), iv, AES_ENCRYPTION);
    CHECK("AesSetKey(CBC enc)", ret == 0);
    ret = wc_AesCbcEncrypt(&aes, buf, buf, sizeof(buf));   /* in == out */
    CHECK("CBC in-place encrypt", ret == 0);
    wc_AesFree(&aes);

    /* Decrypt in place across two calls (32 then 16 bytes) -- the 2nd call relies
     * on aes->reg having been advanced to the correct last-ciphertext block. */
    ret = wc_AesInit(&aes, NULL, WC_HUK_DEVID);
    CHECK("AesInit(devId=WC_HUK_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_AesSetKey(&aes, seed, sizeof(seed), iv, AES_DECRYPTION);
    CHECK("AesSetKey(CBC dec)", ret == 0);
    ret = wc_AesCbcDecrypt(&aes, buf, buf, 32);
    if (ret == 0) {
        ret = wc_AesCbcDecrypt(&aes, buf + 32, buf + 32, 16);
    }
    CHECK("CBC in-place multi-call decrypt", ret == 0);
    CHECK("CBC in-place round-trip", memcmp(buf, pt, sizeof(pt)) == 0);
    wc_AesFree(&aes);
    ForceZero(seed, sizeof(seed));   /* scrub key material before return */
}

/* A non-12-byte GCM IV must hard-fail, NOT silently fall back to software GCM
 * keyed on the seed -- regression for the IV hard-error fix. */
static void huk_gcm_badiv_test(void)
{
    Aes  aes;
    byte seed[32] __attribute__((aligned(32)));
    byte iv[16]   __attribute__((aligned(32)));   /* 16 bytes, not the supported 12 */
    byte pt[16]   __attribute__((aligned(32)));
    byte ct[16]   __attribute__((aligned(32)));
    byte tag[16]  __attribute__((aligned(32)));
    int  ret;

    memset(seed, 0xA5, sizeof(seed)); memset(iv, 0x11, sizeof(iv));
    memset(pt, 0x33, sizeof(pt));

    dbg_printf("\r\n== AES-GCM non-12-byte IV must hard-fail ==\r\n");
    ret = wc_AesInit(&aes, NULL, WC_HUK_DEVID);
    CHECK("AesInit(devId=WC_HUK_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_AesGcmSetKey(&aes, seed, sizeof(seed));
    CHECK("AesGcmSetKey(seed,32)", ret == 0);
    if (ret != 0) {
        wc_AesFree(&aes);
        return;
    }
    ret = wc_AesGcmEncrypt(&aes, ct, pt, sizeof(pt), iv, sizeof(iv),
                           tag, sizeof(tag), NULL, 0);
    CHECK("16-byte IV rejected (no silent SW fallback)", ret != 0);
    wc_AesFree(&aes);
    ForceZero(seed, sizeof(seed));   /* scrub key material before return */
}

/* HMAC-SHA256 under the HUK-derived key: same seed -> same MAC (deterministic),
 * a different seed -> different MAC. The 32-byte seed is the HMAC key; the port
 * runs the MAC over the HUK-derived secure-key slot (key never in software). */
static void huk_hmac_test(void)
{
    Hmac hmac;
    byte seed[32] __attribute__((aligned(32)));
    byte msg[40]  __attribute__((aligned(32)));
    byte mac1[WC_SHA256_DIGEST_SIZE];
    byte mac2[WC_SHA256_DIGEST_SIZE];
    byte mac3[WC_SHA256_DIGEST_SIZE];
    int  ret;

    memset(seed, 0x5A, sizeof(seed));
    memset(msg, 0x33, sizeof(msg));

    dbg_printf("\r\n== HMAC-SHA256 under HUK-derived key ==\r\n");

    ret = wc_HmacInit(&hmac, NULL, WC_HUK_DEVID);
    CHECK("HmacInit(devId=WC_HUK_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_HmacSetKey(&hmac, WC_SHA256, seed, sizeof(seed));
    CHECK("HmacSetKey(seed,32)", ret == 0);
    if (ret == 0) {
        ret = wc_HmacUpdate(&hmac, msg, sizeof(msg));
    }
    if (ret == 0) {
        ret = wc_HmacFinal(&hmac, mac1);
    }
    CHECK("HmacFinal", ret == 0);
    wc_HmacFree(&hmac);
    if (ret != 0) {
        return;
    }

    /* Same seed + message -> identical MAC. */
    ret = wc_HmacInit(&hmac, NULL, WC_HUK_DEVID);
    if (ret == 0) {
        ret = wc_HmacSetKey(&hmac, WC_SHA256, seed, sizeof(seed));
    }
    if (ret == 0) {
        ret = wc_HmacUpdate(&hmac, msg, sizeof(msg));
    }
    if (ret == 0) {
        ret = wc_HmacFinal(&hmac, mac2);
    }
    wc_HmacFree(&hmac);
    CHECK("deterministic MAC", ret == 0 &&
          memcmp(mac1, mac2, sizeof(mac1)) == 0);

    /* Different seed -> different MAC. */
    seed[0] ^= 0xFF;
    ret = wc_HmacInit(&hmac, NULL, WC_HUK_DEVID);
    if (ret == 0) {
        ret = wc_HmacSetKey(&hmac, WC_SHA256, seed, sizeof(seed));
    }
    if (ret == 0) {
        ret = wc_HmacUpdate(&hmac, msg, sizeof(msg));
    }
    if (ret == 0) {
        ret = wc_HmacFinal(&hmac, mac3);
    }
    wc_HmacFree(&hmac);
    CHECK("wrong seed -> different MAC", ret == 0 &&
          memcmp(mac1, mac3, sizeof(mac1)) != 0);
    ForceZero(seed, sizeof(seed));   /* scrub key material before return */
}

/* HUK-bound ECDSA P-256 sign. A key is generated in software, its private
 * scalar wrapped under the HUK (ECB-encrypted with the HUK device using the
 * seed), then signing goes through the HUK device (which unwraps under the HUK
 * and signs). The signature is checked against the software public key. To
 * exercise the HW ECDSA engine instead of the software-after-unwrap path, set
 * hk.useHwEngine = 1 (requires on-silicon validation). */
static void huk_ecdsa_test(void)
{
    Aes    aes;
    ecc_key key;
    ecc_key huk;
    ecc_key hwpub;
    WC_RNG  rng;
    wc_Rtl8735b_EccKey hk;
    byte    seed[32]    __attribute__((aligned(32)));
    byte    d[32]       __attribute__((aligned(32)));
    byte    wrapped[32] __attribute__((aligned(32)));
    byte    wrapIv[12]  __attribute__((aligned(32)));   /* GCM nonce for the wrap */
    byte    wrapTag[16] __attribute__((aligned(32)));   /* GCM auth tag */
    byte    hash[32]    __attribute__((aligned(32)));
    byte    qx[32];
    byte    qy[32];
    byte    sig[80];
    word32  dLen = sizeof(d);
    word32  qxLen = sizeof(qx);
    word32  qyLen = sizeof(qy);
    word32  sigLen = sizeof(sig);
    int     ret;
    int     verified = 0;
    int     hwVerified = 0;
    int     rngOk = 0;

    memset(seed, 0x6B, sizeof(seed));
    memset(hash, 0x42, sizeof(hash));
    memset(d, 0, sizeof(d));
    /* Zero the key structs up front so the single cleanup can wc_ecc_free them
     * safely even if an earlier step fails before they are initialized. */
    memset(&key, 0, sizeof(key));
    memset(&huk, 0, sizeof(huk));
    memset(&hwpub, 0, sizeof(hwpub));

    dbg_printf("\r\n== ECDSA P-256 under HUK-derived key ==\r\n");

    ret = wc_InitRng(&rng);
    CHECK("InitRng", ret == 0);
    if (ret != 0) {
        goto cleanup;
    }
    rngOk = 1;
    ret = wc_ecc_init(&key);
    if (ret == 0) {
        ret = wc_ecc_make_key(&rng, 32, &key);
    }
    CHECK("ecc_make_key (P-256)", ret == 0);
    if (ret != 0) {
        goto cleanup;
    }
    ret = wc_ecc_export_private_only(&key, d, &dLen);
    CHECK("export private scalar (32B)", ret == 0 && dLen == 32);
    if (ret != 0 || dLen != 32) {
        goto cleanup;
    }

    /* Wrap the scalar under the HUK: AES-GCM-encrypt with the HUK device + seed
     * so the blob is authenticated (a tampered/wrong blob fails at unwrap). */
    memset(wrapIv, 0x24, sizeof(wrapIv));   /* demo nonce; use a fresh one/provision */
    ret = wc_AesInit(&aes, NULL, WC_HUK_DEVID);
    if (ret == 0) {
        ret = wc_AesGcmSetKey(&aes, seed, sizeof(seed));
    }
    if (ret == 0) {
        ret = wc_AesGcmEncrypt(&aes, wrapped, d, sizeof(d), wrapIv, sizeof(wrapIv),
                               wrapTag, sizeof(wrapTag), NULL, 0);
    }
    wc_AesFree(&aes);
    CHECK("wrap scalar under HUK (GCM)", ret == 0);
    if (ret != 0) {
        goto cleanup;
    }

    /* Sign via the HUK-bound key (GCM-unwraps under the HUK, then signs). */
    memset(&hk, 0, sizeof(hk));
    hk.seed = seed;       hk.seedSz = sizeof(seed);
    hk.wrapped = wrapped; hk.wrappedLen = 32;
    hk.plainLen = 32;
    hk.iv = wrapIv;       hk.ivSz = sizeof(wrapIv);
    hk.tag = wrapTag;     hk.tagSz = sizeof(wrapTag);
    /* Route the sign through the HW ECDSA engine (hal_ecdsa, validated on the
     * RTL8735B). Set 0 for the software-after-unwrap path (the port default).
     * hk.otpPrkSel can instead select an OTP-resident key (scalar never in
     * software); see wolfcrypt/src/port/realtek/README.md. */
    hk.useHwEngine = 1;

    ret = wc_ecc_init_ex(&huk, NULL, WC_HUK_DEVID);
    if (ret == 0) {
        ret = wc_ecc_set_curve(&huk, 32, ECC_SECP256R1);
    }
    if (ret == 0) {
        huk.devCtx = &hk;
        ret = wc_ecc_sign_hash(hash, sizeof(hash), sig, &sigLen, &rng, &huk);
    }
    CHECK("HUK ECDSA sign", ret == 0);

    if (ret == 0) {
        ret = wc_ecc_verify_hash(sig, sigLen, hash, sizeof(hash), &verified,
                                 &key);
    }
    CHECK("verify with software public key", ret == 0 && verified == 1);

    /* Verify the same signature through the HW ECDSA engine (general verify
     * offload): import the public point into a WC_HUK_DEVID key so the verify
     * dispatches to hal_ecdsa. A good signature -> res = 1; a tampered hash
     * must -> res = 0 (and still return 0, since a bad signature is not an
     * error). No HUK context is needed for verify -- any P-256 public key. */
    ret = wc_ecc_export_public_raw(&key, qx, &qxLen, qy, &qyLen);
    CHECK("export public point", ret == 0);
    if (ret == 0) {
        ret = wc_ecc_init_ex(&hwpub, NULL, WC_HUK_DEVID);
    }
    if (ret == 0) {
        ret = wc_ecc_import_unsigned(&hwpub, qx, qy, NULL, ECC_SECP256R1);
    }
    if (ret == 0) {
        hwVerified = 0;
        ret = wc_ecc_verify_hash(sig, sigLen, hash, sizeof(hash), &hwVerified,
                                 &hwpub);
    }
    CHECK("HW ECDSA verify (good sig)", ret == 0 && hwVerified == 1);

    if (ret == 0) {
        hash[0] ^= 0xFF;     /* tamper the digest */
        hwVerified = 1;
        ret = wc_ecc_verify_hash(sig, sigLen, hash, sizeof(hash), &hwVerified,
                                 &hwpub);
        hash[0] ^= 0xFF;     /* restore */
    }
    CHECK("HW ECDSA verify (tampered -> reject)", ret == 0 && hwVerified == 0);

cleanup:
    wc_ecc_free(&hwpub);
    wc_ecc_free(&huk);
    wc_ecc_free(&key);
    if (rngOk != 0) {
        wc_FreeRng(&rng);
    }
    /* Scrub the plaintext private scalar, its wrapped form, and the HKDF seed
     * (zero secrets with ForceZero, not memset, per wolfSSL convention). */
    ForceZero(d, sizeof(d));
    ForceZero(wrapped, sizeof(wrapped));
    ForceZero(seed, sizeof(seed));
}

static void wolf_huk_thread(void* param)
{
    int ret;
    (void)param;

    dbg_printf("\r\n=== wolfCrypt AmebaPro2 (RTL8735B) HUK example ===\r\n");

    device_mutex_lock(RT_DEV_LOCK_CRYPTO);

    ret = wolfCrypt_Init();
    CHECK("wolfCrypt_Init", ret == 0);

    if (ret == 0) {
        ret = wc_Rtl8735b_HukRegister(WC_HUK_DEVID);
        CHECK("wc_Rtl8735b_HukRegister", ret == 0);
    }
    if (ret == 0) {
        huk_gcm_test();
        huk_ecb_cbc_test();
        huk_ctr_test();
        huk_gcm_unaligned_test();
        huk_cbc_inplace_test();
        huk_gcm_badiv_test();
        huk_hmac_test();
        huk_ecdsa_test();
        wc_Rtl8735b_HukUnRegister(WC_HUK_DEVID);
    }
    wolfCrypt_Cleanup();

    device_mutex_unlock(RT_DEV_LOCK_CRYPTO);
    dbg_printf("\r\n=== done ===\r\n");
    vTaskDelete(NULL);
}

int main(void)
{
    if (xTaskCreate(wolf_huk_thread, "wolf_huk", STACKSIZE, NULL,
                    tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        dbg_printf("xTaskCreate failed\r\n");
    }
    else {
        vTaskStartScheduler();
    }
    while (1) {
    }
}
