/* wolfCrypt benchmark test application for i.MX RT1060-EVK
 *
 * Copyright (C) 2020 wolfSSL Inc.
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
#include "user_settings.h"
#include "wolfssl/wolfcrypt/settings.h"
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_trng.h"

#include "board.h"
#include <stdio.h>

#include "pin_mux.h"
#include "clock_config.h"
#include "wolfcrypt/benchmark/benchmark.h"
#include "wolfcrypt/test/test.h"
#include "wolfssl/wolfcrypt/wc_port.h"

#include <time.h>

void main(void)
{
    char ch;
    trng_config_t trngConfig;
    /* Init board hardware. */
    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_InitBootClocks();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000U);
    BOARD_InitDebugConsole();
    TRNG_GetDefaultConfig(&trngConfig);
    trngConfig.sampleMode = kTRNG_SampleModeVonNeumann;
    wolfCrypt_Init();
    benchmark_init();
    benchmark_test(NULL);

   while (1)
    {
        ch = GETCHAR();
        PUTCHAR(ch);
    }

}
