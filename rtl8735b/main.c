/* wolfCrypt AmebaPro2 (RTL8735B) HUK example -- built inside the RealTek
 * FreeRTOS SDK. Registers the wolfCrypt HUK crypto-callback device and runs
 * AES-GCM (full payload) / AES-ECB / AES-CBC / AES-CTR, HMAC-SHA256, and ECDSA
 * P-256 under a key derived from the silicon Hardware Unique Key; the working
 * key never enters software. The 256-bit "seed" passed as the key is HKDF input
 * diversifying the HUK. It also runs regression checks: unaligned-buffer GCM
 * (port bounce path), in-place multi-call CBC, and a non-12-byte GCM IV that
 * must hard-fail (no silent software fallback).
 *
 * With WOLFSSL_RTL8735B_AES also enabled it registers a second crypto-callback
 * device (WC_RTL8735B_AES_DEVID) that runs a caller-supplied plaintext AES key
 * directly on the HW engine (no HUK binding), selected per Aes by devId, and
 * exercises it with AES-GCM (published KAT + coexistence), an AES-ECB FIPS-197
 * KAT, the unaligned-buffer bounce path, and software GCM for a non-12-byte IV.
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
 * scalar wrapped under the HUK (AES-GCM-encrypted with the HUK device using the
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

/* A HUK-wrapped P-256 key in the form an application stores it, for example in
 * flash. The seed is not secret, and only the device that wrapped the scalar can
 * unwrap it. The scalar is wrapped with AES-GCM (authenticated, recommended) or
 * AES-ECB (the same blob layout as the STM32 DHUK, for code shared between the
 * two parts). An ECB blob has no IV or tag. */
#define HUK_WRAP_GCM 0
#define HUK_WRAP_ECB 1

typedef struct huk_key_blob {
    byte format;            /* HUK_WRAP_GCM or HUK_WRAP_ECB */
    byte seed[32];
    byte iv[12];
    byte tag[16];
    byte wrapped[32];
    byte qx[32];
    byte qy[32];
} huk_key_blob;

/* Wrap a 32-byte scalar under the HUK with the blob's seed (and IV for GCM). */
static int huk_blob_wrap(huk_key_blob* blob, const byte* d)
{
    Aes aes;
    int ret;

    ret = wc_AesInit(&aes, NULL, WC_HUK_DEVID);
    if (ret != 0) {
        return ret;
    }
    if (blob->format == HUK_WRAP_ECB) {
        ret = wc_AesSetKey(&aes, blob->seed, sizeof(blob->seed), NULL,
                           AES_ENCRYPTION);
        if (ret == 0) {
            ret = wc_AesEcbEncrypt(&aes, blob->wrapped, d,
                                   sizeof(blob->wrapped));
        }
    }
    else {
        ret = wc_AesGcmSetKey(&aes, blob->seed, sizeof(blob->seed));
        if (ret == 0) {
            ret = wc_AesGcmEncrypt(&aes, blob->wrapped, d,
                                   sizeof(blob->wrapped), blob->iv,
                                   sizeof(blob->iv), blob->tag,
                                   sizeof(blob->tag), NULL, 0);
        }
    }
    wc_AesFree(&aes);
    return ret;
}

/* Generate a P-256 key and wrap its scalar under the HUK with a fresh seed (and
 * a fresh IV for GCM). */
static int huk_blob_create(WC_RNG* rng, byte format, huk_key_blob* blob)
{
    ecc_key key;
    byte    d[32] __attribute__((aligned(32)));
    word32  dLen = sizeof(d);
    word32  qxLen = sizeof(blob->qx);
    word32  qyLen = sizeof(blob->qy);
    int     ret;

    memset(blob, 0, sizeof(*blob));
    blob->format = format;
    ret = wc_ecc_init(&key);
    if (ret != 0) {
        return ret;
    }
    ret = wc_ecc_make_key(rng, sizeof(d), &key);
    if (ret == 0) {
        ret = wc_ecc_export_private_only(&key, d, &dLen);
    }
    if (ret == 0 && dLen != sizeof(d)) {
        ret = BUFFER_E;
    }
    if (ret == 0) {
        ret = wc_ecc_export_public_raw(&key, blob->qx, &qxLen, blob->qy, &qyLen);
    }
    if (ret == 0) {
        ret = wc_RNG_GenerateBlock(rng, blob->seed, sizeof(blob->seed));
    }
    if (ret == 0 && format == HUK_WRAP_GCM) {
        ret = wc_RNG_GenerateBlock(rng, blob->iv, sizeof(blob->iv));
    }
    if (ret == 0) {
        ret = huk_blob_wrap(blob, d);
    }
    wc_ecc_free(&key);
    ForceZero(d, sizeof(d));
    return ret;
}

/* Set up a signing key from a stored blob. The key borrows hk and the blob, so
 * both must stay valid until wc_ecc_free. For an ECB blob the iv and tag fields
 * stay NULL/0, which is how the port tells the two formats apart. */
static int huk_blob_load(const huk_key_blob* blob, wc_Rtl8735b_EccKey* hk,
                         ecc_key* key)
{
    int ret;

    memset(hk, 0, sizeof(*hk));
    hk->seed = blob->seed;       hk->seedSz = sizeof(blob->seed);
    hk->wrapped = blob->wrapped; hk->wrappedLen = sizeof(blob->wrapped);
    hk->plainLen = sizeof(blob->wrapped);
    if (blob->format == HUK_WRAP_GCM) {
        hk->iv = blob->iv;       hk->ivSz = sizeof(blob->iv);
        hk->tag = blob->tag;     hk->tagSz = sizeof(blob->tag);
    }

    ret = wc_ecc_init_ex(key, NULL, WC_HUK_DEVID);
    if (ret == 0) {
        ret = wc_ecc_set_curve(key, sizeof(blob->wrapped), ECC_SECP256R1);
    }
    if (ret == 0) {
        key->devCtx = hk;
    }
    return ret;
}

/* Sign with a blob, using a key object set up only from what was stored. */
static int huk_blob_sign(const huk_key_blob* blob, WC_RNG* rng, const byte* hash,
                         word32 hashSz, byte* sig, word32* sigLen)
{
    wc_Rtl8735b_EccKey hk;
    ecc_key key;
    int     ret;

    ret = huk_blob_load(blob, &hk, &key);
    if (ret == 0) {
        ret = wc_ecc_sign_hash(hash, hashSz, sig, sigLen, rng, &key);
    }
    wc_ecc_free(&key);
    return ret;
}

/* Verify a signature with the blob's stored public key. Returns 1 if valid. */
static int huk_blob_verify(const huk_key_blob* blob, const byte* hash,
                           word32 hashSz, const byte* sig, word32 sigLen)
{
    ecc_key pub;
    int     verified = 0;
    int     ret;

    ret = wc_ecc_init(&pub);
    if (ret == 0) {
        ret = wc_ecc_import_unsigned(&pub, blob->qx, blob->qy, NULL,
                                     ECC_SECP256R1);
        if (ret == 0) {
            ret = wc_ecc_verify_hash(sig, sigLen, hash, hashSz, &verified,
                                     &pub);
        }
        wc_ecc_free(&pub);
    }
    return (ret == 0 && verified == 1);
}

/* Wrap a key, reload it from the stored blob, and sign with it, for both wrap
 * formats. A real application writes the blob to flash and reloads it after a
 * reset. */
static void huk_ecdsa_reload_test(void)
{
    static huk_key_blob blob;
    WC_RNG  rng;
    byte    hash[32];
    byte    sig[80];
    word32  sigLen;
    int     format;
    int     ret;

    dbg_printf("\r\n== HUK-wrapped ECDSA key: wrap, store, reload, sign ==\r\n");

    memset(hash, 0x42, sizeof(hash));
    ret = wc_InitRng(&rng);
    CHECK("InitRng", ret == 0);
    if (ret != 0) {
        return;
    }
    for (format = HUK_WRAP_GCM; format <= HUK_WRAP_ECB; format++) {
        const char* name = (format == HUK_WRAP_GCM) ? "GCM" : "ECB";

        dbg_printf("-- %s-wrapped blob --\r\n", name);
        ret = huk_blob_create(&rng, (byte)format, &blob);
        CHECK("create wrapped key blob", ret == 0);
        if (ret != 0) {
            continue;
        }
        sigLen = sizeof(sig);
        ret = huk_blob_sign(&blob, &rng, hash, sizeof(hash), sig, &sigLen);
        CHECK("sign with reloaded blob", ret == 0);
        if (ret == 0) {
            CHECK("verify with stored public key",
                  huk_blob_verify(&blob, hash, sizeof(hash), sig, sigLen));
        }

        /* A damaged GCM blob cannot produce a key: the tag does not match. A
         * damaged ECB blob unwraps to a wrong scalar, so it signs, but the
         * signature does not verify. Neither covers qx/qy. */
        sigLen = sizeof(sig);
        if (format == HUK_WRAP_GCM) {
            blob.tag[0] ^= 0x01;
            ret = huk_blob_sign(&blob, &rng, hash, sizeof(hash), sig, &sigLen);
            CHECK("tampered blob tag -> AES_GCM_AUTH_E", ret == AES_GCM_AUTH_E);
        }
        else {
            blob.wrapped[0] ^= 0x01;
            ret = huk_blob_sign(&blob, &rng, hash, sizeof(hash), sig, &sigLen);
            CHECK("tampered ECB blob signs, signature does not verify",
                  ret == 0 &&
                  !huk_blob_verify(&blob, hash, sizeof(hash), sig, sigLen));
        }
    }
    wc_FreeRng(&rng);
}

#ifdef WOLFSSL_RTL8735B_AES
/* AES-GCM with a plaintext key through the second device (WC_RTL8735B_AES_DEVID).
 * Unlike the HUK device, the key bytes are the actual AES key, so the output
 * matches a published GCM vector. Then the same 32 key bytes are run through
 * both devices to show they coexist and are selected per Aes by devId: the
 * plaintext-key ciphertext differs from the HUK device-bound ciphertext. */
static void plain_gcm_test(void)
{
    Aes  aes;
    /* McGrew & Viega GCM test case 3: 128-bit key, 64-byte payload, no AAD. */
    static const byte katKey[16] = {
        0xfe,0xff,0xe9,0x92,0x86,0x65,0x73,0x1c,
        0x6d,0x6a,0x8f,0x94,0x67,0x30,0x83,0x08
    };
    static const byte katIv[12] = {
        0xca,0xfe,0xba,0xbe,0xfa,0xce,0xdb,0xad,0xde,0xca,0xf8,0x88
    };
    static const byte katPt[64] = {
        0xd9,0x31,0x32,0x25,0xf8,0x84,0x06,0xe5,
        0xa5,0x59,0x09,0xc5,0xaf,0xf5,0x26,0x9a,
        0x86,0xa7,0xa9,0x53,0x15,0x34,0xf7,0xda,
        0x2e,0x4c,0x30,0x3d,0x8a,0x31,0x8a,0x72,
        0x1c,0x3c,0x0c,0x95,0x95,0x68,0x09,0x53,
        0x2f,0xcf,0x0e,0x24,0x49,0xa6,0xb5,0x25,
        0xb1,0x6a,0xed,0xf5,0xaa,0x0d,0xe6,0x57,
        0xba,0x63,0x7b,0x39,0x1a,0xaf,0xd2,0x55
    };
    static const byte katCt[64] = {
        0x42,0x83,0x1e,0xc2,0x21,0x77,0x74,0x24,
        0x4b,0x72,0x21,0xb7,0x84,0xd0,0xd4,0x9c,
        0xe3,0xaa,0x21,0x2f,0x2c,0x02,0xa4,0xe0,
        0x35,0xc1,0x7e,0x23,0x29,0xac,0xa1,0x2e,
        0x21,0xd5,0x14,0xb2,0x54,0x66,0x93,0x1c,
        0x7d,0x8f,0x6a,0x5a,0xac,0x84,0xaa,0x05,
        0x1b,0xa3,0x0b,0x39,0x6a,0x0a,0xac,0x97,
        0x3d,0x58,0xe0,0x91,0x47,0x3f,0x59,0x85
    };
    static const byte katTag[16] = {
        0x4d,0x5c,0x2a,0xf3,0x27,0xcd,0x64,0xa6,
        0x2c,0xf3,0x5a,0xbd,0x2b,0xa6,0xfa,0xb4
    };
    /* HAL crypto engine requires 32-byte-aligned key/iv/buffers. */
    byte key[32]    __attribute__((aligned(32)));
    byte iv[12]     __attribute__((aligned(32)));
    byte pt[64]     __attribute__((aligned(32)));
    byte ct[64]     __attribute__((aligned(32)));
    byte ctHuk[64]  __attribute__((aligned(32)));
    byte dec[64]    __attribute__((aligned(32)));
    byte tag[16]    __attribute__((aligned(32)));
    byte tagHuk[16] __attribute__((aligned(32)));
    int  ret;

    dbg_printf("\r\n== AES-GCM with a plaintext key "
               "(devId=WC_RTL8735B_AES_DEVID) ==\r\n");

    /* Known-answer test: the plaintext key must reproduce the published vector.
     * Stage the vector key into the 32-byte-aligned buffer the HAL DMA wants. */
    memcpy(key, katKey, sizeof(katKey));
    memcpy(iv, katIv, sizeof(iv));
    memcpy(pt, katPt, sizeof(pt));
    ret = wc_AesInit(&aes, NULL, WC_RTL8735B_AES_DEVID);
    CHECK("AesInit(devId=WC_RTL8735B_AES_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_AesGcmSetKey(&aes, key, sizeof(katKey));
    CHECK("AesGcmSetKey(plaintext 128-bit key)", ret == 0);
    if (ret == 0) {
        ret = wc_AesGcmEncrypt(&aes, ct, pt, sizeof(pt), iv, sizeof(iv),
                               tag, sizeof(tag), NULL, 0);
        CHECK("AesGcmEncrypt", ret == 0);
    }
    CHECK("ciphertext matches published vector",
          ret == 0 && memcmp(ct, katCt, sizeof(katCt)) == 0);
    CHECK("tag matches published vector",
          ret == 0 && memcmp(tag, katTag, sizeof(katTag)) == 0);
    if (ret == 0) {
        ret = wc_AesGcmDecrypt(&aes, dec, ct, sizeof(ct), iv, sizeof(iv),
                               tag, sizeof(tag), NULL, 0);
        CHECK("AesGcmDecrypt verifies", ret == 0);
        CHECK("plaintext round-trips", memcmp(dec, pt, sizeof(pt)) == 0);

        /* Negative: a tampered tag must be rejected by the HW decrypt-verify. */
        tag[0] ^= 0xFFu;
        ret = wc_AesGcmDecrypt(&aes, dec, ct, sizeof(ct), iv, sizeof(iv),
                               tag, sizeof(tag), NULL, 0);
        CHECK("tampered tag -> AES_GCM_AUTH_E", ret == AES_GCM_AUTH_E);
    }
    wc_AesFree(&aes);

    /* Coexistence: same 32 key bytes -> plaintext device vs HUK device produce
     * different ciphertext (one uses the key verbatim, the other as a seed). */
    memset(key, 0x5A, sizeof(key));
    ret = wc_AesInit(&aes, NULL, WC_RTL8735B_AES_DEVID);
    CHECK("AesInit(plaintext devId)", ret == 0);
    if (ret == 0) {
        ret = wc_AesGcmSetKey(&aes, key, sizeof(key));
        CHECK("AesGcmSetKey(plaintext 256-bit key)", ret == 0);
    }
    if (ret == 0) {
        ret = wc_AesGcmEncrypt(&aes, ct, pt, sizeof(pt), iv, sizeof(iv),
                               tag, sizeof(tag), NULL, 0);
    }
    wc_AesFree(&aes);
    CHECK("plaintext-key encrypt (256-bit)", ret == 0);
    if (ret == 0) {
        ret = wc_AesInit(&aes, NULL, WC_HUK_DEVID);
        CHECK("AesInit(HUK devId)", ret == 0);
        if (ret == 0) {
            ret = wc_AesGcmSetKey(&aes, key, sizeof(key));
            CHECK("AesGcmSetKey(HUK seed, same 32 bytes)", ret == 0);
        }
        if (ret == 0) {
            ret = wc_AesGcmEncrypt(&aes, ctHuk, pt, sizeof(pt), iv, sizeof(iv),
                                   tagHuk, sizeof(tagHuk), NULL, 0);
        }
        wc_AesFree(&aes);
        CHECK("HUK-seed encrypt (same 32 bytes)", ret == 0);
        CHECK("plaintext CT != HUK CT (distinct keys per devId)",
              ret == 0 && memcmp(ct, ctHuk, sizeof(ct)) != 0);
    }
    ForceZero(key, sizeof(key));   /* scrub key material before return */
}

/* AES-ECB with a plaintext key: FIPS-197 appendix C.1 known-answer vector. The
 * plaintext device runs the key verbatim, so the HW output must match the
 * published ciphertext exactly (the HUK device cannot -- its key is derived). */
static void plain_ecb_test(void)
{
    Aes  aes;
    static const byte katKey[16] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
    };
    static const byte katPt[16] = {
        0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
        0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff
    };
    static const byte katCt[16] = {
        0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,
        0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a
    };
    byte key[16] __attribute__((aligned(32)));
    byte pt[16]  __attribute__((aligned(32)));
    byte ct[16]  __attribute__((aligned(32)));
    byte dec[16] __attribute__((aligned(32)));
    int  ret;

    memcpy(key, katKey, sizeof(key));
    memcpy(pt, katPt, sizeof(pt));

    dbg_printf("\r\n== AES-ECB with a plaintext key (FIPS-197 KAT) ==\r\n");
    ret = wc_AesInit(&aes, NULL, WC_RTL8735B_AES_DEVID);
    CHECK("AesInit(devId=WC_RTL8735B_AES_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_AesSetKey(&aes, key, sizeof(key), NULL, AES_ENCRYPTION);
    CHECK("AesSetKey(ECB enc)", ret == 0);
    if (ret == 0) {
        ret = wc_AesEcbEncrypt(&aes, ct, pt, sizeof(pt));
        CHECK("AesEcbEncrypt", ret == 0);
    }
    CHECK("ciphertext matches FIPS-197 vector",
          ret == 0 && memcmp(ct, katCt, sizeof(katCt)) == 0);
    ret = wc_AesSetKey(&aes, key, sizeof(key), NULL, AES_DECRYPTION);
    CHECK("AesSetKey(ECB dec)", ret == 0);
    if (ret == 0) {
        ret = wc_AesEcbDecrypt(&aes, dec, ct, sizeof(ct));
    }
    CHECK("AesEcb round-trip", ret == 0 && memcmp(dec, pt, sizeof(pt)) == 0);
    wc_AesFree(&aes);
    ForceZero(key, sizeof(key));   /* scrub key material before return */
}

/* AES-GCM with a plaintext key over deliberately-unaligned caller buffers, so
 * the port's bounce-to-aligned path is exercised for the second device too. */
static void plain_gcm_unaligned_test(void)
{
    Aes   aes;
    /* 32-byte-aligned backing; the +1 views below are deliberately unaligned. */
    byte  buf[6][48] __attribute__((aligned(32)));
    byte* key = buf[0] + 1;
    byte* iv  = buf[1] + 1;
    byte* pt  = buf[2] + 1;
    byte* ct  = buf[3] + 1;
    byte* dec = buf[4] + 1;
    byte* tag = buf[5] + 1;
    int   ret;

    memset(key, 0x5A, 16); memset(iv, 0x11, 12);
    memset(pt, 0x33, 32);

    dbg_printf("\r\n== AES-GCM plaintext key, UNALIGNED buffers "
               "(port bounces) ==\r\n");
    ret = wc_AesInit(&aes, NULL, WC_RTL8735B_AES_DEVID);
    CHECK("AesInit(devId=WC_RTL8735B_AES_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_AesGcmSetKey(&aes, key, 16);
    CHECK("AesGcmSetKey (unaligned key)", ret == 0);
    if (ret != 0) {
        wc_AesFree(&aes);
        return;
    }
    ret = wc_AesGcmEncrypt(&aes, ct, pt, 32, iv, 12, tag, 16, NULL, 0);
    CHECK("AesGcmEncrypt (unaligned)", ret == 0);
    ret = wc_AesGcmDecrypt(&aes, dec, ct, 32, iv, 12, tag, 16, NULL, 0);
    CHECK("AesGcmDecrypt (unaligned) verifies", ret == 0);
    CHECK("unaligned round-trip", memcmp(dec, pt, 32) == 0);
    wc_AesFree(&aes);
    ForceZero(key, 16);   /* scrub key material before return */
}

/* The HW path only takes a 12-byte IV, so the plaintext device declines any other
 * length and software GCM serves it with the same literal key. */
static void plain_gcm_long_iv_test(void)
{
    Aes  aes;
    byte key[16] __attribute__((aligned(32)));
    byte iv[16]  __attribute__((aligned(32)));   /* 16 bytes, not the supported 12 */
    byte pt[16]  __attribute__((aligned(32)));
    byte ct[16]  __attribute__((aligned(32)));
    byte tag[16] __attribute__((aligned(32)));
    byte swCt[16];
    byte swTag[16];
    int  ret;

    memset(key, 0x5A, sizeof(key)); memset(iv, 0x11, sizeof(iv));
    memset(pt, 0x33, sizeof(pt));

    dbg_printf("\r\n== AES-GCM plaintext key, non-12-byte IV uses software GCM "
               "==\r\n");
    ret = wc_AesInit(&aes, NULL, WC_RTL8735B_AES_DEVID);
    CHECK("AesInit(devId=WC_RTL8735B_AES_DEVID)", ret == 0);
    if (ret != 0) {
        return;
    }
    ret = wc_AesGcmSetKey(&aes, key, sizeof(key));
    CHECK("AesGcmSetKey(key,16)", ret == 0);
    if (ret != 0) {
        wc_AesFree(&aes);
        return;
    }
    ret = wc_AesGcmEncrypt(&aes, ct, pt, sizeof(pt), iv, sizeof(iv),
                           tag, sizeof(tag), NULL, 0);
    CHECK("AesGcmEncrypt (16-byte IV)", ret == 0);
    wc_AesFree(&aes);
    if (ret == 0) {
        ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
        if (ret == 0) {
            ret = wc_AesGcmSetKey(&aes, key, sizeof(key));
            if (ret == 0) {
                ret = wc_AesGcmEncrypt(&aes, swCt, pt, sizeof(pt), iv, sizeof(iv),
                                       swTag, sizeof(swTag), NULL, 0);
            }
            wc_AesFree(&aes);
        }
        CHECK("16-byte IV matches software GCM with the same key",
              ret == 0 && memcmp(ct, swCt, sizeof(ct)) == 0 &&
              memcmp(tag, swTag, sizeof(tag)) == 0);
    }
    ForceZero(key, sizeof(key));   /* scrub key material before return */
}
#endif /* WOLFSSL_RTL8735B_AES */

static void wolf_huk_thread(void* param)
{
    int ret;
#ifdef WOLFSSL_RTL8735B_AES
    int aesRet = 0;
#endif
    (void)param;

    dbg_printf("\r\n=== wolfCrypt AmebaPro2 (RTL8735B) HUK example ===\r\n");

    device_mutex_lock(RT_DEV_LOCK_CRYPTO);

    ret = wolfCrypt_Init();
    CHECK("wolfCrypt_Init", ret == 0);

    if (ret == 0) {
        ret = wc_Rtl8735b_HukRegister(WC_HUK_DEVID);
        CHECK("wc_Rtl8735b_HukRegister", ret == 0);
    }
#ifdef WOLFSSL_RTL8735B_AES
    /* The plaintext-key AES device is an optional add-on; track its registration
     * in its own result so a failure only skips its demo -- it must never
     * suppress the HUK suite or leak the HUK device registration. */
    if (ret == 0) {
        aesRet = wc_Rtl8735b_AesRegister(WC_RTL8735B_AES_DEVID);
        CHECK("wc_Rtl8735b_AesRegister", aesRet == 0);
    }
#endif
    if (ret == 0) {
        huk_gcm_test();
        huk_ecb_cbc_test();
        huk_ctr_test();
        huk_gcm_unaligned_test();
        huk_cbc_inplace_test();
        huk_gcm_badiv_test();
        huk_hmac_test();
        huk_ecdsa_test();
        huk_ecdsa_reload_test();
#ifdef WOLFSSL_RTL8735B_AES
        if (aesRet == 0) {
            plain_gcm_test();
            plain_ecb_test();
            plain_gcm_unaligned_test();
            plain_gcm_long_iv_test();
            wc_Rtl8735b_AesUnRegister(WC_RTL8735B_AES_DEVID);
        }
#endif
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
