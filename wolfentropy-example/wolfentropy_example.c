/* wolfentropy_example.c
 *
 * Basic wolfEntropy example. Build wolfSSL with --enable-wolfEntropy first.
 * Shows the version, an on demand health test, direct entropy, and the RNG.
 */

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/wolfentropy.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include <stdio.h>

#ifndef HAVE_ENTROPY_MEMUSE
    #error "wolfSSL was not built with --enable-wolfEntropy"
#endif

#define ENTROPY_BYTES   32
#define RANDOM_BYTES    32

/* Prints a buffer as hex with a label.
 */
static void print_hex(const char* label, const byte* buf, word32 sz)
{
    word32 i;

    printf("%-28s", label);
    for (i = 0; i < sz; i++) {
        printf("%02x", buf[i]);
    }
    printf("\n");
}

/* Reports a failed call with the wolfSSL error string.
 */
static void print_error(const char* call, int ret)
{
    printf("%s failed: %d (%s)\n", call, ret, wc_GetErrorString(ret));
}

int main(void)
{
    int ret;
    int cleanupRet;
    int rngReady = 0;
    WC_RNG rng;
    byte entropy[ENTROPY_BYTES];
    byte random[RANDOM_BYTES];

    /* Required first: sets up wolfEntropy and runs its startup health test. */
    ret = wolfCrypt_Init();
    if (ret != 0) {
        print_error("wolfCrypt_Init", ret);
        return 1;
    }

    printf("Entropy source:             %s\n", wc_Entropy_GetVersion());

    /* Re-run the SP 800-90B section 4.4 health tests on request. */
    ret = wc_Entropy_OnDemandTest();
    if (ret != 0) {
        print_error("wc_Entropy_OnDemandTest", ret);
    }
    else {
        printf("On demand health test:      passed\n");
    }

    /* Full strength conditioned entropy, straight from the source. */
    if (ret == 0) {
        ret = wc_Entropy_Get(MAX_ENTROPY_BITS, entropy, sizeof(entropy));
        if (ret != 0) {
            print_error("wc_Entropy_Get", ret);
        }
        else {
            print_hex("wc_Entropy_Get:", entropy, sizeof(entropy));
        }
    }

    /* Normal use: the DRBG seeds itself from wolfEntropy automatically. */
    if (ret == 0) {
        ret = wc_InitRng(&rng);
        if (ret != 0) {
            print_error("wc_InitRng", ret);
        }
        else {
            rngReady = 1;
        }
    }
    if (ret == 0) {
        ret = wc_RNG_GenerateBlock(&rng, random, sizeof(random));
        if (ret != 0) {
            print_error("wc_RNG_GenerateBlock", ret);
        }
        else {
            print_hex("wc_RNG_GenerateBlock:", random, sizeof(random));
        }
    }

    if (rngReady) {
        cleanupRet = wc_FreeRng(&rng);
        if (cleanupRet != 0) {
            print_error("wc_FreeRng", cleanupRet);
        }
        if (ret == 0) {
            ret = cleanupRet;
        }
    }

    cleanupRet = wolfCrypt_Cleanup();
    if (cleanupRet != 0) {
        print_error("wolfCrypt_Cleanup", cleanupRet);
    }
    if (ret == 0) {
        ret = cleanupRet;
    }

    return (ret == 0) ? 0 : 1;
}
