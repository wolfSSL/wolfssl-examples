/* wolfCrypt AmebaPro2 (RTL8735B) HUK-in-TLS example -- built inside the RealTek
 * FreeRTOS SDK. Runs an in-memory TLS 1.2 ECDHE-ECDSA handshake (client + server
 * in one firmware, no networking) where the server's ECDSA P-256 authentication
 * routes through the HUK crypto-callback device (wolfSSL_CTX_SetDevId), i.e. the
 * server signature is produced on the RTL8735B HW ECDSA engine. The ECDHE key
 * agreement and record-layer AES run in software (the engine has no arbitrary
 * scalar-mult path; see the port README). Proves HUK-accelerated server auth in
 * a real TLS handshake.
 *
 * Two server-key models (RTL_TLS_KEY_MODEL):
 *   1 = general HW offload: a plain P-256 server key loaded from certs_test.h;
 *       with devId set, its handshake ECDSA sign dispatches to the HW engine.
 *   2 = device-bound HUK-wrapped key (follow-on; see README).
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
#include <wolfssl/wolfcrypt/port/realtek/rtl8735b.h>

#ifndef RTL_TLS_KEY_MODEL
    #define RTL_TLS_KEY_MODEL 1
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

static void huk_tls_test(void)
{
    WOLFSSL_CTX* cctx = NULL;
    WOLFSSL_CTX* sctx = NULL;
    WOLFSSL*     cssl = NULL;
    WOLFSSL*     sssl = NULL;
    const char   msg[] = "hello from HUK TLS client";
    char         rx[64];
    int          cdone = 0, sdone = 0;
    int          i, ret;

    dbg_printf("\r\n== TLS 1.2 ECDHE-ECDSA (server auth on HUK) ==\r\n");

    sctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
    cctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    CHECK("CTX_new (client+server)", sctx != NULL && cctx != NULL);
    if (sctx == NULL || cctx == NULL) {
        goto cleanup;
    }

    /* RNG for the sign callback's nonce. */
    ret = wc_InitRng(&g_signRng);
    CHECK("sign-callback RNG init", ret == 0);

    /* Route ONLY the server's cert ECDSA sign to the HUK HW engine (PK callback);
     * TLS's own HMAC/PRF, record AES and ECDHE stay in software. */
    wolfSSL_CTX_SetEccSignCb(sctx, huk_ecc_sign_cb);

    /* Restrict to ECDHE-ECDSA + AES-GCM so server auth uses ECDSA (HUK). */
    wolfSSL_CTX_set_cipher_list(sctx, "ECDHE-ECDSA-AES128-GCM-SHA256");
    wolfSSL_CTX_set_cipher_list(cctx, "ECDHE-ECDSA-AES128-GCM-SHA256");

    /* Server: P-256 cert + key (model 1: general HW offload). */
    ret = wolfSSL_CTX_use_certificate_buffer(sctx, serv_ecc_der_256,
            sizeof_serv_ecc_der_256, WOLFSSL_FILETYPE_ASN1);
    CHECK("server use_certificate", ret == WOLFSSL_SUCCESS);
    ret = wolfSSL_CTX_use_PrivateKey_buffer(sctx, ecc_key_der_256,
            sizeof_ecc_key_der_256, WOLFSSL_FILETYPE_ASN1);
    CHECK("server use_PrivateKey", ret == WOLFSSL_SUCCESS);

    /* Client: trust the CA that signed the server cert. */
    ret = wolfSSL_CTX_load_verify_buffer(cctx, ca_ecc_cert_der_256,
            sizeof_ca_ecc_cert_der_256, WOLFSSL_FILETYPE_ASN1);
    if (ret != WOLFSSL_SUCCESS) {
        dbg_printf("load_verify ret=%d\r\n", ret);
    }
    CHECK("client load_verify (CA)", ret == WOLFSSL_SUCCESS);

    wolfSSL_SetIORecv(sctx, recv_server);
    wolfSSL_SetIOSend(sctx, send_server);
    wolfSSL_SetIORecv(cctx, recv_client);
    wolfSSL_SetIOSend(cctx, send_client);

    sssl = wolfSSL_new(sctx);
    cssl = wolfSSL_new(cctx);
    CHECK("wolfSSL_new (client+server)", sssl != NULL && cssl != NULL);
    if (sssl == NULL || cssl == NULL) {
        goto cleanup;
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
    CHECK("TLS handshake completed", cdone == 1 && sdone == 1);

    if (cdone == 1 && sdone == 1) {
        dbg_printf("cipher: %s\r\n", wolfSSL_get_cipher(sssl));
        /* Exchange one application record client -> server. */
        ret = wolfSSL_write(cssl, msg, (int)sizeof(msg));
        CHECK("client write", ret == (int)sizeof(msg));
        memset(rx, 0, sizeof(rx));
        ret = wolfSSL_read(sssl, rx, sizeof(rx) - 1);
        CHECK("server read == client msg",
              ret == (int)sizeof(msg) && memcmp(rx, msg, sizeof(msg)) == 0);
    }

cleanup:
    if (cssl != NULL) wolfSSL_free(cssl);
    if (sssl != NULL) wolfSSL_free(sssl);
    if (cctx != NULL) wolfSSL_CTX_free(cctx);
    if (sctx != NULL) wolfSSL_CTX_free(sctx);
    wc_FreeRng(&g_signRng);
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
        huk_tls_test();
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
