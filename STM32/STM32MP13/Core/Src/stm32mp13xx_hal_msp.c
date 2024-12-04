/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32mp13xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{
  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

}

void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  BSP_IO_Init_t io_init_structure;

  if(heth->Instance==ETH)
  {
    /* Ethernet MSP init:
      REF_CLK  -------------> PA1
      TXD0    --------------> PG13
      TXD1    --------------> PG14
      RXD0    --------------> PC4
      RXD1    --------------> PC5
      TX_EN   --------------> PB11
      CRS_DV  --------------> PC1
      MDC     --------------> PG2
      MDIO    --------------> PA2
     */

    /* Enable GPIOs clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /* Configure PA1, PA2 */
    GPIO_InitStructure.Pin =  GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure PB11 */
    GPIO_InitStructure.Pin = GPIO_PIN_11;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Configure PC1 */
    GPIO_InitStructure.Pin = GPIO_PIN_1;
    GPIO_InitStructure.Alternate = GPIO_AF10_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* Configure PC4 and PC5 */
    GPIO_InitStructure.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStructure.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* Configure PG2, PG13 and PG14 */
    GPIO_InitStructure.Pin = GPIO_PIN_2 | GPIO_PIN_13 | GPIO_PIN_14;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

    /* Configure ETH1_NRST pin */
    io_init_structure.Pin  = MCP23x17_GPIO_PIN_9;
    io_init_structure.Pull = IO_NOPULL;
    io_init_structure.Mode = IO_MODE_OUTPUT_PP;
    BSP_IO_Init(0, &io_init_structure);
    BSP_IO_WritePin(0, MCP23x17_GPIO_PIN_9, IO_PIN_SET);

    /* Configure IRQ */
#if defined (CORE_CA7)
    IRQ_SetPriority(ETH1_IRQn, (25 << 3));
    IRQ_Enable(ETH1_IRQn);
#endif

    /* Enable Ethernet clocks */
    __HAL_RCC_ETH1CK_CLK_ENABLE();
    __HAL_RCC_ETH1MAC_CLK_ENABLE();
    __HAL_RCC_ETH1TX_CLK_ENABLE();
    __HAL_RCC_ETH1RX_CLK_ENABLE();
  }
  else if(heth->Instance == ETH2)
  {
  	  /* USER CODE BEGIN ETH2_MspInit 0 */
  	  /* USER CODE END ETH2_MspInit 0 */

			/* Peripheral clock enable */
			__HAL_RCC_ETH2CK_CLK_ENABLE();
			__HAL_RCC_ETH2MAC_CLK_ENABLE();
			__HAL_RCC_ETH2TX_CLK_ENABLE();
			__HAL_RCC_ETH2RX_CLK_ENABLE();

			__HAL_RCC_GPIOA_CLK_ENABLE();
			__HAL_RCC_GPIOB_CLK_ENABLE();
			__HAL_RCC_GPIOE_CLK_ENABLE();
			__HAL_RCC_GPIOF_CLK_ENABLE();
			__HAL_RCC_GPIOG_CLK_ENABLE();
			/**ETH2 GPIO Configuration
			PA12    ------> ETH2_CRS_DV
			PF6     ------> ETH2_TX_EN
			PG5     ------> ETH2_MDC
			PG8     ------> ETH2_CLK
			PB2     ------> ETH2_MDIO
			PE2     ------> ETH2_RXD1
			PF4     ------> ETH2_RXD0
			PF7     ------> ETH2_TXD0
			PG11    ------> ETH2_TXD1
			*/

			GPIO_InitStructure.Pin = GPIO_PIN_12;
			GPIO_InitStructure.Mode = GPIO_MODE_AF;
			GPIO_InitStructure.Pull = GPIO_NOPULL;
			GPIO_InitStructure.Alternate = GPIO_AF11_ETH2;
			HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

			GPIO_InitStructure.Pin = GPIO_PIN_6 | GPIO_PIN_7;
			GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
			GPIO_InitStructure.Pull = GPIO_NOPULL;
			GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_MEDIUM;
			GPIO_InitStructure.Alternate = GPIO_AF11_ETH2;
			HAL_GPIO_Init(GPIOF, &GPIO_InitStructure);

			GPIO_InitStructure.Pin = GPIO_PIN_5 | GPIO_PIN_11;
			GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
			GPIO_InitStructure.Pull = GPIO_NOPULL;
			GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_MEDIUM;
			GPIO_InitStructure.Alternate = GPIO_AF10_ETH2;
			HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

			GPIO_InitStructure.Pin = GPIO_PIN_8;
			GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
			GPIO_InitStructure.Pull = GPIO_NOPULL;
			GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_MEDIUM;
			GPIO_InitStructure.Alternate = GPIO_AF13_ETH2;
			HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

			GPIO_InitStructure.Pin = GPIO_PIN_2;
			GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
			GPIO_InitStructure.Pull = GPIO_NOPULL;
			GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
			GPIO_InitStructure.Alternate = GPIO_AF11_ETH2;
			HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

			GPIO_InitStructure.Pin = GPIO_PIN_2;
			GPIO_InitStructure.Mode = GPIO_MODE_AF;
			GPIO_InitStructure.Pull = GPIO_NOPULL;
			GPIO_InitStructure.Alternate = GPIO_AF10_ETH2;
			HAL_GPIO_Init(GPIOE, &GPIO_InitStructure);

			GPIO_InitStructure.Pin = GPIO_PIN_4;
			GPIO_InitStructure.Mode = GPIO_MODE_AF;
			GPIO_InitStructure.Pull = GPIO_NOPULL;
			GPIO_InitStructure.Alternate = GPIO_AF11_ETH2;
			HAL_GPIO_Init(GPIOF, &GPIO_InitStructure);

			/* Configure ETH2_NRST pin */
			io_init_structure.Pin  = MCP23x17_GPIO_PIN_10;
			io_init_structure.Pull = IO_NOPULL;
			io_init_structure.Mode = IO_MODE_OUTPUT_PP;
			BSP_IO_Init(0, &io_init_structure);
			BSP_IO_WritePin(0, MCP23x17_GPIO_PIN_10, IO_PIN_SET);

#if defined (CORE_CA7)
	    IRQ_SetPriority(ETH2_IRQn, (25 << 3));
	    IRQ_Enable(ETH2_IRQn);
#endif

			/* USER CODE BEGIN ETH2_MspInit 1 */
  	  /* USER CODE END ETH2_MspInit 1 */
    }
}

/**
 * @brief  DeInitializes ETH MSP.
 * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
 *         the configuration information for ETHERNET module
 * @retval None
 */
void HAL_ETH_MspDeInit(ETH_HandleTypeDef *heth)
{
  if(heth->Instance==ETH)
  {
    __HAL_RCC_ETH1CK_CLK_DISABLE();
    __HAL_RCC_ETH1MAC_CLK_DISABLE();
    __HAL_RCC_ETH1TX_CLK_DISABLE();
    __HAL_RCC_ETH1RX_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1 | GPIO_PIN_2);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_11);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5);
    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_2 | GPIO_PIN_13 | GPIO_PIN_14);
    BSP_IO_DeInit(0);

    IRQ_Disable(ETH1_IRQn);
  }
}

/**
* @brief UART MSP Initialization
* This function configures the hardware resources used in this example
* @param huart: UART handle pointer
* @retval None
*/
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  if(huart->Instance==UART4)
  {
  /* USER CODE BEGIN UART4_MspInit 0 */

  /* USER CODE END UART4_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_UART4_CLK_ENABLE();
    __HAL_RCC_UART4_FORCE_RESET();
    __HAL_RCC_UART4_RELEASE_RESET();

    __HAL_RCC_GPIOD_CLK_ENABLE();

    /**UART4 GPIO Configuration
    PD8     ------> UART4_TX
    PD6     ------> UART4_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN UART4_MspInit 1 */

  /* USER CODE END UART4_MspInit 1 */
  }
}

/**
* @brief USART MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param husart: USART handle pointer
* @retval None
*/
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if(huart->Instance==UART4)
  {
  /* USER CODE BEGIN UART4_MspDeInit 0 */
    __HAL_RCC_UART4_FORCE_RESET();
    __HAL_RCC_UART4_RELEASE_RESET();
  /* USER CODE END UART4_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_UART4_CLK_DISABLE();

    /**UART4 GPIO Configuration
    PD8     ------> UART4_TX
    PD6     ------> UART4_RX
    */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_6);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_8);
  /* USER CODE BEGIN UART4_MspDeInit 1 */

  /* USER CODE END UART4_MspDeInit 1 */
  }

}
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * @}
  */

/**
  * @}
  */

