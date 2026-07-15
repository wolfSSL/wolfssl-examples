#include <stdint.h>
#include <time.h>
#include "user_settings.h"
#include "wolfssl/wolfcrypt/settings.h"
#include "fsl_trng.h"

volatile uint32_t g_systickCounter;
struct timezone;

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    (void)tz;
    tv->tv_sec = g_systickCounter / 1000;
    tv->tv_usec = (g_systickCounter % 1000) * 1000;
    return 0;
}

/*******************************************************************************
 * Code
 ******************************************************************************/
void SysTick_Handler(void)
{
    g_systickCounter++;
}

/* benchmark.c's generic branch calls clock_gettime, which this board has no
 * implementation for; WOLFSSL_USER_CURRTIME routes it here instead. */
double current_time(int reset)
{
    (void)reset;
    return (double)g_systickCounter / 1000.0;
}

int32_t cust_rand_generate_block(uint8_t *rndb, uint32_t sz)
{
    status_t status;
    status = TRNG_GetRandomData(TRNG, rndb,sz);
    if (status != kStatus_Success) {
        PRINTF("ERROR: TRNG STATUS: %d\r\n");
        return -1;
    }
    return 0;
}

