/* freebsd system includes */
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/systm.h>

/* wolfssl includes */
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

MALLOC_DEFINE(M_BSD_EXAMPLE, "bsd_example", "example kernel memory");

static int   wc_aes_test(void);
const char * ko_name = "bsdkm_example";

static int
example_loader(struct module * m, int what, void * arg)
{
    int ret = 0;
    switch (what) {
    case MOD_LOAD:
        printf("info: %s: running wc_aes_test()\n", ko_name);
        ret = wc_aes_test();
        if (ret != 0) {
            return ECANCELED;
        }
        break;
    case MOD_UNLOAD:
        printf("info: %s: unload\n", ko_name);
        break;
    default:
        printf("info: %s: not implemented: %d\n", ko_name, what);
        return EOPNOTSUPP;
    }

    return 0;
}

static int wc_aes_test(void)
{
    int ret = 0;
#if defined(WOLFSSL_SMALL_STACK) && !defined(WOLFSSL_NO_MALLOC)
    Aes *aes = NULL;
#else
    Aes aes[1];
#endif

    /* "Now is the time for all " w/o trailing 0 */
    const byte msg[] = {
        0x6e,0x6f,0x77,0x20,0x69,0x73,0x20,0x74,
        0x68,0x65,0x20,0x74,0x69,0x6d,0x65,0x20,
        0x66,0x6f,0x72,0x20,0x61,0x6c,0x6c,0x20
    };
    const byte verify[] =
    {
        0x95,0x94,0x92,0x57,0x5f,0x42,0x81,0x53,
        0x2c,0xcc,0x9d,0x46,0x77,0xa2,0x33,0xcb
    };
    /* padded to 16-bytes */
    const byte key[] = "0123456789abcdef   ";
    /* padded to 16-bytes */
    const byte iv[]  = "1234567890abcdef   ";
    byte cipher[WC_AES_BLOCK_SIZE * 4];

#if defined(WOLFSSL_SMALL_STACK) && !defined(WOLFSSL_NO_MALLOC)
    if ((aes = (Aes *)malloc(sizeof(*aes), M_BSD_EXAMPLE, M_WAITOK | M_ZERO)) == NULL) {
        printf("error: %s: xts aes alloc failed\n", ko_name);
        return MEMORY_E;
    }
#endif

    ret = wc_AesInit(aes, NULL, INVALID_DEVID);
    if (ret) {
        printf("error: %s: wc_AesXtsInit returned: %d\n", ko_name, ret);
        goto wc_aes_test_end;
    }

    ret = wc_AesSetKey(aes, key, WC_AES_BLOCK_SIZE, iv, AES_ENCRYPTION);
    if (ret) {
        printf("error: %s: wc_AesSetKey returned: %d\n", ko_name, ret);
        goto wc_aes_test_end;
    }

    memset(cipher, 0, sizeof(cipher));
    ret = wc_AesCbcEncrypt(aes, cipher, msg, WC_AES_BLOCK_SIZE);
    if (ret) {
        printf("error: %s: wc_AesCbcEncrypt returned: %d\n", ko_name, ret);
        goto wc_aes_test_end;
    }

    if (XMEMCMP(cipher, verify, WC_AES_BLOCK_SIZE)) {
        printf("error: %s: wc_AesCbcDecrypt failed cipher-verify compare\n",
               ko_name);
        ret = -1;
        goto wc_aes_test_end;
    }

    if (ret == 0) {
        printf("info: %s: wc_aes_test good\n", ko_name);
    }

wc_aes_test_end:
    wc_AesFree(aes);
#if defined(WOLFSSL_SMALL_STACK) && !defined(WOLFSSL_NO_MALLOC)
    if (aes) {
        free(aes, M_BSD_EXAMPLE);
        aes = NULL;
    }
#endif

    return ret;
}

static moduledata_t example_mod = {
    "bsdkm_example", /* name */
    example_loader,  /* loader */
    NULL             /* extra data */
};

DECLARE_MODULE(bsdkm_example, example_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
MODULE_DEPEND(bsdkm_example, libwolfssl, 1, 1, 1);
