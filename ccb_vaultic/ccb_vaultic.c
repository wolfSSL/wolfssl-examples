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

/* Provide default config struct if needed */
ccbVaultIc_Config gDefaultConfig = CCBVAULTIC_CONFIG_DEFAULT;

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
#include "wolfssl/wolfcrypt/types.h"        /* types and X-defines */

#include "wolfssl/wolfcrypt/hmac.h"         /* For HMACSHA256 */
/* Always need hash and hmac for kdf auth */
#include "wolfssl/wolfcrypt/hash.h"         /* For HASH_FLAGS and types */

#ifndef CCBVAULTIC_NO_RSA
#include "wolfssl/wolfcrypt/rsa.h"          /* For RSA_MAX_SIZE and types */
#endif

#ifndef CCBVAULTIC_NO_AES
#include "wolfssl/wolfcrypt/aes.h"          /* For AES_BLOCK_SIZE and types */
#endif

#if defined(CCBVAULTIC_DEBUG_TIMING) && !defined(XNOW)
#include <time.h>
#include <stdint.h>
#define XNOW(...) _Now(__VA_ARGS__)

static uint64_t _Now(void)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) < 0)
        /* Return 0 on error */
        return 0;
    return (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec;
}
#endif

#ifdef CCBVAULTIC_DEBUG
/* Helper to provide simple _HexDump */
static void _HexDump(const char* p, size_t data_len)
{
    XPRINTF("    HD:%p for %lu bytes\n",p, data_len);
    if ( (p == NULL) || (data_len == 0))
        return;
    size_t off = 0;
    for (off = 0; off < data_len; off++)
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


/* WiseKey VaultIC includes */
#include "vaultic_tls.h"
#include "vaultic_config.h"
#include "vaultic_common.h"
#include "vaultic_api.h"
#include "vaultic_structs.h"
#include "auth/vaultic_identity_authentication.h"
#include "vaultic_file_system.h"

/* Key/Group ID's to support temporary wolfSSL usage */
#define CCBVAULTIC_WOLFSSL_GRPID 0xBB
#define CCBVAULTIC_TMPAES_KEYID 0x01
#define CCBVAULTIC_TMPHMAC_KEYID 0x02
#define CCBVAULTIC_TMPRSA_KEYID 0x03

/* Key attributes */
#define VAULTIC_KP_ALL 0xFF  /* Allow all users all key privileges */
#define VAULTIC_PKV_ASSURED VLT_PKV_ASSURED_EXPLICIT_VALIDATION


/* Helper to translate vlt return codes to wolfSSL code */
static int _TranslateError(int vlt_rc)
{
    /* vlt return codes are defined in src/common/vaultic_err.h */
    switch (vlt_rc) {
    case 0:        /* returned on successful init */
    case VLT_OK:
        return 0;
    default:
        /* Default to point to IO */
        return IO_FAILED_E;
    }
}

static int _GetInfo(ccbVaultIc_Context *c, VLT_TARGET_INFO *out_chipInfo)
{
    if ((c == NULL) || (out_chipInfo == NULL)) {
        return BAD_FUNC_ARG;
    }
    XMEMSET(out_chipInfo, 0, sizeof(*out_chipInfo));

    /* Get current chip info */
    c->vlt_rc = VltGetInfo(out_chipInfo);
#ifdef CCBVAULTIC_DEBUG_ALL
    XPRINTF("_GetInfo: vlt_rc:%04X serial:%p firmware:%.*s, \n" \
            "mode:%d, state:%02X, selftests:%d, space:%d\n",
            c->vlt_rc, out_chipInfo->au8Serial,
            (int)sizeof(out_chipInfo->au8Firmware), out_chipInfo->au8Firmware,
            out_chipInfo->enMode, out_chipInfo->enState, out_chipInfo->enSelfTests,
            (int)out_chipInfo->u32Space);
    _HexDump((const char*)out_chipInfo->au8Serial, sizeof(out_chipInfo->au8Serial));
#endif
    return _TranslateError(c->vlt_rc);
}

static int _GetInfoText(ccbVaultIc_Context* c, int text_len, char* text,
        int *out_len)
{
    VLT_TARGET_INFO chipInfo;
    int rc = _GetInfo(c, &chipInfo);
    if ((rc == 0) && (text != NULL) && (text_len > 0)) {
        int len = snprintf(text, text_len,
                "VERSION:%.*s\n"
                "SERIAL:%02X%02X%02X%02X%02X%02X%02X%02X\n"
                "STATE:%02X\n"
                "MODE:%02X\n"
                "ROLE:%02X\n"
                "SELFTEST:%02X\n"
                "SPACE:%d\n",
                (int)sizeof(chipInfo.au8Firmware),
                (const char*)chipInfo.au8Firmware,
                chipInfo.au8Serial[0],
                chipInfo.au8Serial[1],
                chipInfo.au8Serial[2],
                chipInfo.au8Serial[3],
                chipInfo.au8Serial[4],
                chipInfo.au8Serial[5],
                chipInfo.au8Serial[6],
                chipInfo.au8Serial[7],
                chipInfo.enState,
                chipInfo.enMode,
                chipInfo.enRole,
                chipInfo.enSelfTests,
                 (int)chipInfo.u32Space);
        if (out_len != NULL) {
            *out_len = len;
        }
    }
    return rc;
}

static VLT_USER_ID _AuthId2VltUserId(int id)
{
    switch(id) {
    case 0: return VLT_USER0;
    case 1: return VLT_USER1;
    case 2: return VLT_USER2;
    case 3: return VLT_USER3;
    case 4: return VLT_USER4;
    case 5: return VLT_USER5;
    case 6: return VLT_USER6;
    case 7:
    default: break;
    }
    return VLT_USER7;
}
static VLT_ROLE_ID _AuthRole2VltRoleId(int role)
{
    switch(role) {
    case CCBVAULTIC_AUTH_ROLE_NONE:         return VLT_EVERYONE;
    case CCBVAULTIC_AUTH_ROLE_APPROVED:     return VLT_APPROVED_USER;
    case CCBVAULTIC_AUTH_ROLE_UNAPPROVED:   return VLT_NON_APPROVED_USER;
    case CCBVAULTIC_AUTH_ROLE_MANUFACTURER: return VLT_MANUFACTURER;
    default:
        break;
    }
    return VLT_EVERYONE;
}

/* Perform HMAC SHA256 on input data to generate 2 keys up to 32 bytes total */
static int _PerformKdf(int key_len, const char* key,
        int in1_len, const char* in1,
        int in2_len, const char* in2,
        int in3_len, const char* in3,
        int out1_len, char* out1,
        int out2_len, char* out2)
{
    Hmac kdf;
    int rc;
    if (    (key_len   == 0) || (key  == NULL) ||      /* Key is required */
            ((in1_len  <= 0) && (in1  == NULL)) ||
            ((in2_len  <= 0) && (in2  == NULL)) ||
            ((in3_len  <= 0) && (in3  == NULL)) ||
            ((out1_len <= 0) && (out1 == NULL)) ||
            ((out2_len <= 0) && (out2 == NULL)) ){
        return BAD_FUNC_ARG;
    }

    rc = wc_HmacInit(&kdf, NULL, INVALID_DEVID);
    if (rc == 0) {
        int out_count=0;
        byte digest[WC_SHA256_DIGEST_SIZE];
        XMEMSET(digest, 0, sizeof(digest));

        rc = wc_HmacSetKey(&kdf, WC_SHA256, (const byte*)key, key_len);
        if (rc == 0) {
            rc = wc_HmacUpdate(&kdf, (const byte*)in1, in1_len);
        }
        if ((rc == 0) && (in2_len > 0) && (in2 != NULL)) {
            rc = wc_HmacUpdate(&kdf, (const byte*)in2, in2_len);
        }
        if ((rc == 0) && (in3_len > 0) && (in3 != NULL)) {
            rc = wc_HmacUpdate(&kdf, (const byte*)in3, in3_len);
        }
        if (rc == 0) {
            rc = wc_HmacFinal(&kdf, digest);
            if ((rc == 0) && (out1_len > 0)) {
                int this_len = sizeof(digest) - out_count;
                if (this_len > out1_len) this_len = out1_len;
                XMEMCPY(out1, &digest[out_count], this_len);
                XMEMSET(out1 + this_len, 0, out1_len - this_len);
                out_count += this_len;
            }
            if ((rc == 0) && (out2_len > 0)) {
                int this_len = sizeof(digest) - out_count;
                if (this_len > out2_len) this_len = out2_len;
                XMEMCPY(out2, &digest[out_count], this_len);
                XMEMSET(out2 + this_len, 0, out2_len - this_len);
                out_count += this_len;
            }
        }
        wc_HmacFree(&kdf);
    }
#ifdef CCBVAULTIC_DEBUG_ALL
    XPRINTF("_PerformKdf: rc:%d key_len:%d key:%p, \n"
            "in1_len:%d in1:%p, in2_len:%d in2:%p,in3_len:%d in3:%p \n"
            "out1_len:%d out1:%p, out2_len:%d out2:%p,\n",
            rc, key_len, key,
            in1_len, in1, in2_len, in2, in3_len, in3,
            out1_len, out1, out2_len, out2);
    _HexDump(key, key_len);
    _HexDump(in1, in1_len);
    _HexDump(in2, in2_len);
    _HexDump(in3, in3_len);
    _HexDump(out1, out1_len);
    _HexDump(out2, out2_len);
#endif

    return rc;
}

static int _AuthByPin(  ccbVaultIc_Context *c, int id, int role,
                        int pin_len, const char* pin)
{
    if ((pin_len > CCBVAULTIC_AUTH_PIN_LEN_MAX) ||
        (pin_len < CCBVAULTIC_AUTH_PIN_LEN_MIN)) {
        return BAD_FUNC_ARG;
    }
#ifdef CCBVAULTIC_DEBUG_ALL
    XPRINTF("_AuthByPin: c:%p id:%d role:%d, pin_len:%d pin:%p\n",
            c, id, role, pin_len, pin);
    _HexDump(pin, pin_len);
#endif
    /* Auth using password. */
    c->vlt_rc= VltSubmitPassword(
            _AuthId2VltUserId(id),
            _AuthRole2VltRoleId(role),
            (VLT_U8) pin_len,
            (VLT_PU8) pin);
    return _TranslateError(c->vlt_rc);
}

static int _AuthByScp03(    ccbVaultIc_Context *c, int id, int role,
                            int mac_len, char* mac,
                            int enc_len, char* enc)
{
    KEY_BLOB macKey;
    KEY_BLOB encKey;
    KEY_BLOB_ARRAY keys;

#ifdef CCBVAULTIC_DEBUG_ALL
    XPRINTF("_AuthByScp03: c:%p id:%d, role:%d, "
            " mac_len:%d mac:%p, enc_len:%d enc:%p\n",
            c, id, role,
            mac_len, mac, enc_len, enc);
    _HexDump(mac, mac_len);
    _HexDump(enc, enc_len);
#endif
    XMEMSET(&macKey, 0, sizeof(macKey));
    XMEMSET(&encKey, 0, sizeof(encKey));
    XMEMSET(&keys, 0, sizeof(keys));

    macKey.keyType = VLT_KEY_AES_128;
    macKey.keySize = (VLT_U16) mac_len;
    macKey.keyValue = (VLT_PU8) mac;

    encKey.keyType = VLT_KEY_AES_128;
    encKey.keySize = (VLT_U16) enc_len;
    encKey.keyValue = (VLT_PU8) enc;

    keys.u8ArraySize = 2;
    keys.pKeys[0] = &macKey;
    keys.pKeys[1] = &encKey;

    c->vlt_rc=VltAuthInit(
        VLT_AUTH_SCP03,
        _AuthId2VltUserId(id),
        _AuthRole2VltRoleId(role),
        VLT_CMAC_CENC_RMAC_RENC,
        keys);
    return _TranslateError(c->vlt_rc);
}

static int _AuthByKdf(  ccbVaultIc_Context *c, int id, int role,
                        int key_len, const char* key,
                        int label_len, const char* label)
{
    char mac_data[CCBVAULTIC_AUTH_MAC_LEN];
    char enc_data[CCBVAULTIC_AUTH_ENC_LEN];

#ifdef CCBVAULTIC_DEBUG_ALL
    XPRINTF("_AuthByKdf: c:%p id:%d role:%d, "
            "key_len:%d key:%p, label_len:%d label:%p\n",
            c, id, role,
            key_len, key, label_len, label);
    _HexDump(key, key_len);
    _HexDump(label, label_len);
#endif

    int rc = _PerformKdf(   key_len, key,
                            label_len, label,
                            CCBVAULTIC_SERIAL_LEN, c->vlt_serial,
                            CCBVAULTIC_VERSION_LEN, c->vlt_version,
                            sizeof(mac_data), mac_data,
                            sizeof(enc_data), enc_data);
    if (rc == 0) {
        rc = _AuthByScp03(  c, id, role,
                            sizeof(mac_data), mac_data,
                            sizeof(enc_data), enc_data);
    }
    return rc;
}

/* Helper to perform auth as the provided user */
static int _InitAuth(ccbVaultIc_Context *c, const ccbVaultIc_Auth *a)
{
    int rc = 0;
    if ((c == NULL) || (c->initialized != 0) || (a == NULL)) {
        return BAD_FUNC_ARG;
    }

#ifdef CCBVAULTIC_DEBUG_ALL
    XPRINTF("_InitAuth: c:%p a:%p kind:%d\n",
            c, a, a->kind);
#endif

    switch(a->kind) {
    case CCBVAULTIC_AUTH_KIND_NONE:
    {
        /* Ok.  Unexpected, but not an error */
        rc = 0;
    }; break;

    case CCBVAULTIC_AUTH_KIND_PIN:
    {
        rc = _AuthByPin(c, a->id, a->role,
                a->auth.pin.pin_len,a->auth.pin.pin);
    };break;

    case CCBVAULTIC_AUTH_KIND_SCP03:
    {
        char mac_data[CCBVAULTIC_AUTH_MAC_LEN];
        char enc_data[CCBVAULTIC_AUTH_ENC_LEN];

        if (    (a->auth.scp03.mac_len < sizeof(mac_data)) ||
                (a->auth.scp03.enc_len < sizeof(enc_data))) {
            rc = BAD_FUNC_ARG;
            break;
        }

        /* Copy to temp buffer to support const auth */
        XMEMCPY(mac_data, a->auth.scp03.mac, sizeof(mac_data));
        XMEMCPY(enc_data, a->auth.scp03.enc, sizeof(enc_data));
        rc = _AuthByScp03(  c, a->id, a->role,
                            sizeof(mac_data), mac_data,
                            sizeof(enc_data), enc_data);
    };break;

    case CCBVAULTIC_AUTH_KIND_KDF:
    {
        rc = _AuthByKdf(    c, a->id, a->role,
                            a->auth.kdf.key_len, a->auth.kdf.key,
                            a->auth.kdf.label_len, a->auth.kdf.label);
    };break;

    default:
        rc = BAD_FUNC_ARG;
    }
    return rc;
}

/* Helper to check for NULL and uninitialized contexts */
static int _CheckInitializedContext(ccbVaultIc_Context *c)
{
    if ((c == NULL) || (c->initialized == 0)) {
        return BAD_FUNC_ARG;
    }
    return 0;
}

static void _ClearContext(ccbVaultIc_Context *c)
{
    const ccbVaultIc_Config* saveConfig = c->config;
    XMEMSET(c, 0, sizeof(*c));
    c->config=saveConfig;
    c->m = NULL;
    c->aescbc_key = NULL;
}

int ccbVaultIc_GetDefaultConfig(ccbVaultIc_Config* *out_c)
{
    if (out_c == NULL) {
        return BAD_FUNC_ARG;
    }
    *out_c = &gDefaultConfig;
    return 0;
}

int ccbVaultIc_Init(ccbVaultIc_Context *c)
{
    int rc = 0;
    if (c == NULL) {
        return BAD_FUNC_ARG;
    }

    /* Already Initialized? */
    if (c->initialized > 0) {
        /* Increment use count */
        c->initialized++;
        return 0;
    }
    _ClearContext(c);

    /* Open the hardware and authenticate */
#if 0
    c->vlt_rc = vlt_tls_init();
    rc = _TranslateError(c->vlt_rc);
#else
    const ccbVaultIc_Config* config = &gDefaultConfig;
    VLT_INIT_COMMS_PARAMS params;


    /* Override config */
    if (c->config != NULL) {
        config = c->config;
    }

    /* Set timeout, checksum, and interface type */
    XMEMSET(&params, 0, sizeof(params));
    params.VltBlockProtocolParams.u16msSelfTestDelay = config->startup_delay_ms;
    params.VltBlockProtocolParams.u32msTimeout       = config->timeout_ms;
    params.VltBlockProtocolParams.enCheckSumMode     = BLK_PTCL_CHECKSUM_SUM8;
    params.enCommsProtocol                           = VLT_SPI_COMMS;
    params.VltSpiParams.u16BitRate                   = config->spi_rate_khz;

    /* Initialize the API and establish comms*/
    c->vlt_rc = VltApiInit(&params);
    rc = _TranslateError(c->vlt_rc);
    if (rc == 0) {
        VLT_TARGET_INFO chipInfo;

        /* Cancel any active authentication, Ignore errors here */
        VltAuthClose();

        /* Get current chip info */
        rc = _GetInfo(c, &chipInfo);
        if (rc == 0) {
#ifdef CCBVAULTIC_DEBUG
    XPRINTF("ccbVaultIc_Info: serial:%p firmware:%.*s, \n" \
            "mode:%d, state:%d, selftests:%d, space:%d\n",
            chipInfo.au8Serial,
            (int)sizeof(chipInfo.au8Firmware), chipInfo.au8Firmware,
            chipInfo.enMode, chipInfo.enState, chipInfo.enSelfTests,
            (int)chipInfo.u32Space);
    _HexDump((const char*)chipInfo.au8Serial, sizeof(chipInfo.au8Serial));
#endif

            /* Save this data to the context */
            XMEMCPY(c->vlt_serial, chipInfo.au8Serial, sizeof(chipInfo.au8Serial));
            XMEMCPY(c->vlt_version, chipInfo.au8Firmware, sizeof(chipInfo.au8Firmware));

            /* Ensure chip is not TERMINATED and no one is auth'ed */
            if (    (chipInfo.enState == VLT_STATE_TERMINATED) ||
                    (chipInfo.enRole != VLT_EVERYONE)){
                /* Nothing to do.  Return hardware error */
                rc = WC_HW_E;
            }
            if (rc == 0) {
                rc = _InitAuth(c, &config->auth);
            }
        }
    }
#endif

    if (rc == 0) {
        c->initialized = 1;
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

    _ClearContext(c);

    /* Set the return value in the struct */
    /* Close the hardware */
    c->vlt_rc = vlt_tls_close();
}

int ccbVaultIc_SelfTest( ccbVaultIc_Context *c)
{
    int rc = _CheckInitializedContext(c);
    if (rc == 0) {
        //XXX SKIP FOR NOW.  Failure will set TERMINATED
        //c->vlt_rc = VltSelfTest();
        c->vlt_rc = VLT_OK;
        rc = _TranslateError(c->vlt_rc);
    }
    return rc;
}
/* Helper.  Missing XSTRNLEN */
static int ccbVaultIc_Strnlen(const char *s, int n)
{
    int len = 0;
    while( (len < n) && (s[len] != 0)) {
        len++;
    }
    return len;
}

static int _CheckFile(  ccbVaultIc_Context *c,
                        const ccbVaultIc_File *f,
                        int userId, int adminId)
{
    int rc = _CheckInitializedContext(c);
    if (rc == 0) {
        if (    (f == NULL) ||
                (f->name_len < CCBVAULTIC_FILE_NAME_LEN_MIN) ||
                (f->name_len > CCBVAULTIC_FILE_NAME_LEN_MAX) ||
                (f->data_len > CCBVAULTIC_FILE_DATA_LEN_MAX) ||
                (userId < CCBVAULTIC_AUTH_ID_MIN) ||
                (userId > CCBVAULTIC_AUTH_ID_MAX) ||
                (adminId < CCBVAULTIC_AUTH_ID_MIN) ||
                (adminId > CCBVAULTIC_AUTH_ID_MAX)) {
            rc = BAD_FUNC_ARG;
        }
    }
    return rc;
}

static int _OpenFile(   ccbVaultIc_Context *c,
                        const ccbVaultIc_File *f,
                        int *out_dataLen)
{
    int rc = _CheckFile(c, f,
            CCBVAULTIC_AUTH_ID_MIN, CCBVAULTIC_AUTH_ID_MIN  /* Dummy Ids */
            );
    if (rc == 0) {
        VLT_FS_ENTRY_PARAMS  structFileEntry;
        XMEMSET(&structFileEntry, 0, sizeof(structFileEntry));

        c->vlt_rc = VltFsOpenFile(
                (VLT_U16)f->name_len,
                (VLT_U8*)f->name,
                (VLT_BOOL)FALSE,            /* No transaction */
                &structFileEntry);
        rc = _TranslateError(c->vlt_rc);

#ifdef CCBVAULTIC_DEBUG_ALL
    XPRINTF("ccbVaultIc_OpenFile rc:%d (%x) c:%p f:%p name_len:%d name:%.*s, data_len:%d data:%p\n",
            rc, c->vlt_rc, c, f, f->name_len, (int)f->name_len, f->name, f->data_len, f->data);
#endif
        /* Update output on success */
        if ((rc == 0) && (out_dataLen != NULL)) {
            *out_dataLen = structFileEntry.u32FileSize;
        }
   }
   return rc;
}

static void _SetVltUserAccessBit(VLT_USER_ACCESS* u, int userId)
{
    if (u == NULL) {
        return;
    }
    switch(userId) {
    case 0: u->user0 = 1; return;
    case 1: u->user1 = 1; return;
    case 2: u->user2 = 1; return;
    case 3: u->user3 = 1; return;
    case 4: u->user4 = 1; return;
    case 5: u->user5 = 1; return;
    case 6: u->user6 = 1; return;
    case 7: u->user7 = 1; return;
    default: break;
    }
}

int ccbVaultIc_CreateUserFile(  ccbVaultIc_Context *c,
                                const ccbVaultIc_File *f,
                                int userId, int adminId)
{
    int rc = _CheckFile(c, f, userId, adminId);
    if (rc == 0) {
        VLT_USER_ACCESS priv;
        VLT_FS_ENTRY_PARAMS entryParams;

        XMEMSET(&priv, 0, sizeof(priv));
        XMEMSET(&entryParams, 0, sizeof(entryParams));

        /* All privileges for the user and the admin */
        _SetVltUserAccessBit(&priv, userId);
        _SetVltUserAccessBit(&priv, adminId);
        entryParams.filePriv.readPrivilege    = priv;
        entryParams.filePriv.writePrivilege   = priv;
        entryParams.filePriv.deletePrivilege  = priv;
        entryParams.filePriv.executePrivilege = priv;
        entryParams.attribs.readOnly          = 0;      /* Read/Write */
        entryParams.attribs.system            = 0;      /* Non-system */
        entryParams.attribs.hidden            = 0;      /* Visible */
        entryParams.u32FileSize               = 0;      /* Empty */
        entryParams.u8EntryType               = VLT_FILE_ENTRY;

        c->vlt_rc = VltFsCreate(
                (VLT_U16)f->name_len+1,
                (VLT_PU8)f->name,
                &entryParams,
                (VLT_USER_ID) userId);  /* Owner */
        rc = _TranslateError(c->vlt_rc);
#ifdef CCBVAULTIC_DEBUG_ALL
    XPRINTF("ccbVaultIc_CreateUserFile rc:%d (%x) c:%p f:%p name_len:%d name:%.*s\n",
            rc, c->vlt_rc, c, f, f->name_len, (int)f->name_len, f->name);
#endif
    }
    return rc;
}

int ccbVaultIc_DeleteFile(  ccbVaultIc_Context *c,
                            const ccbVaultIc_File *f)
{

    int rc = _CheckFile(c, f,
            CCBVAULTIC_AUTH_ID_MIN, CCBVAULTIC_AUTH_ID_MIN /* Dummy Ids */
            );
    if (rc == 0) {
        c->vlt_rc = VltFsDelete(
                (VLT_U16)f->name_len,
                (VLT_U8*)f->name,
                (VLT_BOOL)FALSE);           /* Not recursive */
        rc = _TranslateError(c->vlt_rc);
#ifdef CCBVAULTIC_DEBUG_ALL
    XPRINTF("ccbVaultIc_DeleteFile rc:%d (%x) c:%p f:%p name_len:%d name:%.*s\n",
            rc, c->vlt_rc, c, f, f->name_len, (int)f->name_len, f->name);
#endif
    }
    return rc;
}

int ccbVaultIc_WriteFile(   ccbVaultIc_Context *c,
                            const ccbVaultIc_File *f)
{
    int rc = _OpenFile(c, f, NULL);
    if (rc == 0) {
        c->vlt_rc = VltFsWriteFile(
                (VLT_U32)VLT_SEEK_FROM_START,
                (VLT_U8*)f->data,
                (VLT_U32)f->data_len,
                (VLT_BOOL)TRUE);                  /* Reclaim space */
        rc = _TranslateError(c->vlt_rc);
#ifdef CCBVAULTIC_DEBUG_ALL
    XPRINTF("ccbVaultIc_WriteFile rc:%d (%x) c:%p f:%p name_len:%d name:%.*s, data_len:%d data:%p\n",
            rc, c->vlt_rc, c, f, f->name_len, (int)f->name_len, f->name, f->data_len, f->data);
    _HexDump(f->data, f->data_len);
#endif

        /* Close and ignore error here */
        VltFsCloseFile();
    }
    return rc;
}

int ccbVaultIc_ReadFile(    ccbVaultIc_Context *c,
                            ccbVaultIc_File *f)
{
    int fileLen = 0;
    int maxLen = f->data_len;
    int rc = _OpenFile(c, f, &fileLen);
    if (rc == 0) {
        VLT_U32 readLen = fileLen;
        if (readLen > maxLen) {
            readLen = maxLen;
        }

        /* Reset file struct data */
        XMEMSET(f->data, 0, maxLen);

        c->vlt_rc = VltFsReadFile(
                (VLT_U32)VLT_SEEK_FROM_START,
                (VLT_U8*) f->data,
                &readLen);
        rc = _TranslateError(c->vlt_rc);

        if (rc == 0) {
            /* Success.  Update file struct */
            f->data_len = readLen;
        }
#ifdef CCBVAULTIC_DEBUG_ALL
    XPRINTF("ccbVaultIc_ReadFile rc:%d (%x) c:%p f:%p name_len:%d name:%.*s, data_len:%d data:%p\n",
            rc, c->vlt_rc, c, f, f->name_len, (int)f->name_len, f->name, f->data_len, f->data);
    _HexDump(f->data, f->data_len);
#endif

        /* Close and ignore error here */
        VltFsCloseFile();
    }
    return rc;
}

static int _DeleteUser(ccbVaultIc_Context *c, const ccbVaultIc_Auth *a)
{
    int rc = _CheckInitializedContext(c);
    if (rc == 0) {
        if (a == NULL) {
            rc = BAD_FUNC_ARG;
        }
        if(rc == 0) {
            VLT_MANAGE_AUTH_DATA authData;
            XMEMSET(&authData, 0, sizeof(authData));
            authData.enOperationID = VLT_DELETE_USER;
            authData.enUserID = _AuthId2VltUserId(a->id);

            c->vlt_rc = VltManageAuthenticationData(&authData);
            rc = _TranslateError(c->vlt_rc);
        }
    }
    return rc;
}
static int _CreateUser_SCP03(ccbVaultIc_Context *c, int id, int role,
                             int mac_len, char* mac,
                            int enc_len, char* enc)
{
    int rc = 0;
    VLT_MANAGE_AUTH_DATA authData;
    XMEMSET(&authData, 0, sizeof(authData));

    authData.enOperationID = VLT_CREATE_USER;
    authData.enUserID = _AuthId2VltUserId(id);
    authData.u8TryCount = 5; /* From VLT example */
    authData.enSecurityOption = VLT_NO_DELETE_ON_LOCK;
    authData.enRoleID = _AuthRole2VltRoleId(role);
    authData.enMethod = VLT_AUTH_SCP03;
    authData.enChannelLevel = VLT_CMAC_CENC;
    authData.data.secret.u8NumberOfKeys = 2;
    authData.data.secret.aKeys[0].enKeyID = VLT_KEY_AES_128;
    authData.data.secret.aKeys[0].u8Mask = 0xBE;
    authData.data.secret.aKeys[0].u16KeyLength = mac_len;
    authData.data.secret.aKeys[0].pu8Key = (VLT_PU8) mac;
    authData.data.secret.aKeys[1].enKeyID = VLT_KEY_AES_128;
    authData.data.secret.aKeys[1].u8Mask = 0xEF;
    authData.data.secret.aKeys[1].u16KeyLength = enc_len;
    authData.data.secret.aKeys[1].pu8Key = (VLT_PU8) enc;

    c->vlt_rc = VltManageAuthenticationData(&authData);
    rc = _TranslateError(c->vlt_rc);
    return rc;
}
static int _CreateUser(ccbVaultIc_Context *c,const ccbVaultIc_Auth *a)
{
    int rc = _CheckInitializedContext(c);
    if (rc == 0) {
        if (a == NULL) {
            rc = BAD_FUNC_ARG;
        }
        if(rc == 0) {
            char mac[CCBVAULTIC_AUTH_MAC_LEN];
            char enc[CCBVAULTIC_AUTH_ENC_LEN];

            XMEMSET (mac, 0, sizeof(mac));
            XMEMSET (enc, 0, sizeof(enc));
            switch (a->kind) {
                case CCBVAULTIC_AUTH_KIND_SCP03:
                {
                    XMEMCPY(mac, a->auth.scp03.mac, a->auth.scp03.mac_len);
                    XMEMCPY(enc, a->auth.scp03.enc, a->auth.scp03.enc_len);
                    rc = _CreateUser_SCP03(c, a->id, a->role,
                                           a->auth.scp03.mac_len, mac,
                                           a->auth.scp03.enc_len, enc);
                };break;
                case CCBVAULTIC_AUTH_KIND_KDF:
                {
                    int rc = _PerformKdf(   a->auth.kdf.key_len, a->auth.kdf.key,
                                            a->auth.kdf.label_len, a->auth.kdf.label,
                                            CCBVAULTIC_SERIAL_LEN, c->vlt_serial,
                                            CCBVAULTIC_VERSION_LEN, c->vlt_version,
                                            sizeof(mac), mac,
                                            sizeof(enc), enc);
                    if (rc == 0) {
                        rc = _CreateUser_SCP03(  c, a->id, a->role,
                                            sizeof(mac), mac,
                                            sizeof(enc), enc);
                    }
                };break;
                case CCBVAULTIC_AUTH_KIND_NONE:
                case CCBVAULTIC_AUTH_KIND_PIN:
                default:
                    rc = BAD_FUNC_ARG;
                    break;
            }
        }
    }
    return rc;
}

static int _SetState(ccbVaultIc_Context *c, VLT_STATE state)
{
    int rc = _CheckInitializedContext(c);
    if (rc == 0) {
        VLT_TARGET_INFO chipInfo;
        rc = _GetInfo(c, &chipInfo);
        if (rc == 0) {
            if (chipInfo.enState != state) {
                c->vlt_rc = VltSetStatus(state);
                rc = _TranslateError(c->vlt_rc);
            }
        }
    }
    return rc;
}
static int _SetCreationState(ccbVaultIc_Context *c)
{
    return _SetState(c, VLT_STATE_CREATION);
}
static int _SetActivatedState(ccbVaultIc_Context *c)
{
    return _SetState(c, VLT_STATE_ACTIVATED);
}

/* Perform the load action as the currently authed user */
int ccbVaultIc_LoadAction(  ccbVaultIc_Context *c,
                            ccbVaultIc_Load *l)
{
    int rc = _CheckInitializedContext(c);
    if (rc == 0) {
        if (l == NULL) {
            rc = BAD_FUNC_ARG;
        }
        /* Read file data into the load structure */
        if (rc == 0) {
            int counter = 0;
            for(counter = 0; counter < l->file_count; counter++) {
                /* Ok to skip empty files */
                if(     (l->file[counter].name != NULL) &&
                        (l->file[counter].name_len > 0) &&
                        (l->file[counter].data != NULL) &&
                        (l->file[counter].data_len > 0)) {
                    rc = ccbVaultIc_ReadFile(c, &l->file[counter]);
                    if (rc != 0) break;
                }
            }
        }
    }
    return rc;
}

/* Perform the provision action as the currently authed user */
int ccbVaultIc_ProvisionAction( ccbVaultIc_Context *c,
                                const ccbVaultIc_Provision *p)
{
    int rc = _CheckInitializedContext(c);
    if (rc == 0) {
        if (p == NULL) {
            rc = BAD_FUNC_ARG;
        }

        if (rc == 0) {
            /* Setting creation mode should delete all users and their files */
            rc = _SetCreationState(c);
        }

        if (rc == 0) {
            /* Update self test configuration */
            // rc = ccbVaultIc_SetSelfTest(c, p->self_test);
        }

        /* Add User */
        if (rc == 0) {
            /* Create the requested user */
            _DeleteUser(c, &p->create);
            rc = _CreateUser(c, &p->create);
        }

        /* Create and write file data from the provision structure */
        if (rc == 0) {
            int counter = 0;
            int userId = p->create.id;
            int adminId = 0;
            const ccbVaultIc_Config *config = c->config;
            if(config == NULL) {
                ccbVaultIc_GetDefaultConfig(&config);
            }
            adminId = config->auth.id;
            for(counter = 0; counter < p->file_count; counter++) {
                if( (p->file[counter].name != NULL) &&
                    (p->file[counter].name_len > 0)) {
                    /* Delete any existing file.  Ignore errors */
                    ccbVaultIc_DeleteFile(c, &p->file[counter]);

                    /* Create the file */
                    rc = ccbVaultIc_CreateUserFile(c, &p->file[counter],
                                                   userId, adminId);
                    if (rc == 0) {
                        /* Write the file */
                        rc = ccbVaultIc_WriteFile(c, &p->file[counter]);
                    }
                    if (rc != 0) break;
                }
            }
        }

        if (rc == 0) {
            /* Set Activated State */
            rc = _SetActivatedState(c);
        }
    }
    return rc;
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

    case CCBVAULTIC_CMD_INFO:
    {
        /* Issue an info command and convert the results to a text string */
        ccbVaultIc_Info* i = info->cmd.ctx;

        char* text = NULL;
        int text_len = 0;

        if(i != NULL) {
            text = i->text;
            text_len = i->text_len;
        }

        rc = _GetInfoText(c, text_len, text, &text_len);
        if(i != NULL) {
            i->text_len = text_len;
        }
    }; break;

    case CCBVAULTIC_CMD_LOADACTION:
    {
        if(_CheckInitializedContext(c) != 0)
            break;
        rc = ccbVaultIc_LoadAction(c, info->cmd.ctx);
    }; break;

    case CCBVAULTIC_CMD_PROVISIONACTION:
    {
        if(_CheckInitializedContext(c) != 0)
            break;
        rc = ccbVaultIc_ProvisionAction(c, info->cmd.ctx);
    }; break;

    case CCBVAULTIC_CMD_SELFTEST:
    {
        if(_CheckInitializedContext(c) != 0)
            break;
        rc = ccbVaultIc_SelfTest(c);
    }; break;

    default:
        break;
    }
#if defined(CCBVAULTIC_DEBUG_ALL)
        XPRINTF("HandleCmdCallback %d: c:%p rc:%d\n", info->cmd.type, c, rc);
#endif

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
            _HexDump((const char*)e,sizeof(e));
            _HexDump((const char*)e_pad,sizeof(e_pad));
            _HexDump((const char*)n,sizeof(n));
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
            rc = _TranslateError(c->vlt_rc);
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
            rc = _TranslateError(c->vlt_rc);
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
            rc = _TranslateError(c->vlt_rc);
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
                c->m = (unsigned char*)XMALLOC(info->hash.inSz, NULL, NULL);
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
            rc = _TranslateError(c->vlt_rc);
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
                rc = _TranslateError(c->vlt_rc);
                if (rc != 0)
                    break;

                c->vlt_rc = VltComputeMessageDigestFinal(
                        &sha_out_len,
                        WC_SHA256_DIGEST_SIZE,
                        info->hash.digest);
#if defined(CCBVAULTIC_DEBUG_ALL)
                XPRINTF("   VltFinal SHA256:%x\n", c->vlt_rc);
#endif
                rc = _TranslateError(c->vlt_rc);
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
            _HexDump(info->hash.digest, WC_SHA256_DIGEST_SIZE);
#endif
                rc = _TranslateError(c->vlt_rc);
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
            _HexDump((void*)aes->devKey, aes->keylen);
#endif
            /* Free the current key buffer if necessary */
            if (c->aescbc_key != NULL) {
                XFREE(c->aescbc_key, NULL, NULL);
                c->aescbc_key    = NULL;
                c->aescbc_keylen = 0;
            }

            /* Allocate key buffer */
            c->aescbc_key = (unsigned char*)XMALLOC(aes->keylen, NULL, NULL);
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
            _HexDump((const char*)c->aescbc_key, c->aescbc_keylen);
#endif
            rc = _TranslateError(c->vlt_rc);
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
        rc = _TranslateError(c->vlt_rc);
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
        rc = _TranslateError(c->vlt_rc);
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
