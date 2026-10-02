/* wolfHAL_board.h
 *
 * Board configuration for the STM32WB55 Nucleo — wolfCrypt + wolfHAL example.
 *
 * Scoped to what the wolfCrypt test needs: a 64 MHz clock, UART1 for output,
 * SysTick for timeouts, and the AES1 and RNG peripherals. wolfHAL's own
 * boards/stm32wb55xx_nucleo brings up SPI, I2C, PKA and more.
 *
 * Two sets of macros matter here:
 *
 *   WHAL_CFG_*_DEV   wolfHAL's driver TUs #include "wolfHAL_board.h" and
 *                    expand these to define their device singletons.
 *   WC_WOLFHAL_*_DEV wolfSSL's wolfhal.c reads these to pick the device it
 *                    dispatches each algorithm on. A mode left out here falls
 *                    back to wolfCrypt's software implementation.
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

#ifndef WOLFHAL_BOARD_H
#define WOLFHAL_BOARD_H

#include <stdint.h>
#include <stddef.h>

#include <wolfHAL/wolfHAL.h>
#include <wolfHAL/platform/st/stm32wb55xx.h>
#include <wolfHAL/timer/systick.h>

extern whal_Uart    g_whalUart;
extern whal_Timeout g_whalTimeout;
extern volatile uint32_t g_tick;

enum {
    LED_PIN,
    UART_TX_PIN,
    UART_RX_PIN,
    PIN_COUNT,
};

#define BOARD_FLASH_DEV ((whal_Flash *)&whal_Stm32wb_Flash_Dev)

/* GPIO — LED on PB5, UART1 TX/RX on PB6/PB7. */
#define WHAL_CFG_STM32WB_GPIO_DEV { \
    .base = WHAL_STM32WB55_GPIO_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_Stm32wb_Gpio_Cfg){ \
        .pinCfg = (const whal_Stm32wb_Gpio_PinCfg[PIN_COUNT]){ \
            [LED_PIN] = WHAL_STM32WB_GPIO_PIN( \
                WHAL_STM32WB_GPIO_PORT_B, 5, WHAL_STM32WB_GPIO_MODE_OUT, \
                WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB_GPIO_SPEED_LOW, \
                WHAL_STM32WB_GPIO_PULL_UP, 0), \
            [UART_TX_PIN] = WHAL_STM32WB_GPIO_PIN( \
                WHAL_STM32WB_GPIO_PORT_B, 6, WHAL_STM32WB_GPIO_MODE_ALTFN, \
                WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB_GPIO_SPEED_FAST, \
                WHAL_STM32WB_GPIO_PULL_UP, 7), \
            [UART_RX_PIN] = WHAL_STM32WB_GPIO_PIN( \
                WHAL_STM32WB_GPIO_PORT_B, 7, WHAL_STM32WB_GPIO_MODE_ALTFN, \
                WHAL_STM32WB_GPIO_OUTTYPE_PUSHPULL, WHAL_STM32WB_GPIO_SPEED_FAST, \
                WHAL_STM32WB_GPIO_PULL_UP, 7), \
        }, \
        .pinCount = PIN_COUNT, \
    }, \
}

/* Flash — only needed to raise the wait states before the clock goes to 64 MHz. */
#define WHAL_CFG_STM32WB_FLASH_DEV { \
    .driver = WHAL_STM32WB55_FLASH_DRIVER, \
    .base   = WHAL_STM32WB55_FLASH_BASE, \
    .cfg    = (void *)&(const whal_Stm32wb_Flash_Cfg){ \
        .timeout   = &g_whalTimeout, \
        .startAddr = 0x08000000, \
        .size      = 0x80000, /* 512 KB; upper half reserved for the BLE stack */ \
    }, \
}

/* SysTick at 1 ms — drives the timeout the AES and RNG polls are bounded by. */
#define WHAL_CFG_SYSTICK_DEV { \
    .base = WHAL_CORTEX_M4_SYSTICK_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_SysTick_Cfg){ \
        .cyclesPerTick = 64000000 / 1000, \
        .clkSrc  = WHAL_SYSTICK_CLKSRC_SYSCLK, \
        .tickInt = WHAL_SYSTICK_TICKINT_ENABLED, \
    }, \
}

/* RNG. */
#define WHAL_CFG_STM32WB_RNG_DEV { \
    .base = WHAL_STM32WB55_RNG_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_Stm32wb_Rng_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

/* AES1. stm32wb_aes.c defines all seven devices unconditionally, so every
 * initializer has to be present even though only four are dispatched below.
 * The GCM and CCM state buffers are static in that TU. */
#define WHAL_CFG_STM32WB_AES_DEV { \
    .base = WHAL_STM32WB55_AES1_BASE, \
    /* .driver: direct API mapping */ \
    .cfg  = (void *)&(const whal_Stm32wb_Aes_Cfg){ \
        .timeout = &g_whalTimeout, \
    }, \
}

#define WHAL_CFG_STM32WB_AES_ECB_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wb_Aes_Dev, \
}

#define WHAL_CFG_STM32WB_AES_CBC_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wb_Aes_Dev, \
}

#define WHAL_CFG_STM32WB_AES_CTR_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wb_Aes_Dev, \
}

#define WHAL_CFG_STM32WB_AES_GCM_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wb_Aes_Dev, \
    .state  = &g_stm32wbAesGcmDevState, \
}

#define WHAL_CFG_STM32WB_AES_GMAC_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wb_Aes_Dev, \
}

#define WHAL_CFG_STM32WB_AES_CCM_DEV { \
    .crypto = (whal_Crypto *)&whal_Stm32wb_Aes_Dev, \
    .state  = &g_stm32wbAesCcmDevState, \
}

/* What wolfSSL's wolfhal.c dispatches each algorithm on. CTR and GMAC are
 * deliberately absent — wolfCrypt has no crypto-callback path for them, and
 * anything not named here runs in software. */
#define WC_WOLFHAL_AES_ECB_DEV ((whal_AesEcb *)&whal_Stm32wb_AesEcb_Dev)
#define WC_WOLFHAL_AES_CBC_DEV ((whal_AesCbc *)&whal_Stm32wb_AesCbc_Dev)
#define WC_WOLFHAL_AES_GCM_DEV ((whal_AesGcm *)&whal_Stm32wb_AesGcm_Dev)
#define WC_WOLFHAL_AES_CCM_DEV ((whal_AesCcm *)&whal_Stm32wb_AesCcm_Dev)
#define WC_WOLFHAL_RNG_DEV     ((whal_Rng *)&whal_Stm32wb_Rng_Dev)

whal_Error Board_Init(void);
whal_Error Board_Deinit(void);
uint32_t   Board_GetTick(void);

#endif /* WOLFHAL_BOARD_H */
