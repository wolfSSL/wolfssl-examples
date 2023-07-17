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

/* System includes */
#include <stdlib.h>  /* For NULL */
#include <string.h>  /* For memset/cpy */

/* wolfSSL configuration */
#include "wolfssl/options.h"

/* wolfCrypt includes */
#include "wolfssl/wolfcrypt/cryptocb.h"
#include "wolfssl/wolfcrypt/error-crypt.h"

#include "wolfssl/wolfcrypt/wc_port.h"
#include "wolfssl/wolfcrypt/types.h"

#include "wolfssl/wolfcrypt/hash.h"  /* For HASH_FLAGS and types */
#include "wolfssl/wolfcrypt/rsa.h"   /* For RSA_MAX_SIZE */

/* WiseKey VaultIC includes */
#include "vaultic_tls.h"
#include "vaultic_config.h"
#include "vaultic_common.h"
#include "vaultic_api.h"
#include "vaultic_structs.h"

/* Local include */
#include "ccb_vaultic.h"

/* Defined options:
 * DEBUG_VAULTIC: Print copious callback info using printf
 * NO_WKVIC_SHA: Do not handle SHA256 callback
 * NO_WKVIC_RSA: Do not handle RSA callback
 * NO_WKVIC_AES: Do not handle AES callback
 */


/* Forward declarations */
static int HandlePkCallback(int devId, wc_CryptoInfo* info, ccbVaultIc_Context *c);
static int HandleHashCallback(int devId, wc_CryptoInfo* info, ccbVaultIc_Context *c);
static int HandleCipherCallback(int devId, wc_CryptoInfo* info, ccbVaultIc_Context *c);

static void hexdump(const unsigned char* p, size_t len)
{
    printf("    HD:%p for %lu bytes\n",p, len);
    if(!p || !len) return;
    size_t off=0;
    for (off=0; off < len; off++)
    {
        if(off%16 ==0) printf("    ");
        printf("%02X ", p[off]);
        if(off%16 ==15) printf("\n");
    }
    if(off%16 !=15) printf("\n");
}

int ccbVaultIc_Init(ccbVaultIc_Context *c)
{
    if(!c) {
        return BAD_FUNC_ARG;
    }
    memset(c, 0, sizeof(*c));
    c->vlt_rc=vlt_tls_init();
    if(c->vlt_rc!=0) {
        return WC_INIT_E;
    }
    c->initialized=1;
    return 0;
}

int ccbVaultIc_Cleanup(ccbVaultIc_Context *c)
{
    if(!c) {
        return BAD_FUNC_ARG;
    }

    /* Free allocated buffers */
    if(c->m) free(c->m);
    if(c->aescbc_key) free(c->aescbc_key);

    memset(c, 0, sizeof(*c));
    c->vlt_rc=vlt_tls_close();
    if(c->vlt_rc!=0) {
        return WC_CLEANUP_E;
    }
    return 0;
}

int ccbVaultIc_CryptoDevCb(int devId,
                               wc_CryptoInfo* info,
                               void* ctx)
{
    ccbVaultIc_Context *c=(ccbVaultIc_Context*)ctx;
    int rc = CRYPTOCB_UNAVAILABLE;
    (void)devId;
    if(!info || !c || !c->initialized) {
        /* Invalid info or context */
#if defined(DEBUG_VAULTIC)
        printf("Invalid callback. info:%p c:%p c->init:%d\n",
                info, c, c ? c->initialized : -1);
#endif
        return rc;
    }
    switch(info->algo_type) {
    case WC_ALGO_TYPE_NONE:
#if defined(DEBUG_VAULTIC)
        printf(" CryptoDevCb NONE:\n");
#endif
        /* Nothing to do */
        break;

    case WC_ALGO_TYPE_HASH:
#if defined(DEBUG_VAULTIC)
        printf(" CryptoDevCb HASH: Type:%d\n", info->hash.type);
#endif
#if !defined(NO_SHA) || !defined(NO_SHA256)
        /* Perform a hash */
        rc = HandleHashCallback(devId, info, ctx);
#endif
        break;

    case WC_ALGO_TYPE_CIPHER:
#if defined(DEBUG_VAULTIC)
        printf(" CryptoDevCb CIPHER: Type:%d\n", info->cipher.type);
#endif
#if !defined(NO_AES)
        /* Perform a symmetric cipher */
        rc = HandleCipherCallback(devId, info, ctx);
#endif
        break;

    case WC_ALGO_TYPE_PK:
#if defined(DEBUG_VAULTIC)
        printf(" CryptoDevCb PK: Type:%d\n", info->pk.type);
#endif
#if !defined(NO_RSA) || defined(HAVE_ECC)
        /* Perform a PKI operation */
        rc = HandlePkCallback(devId,info,ctx);
#endif /* !defined(NO_RSA) || defined(HAVE_ECC) */
        break;

    case WC_ALGO_TYPE_RNG:
#if defined(DEBUG_VAULTIC)
        printf(" CryptoDevCb RNG: Out:%p Sz:%d\n", info->rng.out, info->rng.sz);
#endif
#if !defined(WC_NO_RNG)
        /* Put info->rng.sz random bytes into info->rng.out*/
        /* TODO rc = VaultIC_Random(); */
        rc = CRYPTOCB_UNAVAILABLE;
#endif
        break;

    case WC_ALGO_TYPE_SEED:
#if defined(DEBUG_VAULTIC)
        printf(" CryptoDevCb SEED: Seed:%p Sz:%d\n", info->seed.seed,
                info->seed.sz);
#endif
#if !defined(WC_NO_RNG)
        /* Get info->seed.sz seed bytes from info->seed.seed*/
        /* TODO rc = VaultIC_Seed(); */
#endif
        break;

    case WC_ALGO_TYPE_HMAC:
#if defined(DEBUG_VAULTIC)
        printf(" CryptoDevCb HMAC:\n");
#endif
        break;

    case WC_ALGO_TYPE_CMAC:
#if defined(DEBUG_VAULTIC)
        printf(" CryptoDevCb CMAC:\n");
#endif
        break;

    default:
#if defined(DEBUG_VAULTIC)
        printf(" CryptoDevCb UNKNOWN\n");
#endif
        break;
    }
    return rc;
}

static int HandlePkCallback(int devId, wc_CryptoInfo* info, ccbVaultIc_Context *c)
{
    int rc = CRYPTOCB_UNAVAILABLE;
    switch(info->pk.type) {
    case WC_PK_TYPE_NONE:
    #if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback NONE\n");
    #endif
        break;

    case WC_PK_TYPE_RSA:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback RSA: Type:%d\n",info->pk.rsa.type);
#endif
#if !defined(NO_WKVIC_RSA)
        {

            if((info->pk.rsa.type == RSA_PUBLIC_DECRYPT) || /* RSA Verify */
               (info->pk.rsa.type == RSA_PUBLIC_ENCRYPT))   /* RSA Encrypt */
            {

                byte    e[sizeof(uint32_t)] = {0};
                byte    n[RSA_MAX_SIZE / 8] = {0};
                word32  eSz = sizeof(e);
                word32  nSz = sizeof(n);

                rc = wc_RsaFlattenPublicKey(info->pk.rsa.key, e, &eSz, n, &nSz);

                /* VaultIC requires e to be MSB-padded to 4-byte multiples*/
                byte e_pad[sizeof(e)] = {0};
                memcpy(&e_pad[(sizeof(e_pad)-eSz)],e,eSz);

#if defined(DEBUG_VAULTIC)
                printf("   RSA Flatten Pub Key:%d, eSz:%u nSz:%u\n", rc, eSz, nSz);
                hexdump(e,sizeof(e));
                hexdump(e_pad,sizeof(e_pad));

                hexdump(n,sizeof(n));
#endif
                /* Allow all privileges */
                VLT_FILE_PRIVILEGES keyPrivileges = {
                    .u8Read=0xFF,
                    .u8Write=0xFF,
                    .u8Delete=0xFF,
                    .u8Execute=0xFF,
                };

                VLT_KEY_OBJECT tmpRsaKey= {
                    .enKeyID=VLT_KEY_RSAES_PUB,
                    .data.RsaPubKey.u16NLen=nSz,
                    .data.RsaPubKey.pu8N=n,
                    .data.RsaPubKey.u16ELen=sizeof(e_pad),
                    .data.RsaPubKey.pu8E=e_pad,
                    .data.RsaPubKey.enAssurance=VLT_PKV_ASSURED_EXPLICIT_VALIDATION,
                };

                /* Try to delete the tmp rsa key.  Ignore errors here */
                VltDeleteKey(
                        CCBVAULTIC_WOLFSSL_GRPID,
                        CCBVAULTIC_TMPRSA_KEYID);
                int vlt_rc=0;
                vlt_rc=VltPutKey(
                        CCBVAULTIC_WOLFSSL_GRPID,
                        CCBVAULTIC_TMPRSA_KEYID,
                        &keyPrivileges,
                        &tmpRsaKey);
#if defined(DEBUG_VAULTIC)
                printf("   VLT PutKey:%x\n", vlt_rc);
#endif

                /* Initialize Algo for RSA Pub Decrypt */
                VLT_ALGO_PARAMS rsapub_algo_params = {
                        .u8AlgoID=VLT_ALG_CIP_RSAES_X509,
                        /* Raw RSA.  No other data */
                    };

                VLT_U32 out_len=0;
                vlt_rc=VltInitializeAlgorithm(
                        CCBVAULTIC_WOLFSSL_GRPID,
                        CCBVAULTIC_TMPRSA_KEYID,
                        VLT_ENCRYPT_MODE,&rsapub_algo_params);
#if defined(DEBUG_VAULTIC)
                printf("   VLT InitAlgo:%x\n", vlt_rc);
#endif
                vlt_rc=VltEncrypt(info->pk.rsa.inLen, info->pk.rsa.in,
                            &out_len,
                            info->pk.rsa.inLen, info->pk.rsa.out);
                if(info->pk.rsa.outLen) *(info->pk.rsa.outLen)=out_len;
#if defined(DEBUG_VAULTIC)
                printf("   VLT Encrypt:%x inSz:%u outSz:%lu\n", vlt_rc, info->pk.rsa.inLen, out_len);
#endif

                /* Delete the tmp aes key */
                VltDeleteKey(
                        CCBVAULTIC_WOLFSSL_GRPID,
                        CCBVAULTIC_TMPRSA_KEYID);

                /* Update return value to indicate success */
                rc=0;
            } else {
                /* Not supported */
                break;
            }
        };
#endif
        break;

    case WC_PK_TYPE_DH:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback DH\n");
#endif
        break;

    case WC_PK_TYPE_ECDH:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback ECDH\n");
#endif
        break;

    case WC_PK_TYPE_ECDSA_SIGN:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback ECDSA_SIGN\n");
#endif
        break;

    case WC_PK_TYPE_ECDSA_VERIFY:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback ECDSA_VERIFY\n");
#endif
        break;

    case WC_PK_TYPE_ED25519_SIGN:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback ED25519_SIGN\n");
#endif
        break;

    case WC_PK_TYPE_CURVE25519:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback CURVE25519\n");
#endif
        break;

    case WC_PK_TYPE_RSA_KEYGEN:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback RSA_KEYGEN\n");
#endif
        break;

    case WC_PK_TYPE_EC_KEYGEN:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback EC_KEYGEN\n");
#endif
        break;

    case WC_PK_TYPE_RSA_CHECK_PRIV_KEY:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback RSA_CHECK_PRIV_KEY\n");
#endif
        break;

    case WC_PK_TYPE_EC_CHECK_PRIV_KEY:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback EC_CHECK_PRIV_KEY\n");
#endif
        break;

    case WC_PK_TYPE_ED448:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback ED448\n");
#endif
        break;

    case WC_PK_TYPE_CURVE448:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback CRUVE448\n");
#endif
        break;

    case WC_PK_TYPE_ED25519_VERIFY:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback ED25519_VERIFY\n");
#endif
        break;

    case WC_PK_TYPE_ED25519_KEYGEN:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback ED25519_KEYGEN\n");
#endif
        break;

    case WC_PK_TYPE_CURVE25519_KEYGEN:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback CURVE25519_KEYGEN\n");
#endif
        break;

    case WC_PK_TYPE_RSA_GET_SIZE:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback RSA_GET_SIZE\n");
#endif
        break;

    default:
#if defined(DEBUG_VAULTIC)
        printf("  HandlePkCallback UNKNOWN\n");
#endif
        break;
    }
    return rc;
}

static int HandleHashCallback(int devId, wc_CryptoInfo* info, ccbVaultIc_Context *c)
{
    int rc = CRYPTOCB_UNAVAILABLE;
    int finalize=0;
    /* Finalize sha? */
    if((info->hash.in == NULL) && (info->hash.inSz==0)) {
        finalize=1;
    }

    switch(info->hash.type) {
    case WC_HASH_TYPE_NONE:
#if defined(DEBUG_VAULTIC)
        printf("  HandleHashCallback NONE\n");
#endif
        break;
    case WC_HASH_TYPE_SHA:
#if defined(DEBUG_VAULTIC)
        printf("  HandleHashCallback SHA\n");
#endif
        break;
    case WC_HASH_TYPE_SHA224:
#if defined(DEBUG_VAULTIC)
        printf("  HandleHashCallback SHA224\n");
#endif
        break;
    case WC_HASH_TYPE_SHA256:
#if defined(DEBUG_VAULTIC)
        printf("  HandleHashCallback SHA256. In:%p InSz:%u Digest:%p c->m:%p c->m_len:%lu c->t:%d\n",
                info->hash.in, info->hash.inSz, info->hash.digest, c->m, c->m_len, c->hash_type);
#endif
#if !defined(NO_WKVIC_SHA)
        /*
         *  info->hash.flag | WC_HASH_FLAGS_WILL_COPY --> Must buffer entire message
         *  info->hash.in != NULL                     --> Update
         *  info->hash.digest != NULL                 --> Final
         */
        {
            /* III Buffer all messages */
            if(c->hash_type != info->hash.type) {
                /* New/different hash than last time.  Erase state */
#if defined(DEBUG_VAULTIC)
                printf("   New Hash\n");
#endif
                if(c->m) free(c->m);
                c->m=NULL;
                c->m_len=0;
                c->hash_type = info->hash.type;
            }
            /* Update needed? */
            if(info->hash.in && (info->hash.inSz > 0)) {
                /* Buffer data */
                if(c->m) {
                    /* Realloc and add new data in */
                    void *new_buf=realloc(c->m,c->m_len + info->hash.inSz);
                    if(!new_buf) {
                        /* Failure to allocate.  Must return error */
#if defined(DEBUG_VAULTIC)
                        printf("   Failed to realloc. New size:%lu\n", c->m_len+info->hash.inSz);
#endif
                        rc = MEMORY_E;
                        break;
                    }
                    c->m=new_buf;
#if defined(DEBUG_VAULTIC)
                    printf("   Realloc to %p. New size:%lu\n", c->m, c->m_len+info->hash.inSz);
#endif
                    } else {
                    c->m = malloc(info->hash.inSz);
                    if(!c->m) {
                        /* Failure to allocate.  Must return error */
#if defined(DEBUG_VAULTIC)
                        printf("   Failed to alloc. Size:%u\n", info->hash.inSz);
#endif
                        rc = MEMORY_E;
                        break;
                    }
#if defined(DEBUG_VAULTIC)
                    printf("   Alloc to %p. Size:%u\n", c->m, info->hash.inSz);
#endif
                    c->m_len=0;
                }
                memcpy(c->m + c->m_len, info->hash.in, info->hash.inSz);
                c->m_len += info->hash.inSz;
#if defined(DEBUG_VAULTIC)
                printf("   Buffered to %p. Buffer size:%lu\n", c->m, c->m_len);
#endif
                rc = 0;
            }
            /* Finalize needed? */
            if(info->hash.digest) {
                /* Initialize for Hashing */
                VLT_U8 sha_out_len=0;
                VLT_ALGO_PARAMS sha256_algo_params = {
                        .u8AlgoID=VLT_ALG_DIG_SHA256,
                };
                int vlt_rc=0;
                vlt_rc=VltInitializeAlgorithm(0,0, VLT_DIGEST_MODE, &sha256_algo_params);

#if defined(DEBUG_VAULTIC)
                printf("   VltInit SHA256:%x\n", vlt_rc);
                memset(info->hash.digest, 0, WC_SHA256_DIGEST_SIZE);
#endif
                /* No data sent?  Likely test case.  Needs 2 steps */
                if(c->m == NULL)
                {
                    vlt_rc=VltUpdateMessageDigest(c->m_len,
                            c->m);
#if defined(DEBUG_VAULTIC)
                    printf("   VltUpdate SHA256:%x\n", vlt_rc);
#endif
                    vlt_rc=VltComputeMessageDigestFinal(
                            &sha_out_len,
                            WC_SHA256_DIGEST_SIZE,
                            info->hash.digest);
#if defined(DEBUG_VAULTIC)
                    printf("   VltFinal SHA256:%x\n", vlt_rc);
#endif
                }
                vlt_rc=VltComputeMessageDigest(c->m_len,
                        c->m,
                        &sha_out_len,
                        WC_SHA256_DIGEST_SIZE,
                        info->hash.digest);
#if defined(DEBUG_VAULTIC)
                printf("   VltCompute SHA256:%x\n", vlt_rc);
                hexdump(info->hash.digest, WC_SHA256_DIGEST_SIZE);
#endif
                /* Deallocate/clear if this hash was NOT a copy */
#if defined(DEBUG_VAULTIC)
                printf("   Hash flags:%x\n", info->hash.sha256 ? info->hash.sha256->flags : -1);
#endif
                if(     !info->hash.sha256 ||
                        !(info->hash.sha256->flags&WC_HASH_FLAG_ISCOPY)) {
#if defined(DEBUG_VAULTIC)
                    printf("   Freeing hash state\n");
#endif
                    if(c->m) free(c->m);
                    c->m = NULL;
                    c->m_len = 0;
                    c->hash_type = WC_HASH_TYPE_NONE;
                }

                rc=0;
            }
        }
#endif
        break;
    case WC_HASH_TYPE_SHA384:
#if defined(DEBUG_VAULTIC)
        printf("  HandleHashCallback SHA384\n");
#endif
        break;
    case WC_HASH_TYPE_SHA512:
#if defined(DEBUG_VAULTIC)
        printf("  HandleHashCallback SHA512\n");
#endif
        break;
    default:
        break;
    }
    return rc;
}

static int HandleCipherCallback(int devId, wc_CryptoInfo* info, ccbVaultIc_Context *c)
{
    int rc = CRYPTOCB_UNAVAILABLE;
    switch(info->cipher.type) {
    case WC_CIPHER_NONE:
#if defined(DEBUG_VAULTIC)
        printf("  HandleCipherCallback NONE\n");
#endif
        break;

    case WC_CIPHER_AES:
#if defined(DEBUG_VAULTIC)
        printf("  HandleCipherCallback AES\n");
#endif
        break;

    case WC_CIPHER_AES_CBC:
#if defined(DEBUG_VAULTIC)
        printf("  HandleCipherCallback AES_CBC\n");
#endif
#if !defined(NO_WKVIC_AES)
        {
            Aes* aes=info->cipher.aescbc.aes;
            int encrypt=info->cipher.enc;
            VLT_U32 out_len=0;
            if(!aes) break;

            /* Support AES128 for now */
            if(aes->keylen != AES_128_KEY_SIZE) break;

            /* Check number of blocks */
            unsigned int blocks = info->cipher.aescbc.sz / AES_BLOCK_SIZE;
            if(blocks == 0) break;

            /* Check if key is not he same as last time */
            if(     (c->aescbc_key == NULL) ||
                    (c->aescbc_keylen != aes->keylen) ||
                    (!memcmp(c->aescbc_key, aes->devKey,aes->keylen))) {
                if(c->aescbc_key) {
                    free(c->aescbc_key);
                    c->aescbc_key=NULL;
                    c->aescbc_keylen=0;
                }
                /* Allocate key buffer */
                c->aescbc_key=malloc(aes->keylen);
                if(!c->aescbc_key) break;

                c->aescbc_keylen=aes->keylen;
                memcpy(c->aescbc_key,aes->devKey,aes->keylen);

                /* Allow all privileges */
                VLT_FILE_PRIVILEGES keyPrivileges = {
                    .u8Read=0xFF,
                    .u8Write=0xFF,
                    .u8Delete=0xFF,
                    .u8Execute=0xFF,
                };

                VLT_KEY_OBJECT tmpAesKey= {
                    .enKeyID=VLT_KEY_AES_128,
                    .data.SecretKey.u8Mask=0,
                    .data.SecretKey.u16KeyLength=c->aescbc_keylen,
                    .data.SecretKey.pu8Key=(VLT_PU8)(c->aescbc_key),
                };

                /* Try to delete the tmp aes key.  Ignore errors here */
                VltDeleteKey(
                        CCBVAULTIC_WOLFSSL_GRPID,
                        CCBVAULTIC_TMPAES_KEYID);

                /* Putkey aes->devKey, aes->keylen */
                VltPutKey(
                        CCBVAULTIC_WOLFSSL_GRPID,
                        CCBVAULTIC_TMPAES_KEYID,
                        &keyPrivileges,
                        &tmpAesKey);

            }

            /* Initialize Algo for AES-CBC */
            VLT_ALGO_PARAMS aescbc_algo_params = {
                    .u8AlgoID=VLT_ALG_CIP_AES,
                    .data.SymCipher.enMode= BLOCK_MODE_CBC,
                    .data.SymCipher.enPadding= PADDING_NONE,
                    .data.SymCipher.u8IvLength= AES_BLOCK_SIZE,
                    .data.SymCipher.u8Iv={0},
                };
            memcpy(aescbc_algo_params.data.SymCipher.u8Iv,aes->reg,
                    AES_BLOCK_SIZE);

            /* Perform encrypt/decrypt*/
            if(encrypt) {
                VltInitializeAlgorithm(
                        CCBVAULTIC_WOLFSSL_GRPID,
                        CCBVAULTIC_TMPAES_KEYID,
                        VLT_ENCRYPT_MODE,
                        &aescbc_algo_params);
                VltEncrypt(info->cipher.aescbc.sz, info->cipher.aescbc.in,
                            &out_len,
                            info->cipher.aescbc.sz, info->cipher.aescbc.out);
                const byte *last_block = info->cipher.aescbc.out + (blocks -1) * AES_BLOCK_SIZE;
                memcpy(aes->reg, last_block, AES_BLOCK_SIZE);
            } else {
                VltInitializeAlgorithm(
                        CCBVAULTIC_WOLFSSL_GRPID,
                        CCBVAULTIC_TMPAES_KEYID,
                        VLT_DECRYPT_MODE,
                        &aescbc_algo_params);
                VltDecrypt(info->cipher.aescbc.sz, info->cipher.aescbc.in,
                            &out_len,
                            info->cipher.aescbc.sz, info->cipher.aescbc.out);
                const byte *last_block = info->cipher.aescbc.in + (blocks -1) * AES_BLOCK_SIZE;
                memcpy(aes->reg, last_block, AES_BLOCK_SIZE);
            }

#if 0
            /* Delete the tmp aes key */
            VltDeleteKey(
                    CCBVAULTIC_WOLFSSL_GRPID,
                    CCBVAULTIC_TMPAES_KEYID);
#endif
            /* Update return value to indicate success */
            rc=0;
        }
#endif /* NO_WKVIC_AES */
        break;

    case WC_CIPHER_AES_GCM:
#if defined(DEBUG_VAULTIC)
        printf("  HandleCipherCallback AES_GCM\n");
#endif
        break;

    case WC_CIPHER_AES_CTR:
#if defined(DEBUG_VAULTIC)
        printf("  HandleCipherCallback AES_CTR\n");
#endif
        break;

    case WC_CIPHER_AES_XTS:
#if defined(DEBUG_VAULTIC)
        printf("  HandleCipherCallback AES_XTS\n");
#endif
        break;

    case WC_CIPHER_AES_CFB:
#if defined(DEBUG_VAULTIC)
        printf("  HandleCipherCallback AES_CFB\n");
#endif
        break;

    case WC_CIPHER_AES_CCM:
#if defined(DEBUG_VAULTIC)
        printf("  HandleCipherCallback AES_CCM\n");
#endif
        break;

    case WC_CIPHER_AES_ECB:
#if defined(DEBUG_VAULTIC)
        printf("  HandleCipherCallback AES_ECB\n");
#endif
        break;
    default:
#if defined(DEBUG_VAULTIC)
        printf("  HandleCipherCallback UNKNOWN\n");
#endif
        break;
    }
    return rc;
}
