/*
 * ccb_vaultic.h
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

#ifndef CCB_VAULTIC_H
#define CCB_VAULTIC_H

/* VaultIC DevID MSBs are ASCII "VI" */
#define CCBVAULTIC_DEVID (0x56490000ul)
#define CCBVAULTIC420_DEVID (CCBVAULTIC_DEVID + 0x0420)

#ifdef HAVE_CCBVAULTIC

/*
 * Implementation of wolfCrypt cryptocb callbacks
 *
 * The wolfSSL port of the Wisekey VaultIC provides a wrapper library to allow
 * the VaultIC to be used as an external crypto provider.  This library depends
 * on the Wisekey-provided VaultIC interface libraries that have been statically
 * compiled in the proper hardware configuration.
 */

/* Fixed sizes and values */
enum {
    CCBVAULTIC_SERIAL_LEN               = 8,
    CCBVAULTIC_VERSION_LEN              = 32,

    CCBVAULTIC_CMD_INFO                 = 0x8000,
    CCBVAULTIC_CMD_LOADACTION           = 0x8001,
    CCBVAULTIC_CMD_PROVISIONACTION      = 0x8002,
    CCBVAULTIC_CMD_SELFTEST             = 0x8003,

    CCBVAULTIC_INFO_LEN                 = 128,
};

/* Configuration choices */
enum {
    CCBVAULTIC_FAST_START_MS   = 700,
    CCBVAULTIC_SLOW_START_MS   = 5000,
    CCBVAULTIC_APDU_TIMEOUT_MS = 5000,
    CCBVAULTIC_SPI_RATE_KHZ    = 5000,
};


/* Authenticated User:
 *
 */
enum {
    CCBVAULTIC_AUTH_ID_MIN              = 0,
    CCBVAULTIC_AUTH_ID_MAX              = 7,

    CCBVAULTIC_AUTH_ROLE_NONE           = 0,
    CCBVAULTIC_AUTH_ROLE_MANUFACTURER   = 1,
    CCBVAULTIC_AUTH_ROLE_UNAPPROVED     = 2,
    CCBVAULTIC_AUTH_ROLE_APPROVED       = 3,

    CCBVAULTIC_AUTH_KIND_NONE           = 0,
    CCBVAULTIC_AUTH_KIND_PIN            = 1,
    CCBVAULTIC_AUTH_KIND_SCP03          = 2,
    CCBVAULTIC_AUTH_KIND_KDF            = 3,

    CCBVAULTIC_AUTH_PIN_LEN_MIN = 0x4,
    CCBVAULTIC_AUTH_PIN_LEN_MAX = 0x20,

    CCBVAULTIC_AUTH_MAC_LEN = 0x10,          /* AES128 keys only */
    CCBVAULTIC_AUTH_ENC_LEN = 0x10,          /* AES128 keys only */

    CCBVAULTIC_AUTH_KDF_KEY_LEN_MIN = 0x10,
    CCBVAULTIC_AUTH_KDF_KEY_LEN_MAX = 0x40,
    CCBVAULTIC_AUTH_KDF_LABEL_LEN_MIN = 0x0,
    CCBVAULTIC_AUTH_KDF_LABEL_LEN_MAX = 0x40,
};

typedef struct {
    int id;         /* between CCBVAULTIC_AUTH_ID_MIN/MAX */
    int role;       /* enum of CCBVAULTIC_AUTH_ROLE_xxx */
    int kind;       /* enum of CCBVAULTIC_AUTH_KIND_xxx */
    union {         /* based on kind */
        struct {
            size_t pin_len;
            char* pin;
        } pin;
        struct {
            size_t mac_len;
            char* mac;
            size_t enc_len;
            char* enc;
        } scp03;
        struct {
            size_t key_len;
            char* key;
            size_t label_len;
            char* label;
        } kdf;
    } auth;
} ccbVaultIc_Auth;

typedef struct {
    int startup_delay_ms;
    int timeout_ms;
    int spi_rate_khz;

    ccbVaultIc_Auth auth;
} ccbVaultIc_Config;

/* Storage for default auth config.  Declared in ccb_vaultic.c */
extern ccbVaultIc_Config gDefaultConfig;

typedef struct {
    int initialized;
    const ccbVaultIc_Config* config;
    int vlt_rc;
    char vlt_serial[CCBVAULTIC_SERIAL_LEN];
    char vlt_version[CCBVAULTIC_VERSION_LEN];

    /* Buffer to store message during SHA with will_copy flag set */
    int hash_type;  /* enum wc_HashType */
    unsigned char *m;
    size_t m_len;

    /* Buffer to cache aes key between invocations */
    unsigned char *aescbc_key;
    size_t aescbc_keylen;
} ccbVaultIc_Context;

/* ccbVaultIc_Context static initializer */
#define CCBVAULTIC_CONTEXT_INITIALIZER   \
    {                                    \
        .initialized = 0,                \
        .config = NULL                   \
    }


enum {
    CCBVAULTIC_FILE_NAME_LEN_MIN = 0x1,     /* At  least a NULL */
    CCBVAULTIC_FILE_NAME_LEN_MAX = 0x10,    /* 16 byte limit for names */
    CCBVAULTIC_FILE_DATA_LEN_MAX = 0x10000, /* 16kB limit for files */
};
typedef struct {
    int name_len;
    char* name;  /* Path, start with / */
    int data_len;
    char* data;
} ccbVaultIc_File;


/* Provision Action
 *  Must authenticate as MANUFACTURER role first. Then invoke provision action:
 *   -sets chip to CREATION state: VltSetStatus(VLT_STATE_CREATION)
 *   -sets power on self test to self_test: VltSetConfig(...)
 *   -deletes all other users: VltManageAuthenticationData(...)
 *   -creates auth user using id, role, and auth kind
 *   -creates and copies file_count files from file[] with auth and current
 *    user id with all privs
 *   -sets chip to ACTIVATED state
 */
enum {
    CCBVAULTIC_PROV_SELFTEST_OPTIONAL = 0,
    CCBVAULTIC_PROV_SELFTEST_REQUIRED = 1,
};

/* Data provided to Provision action */
typedef struct {
    int self_test;
    ccbVaultIc_Auth create;
    int file_count;
    const ccbVaultIc_File *file;
} ccbVaultIc_Provision;

/* Data provided to Load action */
typedef struct {
    int file_count;
    ccbVaultIc_File* file;
} ccbVaultIc_Load;

typedef struct {
    int text_len;
    char* text;
} ccbVaultIc_Info;

typedef struct {
    int index;
    int label_len;
    char* label;
    int data_len;
    char* data;
} ccbVaultIc_Nvm;

/* Get a pointer to the default configuration data, which is used when the
 * context passed into ccbVaultIc_Init has c->config == NULL.  This will happen
 * when the the CryptoCb_Register() is invoked with a NULL context.
 */
int ccbVaultIc_GetDefaultConfig(ccbVaultIc_Config* *out_c);

/* Initialize the VaultIC library and clear the context.
 * Returns: 0 on success
 *          BAD_FUNC_ARGS with NULL context
 *          WC_INIT_E on error initializing the VaultIC.
 *                    c->vlt_rc will have error code
 */
int ccbVaultIc_Init(            ccbVaultIc_Context *c);

/* Close the VaultIC library. */
void ccbVaultIc_Cleanup(        ccbVaultIc_Context *c);

/* Create a file on the VaultIc, accessible by only the user and manufacturer */
int ccbVaultIc_CreateUserFile(  ccbVaultIc_Context *c,
                                const ccbVaultIc_File *f,
                                int userId, int manuId);

/* Delete a file on the VaultIC*/
int ccbVaultIc_DeleteFile(      ccbVaultIc_Context *c,
                                const ccbVaultIc_File *f);

/* Write file data into the vaultic from the file struct */
int ccbVaultIc_WriteFile(       ccbVaultIc_Context *c,
                                const ccbVaultIc_File *f);

/* Read file data from the vaultic into the file struct */
int ccbVaultIc_ReadFile(        ccbVaultIc_Context *c,
                                ccbVaultIc_File *f);

/* Perform the load action as the currently authed user */
int ccbVaultIc_LoadAction(      ccbVaultIc_Context *c,
                                ccbVaultIc_Load *l);

/* Perform the provision action as the currently authed user */
int ccbVaultIc_ProvisionAction( ccbVaultIc_Context *c,
                                const ccbVaultIc_Provision *p);

/* Perform selftest.  Chip will timeout on failure */
int ccbVaultIc_SelfTest( ccbVaultIc_Context *c);

#ifdef WOLF_CRYPTO_CB
#include "wolfssl/wolfcrypt/cryptocb.h"  /* For wc_CryptInfo */

/* Register this callback and associate with a context using:
 *      ccbVaultIc_Context ctx=CCBVAULTIC_CONTEXT_INITIALIZER;
 *      ccbVaultIc_Init(&ctx);
 *      wc_CryptoCb_RegisterDevice(
 *                      CCBVAULTIC420_DEVID,
 *                      ccbVaultIc_CryptoCb,
 *                      &ctx);
 *      wc_Aes aes;
 *      wc_AesInit(&aes, NULL, CCBVAULTIC420_DEVID);
 * Returns: 0 on success
 *          CRYPTOCB_UNAVAILABLE if not initialized or not implemented
 *          MEMORY_E if memory allocation fails
 */
int ccbVaultIc_CryptoCb(int devId,
                        wc_CryptoInfo* info,
                        void* ctx);
#endif  /* WOLF_CRYPTO_CB */



#define CCBVAULTIC_AUTH_ID_DEFAULT 1
#define CCBVAULTIC_SCP03_MAC_DEFAULT \
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
#define CCBVAULTIC_SCP03_ENC_DEFAULT \
    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
#define CCBVAULTIC_AUTH_DEFAULT                                     \
    {                                                               \
        .id          = CCBVAULTIC_AUTH_ID_DEFAULT,                  \
        .role        = CCBVAULTIC_AUTH_ROLE_UNAPPROVED,             \
        .kind        = CCBVAULTIC_AUTH_KIND_SCP03,                  \
        .auth.scp03 = {                                             \
            .mac_len = sizeof(CCBVAULTIC_SCP03_MAC_DEFAULT) - 1,    \
            .mac     = CCBVAULTIC_SCP03_MAC_DEFAULT,                \
            .enc_len = sizeof(CCBVAULTIC_SCP03_ENC_DEFAULT) - 1,    \
            .enc     = CCBVAULTIC_SCP03_ENC_DEFAULT,                \
        }                                                           \
    }

#define CCBVAULTIC_CONFIG_DEFAULT                                   \
    {                                                               \
        .startup_delay_ms = CCBVAULTIC_FAST_START_MS,               \
        .timeout_ms       = CCBVAULTIC_APDU_TIMEOUT_MS,             \
        .spi_rate_khz     = CCBVAULTIC_SPI_RATE_KHZ,                \
        .auth             = CCBVAULTIC_AUTH_DEFAULT,                \
    }

#endif  /* HAVE_CCBVAULTIC */

#endif /* CCB_VAULTIC_H_ */
