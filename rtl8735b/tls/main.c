/* wolfCrypt AmebaPro2 (RTL8735B) HUK-in-TLS example -- built inside the RealTek
 * FreeRTOS SDK. Runs an in-memory TLS 1.2 ECDHE-ECDSA handshake (client + server
 * in one firmware, no networking) where the server's ECDSA P-256 authentication
 * routes through the HUK crypto-callback device (wolfSSL_CTX_SetDevId), i.e. the
 * server signature is produced on the RTL8735B HW ECDSA engine. The ECDHE key
 * agreement and record-layer AES run in software (the engine has no arbitrary
 * scalar-mult path; see the port README). Proves HUK-accelerated server auth in
 * a real TLS handshake.
 *
 * Two handshakes run:
 *   1. Server auth: a plain P-256 server key from certs_test.h; its handshake
 *      ECDSA sign dispatches to the HW engine.
 *   2. Mutual auth with a device-bound client key: the client's P-256 scalar is
 *      wrapped under the HUK (AES-GCM) and only the wrapped blob is kept. The
 *      client loads its certificate and public key, and its CertificateVerify
 *      signature comes from the blob. A tampered blob must fail the handshake.
 *
 * Build: configure with -DEXAMPLE=wolfcrypt_huk_tls (see wolfcrypt_huk_tls.cmake).
 */

#include <string.h>

#include "platform_stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "device_lock.h"
#include "hal_trng_sec.h"

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/certs_test.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/port/realtek/rtl8735b.h>

/* ForceZero, inlined the same way wolfCrypt's test.c does. */
#ifdef NO_INLINE
    #include <wolfssl/wolfcrypt/misc.h>
#else
    #define WOLFSSL_MISC_INCLUDED
    #include <wolfcrypt/src/misc.c>
#endif

#define STACKSIZE   (16 * 1024)
#define BUFFER_SIZE 2048

#define CHECK(label, cond) \
    dbg_printf("[%s] %s\r\n", (cond) ? "PASS" : "FAIL", (label))

/* wolfCrypt RNG seed hook (user_settings: CUSTOM_RAND_GENERATE_SEED), from the
 * AmebaPro2 secure hardware TRNG. */
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

/* In-memory transport: two shared buffers with WANT_READ/WANT_WRITE semantics.
 * client -> server via server_buffer; server -> client via client_buffer. */
static unsigned char client_buffer[BUFFER_SIZE];
static int client_buffer_sz = 0;
static unsigned char server_buffer[BUFFER_SIZE];
static int server_buffer_sz = 0;

static int recv_client(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    (void)ssl; (void)ctx;
    if (client_buffer_sz > 0) {
        if (sz > client_buffer_sz) {
            sz = client_buffer_sz;
        }
        memcpy(buff, client_buffer, sz);
        if (sz < client_buffer_sz) {
            memmove(client_buffer, client_buffer + sz, client_buffer_sz - sz);
        }
        client_buffer_sz -= sz;
        return sz;
    }
    return WOLFSSL_CBIO_ERR_WANT_READ;
}

static int send_client(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    (void)ssl; (void)ctx;
    if (server_buffer_sz < BUFFER_SIZE) {
        if (sz > BUFFER_SIZE - server_buffer_sz) {
            sz = BUFFER_SIZE - server_buffer_sz;
        }
        memcpy(server_buffer + server_buffer_sz, buff, sz);
        server_buffer_sz += sz;
        return sz;
    }
    return WOLFSSL_CBIO_ERR_WANT_WRITE;
}

static int recv_server(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    (void)ssl; (void)ctx;
    if (server_buffer_sz > 0) {
        if (sz > server_buffer_sz) {
            sz = server_buffer_sz;
        }
        memcpy(buff, server_buffer, sz);
        if (sz < server_buffer_sz) {
            memmove(server_buffer, server_buffer + sz, server_buffer_sz - sz);
        }
        server_buffer_sz -= sz;
        return sz;
    }
    return WOLFSSL_CBIO_ERR_WANT_READ;
}

static int send_server(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    (void)ssl; (void)ctx;
    if (client_buffer_sz < BUFFER_SIZE) {
        if (sz > BUFFER_SIZE - client_buffer_sz) {
            sz = BUFFER_SIZE - client_buffer_sz;
        }
        memcpy(client_buffer + client_buffer_sz, buff, sz);
        client_buffer_sz += sz;
        return sz;
    }
    return WOLFSSL_CBIO_ERR_WANT_WRITE;
}

/* RNG for the ECDSA sign callback (the PK callback passes no rng). */
static WC_RNG g_signRng;

/* ECC sign PK callback: route ONLY the server's cert signature to the HUK HW
 * ECDSA engine. A temp ecc_key with devId=WC_HUK_DEVID makes wc_ecc_sign_hash
 * dispatch to the port's general HW offload (signs with the key's own scalar);
 * the rest of TLS (PRF/HMAC, record AES, ECDHE) stays in software. */
static int huk_ecc_sign_cb(WOLFSSL* ssl, const byte* in, word32 inSz,
        byte* out, word32* outSz, const byte* keyDer, word32 keySz, void* ctx)
{
    ecc_key key;
    word32  idx = 0;
    int     ret;
    (void)ssl; (void)ctx;

    ret = wc_ecc_init_ex(&key, NULL, WC_HUK_DEVID);
    if (ret == 0) {
        ret = wc_EccPrivateKeyDecode(keyDer, &idx, &key, keySz);
        if (ret == 0) {
            ret = wc_ecc_sign_hash(in, inSz, out, outSz, &g_signRng, &key);
        }
        wc_ecc_free(&key);
    }
    return ret;
}

/* A HUK-wrapped P-256 private key as an application stores it (for example in
 * flash). The seed is not secret; only the device that wrapped the scalar can
 * unwrap it. The scalar is wrapped with AES-GCM (authenticated, recommended) or
 * AES-ECB (the STM32 DHUK blob layout); an ECB blob has no IV or tag. */
#define HUK_WRAP_GCM 0
#define HUK_WRAP_ECB 1

typedef struct huk_key_blob {
    byte format;            /* HUK_WRAP_GCM or HUK_WRAP_ECB */
    byte seed[32];
    byte iv[12];
    byte tag[16];
    byte wrapped[32];
} huk_key_blob;

/* Provisioning step: wrap the client's private scalar under the HUK with a
 * fresh seed (and IV for GCM). The plaintext key from certs_test.h stands in
 * for a key whose certificate was issued at manufacture; after this only the
 * blob is kept. */
static int huk_wrap_client_key(WC_RNG* rng, byte format, huk_key_blob* blob)
{
    ecc_key key;
    Aes     aes;
    byte    d[32] __attribute__((aligned(32)));
    word32  dLen = sizeof(d);
    word32  idx = 0;
    int     ret;

    if (format != HUK_WRAP_GCM && format != HUK_WRAP_ECB) {
        return BAD_FUNC_ARG;
    }
    memset(blob, 0, sizeof(*blob));
    blob->format = format;
    ret = wc_ecc_init(&key);
    if (ret != 0) {
        return ret;
    }
    ret = wc_EccPrivateKeyDecode(ecc_clikey_der_256, &idx, &key,
                                 sizeof_ecc_clikey_der_256);
    if (ret == 0) {
        ret = wc_ecc_export_private_only(&key, d, &dLen);
    }
    if (ret == 0 && dLen != sizeof(d)) {
        ret = BUFFER_E;
    }
    wc_ecc_free(&key);
    if (ret == 0) {
        ret = wc_RNG_GenerateBlock(rng, blob->seed, sizeof(blob->seed));
    }
    if (ret == 0 && format == HUK_WRAP_GCM) {
        ret = wc_RNG_GenerateBlock(rng, blob->iv, sizeof(blob->iv));
    }
    if (ret == 0) {
        ret = wc_AesInit(&aes, NULL, WC_HUK_DEVID);
        if (ret == 0) {
            if (format == HUK_WRAP_GCM) {
                ret = wc_AesGcmSetKey(&aes, blob->seed, sizeof(blob->seed));
                if (ret == 0) {
                    ret = wc_AesGcmEncrypt(&aes, blob->wrapped, d, sizeof(d),
                                           blob->iv, sizeof(blob->iv),
                                           blob->tag, sizeof(blob->tag),
                                           NULL, 0);
                }
            }
            else {
                ret = wc_AesSetKey(&aes, blob->seed, sizeof(blob->seed), NULL,
                                   AES_ENCRYPTION);
                if (ret == 0) {
                    ret = wc_AesEcbEncrypt(&aes, blob->wrapped, d, sizeof(d));
                }
            }
            wc_AesFree(&aes);
        }
    }
    ForceZero(d, sizeof(d));
    return ret;
}

/* Client ECC sign PK callback: sign the CertificateVerify hash with the
 * HUK-wrapped key passed as the callback context (wolfSSL_SetEccSignCtx).
 * keyDer is the public key the client loaded and is not needed here. For an
 * ECB blob the iv and tag fields stay NULL/0, which is how the port tells the
 * two formats apart, so an unknown format byte is rejected rather than
 * falling back to the unauthenticated ECB unwrap. */
static int huk_wrapped_sign_cb(WOLFSSL* ssl, const byte* in, word32 inSz,
        byte* out, word32* outSz, const byte* keyDer, word32 keySz, void* ctx)
{
    const huk_key_blob* blob = (const huk_key_blob*)ctx;
    wc_Rtl8735b_EccKey  hk;
    ecc_key             key;
    int                 ret;
    (void)ssl; (void)keyDer; (void)keySz;

    if (blob == NULL ||
            (blob->format != HUK_WRAP_GCM && blob->format != HUK_WRAP_ECB)) {
        return BAD_FUNC_ARG;
    }
    memset(&hk, 0, sizeof(hk));
    hk.seed    = blob->seed;    hk.seedSz     = sizeof(blob->seed);
    hk.wrapped = blob->wrapped; hk.wrappedLen = sizeof(blob->wrapped);
    hk.plainLen = sizeof(blob->wrapped);
    if (blob->format == HUK_WRAP_GCM) {
        hk.iv  = blob->iv;      hk.ivSz       = sizeof(blob->iv);
        hk.tag = blob->tag;     hk.tagSz      = sizeof(blob->tag);
    }

    ret = wc_ecc_init_ex(&key, NULL, WC_HUK_DEVID);
    if (ret == 0) {
        ret = wc_ecc_set_curve(&key, sizeof(blob->wrapped), ECC_SECP256R1);
        if (ret == 0) {
            key.devCtx = &hk;
            ret = wc_ecc_sign_hash(in, inSz, out, outSz, &g_signRng, &key);
        }
        wc_ecc_free(&key);
    }
    return ret;
}

/* Run one in-memory handshake and send one record client -> server. signCtx,
 * when set, is the client's sign-callback context. Returns 0 when the
 * handshake completed and the record arrived intact. */
static int huk_tls_session(WOLFSSL_CTX* cctx, WOLFSSL_CTX* sctx, void* signCtx)
{
    WOLFSSL*   cssl;
    WOLFSSL*   sssl;
    const char msg[] = "hello from HUK TLS client";
    char       rx[64];
    int        cdone = 0, sdone = 0;
    int        i, ret = -1;

    client_buffer_sz = 0;
    server_buffer_sz = 0;
    sssl = wolfSSL_new(sctx);
    cssl = wolfSSL_new(cctx);
    if (sssl == NULL || cssl == NULL) {
        dbg_printf("wolfSSL_new failed\r\n");
        goto cleanup;
    }
    if (signCtx != NULL) {
        wolfSSL_SetEccSignCtx(cssl, signCtx);
    }

    /* Drive the handshake: interleave connect/accept until both complete. */
    for (i = 0; i < 20 && (cdone == 0 || sdone == 0); i++) {
        if (cdone == 0) {
            ret = wolfSSL_connect(cssl);
            if (ret == WOLFSSL_SUCCESS) {
                cdone = 1;
            }
            else if (wolfSSL_get_error(cssl, ret) != WOLFSSL_ERROR_WANT_READ &&
                     wolfSSL_get_error(cssl, ret) != WOLFSSL_ERROR_WANT_WRITE) {
                dbg_printf("client err %d\r\n", wolfSSL_get_error(cssl, ret));
                break;
            }
        }
        if (sdone == 0) {
            ret = wolfSSL_accept(sssl);
            if (ret == WOLFSSL_SUCCESS) {
                sdone = 1;
            }
            else if (wolfSSL_get_error(sssl, ret) != WOLFSSL_ERROR_WANT_READ &&
                     wolfSSL_get_error(sssl, ret) != WOLFSSL_ERROR_WANT_WRITE) {
                dbg_printf("server err %d\r\n", wolfSSL_get_error(sssl, ret));
                break;
            }
        }
    }
    ret = -1;
    if (cdone == 1 && sdone == 1) {
        dbg_printf("cipher: %s\r\n", wolfSSL_get_cipher(sssl));
        memset(rx, 0, sizeof(rx));
        if (wolfSSL_write(cssl, msg, (int)sizeof(msg)) == (int)sizeof(msg) &&
                wolfSSL_read(sssl, rx, sizeof(rx) - 1) == (int)sizeof(msg) &&
                memcmp(rx, msg, sizeof(msg)) == 0) {
            ret = 0;
        }
    }

cleanup:
    if (cssl != NULL) wolfSSL_free(cssl);
    if (sssl != NULL) wolfSSL_free(sssl);
    return ret;
}

/* Server side shared by both handshakes: P-256 cert + key, ECDSA signed on the
 * HW engine, ECDHE-ECDSA + AES-GCM only so server auth uses ECDSA. */
static int huk_tls_server_ctx(WOLFSSL_CTX* sctx)
{
    wolfSSL_CTX_SetEccSignCb(sctx, huk_ecc_sign_cb);
    wolfSSL_CTX_set_cipher_list(sctx, "ECDHE-ECDSA-AES128-GCM-SHA256");
    wolfSSL_SetIORecv(sctx, recv_server);
    wolfSSL_SetIOSend(sctx, send_server);
    if (wolfSSL_CTX_use_certificate_buffer(sctx, serv_ecc_der_256,
            sizeof_serv_ecc_der_256, WOLFSSL_FILETYPE_ASN1)
            != WOLFSSL_SUCCESS) {
        return -1;
    }
    if (wolfSSL_CTX_use_PrivateKey_buffer(sctx, ecc_key_der_256,
            sizeof_ecc_key_der_256, WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS) {
        return -1;
    }
    return 0;
}

/* Client side shared by both handshakes: trust the CA that signed the server
 * cert. */
static int huk_tls_client_ctx(WOLFSSL_CTX* cctx)
{
    wolfSSL_CTX_set_cipher_list(cctx, "ECDHE-ECDSA-AES128-GCM-SHA256");
    wolfSSL_SetIORecv(cctx, recv_client);
    wolfSSL_SetIOSend(cctx, send_client);
    if (wolfSSL_CTX_load_verify_buffer(cctx, ca_ecc_cert_der_256,
            sizeof_ca_ecc_cert_der_256, WOLFSSL_FILETYPE_ASN1)
            != WOLFSSL_SUCCESS) {
        return -1;
    }
    return 0;
}

static void huk_tls_test(void)
{
    WOLFSSL_CTX* cctx;
    WOLFSSL_CTX* sctx;

    dbg_printf("\r\n== TLS 1.2 ECDHE-ECDSA (server auth on HUK) ==\r\n");

    sctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
    cctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    CHECK("CTX_new (client+server)", sctx != NULL && cctx != NULL);
    if (sctx != NULL && cctx != NULL) {
        CHECK("server cert + key", huk_tls_server_ctx(sctx) == 0);
        CHECK("client load_verify (CA)", huk_tls_client_ctx(cctx) == 0);
        CHECK("handshake + record (server signs on HW)",
              huk_tls_session(cctx, sctx, NULL) == 0);
    }
    if (cctx != NULL) wolfSSL_CTX_free(cctx);
    if (sctx != NULL) wolfSSL_CTX_free(sctx);
}

static void huk_mtls_test(void)
{
    WOLFSSL_CTX* cctx;
    WOLFSSL_CTX* sctx;
    huk_key_blob blob;
    huk_key_blob bad;
    int          format;
    int          ret;

    dbg_printf("\r\n== TLS 1.2 mutual auth, client key HUK-wrapped ==\r\n");

    sctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
    cctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    CHECK("CTX_new (client+server)", sctx != NULL && cctx != NULL);
    if (sctx == NULL || cctx == NULL) {
        goto cleanup;
    }

    /* Server requires a client certificate and trusts the self-signed client
     * cert. */
    ret = huk_tls_server_ctx(sctx);
    if (ret == 0) {
        wolfSSL_CTX_set_verify(sctx, WOLFSSL_VERIFY_PEER |
                               WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
        if (wolfSSL_CTX_load_verify_buffer(sctx, cliecc_cert_der_256,
                sizeof_cliecc_cert_der_256, WOLFSSL_FILETYPE_ASN1)
                != WOLFSSL_SUCCESS) {
            ret = -1;
        }
    }
    CHECK("server cert + key, client cert required", ret == 0);
    if (ret != 0) {
        goto cleanup;
    }

    /* Client: the sign callback is set before the key is loaded, so wolfSSL
     * accepts the public key in place of the private key and sends every
     * signature to the callback. */
    ret = huk_tls_client_ctx(cctx);
    if (ret == 0) {
        wolfSSL_CTX_SetEccSignCb(cctx, huk_wrapped_sign_cb);
        if (wolfSSL_CTX_use_certificate_buffer(cctx, cliecc_cert_der_256,
                sizeof_cliecc_cert_der_256, WOLFSSL_FILETYPE_ASN1)
                != WOLFSSL_SUCCESS ||
            wolfSSL_CTX_use_PrivateKey_buffer(cctx, ecc_clikeypub_der_256,
                sizeof_ecc_clikeypub_der_256, WOLFSSL_FILETYPE_ASN1)
                != WOLFSSL_SUCCESS) {
            ret = -1;
        }
    }
    CHECK("client cert + public key only", ret == 0);
    if (ret != 0) {
        goto cleanup;
    }

    /* A tampered GCM blob fails its unwrap on the client. A tampered ECB blob
     * unwraps to a wrong scalar and signs, and the server rejects the
     * CertificateVerify. Either way the handshake fails. */
    for (format = HUK_WRAP_GCM; format <= HUK_WRAP_ECB; format++) {
        dbg_printf("-- %s-wrapped client key --\r\n",
                   (format == HUK_WRAP_GCM) ? "GCM" : "ECB");
        ret = huk_wrap_client_key(&g_signRng, (byte)format, &blob);
        CHECK("wrap the client private key under the HUK", ret == 0);
        if (ret != 0) {
            continue;
        }
        CHECK("mutual handshake + record (client signs from the HUK blob)",
              huk_tls_session(cctx, sctx, &blob) == 0);

        memcpy(&bad, &blob, sizeof(bad));
        bad.wrapped[0] ^= 0x01;
        CHECK("tampered HUK blob fails the handshake",
              huk_tls_session(cctx, sctx, &bad) != 0);
    }

cleanup:
    if (cctx != NULL) wolfSSL_CTX_free(cctx);
    if (sctx != NULL) wolfSSL_CTX_free(sctx);
}

static void wolf_tls_thread(void* param)
{
    int ret;
    (void)param;

    dbg_printf("\r\n=== wolfCrypt AmebaPro2 (RTL8735B) HUK-in-TLS example ===\r\n");
    device_mutex_lock(RT_DEV_LOCK_CRYPTO);

    ret = wolfSSL_Init();
    CHECK("wolfSSL_Init", ret == WOLFSSL_SUCCESS);
    if (ret == WOLFSSL_SUCCESS) {
        ret = wc_Rtl8735b_HukRegister(WC_HUK_DEVID);
        CHECK("wc_Rtl8735b_HukRegister", ret == 0);
    }
    if (ret == 0) {
        ret = wc_InitRng(&g_signRng);
        CHECK("sign-callback RNG init", ret == 0);
        if (ret == 0) {
            huk_tls_test();
            huk_mtls_test();
            wc_FreeRng(&g_signRng);
        }
        wc_Rtl8735b_HukUnRegister(WC_HUK_DEVID);
    }
    wolfSSL_Cleanup();

    device_mutex_unlock(RT_DEV_LOCK_CRYPTO);
    dbg_printf("\r\n=== done ===\r\n");
    vTaskDelete(NULL);
}

int main(void)
{
    if (xTaskCreate(wolf_tls_thread, "wolf_tls", STACKSIZE, NULL,
                    tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        dbg_printf("xTaskCreate failed\r\n");
    }
    else {
        vTaskStartScheduler();
    }
    while (1) {
    }
}
