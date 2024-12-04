/**
  ******************************************************************************
  * @file    startup_stm32mp135fxx_ca7.c
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-A7 Device Peripheral Access Layer A7 Startup source file
  *          for GCC toolchain
  *   This file :
  *      - Sets up initial PC => Reset Handler
  *      - Puts all A7 cores except first one on "Wait for interrupt"
  *      - Configures cores
  *      - Sets up Exception vectors
  *      - Sets up stacks for all exception modes
  *      - Call SystemInit to initialize IRQs, caches, MMU, ...
  *
  ******************************************************************************
  * Copyright (c) 2009-2018 ARM Limited. All rights reserved.
  *
  * SPDX-License-Identifier: Apache-2.0
  *
  * Licensed under the Apache License, Version 2.0 (the License); you may
  * not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an AS IS BASIS, WITHOUT
  * See the License for the specific language governing permissions and
  * limitations under the License.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023-2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include "stm32mp13xx.h"
#include "irq_ctrl.h"
/*----------------------------------------------------------------------------
  Definitions
 *----------------------------------------------------------------------------*/
#define USR_MODE 0x10            /* User mode */
#define FIQ_MODE 0x11            /* Fast Interrupt Request mode */
#define IRQ_MODE 0x12            /* Interrupt Request mode */
#define SVC_MODE 0x13            /* Supervisor mode */
#define ABT_MODE 0x17            /* Abort mode */
#define UND_MODE 0x1B            /* Undefined Instruction mode */
#define SYS_MODE 0x1F            /* System mode */

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
void Vectors       (void) __attribute__ ((naked, section("RESET")));
void Reset_Handler (void) __attribute__ ((naked, target("arm")));

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
void Undef_Handler (void) __attribute__ ((weak, alias("Default_Handler")));
void PAbt_Handler  (void) __attribute__ ((weak, alias("Default_Handler")));
void DAbt_Handler  (void) __attribute__ ((weak, alias("Default_Handler")));
void Rsvd_Handler  (void) __attribute__ ((weak, alias("Default_Handler")));
void FIQ_Handler   (void) __attribute__ ((weak, alias("Default_Handler")));

/*----------------------------------------------------------------------------
  SoC External Interrupt Handler
 *----------------------------------------------------------------------------*/
void __attribute__ ((weak)) RESERVED_IRQHandler(void) {}
void __attribute__ ((weak)) SGI0_IRQHandler(void) {}
void __attribute__ ((weak)) SGI1_IRQHandler(void) {}
void __attribute__ ((weak)) SGI2_IRQHandler(void) {}
void __attribute__ ((weak)) SGI3_IRQHandler(void) {}
void __attribute__ ((weak)) SGI4_IRQHandler(void) {}
void __attribute__ ((weak)) SGI5_IRQHandler(void) {}
void __attribute__ ((weak)) SGI6_IRQHandler(void) {}
void __attribute__ ((weak)) SGI7_IRQHandler(void) {}
void __attribute__ ((weak)) SGI8_IRQHandler(void) {}
void __attribute__ ((weak)) SGI9_IRQHandler(void) {}
void __attribute__ ((weak)) SGI10_IRQHandler(void) {}
void __attribute__ ((weak)) SGI11_IRQHandler(void) {}
void __attribute__ ((weak)) SGI12_IRQHandler(void) {}
void __attribute__ ((weak)) SGI13_IRQHandler(void) {}
void __attribute__ ((weak)) SGI14_IRQHandler(void) {}
void __attribute__ ((weak)) SGI15_IRQHandler(void) {}

void __attribute__ ((weak)) VirtualMaintenanceInterrupt_IRQHandler(void) {}
void __attribute__ ((weak)) HypervisorTimer_IRQHandler(void) {}
void __attribute__ ((weak)) VirtualTimer_IRQHandler(void) {}
void __attribute__ ((weak)) Legacy_nFIQ_IRQHandler(void) {}
void __attribute__ ((weak)) SecurePhysicalTimer_IRQHandler(void) {}
void __attribute__ ((weak)) NonSecurePhysicalTimer_IRQHandler(void) {}
void __attribute__ ((weak)) Legacy_nIRQ_IRQHandler(void) {}

void __attribute__ ((weak)) PVD_AVD_IRQHandler(void) {}
void __attribute__ ((weak)) TAMP_IRQHandler(void) {}
void __attribute__ ((weak)) RTC_WKUP_ALARM_IRQHandler(void) {}
void __attribute__ ((weak)) TZC_IT_IRQHandler(void) {}
void __attribute__ ((weak)) RCC_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI0_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI1_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI2_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI3_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI4_IRQHandler(void) {}
void __attribute__ ((weak)) DMA1_Stream0_IRQHandler(void) {}
void __attribute__ ((weak)) DMA1_Stream1_IRQHandler(void) {}
void __attribute__ ((weak)) DMA1_Stream2_IRQHandler(void) {}
void __attribute__ ((weak)) DMA1_Stream3_IRQHandler(void) {}
void __attribute__ ((weak)) DMA1_Stream4_IRQHandler(void) {}
void __attribute__ ((weak)) DMA1_Stream5_IRQHandler(void) {}
void __attribute__ ((weak)) DMA1_Stream6_IRQHandler(void) {}
void __attribute__ ((weak)) ADC1_IRQHandler(void) {}
void __attribute__ ((weak)) ADC2_IRQHandler(void) {}
void __attribute__ ((weak)) FDCAN1_IT0_IRQHandler(void) {}
void __attribute__ ((weak)) FDCAN2_IT0_IRQHandler(void) {}
void __attribute__ ((weak)) FDCAN1_IT1_IRQHandler(void) {}
void __attribute__ ((weak)) FDCAN2_IT1_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI5_IRQHandler(void) {}
void __attribute__ ((weak)) TIM1_BRK_IRQHandler(void) {}
void __attribute__ ((weak)) TIM1_UP_IRQHandler(void) {}
void __attribute__ ((weak)) TIM1_TRG_COM_IRQHandler(void) {}
void __attribute__ ((weak)) TIM1_CC_IRQHandler(void) {}
void __attribute__ ((weak)) TIM2_IRQHandler(void) {}
void __attribute__ ((weak)) TIM3_IRQHandler(void) {}
void __attribute__ ((weak)) TIM4_IRQHandler(void) {}
void __attribute__ ((weak)) I2C1_EV_IRQHandler(void) {}
void __attribute__ ((weak)) I2C1_ER_IRQHandler(void) {}
void __attribute__ ((weak)) I2C2_EV_IRQHandler(void) {}
void __attribute__ ((weak)) I2C2_ER_IRQHandler(void) {}
void __attribute__ ((weak)) SPI1_IRQHandler(void) {}
void __attribute__ ((weak)) SPI2_IRQHandler(void) {}
void __attribute__ ((weak)) USART1_IRQHandler(void) {}
void __attribute__ ((weak)) USART2_IRQHandler(void) {}
void __attribute__ ((weak)) USART3_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI10_IRQHandler(void) {}
void __attribute__ ((weak)) RTC_TIMESTAMP_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI11_IRQHandler(void) {}
void __attribute__ ((weak)) TIM8_BRK_IRQHandler(void) {}
void __attribute__ ((weak)) TIM8_UP_IRQHandler(void) {}
void __attribute__ ((weak)) TIM8_TRG_COM_IRQHandler(void) {}
void __attribute__ ((weak)) TIM8_CC_IRQHandler(void) {}
void __attribute__ ((weak)) DMA1_Stream7_IRQHandler(void) {}
void __attribute__ ((weak)) FMC_IRQHandler(void) {}
void __attribute__ ((weak)) SDMMC1_IRQHandler(void) {}
void __attribute__ ((weak)) TIM5_IRQHandler(void) {}
void __attribute__ ((weak)) SPI3_IRQHandler(void) {}
void __attribute__ ((weak)) UART4_IRQHandler(void) {}
void __attribute__ ((weak)) UART5_IRQHandler(void) {}
void __attribute__ ((weak)) TIM6_IRQHandler(void) {}
void __attribute__ ((weak)) TIM7_IRQHandler(void) {}
void __attribute__ ((weak)) DMA2_Stream0_IRQHandler(void) {}
void __attribute__ ((weak)) DMA2_Stream1_IRQHandler(void) {}
void __attribute__ ((weak)) DMA2_Stream2_IRQHandler(void) {}
void __attribute__ ((weak)) DMA2_Stream3_IRQHandler(void) {}
void __attribute__ ((weak)) DMA2_Stream4_IRQHandler(void) {}
void __attribute__ ((weak)) ETH1_IRQHandler(void) {}
void __attribute__ ((weak)) ETH1_WKUP_IRQHandler(void) {}
void __attribute__ ((weak)) FDCAN_CAL_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI6_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI7_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI8_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI9_IRQHandler(void) {}
void __attribute__ ((weak)) DMA2_Stream5_IRQHandler(void) {}
void __attribute__ ((weak)) DMA2_Stream6_IRQHandler(void) {}
void __attribute__ ((weak)) DMA2_Stream7_IRQHandler(void) {}
void __attribute__ ((weak)) USART6_IRQHandler(void) {}
void __attribute__ ((weak)) I2C3_EV_IRQHandler(void) {}
void __attribute__ ((weak)) I2C3_ER_IRQHandler(void) {}
void __attribute__ ((weak)) USBH_PORT1_IRQHandler(void) {}
void __attribute__ ((weak)) USBH_PORT2_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI12_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI13_IRQHandler(void) {}
void __attribute__ ((weak)) DCMIPP_IRQHandler(void) {}
void __attribute__ ((weak)) CRYP1_IRQHandler(void) {}
void __attribute__ ((weak)) HASH1_IRQHandler(void) {}
void __attribute__ ((weak)) SAES_IRQHandler(void) {}
void __attribute__ ((weak)) UART7_IRQHandler(void) {}
void __attribute__ ((weak)) UART8_IRQHandler(void) {}
void __attribute__ ((weak)) SPI4_IRQHandler(void) {}
void __attribute__ ((weak)) SPI5_IRQHandler(void) {}
void __attribute__ ((weak)) SAI1_IRQHandler(void) {}
void __attribute__ ((weak)) LTDC_IRQHandler(void) {}
void __attribute__ ((weak)) LTDC_ER_IRQHandler(void) {}
void __attribute__ ((weak)) SAI2_IRQHandler (void) {}
void __attribute__ ((weak)) QUADSPI_IRQHandler(void) {}
void __attribute__ ((weak)) LPTIM1_IRQHandler(void) {}
void __attribute__ ((weak)) I2C4_EV_IRQHandler(void) {}
void __attribute__ ((weak)) I2C4_ER_IRQHandler(void) {}
void __attribute__ ((weak)) SPDIF_RX_IRQHandler (void) {}
void __attribute__ ((weak)) OTG_IRQHandler(void) {}
void __attribute__ ((weak)) ETH2_IRQHandler(void) {}
void __attribute__ ((weak)) ETH2_WKUP_IRQHandler(void) {}
void __attribute__ ((weak)) DMAMUX1_OVR_IRQHandler(void) {}
void __attribute__ ((weak)) DMAMUX2_OVR_IRQHandler(void) {}
void __attribute__ ((weak)) TIM15_IRQHandler(void) {}
void __attribute__ ((weak)) TIM16_IRQHandler(void) {}
void __attribute__ ((weak)) TIM17_IRQHandler(void) {}
void __attribute__ ((weak)) TIM12_IRQHandler(void) {}
void __attribute__ ((weak)) PKA_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI14_IRQHandler(void) {}
void __attribute__ ((weak)) MDMA_IRQHandler(void) {}
void __attribute__ ((weak)) SDMMC2_IRQHandler(void) {}
void __attribute__ ((weak)) EXTI15_IRQHandler(void) {}
void __attribute__ ((weak)) MDMA_SEC_IT_IRQHandler(void) {}
void __attribute__ ((weak)) TIM13_IRQHandler(void) {}
void __attribute__ ((weak)) TIM14_IRQHandler(void) {}
void __attribute__ ((weak)) RNG1_IRQHandler(void) {}
void __attribute__ ((weak)) I2C5_EV_IRQHandler(void) {}
void __attribute__ ((weak)) I2C5_ER_IRQHandler(void) {}
void __attribute__ ((weak)) LPTIM2_IRQHandler(void) {}
void __attribute__ ((weak)) LPTIM3_IRQHandler(void) {}
void __attribute__ ((weak)) LPTIM4_IRQHandler(void) {}
void __attribute__ ((weak)) LPTIM5_IRQHandler(void) {}
void __attribute__ ((weak)) ETH1_LPI_IRQHandler(void) {}
void __attribute__ ((weak)) ETH2_LPI_IRQHandler(void) {}
void __attribute__ ((weak)) RCC_WAKEUP__IRQHandler(void) {}
void __attribute__ ((weak)) DTS_IRQHandler(void) {}
void __attribute__ ((weak)) MPU_WAKEUP_PIN_IRQHandler(void) {}
void __attribute__ ((weak)) IWDG1_IRQHandler(void) {}
void __attribute__ ((weak)) IWDG2_IRQHandler(void) {}
void __attribute__ ((weak)) TAMP_SIRQHandler(void) {}
void __attribute__ ((weak)) RTC_WKUP_ALARM_S_IRQHandler(void) {}
void __attribute__ ((weak)) RTC_TS_S_IRQHandler(void) {}
void __attribute__ ((weak)) LTDC_SEC_IRQHandler(void) {}
void __attribute__ ((weak)) LTDC_SEC_ER_IRQHandler(void) {}
void __attribute__ ((weak)) PMUIRQ0_IRQHandler(void) {}
void __attribute__ ((weak)) COMMRX0_IRQHandler(void) {}
void __attribute__ ((weak)) COMMTX0_IRQHandler(void) {}
void __attribute__ ((weak)) AXIERRIRQ_IRQHandler(void) {}
void __attribute__ ((weak)) DDRPERFM_IRQHandler(void) {}
void __attribute__ ((weak)) nCTIIRQ0_IRQHandler(void) {}
void __attribute__ ((weak)) MCE_IRQHandler(void) {}
void __attribute__ ((weak)) DFSDM1_IRQHandler(void) {}
void __attribute__ ((weak)) DFSDM2_IRQHandler(void) {}
void __attribute__ ((weak)) DMA3_Stream0_IRQHandler(void) {}
void __attribute__ ((weak)) DMA3_Stream1_IRQHandler(void) {}
void __attribute__ ((weak)) DMA3_Stream2_IRQHandler(void) {}
void __attribute__ ((weak)) DMA3_Stream3_IRQHandler(void) {}
void __attribute__ ((weak)) DMA3_Stream4_IRQHandler(void) {}
void __attribute__ ((weak)) DMA3_Stream5_IRQHandler(void) {}
void __attribute__ ((weak)) DMA3_Stream6_IRQHandler(void) {}
void __attribute__ ((weak)) DMA3_Stream7_IRQHandler(void) {}

IRQHandler_t IRQ_Vector_Table[MAX_IRQ_n] = {
    /* Software Generated Interrupts */
    SGI0_IRQHandler,                    /* 0 */
    SGI1_IRQHandler,
    SGI2_IRQHandler,
    SGI3_IRQHandler,
    SGI4_IRQHandler,
    SGI5_IRQHandler,
    SGI6_IRQHandler,
    SGI7_IRQHandler,
    SGI8_IRQHandler,
    SGI9_IRQHandler,
    SGI10_IRQHandler,                   /* 10 */
    SGI11_IRQHandler,
    SGI12_IRQHandler,
    SGI13_IRQHandler,
    SGI14_IRQHandler,
    SGI15_IRQHandler,
    RESERVED_IRQHandler,
    RESERVED_IRQHandler,
    RESERVED_IRQHandler,
    RESERVED_IRQHandler,
    RESERVED_IRQHandler,                /* 20 */
    RESERVED_IRQHandler,
    RESERVED_IRQHandler,
    RESERVED_IRQHandler,
    RESERVED_IRQHandler,
    /* Private Peripheral Interrupts */
    VirtualMaintenanceInterrupt_IRQHandler,
    HypervisorTimer_IRQHandler,
    VirtualTimer_IRQHandler,
    Legacy_nFIQ_IRQHandler,
    SecurePhysicalTimer_IRQHandler,
    NonSecurePhysicalTimer_IRQHandler,  /* 30 */
    Legacy_nIRQ_IRQHandler,
    /******  STM32 specific Interrupt Numbers **********************************************************************/
    RESERVED_IRQHandler,
    PVD_AVD_IRQHandler,
    TAMP_IRQHandler,
    RTC_WKUP_ALARM_IRQHandler,
    TZC_IT_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,                   /* 40 */
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Stream0_IRQHandler,
    DMA1_Stream1_IRQHandler,
    DMA1_Stream2_IRQHandler,
    DMA1_Stream3_IRQHandler,
    DMA1_Stream4_IRQHandler,
    DMA1_Stream5_IRQHandler,
    DMA1_Stream6_IRQHandler,
    ADC1_IRQHandler,                    /* 50 */
    ADC2_IRQHandler,
    FDCAN1_IT0_IRQHandler,
    FDCAN2_IT0_IRQHandler,
    FDCAN1_IT1_IRQHandler,
    FDCAN2_IT1_IRQHandler,
    EXTI5_IRQHandler,
    TIM1_BRK_IRQHandler,
    TIM1_UP_IRQHandler,
    TIM1_TRG_COM_IRQHandler,
    TIM1_CC_IRQHandler,                 /* 60 */
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM4_IRQHandler,
    I2C1_EV_IRQHandler,
    I2C1_ER_IRQHandler,
    I2C2_EV_IRQHandler,
    I2C2_ER_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,                  /* 70 */
    USART2_IRQHandler,
    USART3_IRQHandler,
    EXTI10_IRQHandler,
    RTC_TIMESTAMP_IRQHandler,
    EXTI11_IRQHandler,
    TIM8_BRK_IRQHandler,
    TIM8_UP_IRQHandler,
    TIM8_TRG_COM_IRQHandler,
    TIM8_CC_IRQHandler,
    DMA1_Stream7_IRQHandler,            /* 80 */
    FMC_IRQHandler,
    SDMMC1_IRQHandler,
    TIM5_IRQHandler,
    SPI3_IRQHandler,
    UART4_IRQHandler,
    UART5_IRQHandler,
    TIM6_IRQHandler,
    TIM7_IRQHandler,
    DMA2_Stream0_IRQHandler,
    DMA2_Stream1_IRQHandler,            /* 90 */
    DMA2_Stream2_IRQHandler,
    DMA2_Stream3_IRQHandler,
    DMA2_Stream4_IRQHandler,
    ETH1_IRQHandler,
    ETH1_WKUP_IRQHandler,
    FDCAN_CAL_IRQHandler,
    EXTI6_IRQHandler,
    EXTI7_IRQHandler,
    EXTI8_IRQHandler,
    EXTI9_IRQHandler,                   /* 100 */
    DMA2_Stream5_IRQHandler,
    DMA2_Stream6_IRQHandler,
    DMA2_Stream7_IRQHandler,
    USART6_IRQHandler,
    I2C3_EV_IRQHandler,
    I2C3_ER_IRQHandler,
    USBH_PORT1_IRQHandler,
    USBH_PORT2_IRQHandler,
    EXTI12_IRQHandler,
    EXTI13_IRQHandler,                  /* 110 */
    DCMIPP_IRQHandler,
    CRYP1_IRQHandler,
    HASH1_IRQHandler,
    SAES_IRQHandler,
    UART7_IRQHandler,
    UART8_IRQHandler,
    SPI4_IRQHandler,
    SPI5_IRQHandler,
    SAI1_IRQHandler,
    LTDC_IRQHandler,                    /* 120 */
    LTDC_ER_IRQHandler,
    SAI2_IRQHandler ,
    QUADSPI_IRQHandler,
    LPTIM1_IRQHandler,
    I2C4_EV_IRQHandler,
    I2C4_ER_IRQHandler,
    SPDIF_RX_IRQHandler ,
    OTG_IRQHandler,
    ETH2_IRQHandler,
    ETH2_WKUP_IRQHandler,               /* 130 */
    DMAMUX1_OVR_IRQHandler,
    DMAMUX2_OVR_IRQHandler,
    TIM15_IRQHandler,
    TIM16_IRQHandler,
    TIM17_IRQHandler,
    TIM12_IRQHandler,
    PKA_IRQHandler,
    EXTI14_IRQHandler,
    MDMA_IRQHandler,
    SDMMC2_IRQHandler,                  /* 140 */
    EXTI15_IRQHandler,
    MDMA_SEC_IT_IRQHandler,
    TIM13_IRQHandler,
    TIM14_IRQHandler,
    RNG1_IRQHandler,
    I2C5_EV_IRQHandler,
    I2C5_ER_IRQHandler,	
    LPTIM2_IRQHandler,
    LPTIM3_IRQHandler,
    LPTIM4_IRQHandler,                  /* 150 */
    LPTIM5_IRQHandler,	
    ETH1_LPI_IRQHandler,
    ETH2_LPI_IRQHandler,
	RCC_WAKEUP__IRQHandler,
    RESERVED_IRQHandler,	
    DTS_IRQHandler,	
    MPU_WAKEUP_PIN_IRQHandler,	
    IWDG1_IRQHandler,
    IWDG2_IRQHandler,	
    TAMP_SIRQHandler,                   /* 160 */	
    RTC_WKUP_ALARM_S_IRQHandler,
    RTC_TS_S_IRQHandler,	
    LTDC_SEC_IRQHandler,
	LTDC_SEC_ER_IRQHandler,
    PMUIRQ0_IRQHandler,
    COMMRX0_IRQHandler,
    COMMTX0_IRQHandler,
    AXIERRIRQ_IRQHandler,
    DDRPERFM_IRQHandler,
    nCTIIRQ0_IRQHandler,                /* 170 */
    MCE_IRQHandler,
    DFSDM1_IRQHandler,
    DFSDM2_IRQHandler,
    DMA3_Stream0_IRQHandler,
    DMA3_Stream1_IRQHandler,
    DMA3_Stream2_IRQHandler,
    DMA3_Stream3_IRQHandler,
    DMA3_Stream4_IRQHandler,
    DMA3_Stream5_IRQHandler,
    DMA3_Stream6_IRQHandler,            /* 180 */
    DMA3_Stream7_IRQHandler,	
};

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector Table
 *----------------------------------------------------------------------------*/
void Vectors(void) {
  __asm__ volatile(
  ".align 7                                         \n"
  ".code 16                                         \n"
  "LDR    PC, =Reset_Handler                        \n"
  "LDR    PC, =Undef_Handler                        \n"
  "LDR    PC, =FreeRTOS_SWI_Handler                          \n"
  "LDR    PC, =PAbt_Handler                         \n"
  "LDR    PC, =DAbt_Handler                         \n"
  "LDR    PC, =Rsvd_Handler                         \n"
  "LDR    PC, =FreeRTOS_IRQ_Handler                          \n"
  "LDR    PC, =FIQ_Handler                          \n"
  );
}

/*----------------------------------------------------------------------------
  Default Handler for Exceptions / Interrupts
 *----------------------------------------------------------------------------*/

void Default_Handler(void) {
  while(1);
}



/*----------------------------------------------------------------------------
  Reset Handler called on controller reset
 *----------------------------------------------------------------------------*/
void Reset_Handler(void) {
  __asm__ volatile(
  ".code 32                                         \n"
  /* Mask interrupts */
  "CPSID   if                                      \n"

  /* Put any cores other than 0 to sleep */
  "MRC     p15, 0, R0, c0, c0, 5                   \n"  /* Read MPIDR */
  "ANDS    R0, R0, #3                              \n"
  "goToSleep:                                      \n"
  "ITT  NE                                         \n"  /* Needed when in Thumb mode for following WFINE instruction */
  "WFINE                                           \n"
  "BNE     goToSleep                               \n"

  /* Reset SCTLR Settings */
  "MRC     p15, 0, R0, c1, c0, 0                   \n"  /* Read CP15 System Control register */
  "BIC     R0, R0, #(0x1 << 12)                    \n"  /* Clear I bit 12 to disable I Cache */
  "BIC     R0, R0, #(0x1 <<  2)                    \n"  /* Clear C bit  2 to disable D Cache */
  "BIC     R0, R0, #0x1                            \n"  /* Clear M bit  0 to disable MMU */
  "BIC     R0, R0, #(0x1 << 11)                    \n"  /* Clear Z bit 11 to disable branch prediction */
  "BIC     R0, R0, #(0x1 << 13)                    \n"  /* Clear V bit 13 to disable hivecs */
  "BIC     R0, R0, #(0x1 << 29)                    \n"  /* Clear AFE bit 29 to enable the full range of access permissions */
  "ORR     R0, R0, #(0x1 << 30)                    \n"  /* Set TE bit to take exceptions in Thumb mode */
  "MCR     p15, 0, R0, c1, c0, 0                   \n"  /* Write value back to CP15 System Control register */
  "ISB                                             \n"

  /* Configure ACTLR */
  "MRC     p15, 0, r0, c1, c0, 1                   \n"  /* Read CP15 Auxiliary Control Register */
  "ORR     r0, r0, #(1 <<  1)                      \n"  /* Enable L2 prefetch hint (UNK/WI since r4p1) */
  "MCR     p15, 0, r0, c1, c0, 1                   \n"  /* Write CP15 Auxiliary Control Register */

  /* Set Vector Base Address Register (VBAR) to point to this application's vector table */
  "LDR    R0, =Vectors                             \n"
  "MCR    p15, 0, R0, c12, c0, 0                   \n"
  "ISB                                             \n"

  /* Setup Stack for each exceptional mode */
  "CPS    %[fiq_mode]                              \n"
  "LDR    SP, =FIQ_STACK                           \n"
  "CPS    %[irq_mode]                              \n"
  "LDR    SP, =IRQ_STACK                           \n"
  "CPS    %[svc_mode]                              \n"
  "LDR    SP, =SVC_STACK                           \n"
  "CPS    %[abt_mode]                              \n"
  "LDR    SP, =ABT_STACK                           \n"
  "CPS    %[und_mode]                              \n"
  "LDR    SP, =UND_STACK                           \n"
  "CPS    %[sys_mode]                              \n"
  "LDR    SP, =SYS_STACK                           \n"

  /* Call SystemInit */
  "BL     SystemInit                               \n"

  /* Unmask interrupts */
  "CPSIE  if                                       \n"

  /* Initialize libc */
  "BL __libc_init_array \n"
  /* Call __main */
  "BL     main                                     \n"
  ::[usr_mode] "M" (USR_MODE),
    [fiq_mode] "M" (FIQ_MODE),
    [irq_mode] "M" (IRQ_MODE),
    [svc_mode] "M" (SVC_MODE),
    [abt_mode] "M" (ABT_MODE),
    [und_mode] "M" (UND_MODE),
    [sys_mode] "M" (SYS_MODE));
}

/*----------------------------------------------------------------------------
  Reset_Handler is declared as a 32 bit function  - target("arm") instead of Thumb.
  Any function added after Reset_Handler() will conserve this mode declaration.
  To keep Thumb mode, new function should be added before Reset_Handler().
 *----------------------------------------------------------------------------*/
