#define WOLFSSL_TRACK_MEMORY
#define HAVE_STACK_SIZE

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/random.h>

#include <wolfssl/ssl.h>
#include <wolfssl/test.h>

static RNG rng;
static ecc_key genKey;

static void* do_it(void* args)
{
    int ret;

    InitMemoryTracker();

    ret = wc_ecc_make_key(&rng, 32, &genKey);
    if (ret < 0) {
        printf("ecc make key failed\n");
    }

    ShowMemoryTracker();

    (void)args;

    return 0;
}


int main()
{
    int ret = wc_InitRng(&rng);
    if (ret < 0) {
        printf("Init RNG failed\n");
    }

    StackSizeCheck(NULL, do_it);
    printf("sizeof RNG = %lu\n", sizeof(RNG));
    printf("sizeof ecc_key = %lu\n", sizeof(ecc_key));

    wc_FreeRng(&rng);

    return 0;
}
