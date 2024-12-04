/**
  ******************************************************************************
  * @file    stm32mp13xx_disco_conf.h
  * @author  MCD Application Team
  * @brief   configuration file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32MP13XX_DISCO_CONFIG_H
#define __STM32MP13XX_DISCO_CONFIG_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32mp13xx_hal.h"


/* Activation of Critical Section */
#define USE_BSP_CRITICAL_SECTION           0U

/* Activation of PMIC */
#define USE_STPMIC1x                        1U

/* Usage of COM feature */
#define USE_BSP_COM_FEATURE                 0U
/* Activation of COM port for log output */
#define USE_COM_LOG                         0U

#if (USE_BSP_CRITICAL_SECTION == 1)
#include "lock_resource.h"
#define BSP_ENTER_CRITICAL_SECTION       PERIPH_LOCK
#define BSP_EXIT_CRITICAL_SECTION        PERIPH_UNLOCK
#else
#define BSP_ENTER_CRITICAL_SECTION(periph)       ((void)0)
#define BSP_EXIT_CRITICAL_SECTION(periph)        ((void)0)
#endif

/* IRQ priorities */
#define BSP_PMU1_IT_PRIORITY                   0x03UL
#define BSP_BUTTON_WAKEUP_IT_PRIORITY          0x0FUL
#define BSP_BUTTON_USER_IT_PRIORITY            0x0FUL
#define BSP_BUTTON_USER2_IT_PRIORITY           0x0FUL
#define BSP_BUTTON_TAMPER_IT_PRIORITY          0x0FUL

#endif /* __STM32MP13XX_DISCO_CONFIG_H */

