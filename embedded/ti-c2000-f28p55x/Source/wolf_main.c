/* wolf_main.c
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

/* Bare-metal entry point and board support for the wolfCrypt test/benchmark
 * on the TI LAUNCHXL-F28P55X (TMS320F28P550SJ, C2000 C28x DSP).
 *
 * Output: by default printf goes to the CCS console over JTAG (CIO), which
 * needs no wiring - good for first light.  Define WOLF_C2000_SCI_STDOUT to
 * route stdout to SCIA (GPIO28/29), the XDS110 virtual COM, at 115200 8N1 for
 * headless runs.
 *
 * The C28x has no hardware TRNG, so my_rng_seed_gen() is a DEV-ONLY counter
 * seed - NOT secure.  ML-DSA verify and the hash tests are deterministic and
 * do not consume it.
 */

#include "driverlib.h"
#include "device.h"

#include <stdio.h>
#include <stddef.h>
#ifdef WOLF_C2000_SCI_STDOUT
#include <file.h>
#endif

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/sha512.h>
#ifdef WOLF_SHA1
#include <wolfssl/wolfcrypt/sha.h>
#endif
#include <wolfssl/wolfcrypt/sha3.h>
#include <wolfssl/wolfcrypt/wc_mldsa.h>
#ifdef WOLF_MLKEM
#include <wolfssl/wolfcrypt/wc_mlkem.h>
#endif
#ifdef WOLF_AES
#include <wolfssl/wolfcrypt/aes.h>
#endif
#ifdef WOLF_HWAES
#include <wolfssl/wolfcrypt/port/ti/ti-c2000.h>
#endif
#ifdef WOLF_ENTROPY
#include <wolfssl/wolfcrypt/port/ti/ti-c2000-entropy.h>
#endif
#ifdef WOLF_ENTROPY_PROBE
#include "entropy_probe.h"
#endif
#ifdef WOLF_25519
#include <wolfssl/wolfcrypt/curve25519.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/random.h>
#endif
#ifdef WOLF_448
#include <wolfssl/wolfcrypt/curve448.h>
#include <wolfssl/wolfcrypt/ed448.h>
#endif
#ifdef WOLF_HKDF
#include <wolfssl/wolfcrypt/hmac.h>
#include <wolfssl/wolfcrypt/kdf.h>
#endif
#ifdef WOLF_CHACHA
#include <wolfssl/wolfcrypt/chacha20_poly1305.h>
#endif
#ifdef WOLF_AESEXTRA
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/cmac.h>
#endif
#ifdef WOLF_RSA
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/random.h>
#endif
#ifdef WOLF_DH
#include <wolfssl/wolfcrypt/dh.h>
#endif
#ifdef WOLF_RSASIGN
#include <wolfssl/wolfcrypt/rsa.h>
#include "rsa2048_priv.h"
#endif
#ifdef WOLF_ECC
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/integer.h>
#endif
#include <wolfcrypt/test/test.h>
#include <wolfcrypt/benchmark/benchmark.h>
#if defined(WOLF_MLDSA_OCTETS) || defined(WOLF_SECUREBOOT)
/* The octet-boundary image carries its own seed-derived vectors for all three
 * parameter sets (it needs the matching private key to produce the pre-hash
 * signatures), and reuses the same kat_mldsa87_* names.  The canonical FIPS 204
 * ML-DSA-87 vectors from wolfcrypt/test/test.c stay in the default image. */
#include <wolfssl/wolfcrypt/hash.h>
#include "mldsa_octet_kat.h"
#ifdef WOLFSSL_MLDSA_VERIFY_PRECOMP_A
#include "mldsa87_precomp_a.h"
#endif
#else
#include "mldsa87_kat.h"
#endif
#ifdef WOLF_ECC
#include "ecc_p256_kat.h"
#ifdef WOLF_ECC384
#include "ecc_p384_kat.h"
#endif
#endif
#ifdef WOLF_RSA
#include "rsa2048_kat.h"
#endif

/* ------------------------------------------------------------------------- */
/* SCIA console (XDS110 virtual COM)                                         */
/* ------------------------------------------------------------------------- */
static void c2000_sciInit(void)
{
    /* RX pin */
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCIRXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_QUAL_ASYNC);

    /* TX pin */
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCITXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_QUAL_ASYNC);

    SCI_performSoftwareReset(SCIA_BASE);
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 115200,
                  (SCI_CONFIG_WLEN_8 | SCI_CONFIG_STOP_ONE | SCI_CONFIG_PAR_NONE));
    SCI_resetChannels(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
    SCI_resetTxFIFO(SCIA_BASE);
    SCI_enableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);
}

/* RAM capture buffer: every stdout byte is mirrored here so the host can read
 * program output back over JTAG (symbol g_log / g_logpos), independent of SCI
 * routing or CIO.  Lives in .bss (RAM); survives a warm reset.  The log only
 * ever holds a few KB of text, so the RAM-tight builds get the small size. */
#if defined(WOLF_MLDSA_SIGN) || defined(WOLF_MLKEM)
    /* RAM-tight: the ML-DSA sign and ML-KEM-768 builds need the RAM for their
     * large key/poly state, so the capture buffer shrinks. */
    #define WOLF_LOG_SZ 2048
#else
    /* Everything else has RAM to spare: capture the whole run. */
    #define WOLF_LOG_SZ 12288
#endif
volatile char          g_log[WOLF_LOG_SZ];
volatile unsigned long g_logpos = 0;

#ifdef WOLF_STACK_PROFILE
/* Stack high-water profiling.  The C28x SP grows UP from the base of the .stack
 * section; paint the as-yet-unused upper stack with a sentinel just before the
 * call, then scan from the top down afterwards to find the deepest address
 * touched.  Bounds come from the TI linker (__stack / __TI_STACK_END) so this
 * is correct for both the verify (16 KW) and sign (12 KW) stack windows. */
extern unsigned int __stack;        /* .stack low address  (TI linker) */
extern unsigned int __TI_STACK_END; /* .stack high address (TI linker) */
#define WOLF_STK_BASE ((unsigned long)(&__stack))
#define WOLF_STK_TOP  ((unsigned long)(&__TI_STACK_END))
volatile unsigned long g_stk_paint = 0; /* address paint started from   */
volatile unsigned long g_stk_peak  = 0; /* highest address left touched */
static void wolf_paint_stack(void)
{
    unsigned int  marker;
    unsigned int* p   = &marker + 8;                  /* just above this frame */
    unsigned int* top = (unsigned int*)WOLF_STK_TOP;
    g_stk_paint = (unsigned long)&marker;
    while (p < top) {
        *p = 0xCCCCU;
        p++;
    }
}
static void wolf_measure_stack(void)
{
    unsigned int* p    = (unsigned int*)(WOLF_STK_TOP - 1UL);
    unsigned int* base = (unsigned int*)WOLF_STK_BASE;
    while ((p > base) && (*p == 0xCCCCU)) {
        p--;
    }
    g_stk_peak = (unsigned long)p;
}
#endif /* WOLF_STACK_PROFILE */

#ifdef WOLF_MEM_PROFILE
/* Heap high-water tracking.  Installed via wolfSSL_SetAllocators() so every
 * XMALLOC/XFREE/XREALLOC routes through here.  Each block is prefixed with its
 * cell count so free/realloc can adjust the running total.  Counts are in
 * 16-bit cells; x2 gives octets.  The 1-prefix per allocation is a small,
 * noted overhead. */
#include <stdlib.h>
#include <wolfssl/wolfcrypt/memory.h>
volatile unsigned long g_heap_cur  = 0;
volatile unsigned long g_heap_peak = 0;
static void wolf_heap_reset(void)
{
    g_heap_cur  = 0;
    g_heap_peak = 0;
}
static void* wolf_prof_malloc(size_t n)
{
    size_t* p;
    if (n == 0) {
        n = 1;
    }
    p = (size_t*)malloc(n + sizeof(size_t));
    if (p == NULL) {
        return NULL;
    }
    p[0] = n;
    g_heap_cur += (unsigned long)n;
    if (g_heap_cur > g_heap_peak) {
        g_heap_peak = g_heap_cur;
    }
    return (void*)(p + 1);
}
static void wolf_prof_free(void* ptr)
{
    size_t* p;
    if (ptr == NULL) {
        return;
    }
    p = ((size_t*)ptr) - 1;
    g_heap_cur -= (unsigned long)p[0];
    free(p);
}
static void* wolf_prof_realloc(void* ptr, size_t n)
{
    size_t* p;
    size_t  old;
    void*   nw;
    if (ptr == NULL) {
        return wolf_prof_malloc(n);
    }
    p   = ((size_t*)ptr) - 1;
    old = p[0];
    nw  = wolf_prof_malloc(n);
    if (nw != NULL) {
        XMEMCPY(nw, ptr, (old < n) ? old : n);
        wolf_prof_free(ptr);
    }
    return nw;
}
static void wolf_mem_install(void)
{
    (void)wolfSSL_SetAllocators(wolf_prof_malloc, wolf_prof_free,
                                wolf_prof_realloc);
}
/* Bracket an operation: reset the heap counter + paint the stack, then after
 * the call report the stack high-water and heap peak in octets. */
#define MEMPROF_BEGIN()      do { wolf_heap_reset(); wolf_paint_stack(); } \
                             while (0)
#define MEMPROF_END(nm, op)  do { wolf_measure_stack(); \
    printf("MEMPROF %s %s: stack=%lu octets, heap_peak=%lu octets\r\n", \
        (nm), (op), (g_stk_peak - WOLF_STK_BASE + 1UL) * 2UL, \
        g_heap_peak * 2UL); } while (0)
#else
#define MEMPROF_BEGIN()      do { } while (0)
#define MEMPROF_END(nm, op)  do { } while (0)
#endif /* WOLF_MEM_PROFILE */

static void wolf_logbyte(char c)
{
    if (g_logpos < (unsigned long)sizeof(g_log)) {
        g_log[g_logpos++] = c;
    }
}

#ifdef WOLF_C2000_SCI_STDOUT
/* Minimal CGT low-level I/O device that writes to SCIA, so printf/stdout can
 * be redirected with add_device()/freopen().  Only write() does real work. */
int    WOLF_open(const char *path, unsigned flags, int llv_fd);
int    WOLF_close(int dev_fd);
int    WOLF_read(int dev_fd, char *buf, unsigned count);
int    WOLF_write(int dev_fd, const char *buf, unsigned count);
fpos_t WOLF_lseek(int dev_fd, fpos_t offset, int origin);
int    WOLF_unlink(const char *path);
int    WOLF_rename(const char *old_name, const char *new_name);

int WOLF_open(const char *path, unsigned flags, int llv_fd)
{
    (void)path;
    (void)flags;
    (void)llv_fd;
    return 0;
}

int WOLF_close(int dev_fd)
{
    (void)dev_fd;
    return 0;
}

int WOLF_read(int dev_fd, char *buf, unsigned count)
{
    (void)dev_fd;
    (void)buf;
    (void)count;
    return 0;
}

int WOLF_write(int dev_fd, const char *buf, unsigned count)
{
    unsigned i;
    (void)dev_fd;
    for (i = 0; i < count; i++) {
        wolf_logbyte(buf[i]);
        if (buf[i] == '\n') {
            SCI_writeCharBlockingFIFO(SCIA_BASE, (uint16_t)'\r');
        }
        SCI_writeCharBlockingFIFO(SCIA_BASE, (uint16_t)(buf[i] & 0xFF));
    }
    return (int)count;
}

fpos_t WOLF_lseek(int dev_fd, fpos_t offset, int origin)
{
    (void)dev_fd;
    (void)offset;
    (void)origin;
    return (fpos_t)0;
}

int WOLF_unlink(const char *path)
{
    (void)path;
    return 0;
}

int WOLF_rename(const char *old_name, const char *new_name)
{
    (void)old_name;
    (void)new_name;
    return 0;
}

static void c2000_stdoutToSci(void)
{
    add_device("wolf", _SSA, WOLF_open, WOLF_close, WOLF_read, WOLF_write,
               WOLF_lseek, WOLF_unlink, WOLF_rename);
    freopen("wolf:", "w", stdout);
    setvbuf(stdout, NULL, _IONBF, 0); /* unbuffered: lines appear promptly */
}
#endif /* WOLF_C2000_SCI_STDOUT */

/* ------------------------------------------------------------------------- */
/* Benchmark time source - free-running CPU Timer 0 at SYSCLK                 */
/* ------------------------------------------------------------------------- */
static void c2000_timerInit(void)
{
    CPUTimer_stopTimer(CPUTIMER0_BASE);
    CPUTimer_setPeriod(CPUTIMER0_BASE, 0xFFFFFFFFUL);
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0U);
    CPUTimer_setEmulationMode(CPUTIMER0_BASE,
                              CPUTIMER_EMULATIONMODE_RUNFREE);
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
    CPUTimer_startTimer(CPUTIMER0_BASE);
}

/* benchmark.c (WOLFSSL_USER_CURRTIME) calls this for elapsed seconds.
 * CPU Timer 0 is a 32-bit down-counter that auto-reloads from 0xFFFFFFFF; we
 * accumulate in 64 bits across reloads.  NOTE: a single measured interval must
 * be shorter than 2^32 / SYSCLK (~28.6 s at 150 MHz) for the wrap handling to
 * be exact - true for all BENCH_EMBEDDED cases here. */
double current_time(int reset)
{
    static unsigned long long accum = 0;
    static uint32_t last = 0;
    static int started = 0;
    uint32_t now;

    now = CPUTimer_getTimerCount(CPUTIMER0_BASE);

    if (!started || reset) {
        accum = 0;
        last = now;
        started = 1;
        return 0.0;
    }

    if (last >= now) {
        accum += (unsigned long long)(last - now);
    }
    else {
        /* counter reloaded once since the last sample */
        accum += (unsigned long long)last +
                 (0xFFFFFFFFULL - (unsigned long long)now) + 1ULL;
    }
    last = now;

    return (double)accum / (double)DEVICE_SYSCLK_FREQ;
}

#ifdef WOLF_MLKEM
/* Dump head bytes + ".." + tail bytes of a buffer as hex, matching the host
 * reference dumper, to localize a CHAR_BIT==16 divergence. */
static void kem_dump(const char* name, const byte* b, int len, int head,
                     int tail)
{
    int i;
    printf("%s[%d]: ", name, len);
    for (i = 0; i < head && i < len; i++) {
        printf("%02x", (unsigned)(b[i] & 0xFF));
    }
    if (len > head + tail) {
        printf("..");
        for (i = len - tail; i < len; i++) {
            printf("%02x", (unsigned)(b[i] & 0xFF));
        }
    }
    printf("\r\n");
}

/* Deterministic ML-KEM keygen+encap+decap round-trip (self-consistent: the two
 * shared secrets must match).  No external vectors needed. */
static int mlkem_roundtrip(int type, const char* name)
{
    static MlKemKey k;
    static byte ct[WC_ML_KEM_MAX_CIPHER_TEXT_SIZE];
    static byte ss1[WC_ML_KEM_SS_SZ];
    static byte ss2[WC_ML_KEM_SS_SZ];
    static byte seed[64];
    static byte coins[32];
    word32 ctSz = 0;
    int ret, ok = 0, i;

    for (i = 0; i < 64; i++) { seed[i]  = (byte)i; }
    for (i = 0; i < 32; i++) { coins[i] = (byte)(0x40 + i); }
    ret = wc_MlKemKey_Init(&k, type, NULL, INVALID_DEVID);
    if (ret == 0) {
        MEMPROF_BEGIN();
        ret = wc_MlKemKey_MakeKeyWithRandom(&k, seed, 64);
        MEMPROF_END(name, "make");
    }
    if (ret == 0) { ret = wc_MlKemKey_CipherTextSize(&k, &ctSz); }
    if (ret == 0) {
        MEMPROF_BEGIN();
        ret = wc_MlKemKey_EncapsulateWithRandom(&k, ct, ss1, coins, 32);
        MEMPROF_END(name, "encapsulate");
    }
    if (ret == 0) {
        MEMPROF_BEGIN();
        ret = wc_MlKemKey_Decapsulate(&k, ss2, ct, ctSz);
        MEMPROF_END(name, "decapsulate");
    }
    if (ret == 0) { ok = (XMEMCMP(ss1, ss2, WC_ML_KEM_SS_SZ) == 0); }
    printf("%s encap/decap round-trip: %s (ret=%d ss_match=%d)\r\n",
           name, (ret == 0 && ok) ? "PASS" : "FAIL", ret, ok);
#ifdef WOLF_MLKEM_BENCH
    if (ret == 0) {
        double t;
        int b;
        (void)current_time(1);
        for (b = 0; b < 10; b++) {
            (void)wc_MlKemKey_MakeKeyWithRandom(&k, seed, 64);
        }
        t = current_time(0);
        printf("%s keygen bench: 10 ops in %.3f s = %.2f ms/op\r\n",
               name, t, t * 1000.0 / 10);
        (void)current_time(1);
        for (b = 0; b < 10; b++) {
            (void)wc_MlKemKey_EncapsulateWithRandom(&k, ct, ss1, coins, 32);
        }
        t = current_time(0);
        printf("%s encap bench: 10 ops in %.3f s = %.2f ms/op\r\n",
               name, t, t * 1000.0 / 10);
        (void)current_time(1);
        for (b = 0; b < 10; b++) {
            (void)wc_MlKemKey_Decapsulate(&k, ss2, ct, ctSz);
        }
        t = current_time(0);
        printf("%s decap bench: 10 ops in %.3f s = %.2f ms/op\r\n",
               name, t, t * 1000.0 / 10);
    }
#endif /* WOLF_MLKEM_BENCH */
#ifdef WOLF_MEM_PROFILE
    printf("MEMPROF %s key_sizeof=%lu octets\r\n", name,
           (unsigned long)sizeof(k) * 2UL);
#endif
    wc_MlKemKey_Free(&k);
    return ret;
}
#endif /* WOLF_MLKEM */

#if defined(WOLF_MLDSA_SIGN)
/* Shared ML-DSA sign work buffers (RAM is tight in the sign image, so the 87
 * round-trip below and the 44/65 helper reuse one key + sig buffer). */
static wc_MlDsaKey sgk;
static byte sg_sig[5000];   /* ML-DSA-87 sig ~4627; covers 44/65/87 */
static byte sg_msg[64];

/* ML-DSA keygen+sign+verify round-trip for a given parameter set
 * (self-consistent: verify of the just-made signature must return res==1). */
static void mldsa_sign_roundtrip(int type, const char* name)
{
    WC_RNG rng;
    word32 sigLen = (word32)sizeof(sg_sig);
    int i, res = 0, ret;

    for (i = 0; i < (int)sizeof(sg_msg); i++) { sg_msg[i] = (byte)(i & 0xFF); }
    ret = wc_InitRng(&rng);
    if (ret == 0) { ret = wc_MlDsaKey_Init(&sgk, NULL, INVALID_DEVID); }
    if (ret == 0) { ret = wc_MlDsaKey_SetParams(&sgk, type); }
    if (ret == 0) { ret = wc_MlDsaKey_MakeKey(&sgk, &rng); }
    if (ret == 0) { ret = wc_MlDsaKey_SignCtx(&sgk, NULL, 0, sg_sig, &sigLen,
                                          sg_msg, (word32)sizeof(sg_msg), &rng); }
    if (ret == 0) { ret = wc_MlDsaKey_VerifyCtx(&sgk, sg_sig, sigLen, NULL, 0,
                                       sg_msg, (word32)sizeof(sg_msg), &res); }
    printf("%s sign/verify round-trip: %s (ret=%d res=%d sigLen=%lu)\r\n",
           name, (ret == 0 && res == 1) ? "PASS" : "FAIL", ret, res,
           (unsigned long)sigLen);
    wc_MlDsaKey_Free(&sgk);
    wc_FreeRng(&rng);
}
#endif /* WOLF_MLDSA_SIGN */

/* ------------------------------------------------------------------------- */
/* wolfSSL port hooks (referenced from user_settings.h)                      */
/* ------------------------------------------------------------------------- */
/* XTIME stub - hashing and ML-DSA verify never use wall-clock time. */
long my_time(long* t)
{
    if (t != NULL) {
        *t = 0;
    }
    return 0;
}

/* The RNG now uses the real SHA-256 Hash-DRBG seeded by WOLFSSL_GENSEED_FORTEST
 * (random.c's built-in test seed) - see user_settings.h.  Switching off the old
 * WC_NO_HASHDRBG custom-stub RNG (which leaked heap on every wc_InitRng) is what
 * lets wolfcrypt_test and the benchmark both run in one image. */

#ifdef WOLF_ECC
/* ECDSA P-256 + ECDH bring-up on the C28x (SP single-precision math):
 *   1. RNG-free verify KAT (RFC 6979 raw r/s; no ASN).
 *   2. keygen + sign + verify round-trip (uses the test DRBG).
 *   3. ECDH shared-secret agreement (both directions must match).
 */
static void wolf_ecc_test(void)
{
    /* ecc_key/WC_RNG are large under SP_NO_MALLOC; keep them off the stack so
     * the SP point-math call tree has the full 16 KW stack to work in. */
    static ecc_key key;
    static ecc_key keyA;
    static ecc_key keyB;
    static WC_RNG  rng;
    mp_int  r;
    mp_int  s;
    byte    secretA[32];
    byte    secretB[32];
    word32  lenA;
    word32  lenB;
    int     ret;
    int     res;
    int     verified;
    int     ok;
    word32  i;

    /* r and s are mp_clear()'d on every path below, including ones reached when
     * an earlier init fails and their mp_init() was skipped; zero them first so
     * that cleanup is safe. */
    XMEMSET(&r, 0, sizeof(r));
    XMEMSET(&s, 0, sizeof(s));

    /* 1. Deterministic verify KAT (no RNG; raw r/s -> no ASN/DER). */
    res = 0;
    ret = wc_ecc_init(&key);
    if (ret == 0) {
        ret = wc_ecc_import_unsigned(&key, (byte*)kat_p256_qx,
            (byte*)kat_p256_qy, NULL, ECC_SECP256R1);
    }
    if (ret == 0) {
        ret = mp_init_multi(&r, &s, NULL, NULL, NULL, NULL);
    }
    if (ret == 0) {
        ret = mp_read_unsigned_bin(&r, kat_p256_r, (int)sizeof(kat_p256_r));
    }
    if (ret == 0) {
        ret = mp_read_unsigned_bin(&s, kat_p256_s, (int)sizeof(kat_p256_s));
    }
    if (ret == 0) {
        ret = wc_ecc_verify_hash_ex(&r, &s, kat_p256_hash,
            (word32)sizeof(kat_p256_hash), &res, &key);
    }
    printf("ECDSA P-256 verify KAT: %s (ret=%d res=%d)\r\n",
           (ret == 0 && res == 1) ? "PASS" : "FAIL", ret, res);
    mp_clear(&r);
    mp_clear(&s);
    wc_ecc_free(&key);

    /* 2. keygen + sign + verify round-trip. */
    res = 0;
    verified = 0;
    ret = wc_InitRng(&rng);
    if (ret == 0) {
        ret = wc_ecc_init(&keyA);
    }
    if (ret == 0) {
        ret = wc_ecc_make_key_ex(&rng, 32, &keyA, ECC_SECP256R1);
    }
    if (ret == 0) {
        ret = wc_ecc_set_rng(&keyA, &rng);
    }
    if (ret == 0) {
        ret = mp_init_multi(&r, &s, NULL, NULL, NULL, NULL);
    }
    if (ret == 0) {
        ret = wc_ecc_sign_hash_ex(kat_p256_hash,
            (word32)sizeof(kat_p256_hash), &rng, &keyA, &r, &s);
    }
    if (ret == 0) {
        ret = wc_ecc_verify_hash_ex(&r, &s, kat_p256_hash,
            (word32)sizeof(kat_p256_hash), &verified, &keyA);
    }
    printf("ECDSA P-256 keygen/sign/verify: %s (ret=%d res=%d)\r\n",
           (ret == 0 && verified == 1) ? "PASS" : "FAIL", ret, verified);
    mp_clear(&r);
    mp_clear(&s);

    /* 3. ECDH: a second key + shared secret both ways must agree. */
    ret = wc_ecc_init(&keyB);
    if (ret == 0) {
        ret = wc_ecc_make_key_ex(&rng, 32, &keyB, ECC_SECP256R1);
    }
    if (ret == 0) {
        ret = wc_ecc_set_rng(&keyB, &rng);
    }
    lenA = (word32)sizeof(secretA);
    lenB = (word32)sizeof(secretB);
    if (ret == 0) {
        ret = wc_ecc_shared_secret(&keyA, &keyB, secretA, &lenA);
    }
    if (ret == 0) {
        ret = wc_ecc_shared_secret(&keyB, &keyA, secretB, &lenB);
    }
    ok = (ret == 0) && (lenA == lenB) && (lenA > 0);
    for (i = 0; ok && (i < lenA); i++) {
        if (secretA[i] != secretB[i]) {
            ok = 0;
        }
    }
    printf("ECDH P-256 shared secret: %s (ret=%d len=%lu)\r\n",
           ok ? "PASS" : "FAIL", ret, (unsigned long)lenA);
    wc_ecc_free(&keyB);
    wc_ecc_free(&keyA);
    wc_FreeRng(&rng);
}

#ifdef WOLF_ECC_BENCH
/* Timings for a secure-boot style image check: hash a flash-resident image,
 * then one ECDSA verify.  CPU Timer 0 is sampled every iteration so no single
 * interval can outrun the counter's ~28 s wrap.  A benchmark that reports a
 * rate for work that failed is worse than one that reports nothing, so every
 * loop below stops at the first error and divides by the number of operations
 * that actually completed. */
#ifndef WOLF_IMG_TOTAL
#define WOLF_IMG_TOTAL 131072UL         /* octets hashed in total            */
#endif
#ifndef WOLF_IMG_BLK
#define WOLF_IMG_BLK   4096U            /* size of the flash-resident block  */
#endif
#ifndef WOLF_IMG_CHUNK
#define WOLF_IMG_CHUNK 4096U            /* octets handed to each Update      */
#endif

/* const lands in .const, i.e. flash, so the hash reads at flash wait states
 * the way a real image check does rather than out of RAM.  One octet per
 * 16-bit cell, matching how wolfCrypt addresses bytes on this target.
 *
 * By default the block is smaller than WOLF_IMG_TOTAL and is walked repeatedly
 * to reach the total, so this measures hashing 128 KiB of flash-resident data
 * rather than a single sweep over a 128 KiB address range.  The two are
 * equivalent here: the C28x has no data cache in front of flash, and measuring
 * both on hardware gave 0.740 s for a true single pass over a 128 KiB array
 * against 0.736 s for a repeated 4 KiB block - within run-to-run spread.  The
 * small block is the default because a genuine 128 KiB octet array costs 256 KB
 * of flash (one octet per 16-bit cell) and fills an entire flash bank.  Build
 * with -DWOLF_IMG_BLK=131072 to do the single sweep instead. */
static const unsigned char wolf_img[WOLF_IMG_BLK] = { 0 };

/* Hash WOLF_IMG_TOTAL octets of flash-resident data. */
static void wolf_sha_image_bench(int use384)
{
    static wc_Sha256 sha256;
    static wc_Sha384 sha384;
    unsigned char dig[WC_SHA384_DIGEST_SIZE];
    const char*   nm = use384 ? "SHA-384" : "SHA-256";
    unsigned long off;
    double t;
    int    ret;

    ret = use384 ? wc_InitSha384(&sha384) : wc_InitSha256(&sha256);
    if (ret != 0) {
        printf("%s image bench: init FAIL ret=%d\r\n", nm, ret);
        return;
    }

    (void)current_time(1);
    for (off = 0; off < WOLF_IMG_TOTAL; off += WOLF_IMG_CHUNK) {
        unsigned long left = WOLF_IMG_TOTAL - off;
        word32 n = (word32)((left < WOLF_IMG_CHUNK) ? left : WOLF_IMG_CHUNK);
        /* walk within the block, wrapping when it is smaller than the total */
        unsigned long pos = off % (unsigned long)WOLF_IMG_BLK;

        if (pos + n > (unsigned long)WOLF_IMG_BLK) {
            n = (word32)((unsigned long)WOLF_IMG_BLK - pos);
        }
        ret = use384 ? wc_Sha384Update(&sha384, wolf_img + pos, n)
                     : wc_Sha256Update(&sha256, wolf_img + pos, n);
        (void)current_time(0);
        if (ret != 0) {
            break;
        }
    }
    /* Only finalize a context whose updates all succeeded. */
    if (ret == 0) {
        ret = use384 ? wc_Sha384Final(&sha384, dig)
                     : wc_Sha256Final(&sha256, dig);
    }
    t = current_time(0);

    if (use384) {
        wc_Sha384Free(&sha384);
    }
    else {
        wc_Sha256Free(&sha256);
    }

    if (ret != 0) {
        printf("%s over %lu KiB image: FAIL ret=%d (no timing)\r\n",
               nm, WOLF_IMG_TOTAL / 1024UL, ret);
        return;
    }
    printf("%s over %lu KiB image: %.3f s\r\n", nm, WOLF_IMG_TOTAL / 1024UL, t);
}

static void wolf_ecdsa_bench(int curveId, int keySz, int reps, const char* nm)
{
    static ecc_key bk;
    static WC_RNG  brng;
    static mp_int  br;
    static mp_int  bs;
    unsigned char  dig[WC_SHA384_DIGEST_SIZE];
    double t;
    int    i, ret, res = 0, done;
    int    rngOk = 0, keyOk = 0, rOk = 0, sOk = 0;

    /* A SHA-384 digest; ECDSA truncates it to the curve order for P-256. */
    XMEMSET(dig, 0xA5, sizeof(dig));

    ret = wc_InitRng(&brng);
    if (ret == 0) {
        rngOk = 1;
        ret = wc_ecc_init(&bk);
    }
    if (ret == 0) {
        keyOk = 1;
        ret = wc_ecc_make_key_ex(&brng, keySz, &bk, curveId);
    }
    if (ret == 0) {
        ret = wc_ecc_set_rng(&bk, &brng);
    }
    if (ret == 0) {
        ret = mp_init(&br);
    }
    if (ret == 0) {
        rOk = 1;
        ret = mp_init(&bs);
    }
    if (ret == 0) {
        sOk = 1;
    }
    if (ret != 0) {
        printf("ECDSA %s bench: setup FAIL ret=%d\r\n", nm, ret);
        goto bench_done;
    }

    done = 0;
    (void)current_time(1);
    for (i = 0; i < reps; i++) {
        ret = wc_ecc_sign_hash_ex(dig, (word32)keySz, &brng, &bk, &br, &bs);
        (void)current_time(0);
        if (ret != 0) {
            break;
        }
        done++;
    }
    t = current_time(0);
    if (done == 0) {
        printf("ECDSA %s sign:   FAIL ret=%d (no timing)\r\n", nm, ret);
        goto bench_done;
    }
    printf("ECDSA %s sign:   %d ops in %.3f s = %.1f ms/op%s\r\n",
           nm, done, t, (t * 1000.0) / (double)done,
           (done < reps) ? " [STOPPED EARLY]" : "");

    done = 0;
    (void)current_time(1);
    for (i = 0; i < reps; i++) {
        ret = wc_ecc_verify_hash_ex(&br, &bs, dig, (word32)keySz, &res, &bk);
        (void)current_time(0);
        if ((ret != 0) || (res != 1)) {
            break;
        }
        done++;
    }
    t = current_time(0);
    if (done == 0) {
        printf("ECDSA %s verify: FAIL ret=%d res=%d (no timing)\r\n",
               nm, ret, res);
        goto bench_done;
    }
    printf("ECDSA %s verify: %d ops in %.3f s = %.1f ms/op%s\r\n",
           nm, done, t, (t * 1000.0) / (double)done,
           (done < reps) ? " [STOPPED EARLY]" : "");

bench_done:
    if (sOk) {
        mp_clear(&bs);
    }
    if (rOk) {
        mp_clear(&br);
    }
    if (keyOk) {
        wc_ecc_free(&bk);
    }
    if (rngOk) {
        wc_FreeRng(&brng);
    }
}

static void wolf_ecdh_bench(int curveId, int keySz, int reps, const char* nm)
{
    static ecc_key ka;
    static ecc_key kb;
    static WC_RNG  brng;
    static byte    secret[48];
    word32 len = 0;
    double t;
    int    i, ret, done;
    int    rngOk = 0, kaOk = 0, kbOk = 0;

    ret = wc_InitRng(&brng);
    if (ret == 0) {
        rngOk = 1;
        ret = wc_ecc_init(&ka);
    }
    if (ret == 0) {
        kaOk = 1;
        ret = wc_ecc_init(&kb);
    }
    if (ret == 0) {
        kbOk = 1;
        ret = wc_ecc_make_key_ex(&brng, keySz, &ka, curveId);
    }
    if (ret == 0) {
        ret = wc_ecc_make_key_ex(&brng, keySz, &kb, curveId);
    }
    if (ret == 0) {
        ret = wc_ecc_set_rng(&ka, &brng);
    }
    if (ret != 0) {
        printf("ECDH %s bench: setup FAIL ret=%d\r\n", nm, ret);
        goto ecdh_done;
    }

    done = 0;
    (void)current_time(1);
    for (i = 0; i < reps; i++) {
        len = (word32)sizeof(secret);
        ret = wc_ecc_shared_secret(&ka, &kb, secret, &len);
        (void)current_time(0);
        if (ret != 0) {
            break;
        }
        done++;
    }
    t = current_time(0);
    if (done == 0) {
        printf("ECDH  %s agree:  FAIL ret=%d (no timing)\r\n", nm, ret);
        goto ecdh_done;
    }
    printf("ECDH  %s agree:  %d ops in %.3f s = %.1f ms/op (len=%lu)%s\r\n",
           nm, done, t, (t * 1000.0) / (double)done, (unsigned long)len,
           (done < reps) ? " [STOPPED EARLY]" : "");

ecdh_done:
    if (kbOk) {
        wc_ecc_free(&kb);
    }
    if (kaOk) {
        wc_ecc_free(&ka);
    }
    if (rngOk) {
        wc_FreeRng(&brng);
    }
}

static void wolf_ecc_bench(void)
{
    wolf_sha_image_bench(0);
    wolf_sha_image_bench(1);
    wolf_ecdsa_bench(ECC_SECP256R1, 32, 10, "P-256");
    wolf_ecdh_bench(ECC_SECP256R1, 32, 10, "P-256");
#ifdef WOLF_ECC384
    wolf_ecdsa_bench(ECC_SECP384R1, 48, 5, "P-384");
    wolf_ecdh_bench(ECC_SECP384R1, 48, 5, "P-384");
#endif
}
#endif /* WOLF_ECC_BENCH */


#ifdef WOLF_ECC384
/* P-384 correctness, independent of the timing harness: an RFC 6979 verify KAT
 * over a device-computed SHA-384 digest, then a two-sided ECDH agreement.  The
 * KAT uses externally supplied public-key and signature octets, so it catches
 * the serialization defects a self-generated sign/verify pair cannot. */
static void wolf_ecc384_test(void)
{
    /* Large under SP_NO_MALLOC - keep off the 16 KW stack. */
    static ecc_key k384;
    static ecc_key k384a;
    static ecc_key k384b;
    static WC_RNG  rng384;
    static mp_int  r384;
    static mp_int  s384;
    static wc_Sha384 sha384;
    static byte    secA[48];
    static byte    secB[48];
    unsigned char  dig[WC_SHA384_DIGEST_SIZE];
    word32 lenA, lenB;
    int    ret, res = 0;

    ret = wc_InitSha384(&sha384);
    if (ret == 0) {
        ret = wc_Sha384Update(&sha384, (const byte*)kat_p384_msg,
                              (word32)(sizeof(kat_p384_msg) - 1));
    }
    if (ret == 0) {
        ret = wc_Sha384Final(&sha384, dig);
    }
    wc_Sha384Free(&sha384);

    if (ret == 0) {
        ret = wc_ecc_init(&k384);
    }
    if (ret == 0) {
        ret = wc_ecc_import_unsigned(&k384, (byte*)kat_p384_qx,
                  (byte*)kat_p384_qy, NULL, ECC_SECP384R1);
    }
    if (ret == 0) {
        ret = mp_init(&r384);
    }
    if (ret == 0) {
        ret = mp_init(&s384);
    }
    if (ret == 0) {
        ret = mp_read_unsigned_bin(&r384, kat_p384_r,
                                   (word32)sizeof(kat_p384_r));
    }
    if (ret == 0) {
        ret = mp_read_unsigned_bin(&s384, kat_p384_s,
                                   (word32)sizeof(kat_p384_s));
    }
    if (ret == 0) {
        ret = wc_ecc_verify_hash_ex(&r384, &s384, dig,
                                    (word32)sizeof(dig), &res, &k384);
    }
    printf("ECDSA P-384 verify KAT: %s (ret=%d res=%d)\r\n",
           (ret == 0 && res == 1) ? "PASS" : "FAIL", ret, res);
    mp_clear(&s384);
    mp_clear(&r384);
    wc_ecc_free(&k384);

    /* Two-sided ECDH: both parties must derive the same secret. */
    ret = wc_InitRng(&rng384);
    if (ret == 0) {
        ret = wc_ecc_init(&k384a);
    }
    if (ret == 0) {
        ret = wc_ecc_init(&k384b);
    }
    if (ret == 0) {
        ret = wc_ecc_make_key_ex(&rng384, 48, &k384a, ECC_SECP384R1);
    }
    if (ret == 0) {
        ret = wc_ecc_make_key_ex(&rng384, 48, &k384b, ECC_SECP384R1);
    }
    if (ret == 0) {
        ret = wc_ecc_set_rng(&k384a, &rng384);
    }
    if (ret == 0) {
        ret = wc_ecc_set_rng(&k384b, &rng384);
    }
    lenA = (word32)sizeof(secA);
    lenB = (word32)sizeof(secB);
    if (ret == 0) {
        ret = wc_ecc_shared_secret(&k384a, &k384b, secA, &lenA);
    }
    if (ret == 0) {
        ret = wc_ecc_shared_secret(&k384b, &k384a, secB, &lenB);
    }
    printf("ECDH P-384 shared secret: %s (ret=%d len=%lu)\r\n",
           (ret == 0 && lenA == lenB && lenA == 48U &&
            XMEMCMP(secA, secB, lenA) == 0) ? "PASS" : "FAIL",
           ret, (unsigned long)lenA);

    wc_ecc_free(&k384b);
    wc_ecc_free(&k384a);
    wc_FreeRng(&rng384);
}
#endif /* WOLF_ECC384 */

#endif /* WOLF_ECC */

/* Focused SHA3-256 known-answer test (SHA3-256("abc")), to confirm the
 * 16-bit-byte Keccak path is correct on the C28x. */
static void wolf_sha3_256_test(void)
{
    static const byte kat_msg[3] = { 'a', 'b', 'c' };
    static const byte kat_exp[WC_SHA3_256_DIGEST_SIZE] = {
        0x3a,0x98,0x5d,0xa7,0x4f,0xe2,0x25,0xb2,
        0x04,0x5c,0x17,0x2d,0x6b,0xd3,0x90,0xbd,
        0x85,0x5f,0x08,0x6e,0x3e,0x9d,0x52,0x5b,
        0x46,0xbf,0xe2,0x45,0x11,0x43,0x15,0x32
    };
    wc_Sha3 kat_sha;
    byte    kat_out[WC_SHA3_256_DIGEST_SIZE];
    int     ki;
    int     kok = 1;

    if (wc_InitSha3_256(&kat_sha, NULL, INVALID_DEVID) == 0) {
        (void)wc_Sha3_256_Update(&kat_sha, kat_msg, (word32)sizeof(kat_msg));
        (void)wc_Sha3_256_Final(&kat_sha, kat_out);
        wc_Sha3_256_Free(&kat_sha);
        printf("SHA3-256(abc)=");
        for (ki = 0; ki < (int)sizeof(kat_out); ki++) {
            printf("%02x", (unsigned)(kat_out[ki] & 0xFF));
        }
        printf("\r\n");
        for (ki = 0; ki < (int)sizeof(kat_out); ki++) {
            if (kat_out[ki] != kat_exp[ki]) {
                kok = 0;
            }
        }
        printf("SHA3-256 KAT: %s\r\n", kok ? "PASS" : "FAIL");
    }
    else {
        printf("SHA3-256 KAT: init failed\r\n");
    }
}

/* SHA-256 multi-block KAT (64 bytes 0x00..0x3f) - exercises the full-block
 * input path, not just the single-block Final path. */
static void wolf_sha256_test(void)
{
    static byte s2_msg[64];
    static const byte s2_exp[WC_SHA256_DIGEST_SIZE] = {
        0xfd,0xea,0xb9,0xac,0xf3,0x71,0x03,0x62,
        0xbd,0x26,0x58,0xcd,0xc9,0xa2,0x9e,0x8f,
        0x9c,0x75,0x7f,0xcf,0x98,0x11,0x60,0x3a,
        0x8c,0x44,0x7c,0xd1,0xd9,0x15,0x11,0x08
    };
    wc_Sha256 s2;
    byte s2o[WC_SHA256_DIGEST_SIZE];
    int si, sok = 1;
    for (si = 0; si < 64; si++) { s2_msg[si] = (byte)si; }
    if (wc_InitSha256(&s2) == 0) {
        (void)wc_Sha256Update(&s2, s2_msg, 64);
        (void)wc_Sha256Final(&s2, s2o);
        wc_Sha256Free(&s2);
        printf("SHA-256(64)=");
        for (si = 0; si < (int)sizeof(s2o); si++) {
            printf("%02x", (unsigned)(s2o[si] & 0xFF));
        }
        printf("\r\n");
        for (si = 0; si < (int)sizeof(s2o); si++) {
            if (s2o[si] != s2_exp[si]) { sok = 0; }
        }
        printf("SHA-256 (multiblock) KAT: %s\r\n", sok ? "PASS" : "FAIL");
    }
}

/* SHAKE256 multi-block KAT (256 bytes 0x00..0xff), 32-byte output -
 * exercises multi-block absorb (the path ML-DSA's mu/tr use). */
static void wolf_shake256_test(void)
{
    static byte sk_msg[256];
    static const byte sk_exp[32] = {
        0x33,0x6c,0x8a,0xa7,0xf2,0xb0,0x8b,0xda,
        0x6b,0xd7,0x40,0x2c,0xd2,0xea,0x89,0x76,
        0x0b,0x77,0x28,0xa8,0xb3,0x18,0x02,0xb8,
        0x05,0x24,0x75,0x63,0x61,0x16,0x53,0x66
    };
    wc_Shake sk1;
    byte o1[32];
    int i, match = 1, mu_ok = 1;
    for (i = 0; i < 256; i++) { sk_msg[i] = (byte)i; }
    if (wc_InitShake256(&sk1, NULL, INVALID_DEVID) == 0) {
        (void)wc_Shake256_Update(&sk1, sk_msg, 256);
        (void)wc_Shake256_Final(&sk1, o1, (word32)sizeof(o1));
        printf("SHAKE256(256)=");
        for (i = 0; i < 32; i++) { printf("%02x", (unsigned)(o1[i] & 0xFF)); }
        printf("\r\n");
        for (i = 0; i < 32; i++) { if (o1[i] != sk_exp[i]) { match = 0; } }
        printf("SHAKE256 (multiblock) KAT: %s\r\n", match ? "PASS" : "FAIL");
        wc_Shake256_Free(&sk1);
    }
    /* Same 256 bytes via 3 Updates (100+100+56) - exercises the
     * partial-buffer-fills-to-full absorb across Update calls (ML-DSA's
     * mu/tr feed several Updates). Must match the single-Update result. */
    if (wc_InitShake256(&sk1, NULL, INVALID_DEVID) == 0) {
        (void)wc_Shake256_Update(&sk1, sk_msg, 100);
        (void)wc_Shake256_Update(&sk1, sk_msg + 100, 100);
        (void)wc_Shake256_Update(&sk1, sk_msg + 200, 56);
        (void)wc_Shake256_Final(&sk1, o1, (word32)sizeof(o1));
        for (i = 0; i < 32; i++) { if (o1[i] != sk_exp[i]) { mu_ok = 0; } }
        printf("SHAKE256 (split Updates) KAT: %s\r\n", mu_ok ? "PASS" : "FAIL");
        wc_Shake256_Free(&sk1);
    }
}

#ifndef WOLF_MLDSA_SIGN
/* SHAKE256 large hash (100x1024 absorb -> 250-byte multi-block squeeze), the
 * exact test.c large-hash scenario, compared to test.c's large_digest -
 * validates multi-block absorb + multi-block squeeze on the C28x.  Skipped in
 * the SIGN build: its ~1.4 KW of static buffers would not fit alongside the
 * sign key + 32 KW heap. */
static void wolf_shake256_large_test(void)
{
    /* lg_in/lg_out are static (.bss) rather than stack: 1274 cells would be a
     * large bite out of the 16 KW C28x stack. */
    static byte lg_in[1024];
    static byte lg_out[250];
    static const byte lg_exp[114] = {
        0x90,0x32,0x4a,0xcc,0xd1,0xdf,0xb8,0x0b,0x79,0x1f,0xb8,0xc8,
        0x5b,0x54,0xc8,0xe7,0x45,0xf5,0x60,0x6b,0x38,0x26,0xb2,0x0a,
        0xee,0x38,0x01,0xf3,0xd9,0xfa,0x96,0x9f,0x6a,0xd7,0x15,0xdf,
        0xb6,0xc2,0xf4,0x20,0x33,0x44,0x55,0xe8,0x2a,0x09,0x2b,0x68,
        0x2e,0x18,0x65,0x5e,0x65,0x93,0x28,0xbc,0xb1,0x9e,0xe2,0xb1,
        0x92,0xea,0x98,0xac,0x21,0xef,0x4c,0xe1,0xb4,0xb7,0xbe,0x81,
        0x5c,0x1d,0xd3,0xb7,0x17,0xe5,0xbb,0xc5,0x8c,0x68,0xb7,0xfb,
        0xac,0x55,0x8a,0x9b,0x4d,0x91,0xe4,0x9f,0x72,0xbb,0x6e,0x38,
        0xaf,0x21,0x7d,0x21,0xaa,0x98,0x4e,0x75,0xc4,0xb4,0x1c,0x7c,
        0x50,0x45,0x54,0xf9,0xea,0x26 };
    wc_Shake sk1;
    int li, ok;

    for (li = 0; li < 1024; li++) { lg_in[li] = (byte)(li & 0xFF); }
    if (wc_InitShake256(&sk1, NULL, INVALID_DEVID) == 0) {
        for (li = 0; li < 100; li++)
            (void)wc_Shake256_Update(&sk1, lg_in, 1024);
        (void)wc_Shake256_Final(&sk1, lg_out, (word32)sizeof(lg_out));
        ok = 1;
        for (li = 0; li < 114; li++) if (lg_out[li] != lg_exp[li]) ok = 0;
        printf("SHAKE256 (large multiblock) KAT: %s\r\n",
            ok ? "PASS" : "FAIL");
        wc_Shake256_Free(&sk1);
    }
}
#endif /* !WOLF_MLDSA_SIGN */

/* SHA-512 / SHA-384 multi-block KATs (200 bytes 0x00..0xc7) - exercises the
 * octet-wise word64 byte-I/O (input load, 128-bit length, digest store). */
static void wolf_sha512_test(void)
{
    static byte h5_msg[200];
    static const byte sha512_exp[WC_SHA512_DIGEST_SIZE] = {
        0x98,0x60,0x58,0xe9,0x89,0x5e,0x2c,0x2a,
        0xb8,0xf9,0xe8,0xcb,0xdf,0x80,0x1d,0xb1,
        0x2a,0x44,0x84,0x2a,0x56,0xa9,0x1d,0x5a,
        0x4e,0x87,0xb1,0xfc,0x98,0xb2,0x93,0x72,
        0x2c,0x46,0x64,0x14,0x2e,0x42,0xc3,0xc5,
        0x51,0xff,0x89,0x86,0x46,0x26,0x8c,0xd9,
        0x2b,0x84,0xed,0x23,0x0b,0x8c,0x94,0xbe,
        0xd7,0x79,0x8d,0x4f,0x27,0xcd,0x74,0x65
    };
    static const byte sha384_exp[WC_SHA384_DIGEST_SIZE] = {
        0x7e,0xa4,0xbb,0x25,0x34,0xc6,0x70,0x36,
        0xf4,0x9d,0xe7,0xbe,0xb5,0xfe,0x8a,0x24,
        0x78,0xdf,0x04,0xff,0x3f,0xef,0x40,0xa9,
        0xcd,0x49,0x23,0x99,0x9a,0x59,0x0e,0x99,
        0x12,0xdf,0x12,0x97,0x21,0x7c,0xe1,0xa0,
        0x21,0xaa,0x2f,0xb1,0x01,0x34,0x98,0xb8
    };
    wc_Sha512 h512;
    wc_Sha384 h384;
    byte h5o[WC_SHA512_DIGEST_SIZE];
    int hi, h5ok = 1, h3ok = 1;
    for (hi = 0; hi < 200; hi++) { h5_msg[hi] = (byte)(hi & 0xFF); }
    if (wc_InitSha512(&h512) == 0) {
        (void)wc_Sha512Update(&h512, h5_msg, 200);
        (void)wc_Sha512Final(&h512, h5o);
        wc_Sha512Free(&h512);
        for (hi = 0; hi < WC_SHA512_DIGEST_SIZE; hi++) {
            if (h5o[hi] != sha512_exp[hi]) { h5ok = 0; }
        }
        printf("SHA-512 (multiblock) KAT: %s\r\n", h5ok ? "PASS" : "FAIL");
    }
    if (wc_InitSha384(&h384) == 0) {
        (void)wc_Sha384Update(&h384, h5_msg, 200);
        (void)wc_Sha384Final(&h384, h5o);
        wc_Sha384Free(&h384);
        for (hi = 0; hi < WC_SHA384_DIGEST_SIZE; hi++) {
            if (h5o[hi] != sha384_exp[hi]) { h3ok = 0; }
        }
        printf("SHA-384 (multiblock) KAT: %s\r\n", h3ok ? "PASS" : "FAIL");
    }
}

#ifdef WOLF_SHA1
static void wolf_sha1_test(void)
{
    /* SHA-1("abc") = a9993e36 4706816a ba3e2571 7850c26c 9cd0d89d */
    static const byte sha1_in[3] = { 0x61, 0x62, 0x63 };
    static const byte sha1_exp[WC_SHA_DIGEST_SIZE] = {
        0xa9,0x99,0x3e,0x36,0x47,0x06,0x81,0x6a,0xba,0x3e,
        0x25,0x71,0x78,0x50,0xc2,0x6c,0x9c,0xd0,0xd8,0x9d };
    wc_Sha sha1;
    byte sha1o[WC_SHA_DIGEST_SIZE];
    int s1ok = 1, si;
    if (wc_InitSha(&sha1) == 0) {
        (void)wc_ShaUpdate(&sha1, sha1_in, sizeof(sha1_in));
        (void)wc_ShaFinal(&sha1, sha1o);
        wc_ShaFree(&sha1);
        for (si = 0; si < WC_SHA_DIGEST_SIZE; si++) {
            if (sha1o[si] != sha1_exp[si]) { s1ok = 0; }
        }
        printf("SHA-1(abc) KAT: %s\r\n", s1ok ? "PASS" : "FAIL");
    }
}
#endif /* WOLF_SHA1 */

#if !defined(WOLF_MLDSA_SIGN) && !defined(WOLF_MLDSA_OCTETS) && \
    !defined(WOLF_SECUREBOOT)
/* ML-DSA-87 verify known-answer test (real pk/msg/sig from test.c).  This
 * is the primary deliverable: a deterministic, RNG-free verify on HW.
 * Key struct is static (large; WOLFSSL_MLDSA_VERIFY_NO_MALLOC pins buffers
 * into it). msg matches test.c's mldsa_param_vfy_test: msg[i] = (byte)i.
 * Skipped in the SIGN build: its static verify key would not fit alongside
 * the sign key + 32 KW heap, and the sign round-trip below also exercises
 * verify (of a freshly produced signature).  Skipped in the SECUREBOOT build
 * too: that image calls wolf_secureboot_test() instead, so the guard here has
 * to match the call site or this is compiled and never used. */
static void wolf_mldsa87_verify_test(void)
{
    /* mldsa_key is static (.bss): WOLFSSL_MLDSA_VERIFY_NO_MALLOC pins the
     * verify workspace inside it, which is far too large for the 16 KW C28x
     * stack.  md_msg, by contrast, is deliberately left on the stack - it is
     * only 512 cells and it keeps the WOLF_STACK_PROFILE figure below a
     * true end-to-end measurement of the verify path. */
    static wc_MlDsaKey mldsa_key;
    byte md_msg[512];
    int  md_i;
    int  md_res = 0;
    int  md_ret;
#ifdef WOLF_MLDSA_VERIFY_BENCH
    int    bi;
    int    br = 0;
    double t;
#endif

    /* test.c builds this as msg[i] = (byte)i.  On an 8-bit-byte host that
     * wraps mod 256; a wolfCrypt 'byte' buffer must hold octet values, so
     * mask explicitly (a C28x 'byte' is a 16-bit cell and would otherwise
     * store 256..511 verbatim, corrupting the octet-wise hash input). */
    for (md_i = 0; md_i < (int)sizeof(md_msg); md_i++) {
        md_msg[md_i] = (byte)(md_i & 0xFF);
    }
    md_ret = wc_MlDsaKey_Init(&mldsa_key, NULL, INVALID_DEVID);
    if (md_ret == 0) {
        md_ret = wc_MlDsaKey_SetParams(&mldsa_key, WC_ML_DSA_87);
    }
    if (md_ret == 0) {
        md_ret = wc_MlDsaKey_ImportPubRaw(&mldsa_key, kat_mldsa87_pub,
            (word32)sizeof(kat_mldsa87_pub));
    }
    if (md_ret == 0) {
#ifdef WOLF_MEM_PROFILE
        wolf_heap_reset();
#endif
#ifdef WOLF_STACK_PROFILE
        wolf_paint_stack();
#endif
        md_ret = wc_MlDsaKey_VerifyCtx(&mldsa_key, kat_mldsa87_sig,
            (word32)sizeof(kat_mldsa87_sig), NULL, 0, md_msg,
            (word32)sizeof(md_msg), &md_res);
#ifdef WOLF_STACK_PROFILE
        wolf_measure_stack();
#endif
    }
    printf("ML-DSA-87 verify KAT: %s (ret=%d res=%d)\r\n",
           (md_ret == 0 && md_res == 1) ? "PASS" : "FAIL", md_ret, md_res);
#ifdef WOLF_MLDSA_VERIFY_BENCH
    /* Time the deterministic verify (CPU Timer 0 via current_time); 10 ops
     * keeps the interval under the ~28 s single-measurement wrap. */
    if ((md_ret == 0) && (md_res == 1)) {
        (void)current_time(1);
        for (bi = 0; bi < 10; bi++) {
            (void)wc_MlDsaKey_VerifyCtx(&mldsa_key, kat_mldsa87_sig,
                (word32)sizeof(kat_mldsa87_sig), NULL, 0, md_msg,
                (word32)sizeof(md_msg), &br);
        }
        t = current_time(0);
        printf("ML-DSA-87 verify bench: 10 ops in %.3f s = %.2f ms/op, "
               "%.2f ops/sec\r\n", t, (t * 1000.0) / 10.0, 10.0 / t);
    }
#endif
#ifdef WOLF_STACK_PROFILE
    printf("ML-DSA-87 verify: stack peak=0x%lx used=%lu words (key struct "
           "sizeof=%lu words); heap: VERIFY_NO_MALLOC\r\n",
           g_stk_peak, (g_stk_peak - WOLF_STK_BASE + 1UL),
           (unsigned long)sizeof(mldsa_key));
#endif
#ifdef WOLF_MEM_PROFILE
    printf("MEMPROF ML-DSA-87 verify: stack=%lu octets, heap_peak=%lu "
           "octets, key_sizeof=%lu octets\r\n",
           (g_stk_peak - WOLF_STK_BASE + 1UL) * 2UL, g_heap_peak * 2UL,
           (unsigned long)sizeof(mldsa_key) * 2UL);
#endif
    wc_MlDsaKey_Free(&mldsa_key);
}
#endif /* !WOLF_MLDSA_SIGN && !WOLF_MLDSA_OCTETS */

#ifdef WOLF_MLDSA_OCTETS
/* ------------------------------------------------------------------------- */
/* ML-DSA octet-boundary tests (make MLDSA=1)                                */
/* ------------------------------------------------------------------------- */
/* Proves three things on 16-bit-byte silicon: verify is correct at all three
 * parameter sets (level 44 matters most - its w1 encoder packs 6-bit values, so
 * it is the only one that can push a cell above 255); the HashML-DSA path
 * wc_MlDsaKey_VerifyCtxHash() works with a non-empty context; and a key and
 * signature that arrive PACKED verify once wc_UnpackOctets() expands them.
 * One key struct is shared - WOLFSSL_MLDSA_VERIFY_NO_MALLOC pins the verify
 * workspace inside it, far too big for the 16 KW C28x stack. */
/* Static: an ML-DSA-65 signature is 3309 octets = 3309 cells = 6618 bytes. */
static byte     mo_pub[WC_MLDSA_65_PUB_KEY_SIZE];
static byte     mo_sig[WC_MLDSA_65_SIG_SIZE];
static wc_MlDsaKey mo_key;
static byte     mo_msg[512];

/* hashAlg < 0 selects plain verify over the message; otherwise pre-hash. */
static void mo_verify(const char* what, int type, const byte* pub,
    word32 pubLen, const byte* sig, word32 sigLen, int hashAlg,
    const byte* hash, word32 hashLen)
{
    int res = 0;
    int ret;

    ret = wc_MlDsaKey_Init(&mo_key, NULL, INVALID_DEVID);
    if (ret == 0) {
        ret = wc_MlDsaKey_SetParams(&mo_key, type);
    }
    if (ret == 0) {
        ret = wc_MlDsaKey_ImportPubRaw(&mo_key, pub, pubLen);
    }
    if (ret == 0) {
        if (hashAlg < 0) {
            ret = wc_MlDsaKey_VerifyCtx(&mo_key, sig, sigLen, NULL, 0, mo_msg,
                (word32)sizeof(mo_msg), &res);
        }
        else {
            ret = wc_MlDsaKey_VerifyCtxHash(&mo_key, sig, sigLen,
                (const byte*)KAT_MLDSA_CTX,
                (byte)(sizeof(KAT_MLDSA_CTX) - 1), hash, hashLen, hashAlg,
                &res);
        }
    }
    printf("%s %s (ret=%d res=%d)\r\n", what,
           ((ret == 0) && (res == 1)) ? "PASS" : "FAIL", ret, res);
    wc_MlDsaKey_Free(&mo_key);
}

/* Helpers on their own, odd length so the partial trailing cell is covered. */
static void mo_pack_roundtrip(void)
{
    byte src[65];
    byte packed[65];
    byte back[65];
    int  i;
    int  ret;
    int  ok;

    for (i = 0; i < (int)sizeof(src); i++) {
        src[i] = (byte)((i * 7 + 1) & 0xFF);
    }
    ret = wc_PackOctets(packed, (word32)sizeof(packed), src,
        (word32)sizeof(src), (word32)sizeof(src));
    if (ret == 0) {
        ret = wc_UnpackOctets(back, (word32)sizeof(back), packed,
            (word32)sizeof(packed), (word32)sizeof(src));
    }
    /* Round-trip alone would also pass for an identity implementation, so
     * check the packed layout itself: cell 0 must carry the first
     * WC_OCTETS_PER_BYTE octets, low octet first. */
    ok = (ret == 0) && (XMEMCMP(src, back, sizeof(src)) == 0);
    if (ok) {
        word32 expect = 0;
        word32 e;
        for (e = 0; e < WC_OCTETS_PER_BYTE; e++) {
            expect |= (word32)src[e] << (8 * e);
        }
        ok = ((word32)packed[0] == expect);
    }
    printf("wc_Pack/UnpackOctets round-trip: %s (ret=%d)\r\n",
           ok ? "PASS" : "FAIL", ret);
}

/* Key and signature stored PACKED, expanded before use. */
static void mo_packed_verify(void)
{
    int res = 0;
    int ret;

    ret = wc_UnpackOctets(mo_pub, (word32)sizeof(mo_pub),
        (const byte*)kat_mldsa65_pub_packed,
        (word32)(sizeof(kat_mldsa65_pub_packed) /
                 sizeof(kat_mldsa65_pub_packed[0])),
        (word32)sizeof(mo_pub));
    if (ret == 0) {
        ret = wc_UnpackOctets(mo_sig, (word32)sizeof(mo_sig),
            (const byte*)kat_mldsa65_sig_packed,
            (word32)(sizeof(kat_mldsa65_sig_packed) /
                     sizeof(kat_mldsa65_sig_packed[0])),
            (word32)sizeof(mo_sig));
    }
    /* Expanded buffers must match the plain arrays octet for octet. */
    if (ret == 0) {
        if ((XMEMCMP(mo_pub, kat_mldsa65_pub, sizeof(mo_pub)) != 0) ||
            (XMEMCMP(mo_sig, kat_mldsa65_sig, sizeof(mo_sig)) != 0)) {
            ret = -1;
        }
    }
    if (ret == 0) {
        ret = wc_MlDsaKey_Init(&mo_key, NULL, INVALID_DEVID);
    }
    if (ret == 0) {
        ret = wc_MlDsaKey_SetParams(&mo_key, WC_ML_DSA_65);
    }
    if (ret == 0) {
        ret = wc_MlDsaKey_ImportPubRaw(&mo_key, mo_pub, (word32)sizeof(mo_pub));
    }
    if (ret == 0) {
        ret = wc_MlDsaKey_VerifyCtx(&mo_key, mo_sig, (word32)sizeof(mo_sig),
            NULL, 0, mo_msg, (word32)sizeof(mo_msg), &res);
    }
    printf("ML-DSA-65 packed-signature verify: %s (ret=%d res=%d)\r\n",
           ((ret == 0) && (res == 1)) ? "PASS" : "FAIL", ret, res);
    wc_MlDsaKey_Free(&mo_key);
}

static void wolf_mldsa_octet_test(void)
{
    byte sha256[WC_SHA256_DIGEST_SIZE];
    byte sha512[WC_SHA512_DIGEST_SIZE];
    int  i;
    int  ret;

    /* msg[i] = i & 0xFF.  The mask is not cosmetic here: a C28x cell would
     * otherwise store 256..511 verbatim and corrupt the hash input. */
    for (i = 0; i < (int)sizeof(mo_msg); i++) {
        mo_msg[i] = (byte)(i & 0xFF);
    }
    ret = wc_Sha256Hash(mo_msg, (word32)sizeof(mo_msg), sha256);
    if (ret == 0) {
        ret = wc_Sha512Hash(mo_msg, (word32)sizeof(mo_msg), sha512);
    }
    if (ret != 0) {
        printf("ML-DSA octet tests: SKIP (digest failed ret=%d)\r\n", ret);
        return;
    }

    printf("octet model: CHAR_BIT=%d, one octet per byte cell, %lu octet(s) "
           "per cell when packed; ML-DSA-65 sig = %lu octets = %lu cells = "
           "%lu bytes of RAM\r\n",
           (int)CHAR_BIT, (unsigned long)WC_OCTETS_PER_BYTE,
           (unsigned long)WC_MLDSA_65_SIG_SIZE,
           (unsigned long)WC_MLDSA_65_SIG_SIZE,
           (unsigned long)WC_MLDSA_65_SIG_SIZE * (CHAR_BIT / 8));

    mo_pack_roundtrip();

    mo_verify("ML-DSA-44 verify KAT:", WC_ML_DSA_44, kat_mldsa44_pub,
        (word32)sizeof(kat_mldsa44_pub), kat_mldsa44_sig,
        (word32)sizeof(kat_mldsa44_sig), -1, NULL, 0);
    mo_verify("ML-DSA-65 verify KAT:", WC_ML_DSA_65, kat_mldsa65_pub,
        (word32)sizeof(kat_mldsa65_pub), kat_mldsa65_sig,
        (word32)sizeof(kat_mldsa65_sig), -1, NULL, 0);
    mo_verify("ML-DSA-87 verify KAT:", WC_ML_DSA_87, kat_mldsa87_pub,
        (word32)sizeof(kat_mldsa87_pub), kat_mldsa87_sig,
        (word32)sizeof(kat_mldsa87_sig), -1, NULL, 0);

    mo_verify("ML-DSA-44 VerifyCtxHash SHA-256 KAT:", WC_ML_DSA_44,
        kat_mldsa44_pub, (word32)sizeof(kat_mldsa44_pub),
        kat_mldsa44_sig_ph256, (word32)sizeof(kat_mldsa44_sig_ph256),
        WC_HASH_TYPE_SHA256, sha256, (word32)sizeof(sha256));
    mo_verify("ML-DSA-65 VerifyCtxHash SHA-256 KAT:", WC_ML_DSA_65,
        kat_mldsa65_pub, (word32)sizeof(kat_mldsa65_pub),
        kat_mldsa65_sig_ph256, (word32)sizeof(kat_mldsa65_sig_ph256),
        WC_HASH_TYPE_SHA256, sha256, (word32)sizeof(sha256));
    mo_verify("ML-DSA-87 VerifyCtxHash SHA-256 KAT:", WC_ML_DSA_87,
        kat_mldsa87_pub, (word32)sizeof(kat_mldsa87_pub),
        kat_mldsa87_sig_ph256, (word32)sizeof(kat_mldsa87_sig_ph256),
        WC_HASH_TYPE_SHA256, sha256, (word32)sizeof(sha256));
    mo_verify("ML-DSA-87 VerifyCtxHash SHA-512 KAT:", WC_ML_DSA_87,
        kat_mldsa87_pub, (word32)sizeof(kat_mldsa87_pub),
        kat_mldsa87_sig_ph512, (word32)sizeof(kat_mldsa87_sig_ph512),
        WC_HASH_TYPE_SHA512, sha512, (word32)sizeof(sha512));

    /* Negative case: every check above is positive, so a verify that returned
     * success unconditionally - or a w1 encoder that collapsed distinct
     * commitments - would pass them all.  Flip one octet of the level-44
     * signature and require a clean rejection. */
    {
        static byte bad[WC_MLDSA_44_SIG_SIZE];
        int bres = 1;
        int bret;

        XMEMCPY(bad, kat_mldsa44_sig, sizeof(bad));
        bad[sizeof(bad) / 2] ^= 0x01;

        bret = wc_MlDsaKey_Init(&mo_key, NULL, INVALID_DEVID);
        if (bret == 0) {
            bret = wc_MlDsaKey_SetParams(&mo_key, WC_ML_DSA_44);
        }
        if (bret == 0) {
            bret = wc_MlDsaKey_ImportPubRaw(&mo_key, kat_mldsa44_pub,
                (word32)sizeof(kat_mldsa44_pub));
        }
        if (bret == 0) {
            bret = wc_MlDsaKey_VerifyCtx(&mo_key, bad, (word32)sizeof(bad),
                NULL, 0, mo_msg, (word32)sizeof(mo_msg), &bres);
        }
        /* A corrupt signature must be rejected, not error out. */
        printf("ML-DSA-44 corrupted-signature reject: %s (ret=%d res=%d)\r\n",
               ((bret == 0) && (bres == 0)) ? "PASS" : "FAIL", bret, bres);
        wc_MlDsaKey_Free(&mo_key);
    }

    mo_packed_verify();
}
#endif /* WOLF_MLDSA_OCTETS */

#ifdef WOLF_SECUREBOOT
/* ------------------------------------------------------------------------- */
/* Pure-mode ML-DSA secure boot (make SECUREBOOT=1)                          */
/* ------------------------------------------------------------------------- */
/* Verifies a firmware image that is stored PACKED in flash - two octets per
 * 16-bit cell, the layout a host signing tool and the C28x programmer produce -
 * without ever holding the image in RAM.
 *
 * ML-DSA has no streaming interface, but the message reaches the algorithm only
 * through mu = SHAKE256(tr || 0x00 || ctxLen || ctx || M), and a hash streams.
 * wc_MlDsaKey_VerifyMu() takes mu directly (ExternalMu-ML-DSA), so the image is
 * read a chunk at a time, expanded with wc_UnpackOctets(), and absorbed.
 * RAM cost is the chunk buffer plus 128 octets, whatever the image size. */

#define SB_CHUNK 256            /* octets per flash read */

static wc_MlDsaKey sb_key;      /* .bss: too big for the 16 KW C28x stack */

/* Build mu over the packed image.  flip < 0 leaves the image intact; otherwise
 * one octet is corrupted, to prove a bad image is rejected. */
static int sb_build_mu(byte* mu, long flip)
{
    wc_Shake sh;
    byte tr[MLDSA_TR_SZ];
    byte buf[SB_CHUNK];
    byte prefix[2];
    word32 off;
    int ret;

    /* tr = SHAKE256(raw public key).  Constant for a fixed verification key,
     * so a production bootloader would precompute this at build time. */
    ret = wc_InitShake256(&sh, NULL, INVALID_DEVID);
    if (ret == 0) {
        ret = wc_Shake256_Update(&sh, sb_mldsa87_pub,
            (word32)sizeof(sb_mldsa87_pub));
    }
    if (ret == 0) {
        ret = wc_Shake256_Final(&sh, tr, (word32)sizeof(tr));
    }
    wc_Shake256_Free(&sh);
    if (ret != 0) {
        return ret;
    }

    /* mu = SHAKE256(tr || 0x00 || ctxLen || ctx || image).  0x00 selects pure
     * (non pre-hash) mode; the context here is empty. */
    prefix[0] = 0x00;
    prefix[1] = 0x00;
    ret = wc_InitShake256(&sh, NULL, INVALID_DEVID);
    if (ret == 0) {
        ret = wc_Shake256_Update(&sh, tr, (word32)sizeof(tr));
    }
    if (ret == 0) {
        ret = wc_Shake256_Update(&sh, prefix, 2);
    }
    for (off = 0; (ret == 0) && (off < SB_IMG_SZ); off += SB_CHUNK) {
        word32 n = SB_IMG_SZ - off;
        if (n > SB_CHUNK) {
            n = SB_CHUNK;
        }
        ret = wc_UnpackOctets(buf, (word32)sizeof(buf),
            (const byte*)sb_image_packed + (off / WC_OCTETS_PER_BYTE),
            WC_PACKED_CELLS(n), n);
        if ((ret == 0) && (flip >= 0) &&
            ((word32)flip >= off) && ((word32)flip < off + n)) {
            buf[(word32)flip - off] ^= 0x01;
        }
        if (ret == 0) {
            ret = wc_Shake256_Update(&sh, buf, n);
        }
    }
    if (ret == 0) {
        ret = wc_Shake256_Final(&sh, mu, MLDSA_MU_SZ);
    }
    wc_Shake256_Free(&sh);
    return ret;
}

static void sb_verify(const char* what, long flip, int want)
{
    byte mu[MLDSA_MU_SZ];
    int res = -1;
    int ret;

    ret = sb_build_mu(mu, flip);
    if (ret == 0) {
        ret = wc_MlDsaKey_Init(&sb_key, NULL, INVALID_DEVID);
    }
    if (ret == 0) {
        ret = wc_MlDsaKey_SetParams(&sb_key, WC_ML_DSA_87);
    }
    if (ret == 0) {
        ret = wc_MlDsaKey_ImportPubRaw(&sb_key, sb_mldsa87_pub,
            (word32)sizeof(sb_mldsa87_pub));
    }
#ifdef WOLFSSL_MLDSA_VERIFY_PRECOMP_A
    if (ret == 0) {
        /* A was expanded at build time and lives in flash: verify skips the
         * SHAKE128 rejection sampling entirely. */
        ret = wc_MlDsaKey_SetPrecompA(&sb_key, sb_mldsa87_A, SB_A_LEN,
            sb_rho, (word32)sizeof(sb_rho));
    }
#endif
    if (ret == 0) {
        ret = wc_MlDsaKey_VerifyMu(&sb_key, sb_mldsa87_sig,
            (word32)sizeof(sb_mldsa87_sig), mu, MLDSA_MU_SZ, &res);
    }
    printf("%s %s (ret=%d res=%d)\r\n", what,
           ((ret == 0) && (res == want)) ? "PASS" : "FAIL", ret, res);
    wc_MlDsaKey_Free(&sb_key);
}

static void wolf_secureboot_test(void)
{
    printf("secure boot: %lu-octet packed image, %u-octet chunks, "
           "image never resident\r\n",
           (unsigned long)SB_IMG_SZ, (unsigned)SB_CHUNK);
#ifdef WOLFSSL_MLDSA_VERIFY_PRECOMP_A
    printf("matrix A: precomputed in flash (%ld coefficients)\r\n",
           (long)SB_A_LEN);
#else
    printf("matrix A: expanded at run time (SHAKE128 rejection sampling)\r\n");
#endif
    sb_verify("ML-DSA-87 pure-mode packed-image verify:", -1, 1);
    sb_verify("ML-DSA-87 corrupted-image reject:", SB_IMG_SZ / 2, 0);

#ifdef WOLF_MLDSA_VERIFY_BENCH
    /* Time the ML-DSA verify alone (mu is already built), which is the figure
     * a bootloader budget cares about. */
    {
        byte mu[MLDSA_MU_SZ];
        int res = 0;
        int bret;
        int bi;
        double t;

        /* Time mu construction too: for a real firmware image the streamed
         * SHAKE-256 hashing, not the ML-DSA verify, sets the boot budget. */
        (void)current_time(1);
        bret = sb_build_mu(mu, -1);
        t = current_time(0);
        if (bret == 0) {
            printf("mu build (%lu octets streamed): %.2f ms = %.1f KiB/s\r\n",
                   (unsigned long)SB_IMG_SZ, t * 1000.0,
                   ((double)SB_IMG_SZ / 1024.0) / t);
        }
        if (bret == 0) {
            bret = wc_MlDsaKey_Init(&sb_key, NULL, INVALID_DEVID);
        }
        if (bret == 0) {
            bret = wc_MlDsaKey_SetParams(&sb_key, WC_ML_DSA_87);
        }
        if (bret == 0) {
            bret = wc_MlDsaKey_ImportPubRaw(&sb_key, sb_mldsa87_pub,
                (word32)sizeof(sb_mldsa87_pub));
        }
#ifdef WOLFSSL_MLDSA_VERIFY_PRECOMP_A
        if (bret == 0) {
            bret = wc_MlDsaKey_SetPrecompA(&sb_key, sb_mldsa87_A, SB_A_LEN,
                sb_rho, (word32)sizeof(sb_rho));
        }
#endif
        if (bret == 0) {
            (void)current_time(1);
            for (bi = 0; bi < 10; bi++) {
                (void)wc_MlDsaKey_VerifyMu(&sb_key, sb_mldsa87_sig,
                    (word32)sizeof(sb_mldsa87_sig), mu, MLDSA_MU_SZ, &res);
            }
            t = current_time(0);
            printf("ML-DSA-87 VerifyMu bench: 10 ops in %.3f s = %.2f ms/op\r\n",
                   t, (t * 1000.0) / 10.0);
        }
        wc_MlDsaKey_Free(&sb_key);
    }
#endif
}
#endif /* WOLF_SECUREBOOT */

#ifdef WOLF_MLDSA_SIGN
/* ML-DSA-87 sign+verify round-trip (keygen -> sign -> verify).  Exercises
 * the full signer on the C28x.  Uses the DEV RNG stub (NOT secure) - this
 * proves functional correctness, not security. */
static void wolf_mldsa87_sign_test(void)
{
    WC_RNG sg_rng;
    word32 sg_sigLen = (word32)sizeof(sg_sig);
    int sg_i, sg_res = 0, sg_ret;
#ifdef WOLF_MLDSA_SIGN_BENCH
    double t;
    int    b;
#endif

    for (sg_i = 0; sg_i < (int)sizeof(sg_msg); sg_i++) {
        sg_msg[sg_i] = (byte)(sg_i & 0xFF);
    }
    sg_ret = wc_InitRng(&sg_rng);
    if (sg_ret == 0) {
        sg_ret = wc_MlDsaKey_Init(&sgk, NULL, INVALID_DEVID);
    }
    if (sg_ret == 0) {
        sg_ret = wc_MlDsaKey_SetParams(&sgk, WC_ML_DSA_87);
    }
    if (sg_ret == 0) {
        MEMPROF_BEGIN();
        sg_ret = wc_MlDsaKey_MakeKey(&sgk, &sg_rng);
        MEMPROF_END("ML-DSA-87", "keygen");
    }
    printf("ML-DSA-87 MakeKey: ret=%d\r\n", sg_ret);
    if (sg_ret == 0) {
        MEMPROF_BEGIN();
        sg_ret = wc_MlDsaKey_SignCtx(&sgk, NULL, 0, sg_sig, &sg_sigLen,
            sg_msg, (word32)sizeof(sg_msg), &sg_rng);
        MEMPROF_END("ML-DSA-87", "sign");
    }
    printf("ML-DSA-87 Sign: ret=%d sigLen=%lu\r\n", sg_ret,
           (unsigned long)sg_sigLen);
#ifdef WOLF_MEM_PROFILE
    printf("MEMPROF ML-DSA-87 key_sizeof=%lu octets\r\n",
           (unsigned long)sizeof(sgk) * 2UL);
#endif
    if (sg_ret == 0) {
        sg_ret = wc_MlDsaKey_VerifyCtx(&sgk, sg_sig, sg_sigLen, NULL, 0,
            sg_msg, (word32)sizeof(sg_msg), &sg_res);
    }
    printf("ML-DSA-87 sign/verify round-trip: %s (ret=%d res=%d)\r\n",
           (sg_ret == 0 && sg_res == 1) ? "PASS" : "FAIL", sg_ret, sg_res);
#ifdef WOLF_MLDSA_SIGN_BENCH
    if (sg_ret == 0) {
        (void)current_time(1);
        for (b = 0; b < 3; b++) {
            (void)wc_MlDsaKey_MakeKey(&sgk, &sg_rng);
        }
        t = current_time(0);
        printf("ML-DSA-87 keygen bench: 3 ops in %.3f s = %.2f ms/op\r\n",
               t, t * 1000.0 / 3);
        (void)current_time(1);
        for (b = 0; b < 3; b++) {
            sg_sigLen = (word32)sizeof(sg_sig);
            (void)wc_MlDsaKey_SignCtx(&sgk, NULL, 0, sg_sig, &sg_sigLen,
                sg_msg, (word32)sizeof(sg_msg), &sg_rng);
        }
        t = current_time(0);
        printf("ML-DSA-87 sign bench: 3 ops in %.3f s = %.2f ms/op\r\n",
               t, t * 1000.0 / 3);
    }
#endif /* WOLF_MLDSA_SIGN_BENCH */
    wc_MlDsaKey_Free(&sgk);
    wc_FreeRng(&sg_rng);
}
#endif /* WOLF_MLDSA_SIGN */

#ifdef WOLF_MLKEM
static void wolf_mlkem768_test(void)
{
    /* Key struct and key/CT buffers are static (.bss), NOT stack: the C28x
     * stack pointer is 16-bit, so the whole stack is 16 KW (see the linker
     * command file), and MlKemKey plus the encoded public/private key buffers
     * alone would overflow it.  Same rule as README.md "Notes on the C28x". */
    static MlKemKey kemk;
    static byte kem_pk[WC_ML_KEM_768_PUBLIC_KEY_SIZE];
    static byte kem_sk[WC_ML_KEM_768_PRIVATE_KEY_SIZE];
    static byte kem_ct[WC_ML_KEM_768_CIPHER_TEXT_SIZE];
    static byte kem_ss1[WC_ML_KEM_SS_SZ];
    static byte kem_ss2[WC_ML_KEM_SS_SZ];
    static byte kem_seed[64];
    static byte kem_coins[32];
    word32 kem_pklen = (word32)sizeof(kem_pk);
    word32 kem_sklen = (word32)sizeof(kem_sk);
    int kem_ret, kem_ok = 0, i;

    /* Deterministic seed/coins identical to the host reference dumper, so
     * pk/ct/ss can be diffed byte-for-byte to localize a CHAR_BIT==16 bug. */
    for (i = 0; i < 64; i++) {
        kem_seed[i] = (byte)i;
    }
    for (i = 0; i < 32; i++) {
        kem_coins[i] = (byte)(0x40 + i);
    }
    kem_ret = wc_MlKemKey_Init(&kemk, WC_ML_KEM_768, NULL, INVALID_DEVID);
    if (kem_ret == 0) {
        kem_ret = wc_MlKemKey_MakeKeyWithRandom(&kemk, kem_seed, 64);
    }
    printf("ML-KEM-768 MakeKey: ret=%d\r\n", kem_ret);
    if (kem_ret == 0) {
        kem_ret = wc_MlKemKey_EncodePublicKey(&kemk, kem_pk, kem_pklen);
    }
    if (kem_ret == 0) {
        kem_dump("PK", kem_pk, (int)kem_pklen, 32, 16);
    }
    if (kem_ret == 0) {
        kem_ret = wc_MlKemKey_EncodePrivateKey(&kemk, kem_sk, kem_sklen);
    }
    if (kem_ret == 0) {
        kem_dump("SK", kem_sk, (int)sizeof(kem_sk), 32, 16);
    }
    if (kem_ret == 0) {
        kem_ret = wc_MlKemKey_EncapsulateWithRandom(&kemk, kem_ct, kem_ss1,
            kem_coins, 32);
    }
    printf("ML-KEM-768 Encapsulate: ret=%d\r\n", kem_ret);
    if (kem_ret == 0) {
        kem_dump("CT", kem_ct, (int)sizeof(kem_ct), 32, 16);
        kem_dump("SS", kem_ss1, (int)WC_ML_KEM_SS_SZ, 32, 0);
    }
    if (kem_ret == 0) {
        kem_ret = wc_MlKemKey_Decapsulate(&kemk, kem_ss2, kem_ct,
            (word32)sizeof(kem_ct));
    }
    if (kem_ret == 0) {
        kem_ok = (XMEMCMP(kem_ss1, kem_ss2, WC_ML_KEM_SS_SZ) == 0);
    }
    printf("ML-KEM-768 encap/decap round-trip: %s (ret=%d ss_match=%d)\r\n",
           (kem_ret == 0 && kem_ok) ? "PASS" : "FAIL", kem_ret, kem_ok);
    wc_MlKemKey_Free(&kemk);
}
#endif /* WOLF_MLKEM */

#ifdef WOLF_ENTROPY
/* On-target validation of the entropy source.  Beyond the port's SP800-90B
 * health tests, this screens what a stuck or test-only source would fail: the
 * raw noise is neither constant nor grossly biased, and the DRBG seeds.  The
 * bit-count check is coarse - the real min-entropy assessment is the host
 * analysis of an ENTROPY_PROBE=1 capture (wolfSSL IDE/C2000/README.md). */
static void wolf_entropy_test(void)
{
    static byte raw[256];
    static byte s1[32], s2[32];
    WC_RNG rng;
    word32 ones;
    word32 i;
    int b;
    int ret;
    int src;

    ret = wc_c2000_Entropy_Init();
    printf("Entropy init + startup health test: %s (ret=%d)\r\n",
           (ret == 0) ? "PASS" : "FAIL", ret);
    if (ret != 0) {
        return;
    }

    ret = wc_c2000_Entropy_SelfTest();
    printf("Entropy liveness self-test (raw): %s (ret=%d)\r\n",
           (ret == 0) ? "PASS" : "FAIL", ret);

    /* Raw noise sanity per source: population count should sit near half.
     * Source 1 is optional - a build that needs DCC0 elsewhere sets
     * WOLFSSL_C2000_ENTROPY_NUM_SRC to 1. */
    for (src = 0; src < WOLFSSL_C2000_ENTROPY_NUM_SRC; src++) {
        ret = wc_c2000_Entropy_GetRaw(raw, (word32)sizeof(raw), src);
        if (ret != 0) {
            /* raw[] holds stale data on failure, so counting it would report a
             * meaningless balance.  Report the read error instead. */
            printf("Entropy raw src%d bit balance: FAIL (read error %d)\r\n",
                   src, ret);
            continue;
        }
        ones = 0;
        for (i = 0; i < (word32)sizeof(raw); i++) {
            for (b = 0; b < 8; b++) {
                if ((raw[i] >> b) & 1) {
                    ones++;
                }
            }
        }
        /* 2048 bits; accept 40%..60% ones, i.e. counts 820..1228. */
        printf("Entropy raw src%d bit balance: %s (%lu/2048 ones)\r\n",
               src,
               (ones > 819UL && ones < 1229UL) ? "PASS" : "FAIL",
               (unsigned long)ones);
    }

    /* End to end: the DRBG must seed and produce differing blocks. */
    ret = wc_InitRng(&rng);
    printf("wc_InitRng with real entropy: %s (ret=%d)\r\n",
           (ret == 0) ? "PASS" : "FAIL", ret);
    if (ret == 0) {
        ret = wc_RNG_GenerateBlock(&rng, s1, (word32)sizeof(s1));
        if (ret == 0) {
            ret = wc_RNG_GenerateBlock(&rng, s2, (word32)sizeof(s2));
        }
        printf("RNG blocks differ: %s\r\n",
               (ret == 0 && XMEMCMP(s1, s2, sizeof(s1)) != 0)
                   ? "PASS" : "FAIL");
        wc_FreeRng(&rng);
    }
}
#endif /* WOLF_ENTROPY */

#ifdef WOLF_AES
static void wolf_aes_test(void)
{
    /* AES-128 NIST SP800-38A vectors (CBC/CTR/CFB) + McGrew GCM case 2. */
    static const byte k[16] = {
        0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
        0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c};
    static const byte iv[16] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static const byte pt[16] = {
        0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
        0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a};
    static const byte cbc_ct[16] = {
        0x76,0x49,0xab,0xac,0x81,0x19,0xb2,0x46,
        0xce,0xe9,0x8e,0x9b,0x12,0xe9,0x19,0x7d};
    static const byte ctr_iv[16] = {
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
        0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff};
    static const byte ctr_ct[16] = {
        0x87,0x4d,0x61,0x91,0xb6,0x20,0xe3,0x26,
        0x1b,0xef,0x68,0x64,0x99,0x0d,0xb6,0xce};
    static const byte cfb_ct[16] = {
        0x3b,0x3f,0xd9,0x2e,0xb7,0x2d,0xad,0x20,
        0x33,0x34,0x49,0xf8,0xe8,0x3c,0xfb,0x4a};
    static const byte gk[16] = {0};
    static const byte giv[12] = {0};
    static const byte gpt[16] = {0};
    static const byte gct[16] = {
        0x03,0x88,0xda,0xce,0x60,0xb6,0xa3,0x92,
        0xf3,0x28,0xc2,0xb9,0x71,0xb2,0xfe,0x78};
    static const byte gtag[16] = {
        0xab,0x6e,0x47,0xd4,0x2c,0xec,0x13,0xbd,
        0xf5,0x3a,0x67,0xb2,0x12,0x57,0xbd,0xdf};
    static Aes aes;
    static byte o[16], o2[16], tag[16];
    int r;

    /* Must be initialised, and explicitly with INVALID_DEVID: a static Aes
     * zero-fills devId to 0, which is a valid device id, so with WOLF_CRYPTO_CB
     * built in (HWAES=1) every aes.c hook would attempt callback dispatch
     * instead of skipping.  This is the software reference for the HW-vs-SW
     * cross-checks, so it must stay unambiguously software. */
    r = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (r != 0) {
        printf("AES software test: FAIL (init ret=%d)\r\n", r);
        return;
    }

    /* CBC */
    r = wc_AesSetKey(&aes, k, 16, iv, AES_ENCRYPTION);
    if (r == 0) r = wc_AesCbcEncrypt(&aes, o, pt, 16);
    printf("AES-128-CBC encrypt: %s\r\n",
        (r == 0 && XMEMCMP(o, cbc_ct, 16) == 0) ? "PASS" : "FAIL");
    r = wc_AesSetKey(&aes, k, 16, iv, AES_DECRYPTION);
    if (r == 0) r = wc_AesCbcDecrypt(&aes, o2, cbc_ct, 16);
    printf("AES-128-CBC decrypt: %s\r\n",
        (r == 0 && XMEMCMP(o2, pt, 16) == 0) ? "PASS" : "FAIL");

    /* CTR (encrypt == decrypt) */
    r = wc_AesSetKey(&aes, k, 16, ctr_iv, AES_ENCRYPTION);
    if (r == 0) r = wc_AesCtrEncrypt(&aes, o, pt, 16);
    printf("AES-128-CTR: %s\r\n",
        (r == 0 && XMEMCMP(o, ctr_ct, 16) == 0) ? "PASS" : "FAIL");

    /* CFB128 */
    r = wc_AesSetKey(&aes, k, 16, iv, AES_ENCRYPTION);
    if (r == 0) r = wc_AesCfbEncrypt(&aes, o, pt, 16);
    printf("AES-128-CFB encrypt: %s\r\n",
        (r == 0 && XMEMCMP(o, cfb_ct, 16) == 0) ? "PASS" : "FAIL");
    r = wc_AesSetKey(&aes, k, 16, iv, AES_ENCRYPTION);
    if (r == 0) r = wc_AesCfbDecrypt(&aes, o2, cfb_ct, 16);
    printf("AES-128-CFB decrypt: %s\r\n",
        (r == 0 && XMEMCMP(o2, pt, 16) == 0) ? "PASS" : "FAIL");

    /* GCM */
    r = wc_AesGcmSetKey(&aes, gk, 16);
    if (r == 0) r = wc_AesGcmEncrypt(&aes, o, gpt, 16, giv, 12,
        tag, 16, NULL, 0);
    printf("AES-128-GCM encrypt: %s (ct=%d tag=%d)\r\n",
        (r == 0 && XMEMCMP(o, gct, 16) == 0 &&
         XMEMCMP(tag, gtag, 16) == 0) ? "PASS" : "FAIL",
        (r == 0) ? (XMEMCMP(o, gct, 16) == 0) : -1,
        (r == 0) ? (XMEMCMP(tag, gtag, 16) == 0) : -1);
    r = wc_AesGcmDecrypt(&aes, o2, gct, 16, giv, 12, gtag, 16, NULL, 0);
    printf("AES-128-GCM decrypt+verify: %s\r\n",
        (r == 0 && XMEMCMP(o2, gpt, 16) == 0) ? "PASS" : "FAIL");
    wc_AesFree(&aes);
}
#endif /* WOLF_AES */

#ifdef WOLF_HWAES
/* Cross-check the AESA hardware against software AES.
 *
 * Two contexts deliberately: 'hw' carries WOLFSSL_C2000_DEVID so every aes.c
 * hook routes to the callback, 'sw' carries INVALID_DEVID so every hook skips
 * it.  That separation matters -- with HAVE_AES_ECB on, a devId-bearing
 * context would route even the software CTR path's internal wc_AesEcbEncrypt
 * back to hardware.
 *
 * NIST SP800-38A vectors are asserted where we have them; multi-block,
 * split-call and in-place cases are checked hardware-against-software, since
 * software AES is already covered by wolfcrypt_test. */
/* Set only when the AESA device actually registered.  Without it the 'hw'
 * context silently falls back to software and every cross-check would compare
 * software against software and report PASS. */
static int g_aesaReady = 0;

static void hw_report(const char* name, int r, const byte* a, const byte* b,
                      word32 len)
{
    int cmp = XMEMCMP(a, b, len);
    printf("HW %s: %s (ret=%d cmp=%d)\r\n", name,
        (r == 0 && cmp == 0) ? "PASS" : "FAIL", r, cmp);
}

static void wolf_aes_hw_test(void)
{
    /* NIST SP800-38A F.1/F.2/F.5 four-block plaintext. */
    static const byte pt[64] = {
        0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
        0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a,
        0xae,0x2d,0x8a,0x57,0x1e,0x03,0xac,0x9c,
        0x9e,0xb7,0x6f,0xac,0x45,0xaf,0x8e,0x51,
        0x30,0xc8,0x1c,0x46,0xa3,0x5c,0xe4,0x11,
        0xe5,0xfb,0xc1,0x19,0x1a,0x0a,0x52,0xef,
        0xf6,0x9f,0x24,0x45,0xdf,0x4f,0x9b,0x17,
        0xad,0x2b,0x41,0x7b,0xe6,0x6c,0x37,0x10};
    static const byte k128[16] = {
        0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
        0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c};
    static const byte k192[24] = {
        0x8e,0x73,0xb0,0xf7,0xda,0x0e,0x64,0x52,
        0xc8,0x10,0xf3,0x2b,0x80,0x90,0x79,0xe5,
        0x62,0xf8,0xea,0xd2,0x52,0x2c,0x6b,0x7b};
    static const byte k256[32] = {
        0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,
        0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
        0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,
        0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4};
    static const byte iv[16] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static const byte ctr_iv[16] = {
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
        0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff};
    /* First-block published answers (F.1.1, F.2.1, F.5.1). */
    static const byte ecb_ct1[16] = {
        0x3a,0xd7,0x7b,0xb4,0x0d,0x7a,0x36,0x60,
        0xa8,0x9e,0xca,0xf3,0x24,0x66,0xef,0x97};
    static const byte cbc_ct1[16] = {
        0x76,0x49,0xab,0xac,0x81,0x19,0xb2,0x46,
        0xce,0xe9,0x8e,0x9b,0x12,0xe9,0x19,0x7d};
    /* Full 64-octet CTR answer, not just block 1: this vector's counter starts
     * at ...fe ff, so block 2 is the first needing a carry across an octet
     * boundary, and checking only block 1 hides a broken increment. */
    static const byte ctr_ct[64] = {
        0x87,0x4d,0x61,0x91,0xb6,0x20,0xe3,0x26,
        0x1b,0xef,0x68,0x64,0x99,0x0d,0xb6,0xce,
        0x98,0x06,0xf6,0x6b,0x79,0x70,0xfd,0xff,
        0x86,0x17,0x18,0x7b,0xb9,0xff,0xfd,0xff,
        0x5a,0xe4,0xdf,0x3e,0xdb,0xd5,0xd3,0x5e,
        0x5b,0x4f,0x09,0x02,0x0d,0xb0,0x3e,0xab,
        0x1e,0x03,0x1d,0xda,0x2f,0xbe,0x03,0xd1,
        0x79,0x21,0x70,0xa0,0xf3,0x00,0x9c,0xee};

    /* .bss, not stack: the C28x stack is 16 KW and an Aes is not small. */
    static Aes hw, sw;
    static byte oh[64], os[64], dh[64];
    int rh, rs;

    if (!g_aesaReady) {
        printf("HW AES cross-checks: SKIP (AESA not registered)\r\n");
        return;
    }

    rh = wc_AesInit(&hw, NULL, WOLFSSL_C2000_DEVID);
    if (rh != 0) {
        printf("HW AES init (hw ctx): FAIL (ret=%d)\r\n", rh);
        return;
    }
    rs = wc_AesInit(&sw, NULL, INVALID_DEVID);
    if (rs != 0) {
        printf("HW AES init (sw ctx): FAIL (ret=%d)\r\n", rs);
        wc_AesFree(&hw);
        return;
    }

    /* ---- ECB, 64 octets (exercises the multi-block loop) ---- */
    rh = wc_AesSetKey(&hw, k128, 16, NULL, AES_ENCRYPTION);
    rs = wc_AesSetKey(&sw, k128, 16, NULL, AES_ENCRYPTION);
    if (rh == 0) rh = wc_AesEcbEncrypt(&hw, oh, pt, 64);
    if (rs == 0) rs = wc_AesEcbEncrypt(&sw, os, pt, 64);
    hw_report("AES-128-ECB encrypt vs NIST", rh, oh, ecb_ct1, 16);
    hw_report("AES-128-ECB encrypt vs SW", ((rh != 0) ? rh : rs), oh, os, 64);

    rh = wc_AesSetKey(&hw, k128, 16, NULL, AES_DECRYPTION);
    if (rh == 0) rh = wc_AesEcbDecrypt(&hw, dh, oh, 64);
    hw_report("AES-128-ECB decrypt round-trip", rh, dh, pt, 64);

    /* ---- CBC, 64 octets ---- */
    rh = wc_AesSetKey(&hw, k128, 16, iv, AES_ENCRYPTION);
    rs = wc_AesSetKey(&sw, k128, 16, iv, AES_ENCRYPTION);
    if (rh == 0) rh = wc_AesCbcEncrypt(&hw, oh, pt, 64);
    if (rs == 0) rs = wc_AesCbcEncrypt(&sw, os, pt, 64);
    hw_report("AES-128-CBC encrypt vs NIST", rh, oh, cbc_ct1, 16);
    hw_report("AES-128-CBC encrypt vs SW", ((rh != 0) ? rh : rs), oh, os, 64);

    rh = wc_AesSetKey(&hw, k128, 16, iv, AES_DECRYPTION);
    if (rh == 0) rh = wc_AesCbcDecrypt(&hw, dh, oh, 64);
    hw_report("AES-128-CBC decrypt round-trip", rh, dh, pt, 64);

    /* ---- CBC split across calls: proves aes->reg chaining ---- */
    rh = wc_AesSetKey(&hw, k128, 16, iv, AES_ENCRYPTION);
    if (rh == 0) rh = wc_AesCbcEncrypt(&hw, oh, pt, 16);
    if (rh == 0) rh = wc_AesCbcEncrypt(&hw, oh + 16, pt + 16, 48);
    hw_report("AES-128-CBC split-call chain", rh, oh, os, 64);

    /* ---- CBC in-place decrypt: proves the last-block save ---- */
    XMEMCPY(dh, os, 64);
    rh = wc_AesSetKey(&hw, k128, 16, iv, AES_DECRYPTION);
    if (rh == 0) rh = wc_AesCbcDecrypt(&hw, dh, dh, 64);
    hw_report("AES-128-CBC in-place decrypt", rh, dh, pt, 64);

    /* ---- CTR, 64 octets ---- */
    rh = wc_AesSetKey(&hw, k128, 16, ctr_iv, AES_ENCRYPTION);
    rs = wc_AesSetKey(&sw, k128, 16, ctr_iv, AES_ENCRYPTION);
    if (rh == 0) rh = wc_AesCtrEncrypt(&hw, oh, pt, 64);
    if (rs == 0) rs = wc_AesCtrEncrypt(&sw, os, pt, 64);
    hw_report("AES-128-CTR vs NIST (64B)", rh, oh, ctr_ct, 64);
    hw_report("AES-128-CTR SW vs NIST (64B)", rs, os, ctr_ct, 64);
    hw_report("AES-128-CTR vs SW", ((rh != 0) ? rh : rs), oh, os, 64);

    /* ---- CTR split at a non-block boundary: proves aes->left/aes->tmp ---- */
    rh = wc_AesSetKey(&hw, k128, 16, ctr_iv, AES_ENCRYPTION);
    if (rh == 0) rh = wc_AesCtrEncrypt(&hw, oh, pt, 10);
    if (rh == 0) rh = wc_AesCtrEncrypt(&hw, oh + 10, pt + 10, 54);
    hw_report("AES-128-CTR partial split", rh, oh, os, 64);

    /* ---- 192- and 256-bit keys: the 6- and 8-word AES_setKey1 paths ---- */
    rh = wc_AesSetKey(&hw, k192, 24, iv, AES_ENCRYPTION);
    rs = wc_AesSetKey(&sw, k192, 24, iv, AES_ENCRYPTION);
    if (rh == 0) rh = wc_AesCbcEncrypt(&hw, oh, pt, 64);
    if (rs == 0) rs = wc_AesCbcEncrypt(&sw, os, pt, 64);
    hw_report("AES-192-CBC encrypt vs SW", ((rh != 0) ? rh : rs), oh, os, 64);

    rh = wc_AesSetKey(&hw, k256, 32, iv, AES_ENCRYPTION);
    rs = wc_AesSetKey(&sw, k256, 32, iv, AES_ENCRYPTION);
    if (rh == 0) rh = wc_AesCbcEncrypt(&hw, oh, pt, 64);
    if (rs == 0) rs = wc_AesCbcEncrypt(&sw, os, pt, 64);
    hw_report("AES-256-CBC encrypt vs SW", ((rh != 0) ? rh : rs), oh, os, 64);

    /* ECB and CTR at 192/256 too: the accelerator's key schedule differs per
     * key size, and marshalling the longer schedule into its 32-bit registers
     * is exactly the octet handling this port is validating. */
    rh = wc_AesSetKey(&hw, k192, 24, NULL, AES_ENCRYPTION);
    rs = wc_AesSetKey(&sw, k192, 24, NULL, AES_ENCRYPTION);
    if (rh == 0) rh = wc_AesEcbEncrypt(&hw, oh, pt, 64);
    if (rs == 0) rs = wc_AesEcbEncrypt(&sw, os, pt, 64);
    hw_report("AES-192-ECB encrypt vs SW", ((rh != 0) ? rh : rs), oh, os, 64);

    rh = wc_AesSetKey(&hw, k256, 32, NULL, AES_ENCRYPTION);
    rs = wc_AesSetKey(&sw, k256, 32, NULL, AES_ENCRYPTION);
    if (rh == 0) rh = wc_AesEcbEncrypt(&hw, oh, pt, 64);
    if (rs == 0) rs = wc_AesEcbEncrypt(&sw, os, pt, 64);
    hw_report("AES-256-ECB encrypt vs SW", ((rh != 0) ? rh : rs), oh, os, 64);

    rh = wc_AesSetKey(&hw, k192, 24, ctr_iv, AES_ENCRYPTION);
    rs = wc_AesSetKey(&sw, k192, 24, ctr_iv, AES_ENCRYPTION);
    if (rh == 0) rh = wc_AesCtrEncrypt(&hw, oh, pt, 64);
    if (rs == 0) rs = wc_AesCtrEncrypt(&sw, os, pt, 64);
    hw_report("AES-192-CTR encrypt vs SW", ((rh != 0) ? rh : rs), oh, os, 64);

    rh = wc_AesSetKey(&hw, k256, 32, ctr_iv, AES_ENCRYPTION);
    rs = wc_AesSetKey(&sw, k256, 32, ctr_iv, AES_ENCRYPTION);
    if (rh == 0) rh = wc_AesCtrEncrypt(&hw, oh, pt, 64);
    if (rs == 0) rs = wc_AesCtrEncrypt(&sw, os, pt, 64);
    hw_report("AES-256-CTR encrypt vs SW", ((rh != 0) ? rh : rs), oh, os, 64);

    wc_AesFree(&hw);
    wc_AesFree(&sw);
}
#endif /* WOLF_HWAES */

#ifdef WOLF_25519
static void wolf_curve25519_test(void)
{
    /* X25519 RFC 7748 6.1 Diffie-Hellman test vectors (little-endian). */
    static const byte a_priv[32] = {
        0x77,0x07,0x6d,0x0a,0x73,0x18,0xa5,0x7d,0x3c,0x16,0xc1,0x72,0x51,0xb2,0x66,0x45,
        0xdf,0x4c,0x2f,0x87,0xeb,0xc0,0x99,0x2a,0xb1,0x77,0xfb,0xa5,0x1d,0xb9,0x2c,0x2a};
    static const byte a_pub[32] = {
        0x85,0x20,0xf0,0x09,0x89,0x30,0xa7,0x54,0x74,0x8b,0x7d,0xdc,0xb4,0x3e,0xf7,0x5a,
        0x0d,0xbf,0x3a,0x0d,0x26,0x38,0x1a,0xf4,0xeb,0xa4,0xa9,0x8e,0xaa,0x9b,0x4e,0x6a};
    static const byte b_priv[32] = {
        0x5d,0xab,0x08,0x7e,0x62,0x4a,0x8a,0x4b,0x79,0xe1,0x7f,0x8b,0x83,0x80,0x0e,0xe6,
        0x6f,0x3b,0xb1,0x29,0x26,0x18,0xb6,0xfd,0x1c,0x2f,0x8b,0x27,0xff,0x88,0xe0,0xeb};
    static const byte b_pub[32] = {
        0xde,0x9e,0xdb,0x7d,0x7b,0x7d,0xc1,0xb4,0xd3,0x5b,0x61,0xc2,0xec,0xe4,0x35,0x37,
        0x3f,0x83,0x43,0xc8,0x5b,0x78,0x67,0x4d,0xad,0xfc,0x7e,0x14,0x6f,0x88,0x2b,0x4f};
    static const byte x_K[32] = {
        0x4a,0x5d,0x9d,0x5b,0xa4,0xce,0x2d,0xe1,0x72,0x8e,0x3b,0xf4,0x80,0x35,0x0f,0x25,
        0xe0,0x7e,0x21,0xc9,0x47,0xd1,0x9e,0x33,0x76,0xf0,0x9b,0x3c,0x1e,0x16,0x17,0x42};
    /* Ed25519 RFC 8032 Test 1 (empty message). */
    static const byte ed_sk[32] = {
        0x9d,0x61,0xb1,0x9d,0xef,0xfd,0x5a,0x60,0xba,0x84,0x4a,0xf4,0x92,0xec,0x2c,0xc4,
        0x44,0x49,0xc5,0x69,0x7b,0x32,0x69,0x19,0x70,0x3b,0xac,0x03,0x1c,0xae,0x7f,0x60};
    static const byte ed_pk[32] = {
        0xd7,0x5a,0x98,0x01,0x82,0xb1,0x0a,0xb7,0xd5,0x4b,0xfe,0xd3,0xc9,0x64,0x07,0x3a,
        0x0e,0xe1,0x72,0xf3,0xda,0xa6,0x23,0x25,0xaf,0x02,0x1a,0x68,0xf7,0x07,0x51,0x1a};
    static const byte ed_sig[64] = {
        0xe5,0x56,0x43,0x00,0xc3,0x60,0xac,0x72,0x90,0x86,0xe2,0xcc,0x80,0x6e,0x82,0x8a,
        0x84,0x87,0x7f,0x1e,0xb8,0xe5,0xd9,0x74,0xd8,0x73,0xe0,0x65,0x22,0x49,0x01,0x55,
        0x5f,0xb8,0x82,0x15,0x90,0xa3,0x3b,0xac,0xc6,0x1e,0x39,0x70,0x1c,0xf9,0xb4,0x6b,
        0xd2,0x5b,0xf5,0xf0,0x59,0x5b,0xbe,0x24,0x65,0x51,0x41,0x43,0x8e,0x7a,0x10,0x0b};
    static curve25519_key ca, cpa, cb, cpb;
    static ed25519_key ed;
    static WC_RNG rng;
    byte sh[32], sig[64], ap[32], bp[32];
    word32 shLen = sizeof(sh), sigLen = sizeof(sig);
    int r, vres = 0, ci, rngOk;

    /* X25519: shared = X25519(a_priv, b_pub) == X25519(b_priv, a_pub) == K.
     * wolfSSL requires the scalar to be pre-clamped (RFC 7748 sec 5); the
     * raw RFC 6.1 keys are not, so clamp a local copy (X25519 clamps the
     * same bits internally, so the shared secret still equals K).  The
     * default C build enables WOLFSSL_CURVE25519_BLINDING, so each private
     * key needs an RNG set (the result is unaffected). */
    rngOk = (wc_InitRng(&rng) == 0);
    if (!rngOk)
        printf("X25519: RNG init FAIL\r\n");
    for (ci = 0; ci < 32; ci++) { ap[ci] = a_priv[ci]; bp[ci] = b_priv[ci]; }
    ap[0] &= 248; ap[31] &= 127; ap[31] |= 64;
    bp[0] &= 248; bp[31] &= 127; bp[31] |= 64;
    wc_curve25519_init(&ca);  wc_curve25519_init(&cpb);
    r = wc_curve25519_import_private_ex(ap, 32, &ca, EC25519_LITTLE_ENDIAN);
    if (r == 0 && rngOk) r = wc_curve25519_set_rng(&ca, &rng);
    if (r == 0) r = wc_curve25519_import_public_ex(b_pub, 32, &cpb,
                                            EC25519_LITTLE_ENDIAN);
    if (r == 0) r = wc_curve25519_shared_secret_ex(&ca, &cpb, sh, &shLen,
                                            EC25519_LITTLE_ENDIAN);
    printf("X25519 a*Bpub: %s\r\n",
        (r == 0 && shLen == 32 && XMEMCMP(sh, x_K, 32) == 0) ? "PASS":"FAIL");

    wc_curve25519_init(&cb);  wc_curve25519_init(&cpa);
    shLen = sizeof(sh);
    r = wc_curve25519_import_private_ex(bp, 32, &cb, EC25519_LITTLE_ENDIAN);
    if (r == 0 && rngOk) r = wc_curve25519_set_rng(&cb, &rng);
    if (r == 0) r = wc_curve25519_import_public_ex(a_pub, 32, &cpa,
                                            EC25519_LITTLE_ENDIAN);
    if (r == 0) r = wc_curve25519_shared_secret_ex(&cb, &cpa, sh, &shLen,
                                            EC25519_LITTLE_ENDIAN);
    printf("X25519 b*Apub: %s\r\n",
        (r == 0 && shLen == 32 && XMEMCMP(sh, x_K, 32) == 0) ? "PASS":"FAIL");
    wc_curve25519_free(&ca); wc_curve25519_free(&cpb);
    wc_curve25519_free(&cb); wc_curve25519_free(&cpa);
    if (rngOk) wc_FreeRng(&rng);

    /* Ed25519 RFC 8032 Test 1 (empty message): import (re-derives and
     * validates the public key on import), sign, compare to the RFC sig,
     * then verify.  inLen 0 still needs a non-NULL message pointer. */
    wc_ed25519_init(&ed);
    r = wc_ed25519_import_private_key(ed_sk, 32, ed_pk, 32, &ed);
    if (r == 0) r = wc_ed25519_sign_msg(ap, 0, sig, &sigLen, &ed);
    printf("Ed25519 sign:   %s\r\n",
        (r == 0 && sigLen == 64 && XMEMCMP(sig, ed_sig, 64) == 0) ?
        "PASS":"FAIL");
    r = wc_ed25519_verify_msg(ed_sig, 64, ap, 0, &vres, &ed);
    printf("Ed25519 verify: %s\r\n",
        (r == 0 && vres == 1) ? "PASS":"FAIL");
    wc_ed25519_free(&ed);
}
#endif /* WOLF_25519 */

#ifdef WOLF_448
static void wolf_curve448_test(void)
{
    /* X448 RFC 7748 6.2 Diffie-Hellman test vectors (little-endian). */
    static const byte x_a_priv[56] = {
        0x9a,0x8f,0x49,0x25,0xd1,0x51,0x9f,0x57,0x75,0xcf,0x46,0xb0,0x4b,0x58,0x00,0xd4,
        0xee,0x9e,0xe8,0xba,0xe8,0xbc,0x55,0x65,0xd4,0x98,0xc2,0x8d,0xd9,0xc9,0xba,0xf5,
        0x74,0xa9,0x41,0x97,0x44,0x89,0x73,0x91,0x00,0x63,0x82,0xa6,0xf1,0x27,0xab,0x1d,
        0x9a,0xc2,0xd8,0xc0,0xa5,0x98,0x72,0x6b
    };
    static const byte x_a_pub[56] = {
        0x9b,0x08,0xf7,0xcc,0x31,0xb7,0xe3,0xe6,0x7d,0x22,0xd5,0xae,0xa1,0x21,0x07,0x4a,
        0x27,0x3b,0xd2,0xb8,0x3d,0xe0,0x9c,0x63,0xfa,0xa7,0x3d,0x2c,0x22,0xc5,0xd9,0xbb,
        0xc8,0x36,0x64,0x72,0x41,0xd9,0x53,0xd4,0x0c,0x5b,0x12,0xda,0x88,0x12,0x0d,0x53,
        0x17,0x7f,0x80,0xe5,0x32,0xc4,0x1f,0xa0
    };
    static const byte x_b_priv[56] = {
        0x1c,0x30,0x6a,0x7a,0xc2,0xa0,0xe2,0xe0,0x99,0x0b,0x29,0x44,0x70,0xcb,0xa3,0x39,
        0xe6,0x45,0x37,0x72,0xb0,0x75,0x81,0x1d,0x8f,0xad,0x0d,0x1d,0x69,0x27,0xc1,0x20,
        0xbb,0x5e,0xe8,0x97,0x2b,0x0d,0x3e,0x21,0x37,0x4c,0x9c,0x92,0x1b,0x09,0xd1,0xb0,
        0x36,0x6f,0x10,0xb6,0x51,0x73,0x99,0x2d
    };
    static const byte x_b_pub[56] = {
        0x3e,0xb7,0xa8,0x29,0xb0,0xcd,0x20,0xf5,0xbc,0xfc,0x0b,0x59,0x9b,0x6f,0xec,0xcf,
        0x6d,0xa4,0x62,0x71,0x07,0xbd,0xb0,0xd4,0xf3,0x45,0xb4,0x30,0x27,0xd8,0xb9,0x72,
        0xfc,0x3e,0x34,0xfb,0x42,0x32,0xa1,0x3c,0xa7,0x06,0xdc,0xb5,0x7a,0xec,0x3d,0xae,
        0x07,0xbd,0xc1,0xc6,0x7b,0xf3,0x36,0x09
    };
    static const byte x_K[56] = {
        0x07,0xff,0xf4,0x18,0x1a,0xc6,0xcc,0x95,0xec,0x1c,0x16,0xa9,0x4a,0x0f,0x74,0xd1,
        0x2d,0xa2,0x32,0xce,0x40,0xa7,0x75,0x52,0x28,0x1d,0x28,0x2b,0xb6,0x0c,0x0b,0x56,
        0xfd,0x24,0x64,0xc3,0x35,0x54,0x39,0x36,0x52,0x1c,0x24,0x40,0x30,0x85,0xd5,0x9a,
        0x44,0x9a,0x50,0x37,0x51,0x4a,0x87,0x9d
    };
    /* Ed448 RFC 8032 7.4 first test ("Blank": empty message, empty
     * context).  The 57-byte secret/public keys and 114-byte signature. */
    static const byte ed_sk[57] = {
        0x6c,0x82,0xa5,0x62,0xcb,0x80,0x8d,0x10,0xd6,0x32,0xbe,0x89,0xc8,0x51,0x3e,0xbf,
        0x6c,0x92,0x9f,0x34,0xdd,0xfa,0x8c,0x9f,0x63,0xc9,0x96,0x0e,0xf6,0xe3,0x48,0xa3,
        0x52,0x8c,0x8a,0x3f,0xcc,0x2f,0x04,0x4e,0x39,0xa3,0xfc,0x5b,0x94,0x49,0x2f,0x8f,
        0x03,0x2e,0x75,0x49,0xa2,0x00,0x98,0xf9,0x5b
    };
    static const byte ed_pk[57] = {
        0x5f,0xd7,0x44,0x9b,0x59,0xb4,0x61,0xfd,0x2c,0xe7,0x87,0xec,0x61,0x6a,0xd4,0x6a,
        0x1d,0xa1,0x34,0x24,0x85,0xa7,0x0e,0x1f,0x8a,0x0e,0xa7,0x5d,0x80,0xe9,0x67,0x78,
        0xed,0xf1,0x24,0x76,0x9b,0x46,0xc7,0x06,0x1b,0xd6,0x78,0x3d,0xf1,0xe5,0x0f,0x6c,
        0xd1,0xfa,0x1a,0xbe,0xaf,0xe8,0x25,0x61,0x80
    };
    static const byte ed_sig[114] = {
        0x53,0x3a,0x37,0xf6,0xbb,0xe4,0x57,0x25,0x1f,0x02,0x3c,0x0d,0x88,0xf9,0x76,0xae,
        0x2d,0xfb,0x50,0x4a,0x84,0x3e,0x34,0xd2,0x07,0x4f,0xd8,0x23,0xd4,0x1a,0x59,0x1f,
        0x2b,0x23,0x3f,0x03,0x4f,0x62,0x82,0x81,0xf2,0xfd,0x7a,0x22,0xdd,0xd4,0x7d,0x78,
        0x28,0xc5,0x9b,0xd0,0xa2,0x1b,0xfd,0x39,0x80,0xff,0x0d,0x20,0x28,0xd4,0xb1,0x8a,
        0x9d,0xf6,0x3e,0x00,0x6c,0x5d,0x1c,0x2d,0x34,0x5b,0x92,0x5d,0x8d,0xc0,0x0b,0x41,
        0x04,0x85,0x2d,0xb9,0x9a,0xc5,0xc7,0xcd,0xda,0x85,0x30,0xa1,0x13,0xa0,0xf4,0xdb,
        0xb6,0x11,0x49,0xf0,0x5a,0x73,0x63,0x26,0x8c,0x71,0xd9,0x58,0x08,0xff,0x2e,0x65,
        0x26,0x00
    };
    static curve448_key xa, xpa, xb, xpb;
    static ed448_key ed;
    byte sh[56], sig[114];
    word32 shLen = sizeof(sh), sigLen = sizeof(sig);
    int r, vres = 0;

    /* X448: shared = X448(a_priv, b_pub) == X448(b_priv, a_pub) == K.
     * wc_curve448 clamps the scalar internally, so the raw RFC scalars
     * import directly; no caller-side clamp or RNG/blinding is needed. */
    wc_curve448_init(&xa);  wc_curve448_init(&xpb);
    r = wc_curve448_import_private_ex(x_a_priv, 56, &xa, EC448_LITTLE_ENDIAN);
    if (r == 0) r = wc_curve448_import_public_ex(x_b_pub, 56, &xpb,
                                            EC448_LITTLE_ENDIAN);
    if (r == 0) r = wc_curve448_shared_secret_ex(&xa, &xpb, sh, &shLen,
                                            EC448_LITTLE_ENDIAN);
    printf("X448 a*Bpub: %s\r\n",
        (r == 0 && shLen == 56 && XMEMCMP(sh, x_K, 56) == 0) ? "PASS":"FAIL");

    wc_curve448_init(&xb);  wc_curve448_init(&xpa);
    shLen = sizeof(sh);
    r = wc_curve448_import_private_ex(x_b_priv, 56, &xb, EC448_LITTLE_ENDIAN);
    if (r == 0) r = wc_curve448_import_public_ex(x_a_pub, 56, &xpa,
                                            EC448_LITTLE_ENDIAN);
    if (r == 0) r = wc_curve448_shared_secret_ex(&xb, &xpa, sh, &shLen,
                                            EC448_LITTLE_ENDIAN);
    printf("X448 b*Apub: %s\r\n",
        (r == 0 && shLen == 56 && XMEMCMP(sh, x_K, 56) == 0) ? "PASS":"FAIL");
    wc_curve448_free(&xa); wc_curve448_free(&xpb);
    wc_curve448_free(&xb); wc_curve448_free(&xpa);

    /* Ed448 RFC 8032 7.4 (blank): import, sign empty message with empty
     * context, compare to the RFC signature, then verify. */
    wc_ed448_init(&ed);
    r = wc_ed448_import_private_key(ed_sk, 57, ed_pk, 57, &ed);
    if (r == 0) r = wc_ed448_sign_msg(ed_sk, 0, sig, &sigLen, &ed, NULL, 0);
    printf("Ed448 sign:   %s\r\n",
        (r == 0 && sigLen == 114 && XMEMCMP(sig, ed_sig, 114) == 0) ?
        "PASS":"FAIL");
    r = wc_ed448_verify_msg(ed_sig, 114, ed_sk, 0, &vres, &ed, NULL, 0);
    printf("Ed448 verify: %s\r\n",
        (r == 0 && vres == 1) ? "PASS":"FAIL");
    wc_ed448_free(&ed);
}
#endif /* WOLF_448 */

#ifdef WOLF_HKDF
static void wolf_hkdf_test(void)
{
    /* HMAC-SHA256: RFC 4231 test case 2. */
    static const byte hk[4]  = {0x4a,0x65,0x66,0x65}; /* "Jefe" */
    static const byte hd[28] = {
        0x77,0x68,0x61,0x74,0x20,0x64,0x6f,0x20,0x79,0x61,0x20,0x77,0x61,0x6e,
        0x74,0x20,0x66,0x6f,0x72,0x20,0x6e,0x6f,0x74,0x68,0x69,0x6e,0x67,0x3f};
    static const byte hmac_exp[32] = {
        0x5b,0xdc,0xc1,0x46,0xbf,0x60,0x75,0x4e,0x6a,0x04,0x24,0x26,0x08,0x95,
        0x75,0xc7,0x5a,0x00,0x3f,0x08,0x9d,0x27,0x39,0x83,0x9d,0xec,0x58,0xb9,
        0x64,0xec,0x38,0x43};
    /* HKDF-SHA256: RFC 5869 test case 1. */
    static const byte ikm[22] = {
        0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,
        0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b};
    static const byte salt[13] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c};
    static const byte info[10] = {
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9};
    static const byte okm_exp[42] = {
        0x3c,0xb2,0x5f,0x25,0xfa,0xac,0xd5,0x7a,0x90,0x43,0x4f,0x64,0xd0,0x36,
        0x2f,0x2a,0x2d,0x2d,0x0a,0x90,0xcf,0x1a,0x5a,0x4c,0x5d,0xb0,0x2d,0x56,
        0xec,0xc4,0xc5,0xbf,0x34,0x00,0x72,0x08,0xd5,0xb8,0x87,0x18,0x58,0x65};
    Hmac hmac;
    byte mac[32], okm[42];
    int r;

    r = wc_HmacInit(&hmac, NULL, INVALID_DEVID);
    if (r == 0) r = wc_HmacSetKey(&hmac, WC_SHA256, hk, sizeof(hk));
    if (r == 0) r = wc_HmacUpdate(&hmac, hd, sizeof(hd));
    if (r == 0) r = wc_HmacFinal(&hmac, mac);
    printf("HMAC-SHA256:  %s\r\n",
        (r == 0 && XMEMCMP(mac, hmac_exp, 32) == 0) ? "PASS":"FAIL");
    wc_HmacFree(&hmac);

    r = wc_HKDF(WC_SHA256, ikm, sizeof(ikm), salt, sizeof(salt),
                info, sizeof(info), okm, sizeof(okm));
    printf("HKDF-SHA256:  %s\r\n",
        (r == 0 && XMEMCMP(okm, okm_exp, 42) == 0) ? "PASS":"FAIL");
}
#endif /* WOLF_HKDF */

#ifdef WOLF_CHACHA
static void wolf_poly1305_test(void)
{
    /* Standalone Poly1305 (RFC 8439 sec 2.5.2) to isolate MAC vs AEAD. */
    static const byte pk[32] = {
        0x85,0xd6,0xbe,0x78,0x57,0x55,0x6d,0x33,0x7f,0x44,0x52,0xfe,0x42,0xd5,0x06,0xa8,
        0x01,0x03,0x80,0x8a,0xfb,0x0d,0xb2,0xfd,0x4a,0xbf,0xf6,0xaf,0x41,0x49,0xf5,0x1b};
    static const byte pm[34] = {
        0x43,0x72,0x79,0x70,0x74,0x6f,0x67,0x72,0x61,0x70,0x68,0x69,0x63,0x20,0x46,0x6f,
        0x72,0x75,0x6d,0x20,0x52,0x65,0x73,0x65,0x61,0x72,0x63,0x68,0x20,0x47,0x72,0x6f,
        0x75,0x70};
    static const byte ptag_exp[16] = {
        0xa8,0x06,0x1d,0xc1,0x30,0x51,0x36,0xc6,0xc2,0x2b,0x8b,0xaf,0x0c,0x01,0x27,0xa9};
    Poly1305 poly;
    byte ptag[16];
    int pr;
    pr = wc_Poly1305SetKey(&poly, pk, sizeof(pk));
    if (pr == 0) pr = wc_Poly1305Update(&poly, pm, sizeof(pm));
    if (pr == 0) pr = wc_Poly1305Final(&poly, ptag);
    printf("Poly1305 (standalone): %s (t=%02x%02x%02x%02x exp=a8061dc1)\r\n",
        (pr == 0 && XMEMCMP(ptag, ptag_exp, 16) == 0) ? "PASS":"FAIL",
        ptag[0]&0xFF, ptag[1]&0xFF, ptag[2]&0xFF, ptag[3]&0xFF);
}
#endif /* WOLF_CHACHA */

#ifdef WOLF_CHACHA
static void wolf_chacha20_poly1305_test(void)
{
    static const byte key[32] = {
        0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
        0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f};
    static const byte iv[12] = {
        0x07,0x00,0x00,0x00,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47};
    static const byte aad[12] = {
        0x50,0x51,0x52,0x53,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7};
    static const byte pt[114] = {
        0x4c,0x61,0x64,0x69,0x65,0x73,0x20,0x61,0x6e,0x64,0x20,0x47,0x65,0x6e,0x74,0x6c,
        0x65,0x6d,0x65,0x6e,0x20,0x6f,0x66,0x20,0x74,0x68,0x65,0x20,0x63,0x6c,0x61,0x73,
        0x73,0x20,0x6f,0x66,0x20,0x27,0x39,0x39,0x3a,0x20,0x49,0x66,0x20,0x49,0x20,0x63,
        0x6f,0x75,0x6c,0x64,0x20,0x6f,0x66,0x66,0x65,0x72,0x20,0x79,0x6f,0x75,0x20,0x6f,
        0x6e,0x6c,0x79,0x20,0x6f,0x6e,0x65,0x20,0x74,0x69,0x70,0x20,0x66,0x6f,0x72,0x20,
        0x74,0x68,0x65,0x20,0x66,0x75,0x74,0x75,0x72,0x65,0x2c,0x20,0x73,0x75,0x6e,0x73,
        0x63,0x72,0x65,0x65,0x6e,0x20,0x77,0x6f,0x75,0x6c,0x64,0x20,0x62,0x65,0x20,0x69,
        0x74,0x2e};
    static const byte ct0[4]  = {0xd3,0x1a,0x8d,0x34}; /* RFC ciphertext head */
    static const byte tag_exp[16] = {
        0x1a,0xe1,0x0b,0x59,0x4f,0x09,0xe2,0x6a,0x7e,0x90,0x2e,0xcb,0xd0,0x60,0x06,0x91};
    static byte ct[114], tag[16], dec[114];
    int r;

    r = wc_ChaCha20Poly1305_Encrypt(key, iv, aad, sizeof(aad),
            pt, sizeof(pt), ct, tag);
    printf("ChaCha20-Poly1305 encrypt: %s (r=%d)\r\n",
        (r == 0 && XMEMCMP(ct, ct0, 4) == 0 &&
         XMEMCMP(tag, tag_exp, 16) == 0) ? "PASS":"FAIL", r);
    r = wc_ChaCha20Poly1305_Decrypt(key, iv, aad, sizeof(aad),
            ct, sizeof(ct), tag, dec);
    printf("ChaCha20-Poly1305 decrypt+verify: %s (r=%d)\r\n",
        (r == 0 && XMEMCMP(dec, pt, sizeof(pt)) == 0) ? "PASS":"FAIL", r);
}
#endif /* WOLF_CHACHA */

#ifdef WOLF_AESEXTRA
static void wolf_aes_cmac_ccm_test(void)
{
    /* AES-128-CMAC (NIST SP800-38B example, 16-byte message). */
    static const byte ck[16] = {
        0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
        0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c};
    static const byte cmsg[16] = {
        0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
        0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a};
    static const byte cmac_exp[16] = {
        0x07,0x0a,0x16,0xb4,0x6b,0x4d,0x41,0x44,
        0xf7,0x9b,0xdd,0x9d,0xd0,0x4a,0x28,0x7c};
    /* AES-128-CCM (RFC 3610 packet vector #1). */
    static const byte ek[16] = {
        0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
        0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf};
    static const byte enonce[13] = {
        0x00,0x00,0x00,0x03,0x02,0x01,0x00,0xa0,0xa1,0xa2,0xa3,0xa4,0xa5};
    static const byte eaad[8] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};
    static const byte ept[23] = {
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,
        0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e};
    static const byte ect_exp[23] = {
        0x58,0x8c,0x97,0x9a,0x61,0xc6,0x63,0xd2,0xf0,0x66,0xd0,0xc2,
        0xc0,0xf9,0x89,0x80,0x6d,0x5f,0x6b,0x61,0xda,0xc3,0x84};
    static const byte etag_exp[8] = {
        0x17,0xe8,0xd1,0x2c,0xfd,0xf9,0x26,0xe0};
    static Aes aes;
    byte mac[16], cct[23], ctag[8], cdec[23];
    word32 macSz = sizeof(mac);
    int r;

    r = wc_AesCmacGenerate(mac, &macSz, cmsg, sizeof(cmsg), ck, sizeof(ck));
    printf("AES-128-CMAC:  %s\r\n",
        (r == 0 && macSz == 16 && XMEMCMP(mac, cmac_exp, 16) == 0) ?
        "PASS":"FAIL");

    r = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (r == 0) r = wc_AesCcmSetKey(&aes, ek, sizeof(ek));
    if (r == 0) r = wc_AesCcmEncrypt(&aes, cct, ept, sizeof(ept),
        enonce, sizeof(enonce), ctag, sizeof(ctag), eaad, sizeof(eaad));
    printf("AES-128-CCM encrypt: %s\r\n",
        (r == 0 && XMEMCMP(cct, ect_exp, 23) == 0 &&
         XMEMCMP(ctag, etag_exp, 8) == 0) ? "PASS":"FAIL");
    r = wc_AesCcmDecrypt(&aes, cdec, ect_exp, sizeof(ect_exp),
        enonce, sizeof(enonce), etag_exp, sizeof(etag_exp), eaad, sizeof(eaad));
    printf("AES-128-CCM decrypt+verify: %s\r\n",
        (r == 0 && XMEMCMP(cdec, ept, sizeof(ept)) == 0) ? "PASS":"FAIL");
    wc_AesFree(&aes);

}

/* GMAC = AES-GCM authentication of AAD with no plaintext.  Self-consistent:
 * generate then verify (KAT-cross-checked on host). */
static void wolf_aes_gmac_test(void)
{
    static const byte gk[16] = {
        0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
        0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f};
    static const byte giv[12] = {
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b};
    static const byte gaad[16] = {
        0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
        0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f};
    static Aes aes;
    byte gtag[16];
    int r;

    r = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (r == 0) r = wc_AesGcmSetKey(&aes, gk, sizeof(gk));
    if (r == 0) r = wc_AesGcmEncrypt(&aes, NULL, NULL, 0, giv,
        sizeof(giv), gtag, sizeof(gtag), gaad, sizeof(gaad));
    if (r == 0) r = wc_AesGcmDecrypt(&aes, NULL, NULL, 0, giv,
        sizeof(giv), gtag, sizeof(gtag), gaad, sizeof(gaad));
    printf("AES-128-GMAC (gen+verify): %s\r\n", (r == 0) ? "PASS":"FAIL");
    wc_AesFree(&aes);
}
#endif /* WOLF_AESEXTRA */

#ifdef WOLF_AESEXTRA
/* Extra AES modes - self-consistent encrypt/decrypt round-trips. */
static void wolf_aes_modes_test(void)
{
    static const byte mk[32] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f};
    static const byte mpt[32] = {
        0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a,
        0xae,0x2d,0x8a,0x57,0x1e,0x03,0xac,0x9c,0x9e,0xb7,0x6f,0xac,0x45,0xaf,0x8e,0x51};
    static const byte miv[16] = {
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff};
    static byte mct[32], mdec[32];
    int r;
#ifdef WOLFSSL_AES_OFB
    {
        static Aes oaes;
        r = wc_AesInit(&oaes, NULL, INVALID_DEVID);
        if (r == 0) r = wc_AesSetKey(&oaes, mk, 16, miv, AES_ENCRYPTION);
        if (r == 0) r = wc_AesOfbEncrypt(&oaes, mct, mpt, 32);
        if (r == 0) r = wc_AesSetKey(&oaes, mk, 16, miv, AES_ENCRYPTION);
        if (r == 0) r = wc_AesOfbDecrypt(&oaes, mdec, mct, 32);
        printf("AES-128-OFB round-trip: %s (ret=%d)\r\n",
            (r == 0 && XMEMCMP(mdec, mpt, 32) == 0) ? "PASS":"FAIL", r);
        wc_AesFree(&oaes);
    }
#endif
#ifdef WOLFSSL_AES_XTS
    {
        static XtsAes xaes;
        r = wc_AesXtsSetKey(&xaes, mk, 32, AES_ENCRYPTION, NULL, INVALID_DEVID);
        if (r == 0) r = wc_AesXtsEncrypt(&xaes, mct, mpt, 32, miv, 16);
        if (r == 0) r = wc_AesXtsSetKey(&xaes, mk, 32, AES_DECRYPTION, NULL, INVALID_DEVID);
        if (r == 0) r = wc_AesXtsDecrypt(&xaes, mdec, mct, 32, miv, 16);
        printf("AES-128-XTS round-trip: %s\r\n",
            (r == 0 && XMEMCMP(mdec, mpt, 32) == 0) ? "PASS":"FAIL");
        wc_AesXtsFree(&xaes);
    }
#endif
#ifdef WOLFSSL_AES_SIV
    {
        static byte siv[16];
        r = wc_AesSivEncrypt(mk, 32, NULL, 0, miv, 16, mpt, 32, siv, mct);
        if (r == 0) r = wc_AesSivDecrypt(mk, 32, NULL, 0, miv, 16, mct, 32, siv, mdec);
        printf("AES-256-SIV round-trip: %s\r\n",
            (r == 0 && XMEMCMP(mdec, mpt, 32) == 0) ? "PASS":"FAIL");
    }
#endif
#ifdef WOLFSSL_AES_EAX
    {
        static byte etag[16];
        /* one-shot EAX requires a non-NULL authIn pointer; use a 4-byte AAD */
        r = wc_AesEaxEncryptAuth(mk, 16, mct, mpt, 32, miv, 16, etag, 16, miv, 4);
        if (r == 0) r = wc_AesEaxDecryptAuth(mk, 16, mdec, mct, 32, miv, 16,
            etag, 16, miv, 4);
        printf("AES-128-EAX round-trip: %s\r\n",
            (r == 0 && XMEMCMP(mdec, mpt, 32) == 0) ? "PASS":"FAIL");
    }
#endif
}
#endif /* WOLF_AESEXTRA */

#ifdef WOLF_RSA
static void wolf_rsa_verify_test(void)
{
    /* Recover the PKCS#1 v1.5 DigestInfo from the signature with the public
     * key and compare to the expected DigestInfo (SHA-256 prefix + hash). */
    static RsaKey rsaKey;
    /* In RSA_VERIFY_ONLY / SP_NO_MALLOC builds wc_RsaSSL_Verify runs the
     * modexp in place in the caller's buffer, so it must be at least the
     * key size (256 B for RSA-2048), not just the recovered DigestInfo. */
    byte out[256];
    int r, outLen = -1;

    r = wc_InitRsaKey(&rsaKey, NULL);
    if (r == 0) r = wc_RsaPublicKeyDecodeRaw(kat_rsa2048_n,
        (word32)sizeof(kat_rsa2048_n), kat_rsa2048_e,
        (word32)sizeof(kat_rsa2048_e), &rsaKey);
    if (r == 0) outLen = wc_RsaSSL_Verify(kat_rsa2048_sig,
        (word32)sizeof(kat_rsa2048_sig), out, (word32)sizeof(out), &rsaKey);
    printf("RSA-2048 verify: %s (recovered=%d exp=%d)\r\n",
        (outLen == (int)sizeof(kat_rsa2048_digestinfo) &&
         XMEMCMP(out, kat_rsa2048_digestinfo,
                 sizeof(kat_rsa2048_digestinfo)) == 0) ? "PASS":"FAIL",
        outLen, (int)sizeof(kat_rsa2048_digestinfo));
    wc_FreeRsaKey(&rsaKey);
}
#endif /* WOLF_RSA */

#ifdef WOLF_DH
static void wolf_dh_test(void)
{
    /* Deterministic (no RNG) self-consistency: with two fixed private
     * exponents a,b and the FFDHE-2048 generator g, derive pubA=g^a and
     * pubB=g^b (wc_DhAgree computes otherPub^priv mod p), then check
     * pubB^a == pubA^b == g^(ab). */
    static DhKey dh;
    static const byte privA[32] = {
        0x3a,0x1f,0x9c,0x42,0x77,0x05,0xe8,0xb3,0x11,0x6d,0x24,0x8f,0x90,0xcc,0x53,0x2e,
        0x84,0x19,0xbd,0x60,0x7a,0xf2,0x0c,0x95,0x38,0xd1,0x4b,0xe6,0x22,0x7f,0xa3,0x5d};
    static const byte privB[32] = {
        0xc4,0x82,0x6b,0x1d,0x59,0xee,0x30,0x97,0xab,0x40,0x12,0x7e,0xf5,0x68,0x0b,0xd9,
        0x2c,0x71,0x86,0x4f,0x03,0x9a,0xe1,0x55,0xb8,0x27,0x6c,0x90,0x44,0xdf,0x18,0x63};
    static byte pubA[256], pubB[256], secA[256], secB[256];
    const DhParams* p = wc_Dh_ffdhe2048_Get();
    word32 pubASz = sizeof(pubA), pubBSz = sizeof(pubB);
    word32 secASz = sizeof(secA), secBSz = sizeof(secB);
    int r = wc_InitDhKey(&dh);
    if (r == 0)
        r = wc_DhSetKey(&dh, p->p, p->p_len, p->g, p->g_len);
    if (r == 0)
        r = wc_DhAgree(&dh, pubA, &pubASz, privA, sizeof(privA), p->g, p->g_len);
    if (r == 0)
        r = wc_DhAgree(&dh, pubB, &pubBSz, privB, sizeof(privB), p->g, p->g_len);
    if (r == 0)
        r = wc_DhAgree(&dh, secA, &secASz, privA, sizeof(privA), pubB, pubBSz);
    if (r == 0)
        r = wc_DhAgree(&dh, secB, &secBSz, privB, sizeof(privB), pubA, pubASz);
    printf("DH FFDHE-2048 agree: %s (r=%d lenA=%lu lenB=%lu)\r\n",
        (r == 0 && secASz == secBSz && secASz > 0 &&
         XMEMCMP(secA, secB, secASz) == 0) ? "PASS":"FAIL",
        r, (unsigned long)secASz, (unsigned long)secBSz);
    wc_FreeDhKey(&dh);
}
#endif /* WOLF_DH */

#ifdef WOLF_RSASIGN
static void wolf_rsa_sign_test(void)
{
    /* Import a fixed raw private key, sign a 32-byte payload, then verify
     * with the public part and check the recovered payload (self-consistent
     * - exercises the SP private CRT path).  rkey and the 256-cell buffers are
     * static (.bss), not stack: under WOLFSSL_SP_NO_MALLOC RsaKey carries the
     * modexp workspace, which the 16 KW C28x stack cannot absorb. */
    static RsaKey rkey;
    static const byte rs_in[32] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f};
    static byte rs_sig[256];
    static byte rs_out[256];
    word32 rs_sigLen = (word32)sizeof(rs_sig);
    int r, sl, vlen = -1;

    r = wc_InitRsaKey(&rkey, NULL);
    if (r == 0) {
        r = wc_RsaPrivateKeyDecodeRaw(
            kat_rsapriv_n, (word32)sizeof(kat_rsapriv_n),
            kat_rsapriv_e, (word32)sizeof(kat_rsapriv_e),
            kat_rsapriv_d, (word32)sizeof(kat_rsapriv_d),
            kat_rsapriv_u, (word32)sizeof(kat_rsapriv_u),
            kat_rsapriv_p, (word32)sizeof(kat_rsapriv_p),
            kat_rsapriv_q, (word32)sizeof(kat_rsapriv_q),
            kat_rsapriv_dP, (word32)sizeof(kat_rsapriv_dP),
            kat_rsapriv_dQ, (word32)sizeof(kat_rsapriv_dQ), &rkey);
    }
    if (r == 0) {
        sl = wc_RsaSSL_Sign(rs_in, (word32)sizeof(rs_in), rs_sig, rs_sigLen,
            &rkey, NULL);
        if (sl > 0) { rs_sigLen = (word32)sl; } else { r = sl; }
    }
    if (r == 0) {
        vlen = wc_RsaSSL_Verify(rs_sig, rs_sigLen, rs_out,
            (word32)sizeof(rs_out), &rkey);
    }
    printf("RSA-2048 sign/verify round-trip: %s (sigLen=%lu vlen=%d)\r\n",
        (r == 0 && vlen == (int)sizeof(rs_in) &&
         XMEMCMP(rs_out, rs_in, sizeof(rs_in)) == 0) ? "PASS":"FAIL",
        (unsigned long)rs_sigLen, vlen);
    wc_FreeRsaKey(&rkey);
}
#endif /* WOLF_RSASIGN */

/* ------------------------------------------------------------------------- */
/* main                                                                      */
/* ------------------------------------------------------------------------- */
int main(void)
{
#ifndef NO_CRYPT_TEST
    wc_test_ret_t test_ret;
#endif
#ifndef NO_CRYPT_BENCHMARK
    int bench_ret;
#endif

    /* PLL to 150 MHz, flash wait states, disable watchdog, enable clocks. */
    Device_init();
    Device_initGPIO();
    c2000_sciInit();
#ifdef WOLF_C2000_SCI_STDOUT
    c2000_stdoutToSci();
#endif
    c2000_timerInit();

    /* Unbuffered stdout so output appears immediately and survives a crash,
     * whether routed to SCIA or the CCS console (CIO). */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\r\n");
    printf("=== wolfSSL wolfCrypt on TI C2000 LAUNCHXL-F28P55X ===\r\n");

#ifdef WOLF_ENTROPY_PROBE
    /* Measurement-only image: dump raw entropy samples and stop.  Nothing
     * after this runs, which is why the Makefile rejects combining
     * ENTROPY_PROBE=1 with the other image toggles. */
    entropy_probe_run();
    for (;;) {
        /* spin */
    }
#endif

#ifdef WOLF_MEM_PROFILE
    /* Route XMALLOC/XFREE/XREALLOC through the heap high-water tracker. */
    wolf_mem_install();
#endif

#ifdef WOLF_HWAES
    /* wolfCrypt_Init() is mandatory first: it sets every device-table slot to
     * INVALID_DEVID, and RegisterDevice only claims a slot marked that way.
     * Without it the table is BSS-zero and registration fails with BUFFER_E. */
    if (wolfCrypt_Init() != 0) {
        printf("wolfCrypt_Init: FAIL\r\n");
    }
    else if (wc_C2000_Init(WOLFSSL_C2000_DEVID) != 0) {
        printf("C2000 AESA init: FAIL\r\n");
    }
    else {
        printf("C2000 AESA init: PASS\r\n");
        g_aesaReady = 1;
    }
#endif

    wolf_sha3_256_test();
    wolf_sha256_test();
    wolf_shake256_test();
#ifndef WOLF_MLDSA_SIGN
    wolf_shake256_large_test();
#endif
    wolf_sha512_test();
#ifdef WOLF_SHA1
    wolf_sha1_test();
#endif

#ifdef WOLF_SECUREBOOT
    printf("\r\n--- Pure-mode ML-DSA secure boot ---\r\n");
    wolf_secureboot_test();
#elif defined(WOLF_MLDSA_OCTETS)
    printf("\r\n--- ML-DSA octet boundary (44/65/87, pre-hash, packed) ---\r\n");
    wolf_mldsa_octet_test();
#elif !defined(WOLF_MLDSA_SIGN)
    wolf_mldsa87_verify_test();
#else
    wolf_mldsa87_sign_test();
#ifndef WOLFSSL_NO_ML_DSA_44
    mldsa_sign_roundtrip(WC_ML_DSA_44, "ML-DSA-44");
#endif
#ifndef WOLFSSL_NO_ML_DSA_65
    mldsa_sign_roundtrip(WC_ML_DSA_65, "ML-DSA-65");
#endif
#endif /* ML-DSA test selection */

#ifdef WOLF_ECC
    printf("\r\n--- ECDSA/ECDH P-256 (SP) ---\r\n");
    wolf_ecc_test();
#endif

#ifdef WOLF_ECC384
    printf("\r\n--- ECDSA/ECDH P-384 (SP) ---\r\n");
    wolf_ecc384_test();
#endif

#ifdef WOLF_ECC_BENCH
    printf("\r\n--- ECDSA / SHA-384 image-verify timings ---\r\n");
    wolf_ecc_bench();
#endif

#ifdef WOLF_MLKEM
    printf("\r\n--- ML-KEM-768 ---\r\n");
    wolf_mlkem768_test();
#ifndef WOLFSSL_NO_ML_KEM_512
    (void)mlkem_roundtrip(WC_ML_KEM_512, "ML-KEM-512");
#endif
#ifndef WOLFSSL_NO_ML_KEM_1024
    (void)mlkem_roundtrip(WC_ML_KEM_1024, "ML-KEM-1024");
#endif
#endif /* WOLF_MLKEM */

#ifdef WOLF_ENTROPY
    printf("\r\n--- Entropy (oscillator jitter) ---\r\n");
    wolf_entropy_test();
#endif /* WOLF_ENTROPY */

#ifdef WOLF_AES
    printf("\r\n--- AES (CBC/CTR/CFB/GCM) ---\r\n");
    wolf_aes_test();
#endif /* WOLF_AES */

#ifdef WOLF_HWAES
    printf("\r\n--- AES hardware (AESA) vs software ---\r\n");
    wolf_aes_hw_test();
#endif /* WOLF_HWAES */

#ifdef WOLF_25519
    printf("\r\n--- Curve25519 (X25519) + Ed25519 ---\r\n");
    wolf_curve25519_test();
#endif /* WOLF_25519 */

#ifdef WOLF_448
    printf("\r\n--- Curve448 (X448) + Ed448 ---\r\n");
    wolf_curve448_test();
#endif /* WOLF_448 */

#ifdef WOLF_HKDF
    printf("\r\n--- HMAC-SHA256 + HKDF-SHA256 ---\r\n");
    wolf_hkdf_test();
#endif /* WOLF_HKDF */

#ifdef WOLF_CHACHA
    printf("\r\n--- ChaCha20-Poly1305 AEAD (RFC 8439) ---\r\n");
    wolf_poly1305_test();
    wolf_chacha20_poly1305_test();
#endif /* WOLF_CHACHA */

#ifdef WOLF_AESEXTRA
    printf("\r\n--- AES-CMAC / CCM / GMAC ---\r\n");
    wolf_aes_cmac_ccm_test();
    wolf_aes_gmac_test();
    wolf_aes_modes_test();
#endif /* WOLF_AESEXTRA */

#ifdef WOLF_RSA
    printf("\r\n--- RSA-2048 verify (PKCS#1 v1.5, SHA-256) ---\r\n");
    wolf_rsa_verify_test();
#endif /* WOLF_RSA */

#ifdef WOLF_DH
    printf("\r\n--- DH (FFDHE-2048) ---\r\n");
    wolf_dh_test();
#endif /* WOLF_DH */

#ifdef WOLF_RSASIGN
    printf("\r\n--- RSA-2048 sign (PKCS#1 v1.5, SP private/CRT) ---\r\n");
    wolf_rsa_sign_test();
#endif /* WOLF_RSASIGN */

    printf("SYSCLK %lu Hz, CHAR_BIT %d, sizeof(long)=%d sizeof(long long)=%d\r\n",
           (unsigned long)DEVICE_SYSCLK_FREQ, (int)CHAR_BIT,
           (int)sizeof(long), (int)sizeof(long long));

#ifndef NO_CRYPT_TEST
    printf("\r\n--- wolfcrypt_test ---\r\n");
    test_ret = wolfcrypt_test(NULL);
    printf("wolfcrypt_test result: %ld\r\n", (long)test_ret);
#endif

#ifndef NO_CRYPT_BENCHMARK
    g_logpos = 0; /* reset capture so the full benchmark fits the JTAG log */
    printf("\r\n--- benchmark_test ---\r\n");
    bench_ret = benchmark_test(NULL);
    printf("benchmark_test result: %d\r\n", bench_ret);
#endif

    printf("\r\n=== done ===\r\n");

    for (;;) {
        /* spin */
    }
}
