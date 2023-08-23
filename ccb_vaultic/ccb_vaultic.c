/*
 * ccb_vaultic.c
 *
 * Copyright (C) 2023 wolfSSL Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */


/*
 * Compilation options:
 *
 * HAVE_CCBVAULTIC
 *   Enable actual callbacks. This depends on VaultIC hardware libraries
 *
 * Defined options when HAVE_CCBVAULTIC is set:
 *
 * WOLF_CRYPTO_CB_CMD
 *   Enable delayed hardware initialization using Register and Unregister
 *   callback commands
 *
 * CCBVAULTIC_DEBUG: Print useful callback info using XPRINTF
 * CCBVAULTIC_DEBUG_TIMING: Print useful timing info using XPRINTF
 * CCBVAULTIC_DEBUG_ALL: Print copious info using XPRINTF
 *
 * CCBVAULTIC_NO_SHA: Do not handle SHA256 callback
 * CCBVAULTIC_NO_RSA: Do not handle RSA callback
 * CCBVAULTIC_NO_AES: Do not handle AES callback
 *
 * Expected wolfSSL/wolfCrypt defines from wolfcrypt/types.h or settings.h:
 * XMALLOC:  malloc() equivalent
 * XREALLOC: realloc() equivalent
 * XFREE:    free() equivalent
 * XMEMCPY:  memcpy() equivalent
 * XMEMSET:  memset() equivalent
 * XMEMCMP:  memcmp() equivalent
 *
 * Overrideable defines:
 * XPRINTF:  printf() equivalent
 * XNOW:     clock_gettime(CLOCK_MONOTONIC) converted to uint64_t ns
 */

#ifdef HAVE_CCBVAULTIC

/* wolfSSL configuration */
#ifndef WOLFSSL_USER_SETTINGS
    #include "wolfssl/options.h"
#endif
#include "wolfssl/wolfcrypt/settings.h"

/* wolfCrypt includes */
#include "wolfssl/wolfcrypt/cryptocb.h"    /* For wc_cryptInfo */
#include "wolfssl/wolfcrypt/error-crypt.h" /* For error values */

/* Local include */
#include "ccb_vaultic.h"

/* Debug defines */
#ifdef CCBVAULTIC_DEBUG_ALL
    #ifndef CCBVAULTIC_DEBUG
        #define CCBVAULTIC_DEBUG
    #endif
    #ifndef CCBVAULTIC_DEBUG_TIMING
        #define CCBVAULTIC_DEBUG_TIMING
    #endif
#endif

#if defined(CCBVAULTIC_DEBUG) || defined(CCBVAULTIC_DEBUG_TIMING)
    #ifndef XPRINTF
        #define XPRINTF(...) printf(__VA_ARGS__)
    #endif
#endif



/* wolfcrypt includes */
#include "wolfssl/wolfcrypt/types.h"       /* types and X-defines */

#ifndef CCBVAULTIC_NO_SHA
#include "wolfssl/wolfcrypt/hash.h"  /* For HASH_FLAGS and types */
#endif

#ifndef CCBVAULTIC_NO_RSA
#include "wolfssl/wolfcrypt/rsa.h"   /* For RSA_MAX_SIZE and types */
#endif

#ifndef CCBVAULTIC_NO_AES
#include "wolfssl/wolfcrypt/aes.h"   /* For AES_BLOCK_SIZE and types */
#endif

#ifdef CCBVAULTIC_DEBUG_TIMING
    #ifndef XNOW
        #include <time.h>
        #include <stdint.h>
        #define XNOW(...) _now(__VA_ARGS__)

        static uint64_t _now(void)
        {
            struct timespec t;
            if (clock_gettime(CLOCK_MONOTONIC, &t) < 0)
                /* Return 0 on error */
                return 0;
            return (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec;
        }
    #endif
#endif

/* WiseKey VaultIC includes */
#include "vaultic_tls.h"
#include "vaultic_config.h"
#include "vaultic_common.h"
#include "vaultic_api.h"
#include "vaultic_structs.h"

/* Key/Group ID's to support temporary wolfSSL usage */
#define CCBVAULTIC_WOLFSSL_GRPID 0xBB
#define CCBVAULTIC_TMPAES_KEYID 0x01
#define CCBVAULTIC_TMPHMAC_KEYID 0x02
#define CCBVAULTIC_TMPRSA_KEYID 0x03

/* Key attributes */
#define VAULTIC_KP_ALL 0xFF  /* Allow all users all key privileges */
#define VAULTIC_PKV_ASSURED VLT_PKV_ASSURED_EXPLICIT_VALIDATION


#ifdef CCBVAULTIC_DEBUG
/* Helper to provide simple hexdump */
static void hexdump(const unsigned char* p, size_t len)
{
    XPRINTF("    HD:%p for %lu bytes\n",p, len);
    if ( (p == NULL) || (len == 0))
        return;
    size_t off = 0;
    for (off = 0; off < len; off++)
    {
        if ((off % 16) == 0)
            XPRINTF("    ");
        XPRINTF("%02X ", p[off]);
        if ((off % 16) == 15)
            XPRINTF("\n");
    }
    if ( (off%16) != 15)
        XPRINTF("\n");
}
#endif

/* Helper to translate vlt return codes to wolfSSL code */
static int translateError(int vlt_rc)
{
    /* vlt return codes are defined in src/common/vaultic_err.h */
    switch (vlt_rc) {
    case 0:        /* returned on successful init */
    case VLT_OK:
        return 0;
    default:
        /* Default to point to hardware */
        return WC_HW_E;
    }
}

static void clearContext(ccbVaultIc_Context *c)
{
    XMEMSET(c, 0, sizeof(*c));
    c->m = NULL;
    c->aescbc_key = NULL;
}

int ccbVaultIc_Init(ccbVaultIc_Context *c)
{
    int rc = 0;
    if (c == NULL) {
        rc = BAD_FUNC_ARG;
    }

    /* Already Initialized? */
    if ((rc == 0) && (c->initialized >0)) {
        /* Increment use count */
        c->initialized++;
        return 0;
    }
    if (rc == 0) {
        clearContext(c);
        /* Open the hardware and authenticate */
        c->vlt_rc = vlt_tls_init();
        rc = translateError(c->vlt_rc);
    }
    if (rc == 0) {
        c->initialized = 1;
    }
    else {
        /* Override with an init error */
        rc = WC_INIT_E;
    }
#ifdef CCBVAULTIC_DEBUG
    XPRINTF("ccbVaultIc_Init: c:%p c->initialized:%d rc:%d vlt_rc:%d\n",
            c,
            (c == NULL) ? -1 : c->initialized,
            rc,
            (c == NULL) ? -1 : c->vlt_rc);
#endif
    return rc;
}

void ccbVaultIc_Cleanup(ccbVaultIc_Context *c)
{
#ifdef CCBVAULTIC_DEBUG
    XPRINTF("ccbVaultIc_Cleanup c:%p c->initialized:%d\n", c,
            (c == NULL) ? -1 : c->initialized);
#endif
    /* Invalid context or not initialized? */
    if ((c == NULL) ||
        (c->initialized == 0)) {
        return;
    }

    /* Decrement use count */
    c->initialized--;
    if (c->initialized > 0)
        return;

    /* Free allocated buffers */
    if (c->m != NULL)
        XFREE(c->m, NULL, NULL);
    if (c->aescbc_key != NULL)
        XFREE(c->aescbc_key, NULL, NULL);

    clearContext(c);

    /* Set the return value in the struct */
    /* Close the hardware */
    c->vlt_rc = vlt_tls_close();
}

#ifdef WOLF_CRYPTO_CB

/* Forward declarations */
static int HandlePkCallback(int devId, wc_CryptoInfo* info,
        ccbVaultIc_Context *c);
static int HandleHashCallback(int devId, wc_CryptoInfo* info,
        ccbVaultIc_Context *c);
static int HandleCipherCallback(int devId, wc_CryptoInfo* info,
        ccbVaultIc_Context *c);

#ifdef WOLF_CRYPTO_CB_CMD
/* Provide global singleton context to avoid allocation */
static ccbVaultIc_Context localContext = CCBVAULTIC_CONTEXT_INITIALIZER;
static int HandleCmdCallback(int devId, wc_CryptoInfo* info,
        ccbVaultIc_Context *c);
#endif

int ccbVaultIc_CryptoCb(int devId,
                           wc_CryptoInfo* info,
                           void* ctx)
{
    (void)devId;
    ccbVaultIc_Context* c = (ccbVaultIc_Context*)ctx;
    int rc = CRYPTOCB_UNAVAILABLE;

    /* Allow null/uninitialized context for ALGO_TYPE_NUNE */
    if ( (info == NULL) ||
        ((info->algo_type != WC_ALGO_TYPE_NONE) &&
        ((c == NULL) || (c->initialized == 0)))) {
        /* Invalid info or context */
#if defined(CCBVAULTIC_DEBUG)
        XPRINTF("Invalid callback. info:%p c:%p c->init:%d\n",
                info, c, c ? c->initialized : -1);
#endif
        return BAD_FUNC_ARG;
    }

    switch(info->algo_type) {

    case WC_ALGO_TYPE_NONE:
#ifdef WOLF_CRYPTO_CB_CMD
#if defined(CCBVAULTIC_DEBUG)
        XPRINTF(" CryptoDevCb NONE-Command: %d %p\n",
                info->cmd.type, info->cmd.ctx);
#endif
        rc = HandleCmdCallback(devId, info, ctx);
#else
#if defined(CCBVAULTIC_DEBUG)
        XPRINTF(" CryptoDevCb NONE:\n");
#endif
        /* Nothing to do */
#endif
        break;

    case WC_ALGO_TYPE_HASH:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF(" CryptoDevCb HASH: Type:%d\n", info->hash.type);
#endif
#if !defined(NO_SHA) || !defined(NO_SHA256)
        /* Perform a hash */
        rc = HandleHashCallback(devId, info, ctx);
#endif
        break;

    case WC_ALGO_TYPE_CIPHER:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF(" CryptoDevCb CIPHER: Type:%d\n", info->cipher.type);
#endif
#if !defined(NO_AES)
        /* Perform a symmetric cipher */
        rc = HandleCipherCallback(devId, info, ctx);
#endif
        break;

    case WC_ALGO_TYPE_PK:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF(" CryptoDevCb PK: Type:%d\n", info->pk.type);
#endif
#if !defined(NO_RSA) || defined(HAVE_ECC)
        /* Perform a PKI operation */
        rc = HandlePkCallback(devId,info,ctx);
#endif /* !defined(NO_RSA) || defined(HAVE_ECC) */
        break;

    case WC_ALGO_TYPE_RNG:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF(" CryptoDevCb RNG: Out:%p Sz:%d\n",
                info->rng.out, info->rng.sz);
#endif
#if !defined(WC_NO_RNG)
        /* Put info->rng.sz random bytes into info->rng.out*/
        /* TODO rc = VaultIC_Random(); */
#endif
        break;

    case WC_ALGO_TYPE_SEED:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF(" CryptoDevCb SEED: Seed:%p Sz:%d\n", info->seed.seed,
                info->seed.sz);
#endif
#if !defined(WC_NO_RNG)
        /* Get info->seed.sz seed bytes from info->seed.seed*/
        /* TODO rc = VaultIC_Seed(); */
#endif
        break;

    case WC_ALGO_TYPE_HMAC:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF(" CryptoDevCb HMAC:\n");
#endif
        break;

    case WC_ALGO_TYPE_CMAC:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF(" CryptoDevCb CMAC:\n");
#endif
        break;

    default:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF(" CryptoDevCb UNKNOWN: %d\n", info->algo_type);
#endif
        break;
    }
    return rc;
}

#ifdef WOLF_CRYPTO_CB_CMD
static int HandleCmdCallback(int devId, wc_CryptoInfo* info,
        ccbVaultIc_Context *c)
{
    int rc = CRYPTOCB_UNAVAILABLE;
    /* Ok to have null context at this point*/
    switch(info->cmd.type) {

    case WC_CRYPTOCB_CMD_TYPE_REGISTER:
    {
        /* Is the context nonnull already? Nothing to do */
        if (c != NULL)
            break;

        rc = ccbVaultIc_Init(&localContext);
        if (rc == 0) {
            /* Update the info struct to use localContext */
            info->cmd.ctx = &localContext;
        }
    }; break;

    case WC_CRYPTOCB_CMD_TYPE_UNREGISTER:
    {
        /* Is the current context not set? Nothing to do*/
        if (c == NULL)
            break;
        ccbVaultIc_Cleanup(c);
        /* Return success */
        rc = 0;
    }; break;

    default:
        break;
    }
    return rc;
}
#endif

static int HandlePkCallback(int devId, wc_CryptoInfo* info,
        ccbVaultIc_Context *c)
{
    int rc = CRYPTOCB_UNAVAILABLE;

    switch(info->pk.type) {

    case WC_PK_TYPE_NONE:
    {
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback NONE\n");
#endif
    }; break;

    case WC_PK_TYPE_RSA:
    {
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback RSA: Type:%d\n",info->pk.rsa.type);
#endif
#if !defined(CCBVAULTIC_NO_RSA)
        switch(info->pk.rsa.type) {

        case RSA_PUBLIC_DECRYPT:   /* RSA Verify */
        case RSA_PUBLIC_ENCRYPT:   /* RSA Encrypt */
        {                          /* Handle RSA Pub Key op */
            byte    e[sizeof(uint32_t)];
            byte    e_pad[sizeof(e)];
            byte    n[RSA_MAX_SIZE / 8];
            word32  eSz = sizeof(e);
            word32  nSz = sizeof(n);

            VLT_U32 out_len = 0;
            VLT_FILE_PRIVILEGES keyPrivileges;
            VLT_KEY_OBJECT tmpRsaKey;
            VLT_ALGO_PARAMS rsapub_algo_params;

#ifdef CCBVAULTIC_DEBUG_TIMING
            uint64_t ts[6];
            XMEMSET(ts, 0, sizeof(ts));
#endif
            /* Extract key values from RSA context */
            rc = wc_RsaFlattenPublicKey(info->pk.rsa.key, e, &eSz, n, &nSz);
            if (rc != 0)
                break;

            /* VaultIC requires e to be MSB-padded to 4-byte multiples*/
            XMEMSET(e_pad, 0, sizeof(e_pad));
            XMEMCPY(&e_pad[(sizeof(e_pad)-eSz)], e, eSz);

#if defined(CCBVAULTIC_DEBUG_ALL)
            XPRINTF("   RSA Flatten Pub Key:%d, eSz:%u nSz:%u\n",
                    rc, eSz, nSz);
            hexdump(e,sizeof(e));
            hexdump(e_pad,sizeof(e_pad));
            hexdump(n,sizeof(n));
#endif
            /* Set tmpRsaKey privileges */
            keyPrivileges.u8Read    = VAULTIC_KP_ALL;
            keyPrivileges.u8Write   = VAULTIC_KP_ALL;
            keyPrivileges.u8Delete  = VAULTIC_KP_ALL;
            keyPrivileges.u8Execute = VAULTIC_KP_ALL;

            /* Set tmpRsaKey values for public key */
            tmpRsaKey.enKeyID                    = VLT_KEY_RSAES_PUB;
            tmpRsaKey.data.RsaPubKey.u16NLen     = nSz;
            tmpRsaKey.data.RsaPubKey.pu8N        = n;
            tmpRsaKey.data.RsaPubKey.u16ELen     = sizeof(e_pad);
            tmpRsaKey.data.RsaPubKey.pu8E        = e_pad;
            tmpRsaKey.data.RsaPubKey.enAssurance = VAULTIC_PKV_ASSURED;

            /* Set algo to Raw RSA ES*/
            rsapub_algo_params.u8AlgoID = VLT_ALG_CIP_RSAES_X509;


#ifdef CCBVAULTIC_DEBUG_TIMING
            ts[0] = XNOW();
#endif
            /* Try to delete the tmp rsa key.  Ignore errors here */
            VltDeleteKey(
                    CCBVAULTIC_WOLFSSL_GRPID,
                    CCBVAULTIC_TMPRSA_KEYID);

#ifdef CCBVAULTIC_DEBUG_TIMING
            ts[1] = XNOW();
#endif
            c->vlt_rc = VltPutKey(
                    CCBVAULTIC_WOLFSSL_GRPID,
                    CCBVAULTIC_TMPRSA_KEYID,
                    &keyPrivileges,
                    &tmpRsaKey);
#if defined(CCBVAULTIC_DEBUG_ALL)
            XPRINTF("   VLT PutKey:%x\n", c->vlt_rc);
#endif
            rc = translateError(c->vlt_rc);
            if (rc != 0)
                break;

#ifdef CCBVAULTIC_DEBUG_TIMING
            ts[2] = XNOW();
#endif
            /* Initialize Algo for RSA Pub Encrypt */
            c->vlt_rc = VltInitializeAlgorithm(
                    CCBVAULTIC_WOLFSSL_GRPID,
                    CCBVAULTIC_TMPRSA_KEYID,
                    VLT_ENCRYPT_MODE,
                    &rsapub_algo_params);
#if defined(CCBVAULTIC_DEBUG_ALL)
            XPRINTF("   VLT InitAlgo:%x\n", c->vlt_rc);
#endif
            rc = translateError(c->vlt_rc);
            if (rc != 0)
                break;

#ifdef CCBVAULTIC_DEBUG_TIMING
            ts[3] = XNOW();
#endif
            /* Perform the RSA pub key encrypt */
            c->vlt_rc = VltEncrypt(
                    info->pk.rsa.inLen,
                    info->pk.rsa.in,
                    &out_len,
                    info->pk.rsa.inLen,
                    info->pk.rsa.out);

            if (info->pk.rsa.outLen)
                *(info->pk.rsa.outLen) = out_len;
#if defined(CCBVAULTIC_DEBUG_ALL)
            XPRINTF("   VLT Encrypt:%x inSz:%u outSz:%lu\n",
                    c->vlt_rc, info->pk.rsa.inLen, out_len);
#endif
            rc = translateError(c->vlt_rc);
            if (rc != 0)
                break;

#ifdef CCBVAULTIC_DEBUG_TIMING
            ts[4] = XNOW();
            XPRINTF("   RSA Encrypt Times(us): DltKey:%lu PutKey:%lu "
                    "InitAlgo:%lu Encrypt:%lu InSize:%u OutSize:%lu "
                    "KeySize:%u\n",
                    (ts[1]-ts[0])/1000,
                    (ts[2]-ts[1])/1000,
                    (ts[3]-ts[2])/1000,
                    (ts[4]-ts[3])/1000,
                    info->pk.rsa.inLen, out_len,nSz);
#endif
        };break;                     /* Handle RSA Pub Key op */

        case RSA_PRIVATE_ENCRYPT:   /* RSA Sign */
        case RSA_PRIVATE_DECRYPT:   /* RSA Decrypt */
        default:
            /* Not supported */
            break;
        }
#endif
      }; break;

    case WC_PK_TYPE_DH:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback DH\n");
#endif
        break;

    case WC_PK_TYPE_ECDH:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback ECDH\n");
#endif
        break;

    case WC_PK_TYPE_ECDSA_SIGN:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback ECDSA_SIGN\n");
#endif
        break;

    case WC_PK_TYPE_ECDSA_VERIFY:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback ECDSA_VERIFY\n");
#endif
        break;

    case WC_PK_TYPE_ED25519_SIGN:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback ED25519_SIGN\n");
#endif
        break;

    case WC_PK_TYPE_CURVE25519:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback CURVE25519\n");
#endif
        break;

    case WC_PK_TYPE_RSA_KEYGEN:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback RSA_KEYGEN\n");
#endif
        break;

    case WC_PK_TYPE_EC_KEYGEN:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback EC_KEYGEN\n");
#endif
        break;

    case WC_PK_TYPE_RSA_CHECK_PRIV_KEY:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback RSA_CHECK_PRIV_KEY\n");
#endif
        break;

    case WC_PK_TYPE_EC_CHECK_PRIV_KEY:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback EC_CHECK_PRIV_KEY\n");
#endif
        break;

    case WC_PK_TYPE_ED448:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback ED448\n");
#endif
        break;

    case WC_PK_TYPE_CURVE448:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback CRUVE448\n");
#endif
        break;

    case WC_PK_TYPE_ED25519_VERIFY:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback ED25519_VERIFY\n");
#endif
        break;

    case WC_PK_TYPE_ED25519_KEYGEN:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback ED25519_KEYGEN\n");
#endif
        break;

    case WC_PK_TYPE_CURVE25519_KEYGEN:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback CURVE25519_KEYGEN\n");
#endif
        break;

    case WC_PK_TYPE_RSA_GET_SIZE:
#if defined(CCBVAULTIC_DEBUG)
        XPRINTF("  HandlePkCallback RSA_GET_SIZE\n");
#endif
        break;

    default:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandlePkCallback UNKNOWN: %d\n",info->pk.type);
#endif
        break;
    }
    return rc;
}

static int HandleHashCallback(int devId, wc_CryptoInfo* info,
        ccbVaultIc_Context *c)
{
    int rc = CRYPTOCB_UNAVAILABLE;

    switch(info->hash.type) {
    case WC_HASH_TYPE_NONE:

#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleHashCallback NONE\n");
#endif
        break;

    case WC_HASH_TYPE_SHA:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleHashCallback SHA\n");
#endif
        break;

    case WC_HASH_TYPE_SHA224:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleHashCallback SHA224\n");
#endif
        break;

    case WC_HASH_TYPE_SHA256:
    {
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleHashCallback SHA256. In:%p InSz:%u Digest:%p "
                "c->m:%p c->m_len:%lu c->t:%d\n",
                info->hash.in, info->hash.inSz, info->hash.digest,
                c->m, c->m_len, c->hash_type);
#endif
#if !defined(CCBVAULTIC_NO_SHA)
        /*
         *  info->hash.flag | WC_HASH_FLAGS_WILL_COPY --> Buffer entire message
         *  info->hash.in != NULL                     --> Update
         *  info->hash.digest != NULL                 --> Final
         */
#ifdef CCBVAULTIC_DEBUG_TIMING
        uint64_t ts[3];
        XMEMSET(ts, 0, sizeof(ts));
#endif
        /* New/different hashtype than last time? */
        if (c->hash_type != info->hash.type) {
            /* New/different hash than last time.  Erase state */
#if defined(CCBVAULTIC_DEBUG_ALL)
            XPRINTF("   New Hash\n");
#endif
            if (c->m != NULL)
                XFREE(c->m, NULL, NULL);
            c->m         = NULL;
            c->m_len     = 0;
            c->hash_type = info->hash.type;
        }

        /* Update needed? */
        /* III Buffering ALL messages, not just WILL_COPY */
        if (    (info->hash.in != NULL) &&
                (info->hash.inSz > 0)) {
            /* Buffer data */
            if (c->m == NULL) {
                c->m = XMALLOC(info->hash.inSz, NULL, NULL);
                if (c->m == NULL) {
                    /* Failure to allocate.  Must return error */
#if defined(CCBVAULTIC_DEBUG)
                    XPRINTF("   Failed to alloc. Size:%u\n",
                            info->hash.inSz);
#endif
                    rc = MEMORY_E;
                    break;
                }
                c->m_len = 0;
#if defined(CCBVAULTIC_DEBUG_ALL)
                XPRINTF("   Alloc to %p. Size:%u\n", c->m, info->hash.inSz);
#endif
            } else {
                /* Realloc and add new data in */
                void *new_buf = XREALLOC(c->m, c->m_len + info->hash.inSz,
                                                                   NULL, NULL);
                if (new_buf == NULL) {
                    /* Failure to realloc.  Must return error */
#if defined(CCBVAULTIC_DEBUG)
                    XPRINTF("   Failed to XREALLOC. New size:%lu\n",
                            c->m_len + info->hash.inSz);
#endif
                    rc = MEMORY_E;
                    break;
                }
                c->m = new_buf;
#if defined(CCBVAULTIC_DEBUG_ALL)
                XPRINTF("   Realloc to %p. New size:%lu\n",
                        c->m, c->m_len + info->hash.inSz);
#endif
            }

            /* Copy new data into the buffer */
            XMEMCPY(c->m + c->m_len, info->hash.in, info->hash.inSz);
            c->m_len += info->hash.inSz;
#if defined(CCBVAULTIC_DEBUG_ALL)
            XPRINTF("   Buffered to %p. Buffer size:%lu\n", c->m, c->m_len);
#endif
            /* Successful update */
            rc = 0;
        }   /* End of Update */

        /* Finalize needed? */
        if (info->hash.digest != NULL) {
            /* Initialize for Hashing */
            VLT_U8 sha_out_len = 0;
            VLT_ALGO_PARAMS sha256_algo_params;

            sha256_algo_params.u8AlgoID = VLT_ALG_DIG_SHA256;
#ifdef CCBVAULTIC_DEBUG_TIMING
            ts[0] = XNOW();
#endif

            c->vlt_rc = VltInitializeAlgorithm(
                    0,
                    0,
                    VLT_DIGEST_MODE,
                    &sha256_algo_params);

#if defined(CCBVAULTIC_DEBUG_ALL)
            XPRINTF("   VltInit SHA256:%x\n", c->vlt_rc);
#endif
            rc = translateError(c->vlt_rc);
            if (rc != 0)
                break;

#ifdef CCBVAULTIC_DEBUG_TIMING
            ts[1] = XNOW();
#endif
            /* No data buffered?  Likely test case.  Needs 2 steps */
            if (c->m == NULL)
            {
                c->vlt_rc = VltUpdateMessageDigest(
                        c->m_len,
                        c->m);
#if defined(CCBVAULTIC_DEBUG_ALL)
                XPRINTF("   VltUpdate SHA256:%x\n", c->vlt_rc);
#endif
                rc = translateError(c->vlt_rc);
                if (rc != 0)
                    break;

                c->vlt_rc = VltComputeMessageDigestFinal(
                        &sha_out_len,
                        WC_SHA256_DIGEST_SIZE,
                        info->hash.digest);
#if defined(CCBVAULTIC_DEBUG_ALL)
                XPRINTF("   VltFinal SHA256:%x\n", c->vlt_rc);
#endif
                rc = translateError(c->vlt_rc);
                if (rc != 0)
                    break;
            }
            else {
                c->vlt_rc = VltComputeMessageDigest(
                        c->m_len,
                        c->m,
                        &sha_out_len,
                        WC_SHA256_DIGEST_SIZE,
                        info->hash.digest);
#if defined(CCBVAULTIC_DEBUG_ALL)
            XPRINTF("   VltCompute SHA256:%x\n", c->vlt_rc);
            hexdump(info->hash.digest, WC_SHA256_DIGEST_SIZE);
#endif
                rc = translateError(c->vlt_rc);
                if (rc != 0)
                    break;
            }

#ifdef CCBVAULTIC_DEBUG_TIMING
            ts[2] = XNOW();
            XPRINTF("   SHA256 Compute Times(us): InitAlgo:%lu Digest:%lu "
                    "InSize:%lu OutSize:%u\n",
                    (ts[1]-ts[0])/1000,
                    (ts[2]-ts[1])/1000,
                    c->m_len, sha_out_len);
#endif

            /* Deallocate/clear if this hash was NOT a copy*/
#if defined(CCBVAULTIC_DEBUG_ALL)
            XPRINTF("   Hash flags:%x\n",
                    (info->hash.sha256 != NULL) ?
                            info->hash.sha256->flags :
                            -1);
#endif
            /* If there are no flags OR if flags say its a copy then... */
            if ((info->hash.sha256 == NULL) ||
                    !(info->hash.sha256->flags & WC_HASH_FLAG_ISCOPY)) {
#if defined(CCBVAULTIC_DEBUG_ALL)
                XPRINTF("   Freeing hash state\n");
#endif
                if (c->m != NULL)
                    XFREE(c->m, NULL, NULL);
                c->m         = NULL;
                c->m_len     = 0;
                c->hash_type = WC_HASH_TYPE_NONE;
            }

            /* Successful finalize */
            rc = 0;
        }  /* End of finalize */
#endif
    }; break;

    case WC_HASH_TYPE_SHA384:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleHashCallback SHA384\n");
#endif
        break;

    case WC_HASH_TYPE_SHA512:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleHashCallback SHA512\n");
#endif
        break;

    default:
        break;
    }
    return rc;
}

static int HandleCipherCallback(int devId, wc_CryptoInfo* info,
        ccbVaultIc_Context *c)
{
    int rc = CRYPTOCB_UNAVAILABLE;

    switch(info->cipher.type) {

    case WC_CIPHER_NONE:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleCipherCallback NONE\n");
#endif
        break;

    case WC_CIPHER_AES:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleCipherCallback AES\n");
#endif
        break;

    case WC_CIPHER_AES_CBC:
    {
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleCipherCallback AES_CBC\n");
#endif
#if !defined(CCBVAULTIC_NO_AES)
        Aes* aes    = info->cipher.aescbc.aes;
        int encrypt = info->cipher.enc;
        unsigned int blocks = 0;
        const byte *last_block;

        VLT_U32 out_len = 0;
        VLT_FILE_PRIVILEGES keyPrivileges;
        VLT_KEY_OBJECT tmpAesKey;
        VLT_ALGO_PARAMS aescbc_algo_params;
        int vlt_mode = 0;

#ifdef CCBVAULTIC_DEBUG_TIMING
        uint64_t ts[6];
        XMEMSET(ts, 0, sizeof(ts));
#endif
        /* Invalid incoming context? Return error*/
        if (aes == NULL) {
            rc = BAD_FUNC_ARG;
            break;
        }

        /* Only support AES128 */
        if (aes->keylen != AES_128_KEY_SIZE) {
            break;
        }

        /* Check number of blocks */
        blocks = info->cipher.aescbc.sz / AES_BLOCK_SIZE;
        if (blocks == 0) {
            /* Success!  Nothing to do */
            rc = 0;
            break;
        }

        /* Initialize Algo for AES-CBC */
        aescbc_algo_params.u8AlgoID                  = VLT_ALG_CIP_AES;
        aescbc_algo_params.data.SymCipher.enMode     = BLOCK_MODE_CBC;
        aescbc_algo_params.data.SymCipher.enPadding  = PADDING_NONE;
        aescbc_algo_params.data.SymCipher.u8IvLength = AES_BLOCK_SIZE;
        /* Copy in the previous cipher block */
        XMEMCPY(aescbc_algo_params.data.SymCipher.u8Iv,
                                                  aes->reg, AES_BLOCK_SIZE);
        if (encrypt) {
            /* Last cipher text block is within out */
            last_block = info->cipher.aescbc.out +
                                              (blocks - 1) * AES_BLOCK_SIZE;
            vlt_mode = VLT_ENCRYPT_MODE;
        }
        else {
            /* Last cipher text block is within in */
            last_block = info->cipher.aescbc.in +
                                              (blocks - 1) * AES_BLOCK_SIZE;
            vlt_mode = VLT_DECRYPT_MODE;
        }

        /* Check if key is not the same as last time */
        if ((c->aescbc_key    == NULL) ||
            (c->aescbc_keylen != aes->keylen) ||
            (XMEMCMP(c->aescbc_key, aes->devKey, aes->keylen))) {
#if defined(CCBVAULTIC_DEBUG_ALL)
            XPRINTF("   New AES Key: ckey:%p clen:%lu akey:%p alen:%u\n",
                    c->aescbc_key,c->aescbc_keylen, aes->devKey, aes->keylen);
            hexdump((void*)aes->devKey, aes->keylen);
#endif
            /* Free the current key buffer if necessary */
            if (c->aescbc_key != NULL) {
                XFREE(c->aescbc_key, NULL, NULL);
                c->aescbc_key    = NULL;
                c->aescbc_keylen = 0;
            }

            /* Allocate key buffer */
            c->aescbc_key = XMALLOC(aes->keylen, NULL, NULL);
            if (c->aescbc_key == NULL) {
#if defined(CCBVAULTIC_DEBUG)
                XPRINTF("   Failed to allocate new AES Key of size:%u\n",
                        aes->keylen);
#endif
                break;
            }

            /* Copy key into buffer */
            c->aescbc_keylen = aes->keylen;
            XMEMCPY(c->aescbc_key, aes->devKey, aes->keylen);

            /* Set tmpAesKey privileges */
            keyPrivileges.u8Read    = VAULTIC_KP_ALL;
            keyPrivileges.u8Write   = VAULTIC_KP_ALL;
            keyPrivileges.u8Delete  = VAULTIC_KP_ALL;
            keyPrivileges.u8Execute = VAULTIC_KP_ALL;

            /* Set tmpAesKey data values */
            tmpAesKey.enKeyID                     = VLT_KEY_AES_128;
            tmpAesKey.data.SecretKey.u8Mask       = 0;
            tmpAesKey.data.SecretKey.u16KeyLength = c->aescbc_keylen;
            tmpAesKey.data.SecretKey.pu8Key       = (VLT_PU8)(c->aescbc_key);

#ifdef CCBVAULTIC_DEBUG_TIMING
            ts[0] = XNOW();
#endif
            /* Try to delete the tmp aes key.  Ignore errors here */
            VltDeleteKey(
                    CCBVAULTIC_WOLFSSL_GRPID,
                    CCBVAULTIC_TMPAES_KEYID);

#ifdef CCBVAULTIC_DEBUG_TIMING
              ts[1] = XNOW();
#endif
            /* Putkey aes->devKey, aes->keylen */
            c->vlt_rc = VltPutKey(
                    CCBVAULTIC_WOLFSSL_GRPID,
                    CCBVAULTIC_TMPAES_KEYID,
                    &keyPrivileges,
                    &tmpAesKey);
#if defined(CCBVAULTIC_DEBUG_ALL)
            XPRINTF("   VLT PutKey:%x\n", c->vlt_rc);
            hexdump(c->aescbc_key, c->aescbc_keylen);
#endif
            rc = translateError(c->vlt_rc);
            if (rc != 0)
                break;
        }

#ifdef CCBVAULTIC_DEBUG_TIMING
        ts[2] = XNOW();
#endif
        c->vlt_rc = VltInitializeAlgorithm(
                CCBVAULTIC_WOLFSSL_GRPID,
                CCBVAULTIC_TMPAES_KEYID,
                vlt_mode,
                &aescbc_algo_params);
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("   VLT InitAlgo:%x\n", c->vlt_rc);
#endif
        rc = translateError(c->vlt_rc);
        if (rc != 0)
            break;

#ifdef CCBVAULTIC_DEBUG_TIMING
        ts[3] = XNOW();
#endif
        /* Perform encrypt/decrypt*/
        if (encrypt) {
            c->vlt_rc = VltEncrypt(
                    info->cipher.aescbc.sz,
                    info->cipher.aescbc.in,
                    &out_len,
                    info->cipher.aescbc.sz,
                    info->cipher.aescbc.out);
#if defined(CCBVAULTIC_DEBUG_ALL)
            XPRINTF("   VLT Encrypt:%x\n", c->vlt_rc);
#endif
        } else {
            c->vlt_rc = VltDecrypt(
                    info->cipher.aescbc.sz,
                    info->cipher.aescbc.in,
                    &out_len,
                    info->cipher.aescbc.sz,
                    info->cipher.aescbc.out);
#if defined(CCBVAULTIC_DEBUG_ALL)
            XPRINTF("   VLT Decrypt:%x\n", c->vlt_rc);
#endif
        }
        rc = translateError(c->vlt_rc);
        if (rc != 0)
            break;

        /* Update state in AES CBC */
        XMEMCPY(aes->reg, last_block, AES_BLOCK_SIZE);

#ifdef CCBVAULTIC_DEBUG_TIMING
        ts[4] = XNOW();
        XPRINTF("   AES Encrypt(%d) Times(us): DltKey:%lu PutKey:%lu "
                "InitAlgo:%lu Encrypt:%lu InSize:%u OutSize:%lu\n",
                encrypt,
                (ts[1]-ts[0])/1000,
                (ts[2]-ts[1])/1000,
                (ts[3]-ts[2])/1000,
                (ts[4]-ts[3])/1000,
                info->cipher.aescbc.sz, out_len);
#endif

#endif /* CCBVAULTIC_NO_AES */
    }; break;

    case WC_CIPHER_AES_GCM:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleCipherCallback AES_GCM\n");
#endif
        break;

    case WC_CIPHER_AES_CTR:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleCipherCallback AES_CTR\n");
#endif
        break;

    case WC_CIPHER_AES_XTS:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleCipherCallback AES_XTS\n");
#endif
        break;

    case WC_CIPHER_AES_CFB:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleCipherCallback AES_CFB\n");
#endif
        break;

    case WC_CIPHER_AES_CCM:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleCipherCallback AES_CCM\n");
#endif
        break;

    case WC_CIPHER_AES_ECB:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleCipherCallback AES_ECB\n");
#endif
        break;
    default:
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("  HandleCipherCallback UNKNOWN\n");
#endif
        break;
    }
    return rc;
}
#endif /* WOLF_CRYPTO_CB */

#endif /* HAVE_CCBVAULTIC */
