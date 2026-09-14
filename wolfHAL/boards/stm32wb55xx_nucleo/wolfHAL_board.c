/* wolfHAL_board.c
 *
 * Board bring-up for the STM32WB55 Nucleo — wolfCrypt + wolfHAL example.
 *
 * Board_Init() must run before wolfCrypt_Init(): wolfCrypt_Init() registers the
 * wolfHAL crypto callback, and the callback expects the peripherals to be live.
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include "wolfHAL_board.h"

volatile uint32_t g_tick = 0;

void SysTick_Handler(void)
{
    g_tick++;
}

uint32_t Board_GetTick(void)
{
    return g_tick;
}

/* Bounds the AES and RNG status-flag polls: a wedged peripheral surfaces as
 * WC_TIMEOUT_E instead of hanging the test run. */
whal_Timeout g_whalTimeout = {
    .timeoutTicks = 1000, /* 1 s at the 1 ms SysTick */
    .GetTick = Board_GetTick,
};

whal_Uart g_whalUart = {
    .base = WHAL_STM32WB55_UART1_BASE,
    /* .driver: direct API mapping */

    .cfg = &(whal_Stm32wb_Uart_Cfg) {
        .brr     = WHAL_STM32WB_UART_BRR(64000000, 115200),
        .timeout = &g_whalTimeout,
    },
};

static const whal_Stm32wb_Rcc_PeriphClk g_periphClks[] = {
    {WHAL_STM32WB55_GPIOB_GATE},
    {WHAL_STM32WB55_UART1_GATE},
    {WHAL_STM32WB55_RNG_GATE},
    {WHAL_STM32WB55_AES1_GATE},
};
#define PERIPH_CLK_COUNT (sizeof(g_periphClks) / sizeof(g_periphClks[0]))

whal_Error Board_Init(void)
{
    whal_Error err;
    size_t i;

    /* Wait states first — the core would outrun flash once SYSCLK passes
     * ~16 MHz, and the fetch faults before anything else can report it. */
    err = whal_Stm32wb_Flash_Ext_SetLatency(BOARD_FLASH_DEV,
                                            WHAL_STM32WB_FLASH_LATENCY_3);
    if (err)
        return err;

    /* MSI 4 MHz -> VCO 128 MHz -> PLLR /2 = 64 MHz SYSCLK. */
    err = whal_Stm32wb_Rcc_EnableMsi(WHAL_STM32WB_RCC_MSIRANGE_4MHz);
    if (err)
        return err;

    err = whal_Stm32wb_Rcc_EnablePll(&(whal_Stm32wb_Rcc_PllCfg){
        .clkSrc = WHAL_STM32WB_RCC_PLLCLK_SRC_MSI,
        .n = 32, .m = 0, .r = 1, .q = 0, .p = 0,
    });
    if (err)
        return err;

    /* The RNG is clocked from HSI48, not SYSCLK. */
    err = whal_Stm32wb_Rcc_EnableOsc(
        &(whal_Stm32wb_Rcc_OscCfg){WHAL_STM32WB_RCC_HSI48_CFG});
    if (err)
        return err;

    err = whal_Stm32wb_Rcc_SetSysClock(WHAL_STM32WB_RCC_SYSCLK_SRC_PLL);
    if (err)
        return err;

    for (i = 0; i < PERIPH_CLK_COUNT; i++) {
        err = whal_Stm32wb_Rcc_EnablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    err = whal_Gpio_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Uart_Init(&g_whalUart);
    if (err)
        return err;

    /* Before the peripherals that poll against it. */
    err = whal_Timer_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Timer_Start(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Rng_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Crypto_Init(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    return WHAL_SUCCESS;
}

whal_Error Board_Deinit(void)
{
    whal_Error err;
    size_t i;

    err = whal_Crypto_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Rng_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Timer_Stop(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Timer_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    err = whal_Uart_Deinit(&g_whalUart);
    if (err)
        return err;

    err = whal_Gpio_Deinit(WHAL_INTERNAL_DEV);
    if (err)
        return err;

    /* Unwind the clock tree in reverse: gates, then SYSCLK back to MSI, then
     * the sources. MSI stays on as the fallback. */
    for (i = PERIPH_CLK_COUNT; i-- > 0; ) {
        err = whal_Stm32wb_Rcc_DisablePeriphClk(&g_periphClks[i]);
        if (err)
            return err;
    }

    err = whal_Stm32wb_Rcc_SetSysClock(WHAL_STM32WB_RCC_SYSCLK_SRC_MSI);
    if (err)
        return err;

    err = whal_Stm32wb_Rcc_DisablePll();
    if (err)
        return err;

    err = whal_Stm32wb_Rcc_DisableOsc(
        &(whal_Stm32wb_Rcc_OscCfg){WHAL_STM32WB_RCC_HSI48_CFG});
    if (err)
        return err;

    /* Latency back down only after the clock is slow again. */
    err = whal_Stm32wb_Flash_Ext_SetLatency(BOARD_FLASH_DEV,
                                            WHAL_STM32WB_FLASH_LATENCY_0);
    if (err)
        return err;

    return WHAL_SUCCESS;
}
