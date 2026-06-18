/* wolfcrypt_test + wolfcrypt_benchmark on the AmebaPro2 (RTL8735B), built inside
 * the RealTek FreeRTOS SDK.
 *
 * Modes 1 (pure C) and 2 (Thumb-2 / SP Cortex-M asm) run the full wolfCrypt
 * self-test then the benchmark over software crypto -- validating the core
 * library and toolchain on this Cortex-M33 and reporting per-algorithm
 * throughput. Mode 3 registers the HUK crypto-callback device over the silicon
 * engine and runs the benchmark only (AES / HMAC-SHA256 / ECDSA P-256 in
 * hardware via WC_USE_DEVID); the self-test is skipped because the HUK device
 * derives its key from the "key" bytes, so the standard known-answer vectors do
 * not apply to it.
 *
 * Build: configure with -DEXAMPLE=wolfcrypt_test -DRTL_BENCH_MODE=N (see
 * wolfcrypt_test.cmake; N defaults to 2).
 */

#include "platform_stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "osdep_service.h"      /* rtw_get_random_bytes */
#include "hal_timer.h"          /* hal_read_systime_us */

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfcrypt/test/test.h>
#include <wolfcrypt/benchmark/benchmark.h>
#if defined(WOLFSSL_RTL8735B_HUK)
    #include <wolfssl/wolfcrypt/port/realtek/rtl8735b.h>
#endif

#define STACKSIZE 16384            /* words (FreeRTOS) -> 64 KB */

typedef struct func_args {
    int    argc;
    char** argv;
    int    return_code;
} func_args;

/* wolfCrypt RNG seed hook (user_settings: CUSTOM_RAND_GENERATE_SEED). Uses the
 * SDK's rtw_get_random_bytes (the high-level entropy API used by the SDK's own
 * TLS examples); it fills from the hardware-seeded PRNG and always links in the
 * non-TrustZone image. */
int amebapro2_rand_seed(unsigned char* output, unsigned int sz)
{
    /* Fail closed: propagate a non-zero entropy-source error so wolfCrypt's DRBG
     * seeding fails rather than using the unfilled buffer. (The SDK's FreeRTOS
     * implementation returns 0 on success.) */
    if (rtw_get_random_bytes(output, sz) != 0) {
        return -1;
    }
    return 0;
}

/* benchmark time source (user_settings: WOLFSSL_USER_CURRTIME). Free-running
 * microsecond system timer, returned as fractional seconds. */
double current_time(int reset)
{
    (void)reset;
    return (double)hal_read_systime_us() / 1000000.0;
}

/* wolfCrypt wall-clock (user_settings: XTIME). The EVB has no RTC set; return a
 * fixed, plausible time so X.509 validity-period checks in wolfcrypt_test pass. */
time_t amebapro2_time(time_t* t)
{
    time_t now = (time_t)1781222400; /* 2026-06-12T00:00:00Z */
    if (t != NULL) {
        *t = now;
    }
    return now;
}

static void wolf_test_thread(void* param)
{
    func_args args;
    (void)param;

    args.argc = 0;
    args.argv = NULL;
    args.return_code = 0;

    if (wolfCrypt_Init() != 0) {
        printf("wolfCrypt_Init failed\r\n");
        vTaskDelete(NULL);
        return;
    }

#if defined(WOLFSSL_RTL8735B_HUK)
    /* Mode 3: register the HUK crypto-callback device at WC_HUK_DEVID. The
     * benchmark's WC_USE_DEVID routes AES / HMAC / ECDSA through it; everything
     * else falls back to software. */
    if (wc_Rtl8735b_HukRegister(WC_HUK_DEVID) != 0) {
        printf("wc_Rtl8735b_HukRegister failed\r\n");
        wolfCrypt_Cleanup();
        vTaskDelete(NULL);
        return;
    }
    printf("\r\n=== wolfCrypt Benchmark (RTL8735B, HUK hardware) ===\r\n");
    benchmark_test(&args);
    printf("benchmark_test: return code %d\r\n", args.return_code);
    wc_Rtl8735b_HukUnRegister(WC_HUK_DEVID);
#else
    printf("\r\n=== wolfCrypt Test (RTL8735B) ===\r\n");
    wolfcrypt_test(&args);
    printf("wolfcrypt_test: return code %d\r\n", args.return_code);

    printf("\r\n=== wolfCrypt Benchmark (RTL8735B) ===\r\n");
    args.return_code = 0;
    benchmark_test(&args);
    printf("benchmark_test: return code %d\r\n", args.return_code);
#endif

    wolfCrypt_Cleanup();
    printf("\r\n=== done ===\r\n");
    vTaskDelete(NULL);
}

int main(void)
{
    if (xTaskCreate(wolf_test_thread, "wolf_test", STACKSIZE, NULL,
                    tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        printf("xTaskCreate failed\r\n");
    }
    else {
        vTaskStartScheduler();   /* only start the scheduler if the task exists */
    }
    while (1) {
    }
}
