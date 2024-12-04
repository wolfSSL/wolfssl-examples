/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    LwIP/LwIP_UDPTCP_Echo_Server_Netconn_RTOS/Core/Src/main.c
  * @author  MCD Application Team
  * @brief   Main program body
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
#include "cmsis_os.h"
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "tlsecho.h"
#include "app_ethernet.h"
#include "FreeRTOSConfig.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart4;
PKA_HandleTypeDef hpka;
CRYP_HandleTypeDef hcryp1;
__ALIGN_BEGIN static const uint32_t pKeyCRYP1[4] __ALIGN_END = {
                            0x00000000,0x00000000,0x00000000,0x00000000};
__ALIGN_BEGIN static const uint32_t pInitVectCRYP1[4] __ALIGN_END = {
                            0x00000000,0x00000000,0x00000000,0x00000002};
__ALIGN_BEGIN static const uint32_t HeaderCRYP1[1] __ALIGN_END = {
                            0x00000000};

HASH_HandleTypeDef hhash1;
RNG_HandleTypeDef hrng1;
RTC_HandleTypeDef hrtc;
/* USER CODE BEGIN PV */

extern eth_conf_t eth_conf_s[NUM_OF_ETH_INSTANCE];

static osThreadId_t StartThreadHandle;
static const osThreadAttr_t StartThread_attributes = {
  .name = "Thread One",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 1024 * 8
};

static osThreadId_t ethernet_link_thread_handle;
static const osThreadAttr_t ethernet_link_thread_attributes = {
  .name = "Ethernet Link Thread",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 1024 * 16
};

static osThreadId_t DHCPThreadHandle;
static const osThreadAttr_t DHCPThread_attributes = {
  .name = "DHCP Thread",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 1024 * 2
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_UART4_Init(void);
static void MX_RNG1_Init(void);
static void MX_PKA_Init(void);
static void MX_RTC_Init(void);
static void MX_HASH1_Init(void);
static void MX_CRYP1_Init(void);
static void StartThread(void * argument);
static void Netif_Config(void);
/* USER CODE BEGIN PFP */
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

#ifdef __GNUC__
#define GETCHAR_PROTOTYPE int __io_getchar (void)
#else
#define GETCHAR_PROTOTYPE int fgetc(FILE * f)
#endif /* __GNUC__ */


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*Provides own implementation of vApplicationIRQHandler() to call IRQ_Handler()
 *directly without saving the FPU registers on interrupt entry.
 */
void vApplicationIRQHandler(uint32_t ulICCIAR)
{
	/*System IRQ*/
	IRQ_Handler(ulICCIAR);
}

/**
 *
 *
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* USER CODE BEGIN Boot_Mode_Sequence_1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initialize the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();
 
#if (USE_STPMIC1x) && !defined(USE_DDR)
  /* Configure PMIC */
  BSP_PMIC_Init();
  BSP_PMIC_InitRegulators();
#endif

  /* USER CODE BEGIN SysInit */
  /* Initialize all configured peripherals */
  /* Initialize UART */
  MX_UART4_Init();
  MX_RNG1_Init();
  /* PKA and RTC will not init, CRYP1 hangs when in use */
  //MX_PKA_Init();
  //MX_RTC_Init();
  MX_HASH1_Init();
  //MX_CRYP1_Init();

  /* Initialize the IO expander */
  BSP_IOEXPANDER_Init(0, IOEXPANDER_IO_MODE);
  printf("\wolfSSL LwIP RTOS Example started\r\n");

  /* Initialize LEDs */
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_RED);
  BSP_LED_Init(LED_BLUE);

  MX_ETH1_Init();
  //MX_ETH2_Init();

  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  osKernelInitialize();  /* Call init function for freertos objects (in freertos.c) */

  /* Init thread */
  StartThreadHandle = osThreadNew(StartThread, NULL, &StartThread_attributes);
  if (NULL == StartThreadHandle)
  {
    Error_Handler();
  }

  /* Start scheduler */
  osKernelStart();

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was either insufficient FreeRTOS heap memory available for the idle
	and/or timer tasks to be created, or vTaskStartScheduler() was called from
	User mode.  See the memory management section on the FreeRTOS web site for
	more details on the FreeRTOS heap http://www.freertos.org/a00111.html.  The
	mode from which main() is called is set in the C start up code and must be
	a privileged mode (not user mode). */
	for(;;);
}

/**
  * @brief  Start Thread
  * @param  argument not used
  * @retval None
  */
static void StartThread(void * argument)
{
  /* Create tcp_ip stack thread */
  tcpip_init(NULL, NULL);

  /* Initialize the LwIP stack */
  Netif_Config();

  while (1)
  {
  	if (eth_conf_s[0].DHCP_state == DHCP_ADDRESS_ASSIGNED)
  	{
  		break;
  	}
  	else
  	{
  		vTaskDelay(1000);
  	}
  }

  /* Initialize the TLS echo server thread */
  tlsecho_init();

  for( ;; )
  {
    /* Delete the Init Thread */
    osThreadTerminate(StartThreadHandle);
  }
}

/**
  * @brief  Initializes the lwIP stack
  * @param  None
  * @retval None
  */
static void Netif_Config(void)
{
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
  uint8_t i = 0;

  for (i = 0; i < NUM_OF_ETH_INSTANCE; i++)
  {
#if LWIP_DHCP
		ip_addr_set_zero_ip4(&ipaddr);
		ip_addr_set_zero_ip4(&netmask);
		ip_addr_set_zero_ip4(&gw);
#else
		IP_ADDR4(&ipaddr,IP_ADDR0,IP_ADDR1,IP_ADDR2,IP_ADDR3);
		IP_ADDR4(&netmask,NETMASK_ADDR0,NETMASK_ADDR1,NETMASK_ADDR2,NETMASK_ADDR3);
		IP_ADDR4(&gw,GW_ADDR0,GW_ADDR1,GW_ADDR2,GW_ADDR3);
#endif /* LWIP_DHCP */
    /* add the network interface */
    netif_add(&eth_conf_s[i].gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

    /*  Registers the default network interface. */
    if (i == 0)
    {
    	netif_set_default(&eth_conf_s[i].gnetif);
    }
		ethernet_link_status_updated(&eth_conf_s[i].gnetif);
#if LWIP_NETIF_LINK_CALLBACK
		netif_set_link_callback(&eth_conf_s[i].gnetif, ethernet_link_status_updated);

		ethernet_link_thread_handle = osThreadNew(ethernet_link_thread, &eth_conf_s[i].gnetif, &ethernet_link_thread_attributes);
		if (NULL == ethernet_link_thread_handle)
		{
			Error_Handler();
		}
#endif
  }

#if LWIP_DHCP
  /* Start DHCPClient */
  DHCPThreadHandle = osThreadNew(DHCP_Thread, NULL, &DHCPThread_attributes);
  if (NULL == DHCPThreadHandle)
  {
    Error_Handler();
  }
#endif
}

void SystemClock_Config(void)
{
#if !defined(USE_DDR)
  HAL_RCC_DeInit();
  RCC_ClkInitTypeDef RCC_ClkInitStructure;
  RCC_OscInitTypeDef RCC_OscInitStructure;

  /* Enable all available oscillators*/
  RCC_OscInitStructure.OscillatorType = (RCC_OSCILLATORTYPE_HSI |
                                         RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_CSI |
                                         RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE);

  RCC_OscInitStructure.HSIState = RCC_HSI_ON;
  RCC_OscInitStructure.HSEState = RCC_HSE_ON;
  RCC_OscInitStructure.LSEState = RCC_LSE_ON;
  RCC_OscInitStructure.LSIState = RCC_LSI_ON;
  RCC_OscInitStructure.CSIState = RCC_CSI_ON;

  RCC_OscInitStructure.HSICalibrationValue = 0x00; //Default reset value
  RCC_OscInitStructure.CSICalibrationValue = 0x10; //Default reset value
  RCC_OscInitStructure.HSIDivValue = RCC_HSI_DIV1; //Default value

  /* PLL configuration */
  RCC_OscInitStructure.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStructure.PLL.PLLSource = RCC_PLL12SOURCE_HSE;
  RCC_OscInitStructure.PLL.PLLM = 3;
  RCC_OscInitStructure.PLL.PLLN = 81;
  RCC_OscInitStructure.PLL.PLLP = 1;
  RCC_OscInitStructure.PLL.PLLQ = 2;
  RCC_OscInitStructure.PLL.PLLR = 2;
  RCC_OscInitStructure.PLL.PLLFRACV = 0x800;
  RCC_OscInitStructure.PLL.PLLMODE = RCC_PLL_FRACTIONAL;

  RCC_OscInitStructure.PLL2.PLLState = RCC_PLL_ON;
  RCC_OscInitStructure.PLL2.PLLSource = RCC_PLL12SOURCE_HSE;
  RCC_OscInitStructure.PLL2.PLLM = 3;
  RCC_OscInitStructure.PLL2.PLLN = 66;
  RCC_OscInitStructure.PLL2.PLLP = 2;
  RCC_OscInitStructure.PLL2.PLLQ = 2;
  RCC_OscInitStructure.PLL2.PLLR = 1;
  RCC_OscInitStructure.PLL2.PLLFRACV = 0x1400;
  RCC_OscInitStructure.PLL2.PLLMODE = RCC_PLL_FRACTIONAL;

  RCC_OscInitStructure.PLL3.PLLState = RCC_PLL_ON;
  RCC_OscInitStructure.PLL3.PLLSource = RCC_PLL3SOURCE_HSE;
  RCC_OscInitStructure.PLL3.PLLM = 2;
  RCC_OscInitStructure.PLL3.PLLN = 34;
  RCC_OscInitStructure.PLL3.PLLP = 2;
  RCC_OscInitStructure.PLL3.PLLQ = 17;
  RCC_OscInitStructure.PLL3.PLLR = 2;
  RCC_OscInitStructure.PLL3.PLLRGE = RCC_PLL3IFRANGE_1;
  RCC_OscInitStructure.PLL3.PLLFRACV = 0x1a04;
  RCC_OscInitStructure.PLL3.PLLMODE = RCC_PLL_FRACTIONAL;

  RCC_OscInitStructure.PLL4.PLLState = RCC_PLL_ON;
  RCC_OscInitStructure.PLL4.PLLSource = RCC_PLL4SOURCE_HSE;
  RCC_OscInitStructure.PLL4.PLLM = 2;
  RCC_OscInitStructure.PLL4.PLLN = 50;
  RCC_OscInitStructure.PLL4.PLLP = 12;
  RCC_OscInitStructure.PLL4.PLLQ = 60;
  RCC_OscInitStructure.PLL4.PLLR = 6;
  RCC_OscInitStructure.PLL4.PLLRGE = RCC_PLL4IFRANGE_1;
  RCC_OscInitStructure.PLL4.PLLFRACV = 0;
  RCC_OscInitStructure.PLL4.PLLMODE = RCC_PLL_INTEGER;

  /* Enable access to RTC and backup registers */
  SET_BIT(PWR->CR1, PWR_CR1_DBP);
  /* Configure LSEDRIVE value */
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_MEDIUMHIGH);

  if (HAL_RCC_OscConfig(&RCC_OscInitStructure) != HAL_OK) {
    /* HAL RCC configuration error */
    Error_Handler();
  }

  /* Select PLLx as MPU, AXI and MCU clock sources */
  RCC_ClkInitStructure.ClockType = (RCC_CLOCKTYPE_MPU   | RCC_CLOCKTYPE_ACLK  |
                                    RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_PCLK4 |
                                    RCC_CLOCKTYPE_PCLK5 | RCC_CLOCKTYPE_PCLK1 |
                                    RCC_CLOCKTYPE_PCLK6 |
                                    RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3);

  RCC_ClkInitStructure.MPUInit.MPU_Clock = RCC_MPUSOURCE_PLL1;
  RCC_ClkInitStructure.MPUInit.MPU_Div = RCC_MPU_DIV2;
  RCC_ClkInitStructure.AXISSInit.AXI_Clock = RCC_AXISSOURCE_PLL2;
  RCC_ClkInitStructure.AXISSInit.AXI_Div = RCC_AXI_DIV1;
  RCC_ClkInitStructure.MLAHBInit.MLAHB_Clock = RCC_MLAHBSSOURCE_PLL3;
  RCC_ClkInitStructure.MLAHBInit.MLAHB_Div = RCC_MLAHB_DIV1;
  RCC_ClkInitStructure.APB1_Div = RCC_APB1_DIV2;
  RCC_ClkInitStructure.APB2_Div = RCC_APB2_DIV2;
  RCC_ClkInitStructure.APB3_Div = RCC_APB3_DIV2;
  RCC_ClkInitStructure.APB4_Div = RCC_APB4_DIV2;
  RCC_ClkInitStructure.APB5_Div = RCC_APB5_DIV4;
  RCC_ClkInitStructure.APB6_Div = RCC_APB6_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStructure) != HAL_OK) {
    /* HAL RCC configuration error */
    Error_Handler();
  }

/*
  Note : The activation of the I/O Compensation Cell is recommended with communication  interfaces
  (GPIO, SPI, FMC, XSPI ...)  when  operating at  high frequencies(please refer to product datasheet)
  The I/O Compensation Cell activation  procedure requires :
  - The activation of the CSI clock
  - The activation of the SYSCFG clock
  - Enabling the I/O Compensation Cell : setting bit[0] of register SYSCFG_CCCSR

  To do this please uncomment the following code
  */

  /*
  __HAL_RCC_CSI_ENABLE() ;

  __HAL_RCC_SYSCFG_CLK_ENABLE() ;

  HAL_EnableCompensationCell();
   */
#endif /* !USE_DDR */
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  /** Initializes the peripherals clock
  */
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ETH2;
	PeriphClkInit.Eth2ClockSelection = RCC_ETH2CLKSOURCE_PLL4;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}


/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief HASH1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_HASH1_Init(void)
{

  /* USER CODE BEGIN HASH1_Init 0 */

  /* USER CODE END HASH1_Init 0 */

  /* USER CODE BEGIN HASH1_Init 1 */

  /* USER CODE END HASH1_Init 1 */
  hhash1.Init.DataType = HASH_DATATYPE_32B;
  if (HAL_HASH_Init(&hhash1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN HASH1_Init 2 */

  /* USER CODE END HASH1_Init 2 */

}

/**
  * @brief RNG1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG1_Init(void)
{

  /* USER CODE BEGIN RNG1_Init 0 */

  /* USER CODE END RNG1_Init 0 */

  /* USER CODE BEGIN RNG1_Init 1 */

  /* USER CODE END RNG1_Init 1 */
  hrng1.Instance = RNG1;
  hrng1.Init.ClockErrorDetection = RNG_CED_ENABLE;
  if (HAL_RNG_Init(&hrng1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG1_Init 2 */

  /* USER CODE END RNG1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_PrivilegeStateTypeDef privilegeState = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
  hrtc.Init.BinMode = RTC_BINARY_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  privilegeState.rtcPrivilegeFull = RTC_PRIVILEGE_FULL_NO;
  privilegeState.backupRegisterPrivZone = RTC_PRIVILEGE_BKUP_ZONE_NONE;
  privilegeState.backupRegisterStartZone2 = RTC_BKP_DR0;
  privilegeState.backupRegisterStartZone3 = RTC_BKP_DR0;
  if (HAL_RTCEx_PrivilegeModeSet(&hrtc, &privilegeState) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief PKA Initialization Function
  * @param None
  * @retval None
  */
static void MX_PKA_Init(void)
{

  /* USER CODE BEGIN PKA_Init 0 */

  /* USER CODE END PKA_Init 0 */

  /* USER CODE BEGIN PKA_Init 1 */

  /* USER CODE END PKA_Init 1 */
  hpka.Instance = PKA;
  if (HAL_PKA_Init(&hpka) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN PKA_Init 2 */

  /* USER CODE END PKA_Init 2 */

}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */

/**
  * @brief CRYP1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRYP1_Init(void)
{

  /* USER CODE BEGIN CRYP1_Init 0 */

  /* USER CODE END CRYP1_Init 0 */

  /* USER CODE BEGIN CRYP1_Init 1 */

  /* USER CODE END CRYP1_Init 1 */
  hcryp1.Instance = CRYP1;
  hcryp1.Init.DataType = CRYP_DATATYPE_8B;
  hcryp1.Init.KeySize = CRYP_KEYSIZE_128B;
  hcryp1.Init.pKey = (uint32_t *)pKeyCRYP1;
  hcryp1.Init.pInitVect = (uint32_t *)pInitVectCRYP1;
  hcryp1.Init.Algorithm = CRYP_AES_GCM;
  hcryp1.Init.Header = (uint32_t *)HeaderCRYP1;
  hcryp1.Init.HeaderSize = 1;
  hcryp1.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_BYTE;
  hcryp1.Init.KeyIVConfigSkip = CRYP_KEYIVCONFIG_ALWAYS;
  hcryp1.Init.KeyMode = CRYP_KEYMODE_NORMAL;
  if (HAL_CRYP_Init(&hcryp1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRYP1_Init 2 */

  /* USER CODE END CRYP1_Init 2 */

}

static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */

  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_8;
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart4.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart4, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart4, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart4) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */
}
/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */

int __write(int file, char *buf, int size)
{
  (void)file;
  (void)size;
  HAL_UART_Transmit(&huart4, (uint8_t *)buf, 1, 0xFFFF);
  return 1;
}

PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the UART4 and Loop until the end of transmission */
  __write(1, (char *)&ch, 1);
  return ch;
}


GETCHAR_PROTOTYPE
{
  uint8_t ch = 0;
  /* Clear the Overrun flag just before receiving the first character */
  __HAL_UART_CLEAR_OREFLAG(&huart4);

  HAL_UART_Receive(&huart4, (uint8_t *)&ch, 1, 0xFFFF);
  HAL_UART_Transmit(&huart4, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();

  BSP_LED_Off(LED_GREEN);
  BSP_LED_Off(LED_BLUE);

  while (1)
  {
	 BSP_LED_Toggle(LED_RED);
    HAL_Delay(1000);
  }
  /* USER CODE END Error_Handler_Debug */
}


void vConfigureTickInterrupt( void )
{
	  /* Stop Timer */
	  PL1_SetControl(0x0);

	  PL1_SetCounterFrequency(HSI_VALUE);

	  /* Initialize Counter */
	  PL1_SetLoadValue(HSI_VALUE/1000);

	  /* Disable corresponding IRQ */
	  IRQ_Disable(SecurePhyTimer_IRQn);
	  IRQ_ClearPending(SecurePhyTimer_IRQn);

	  IRQ_SetPriority(SecurePhyTimer_IRQn, configUNIQUE_INTERRUPT_PRIORITIES - 2);

	  /* Set edge-triggered IRQ */
	  IRQ_SetMode(SecurePhyTimer_IRQn, IRQ_MODE_TRIG_EDGE);

	  /* Enable corresponding interrupt */
	  IRQ_Enable(SecurePhyTimer_IRQn);

	  /* Start Timer */
	  PL1_SetControl(0x1);

}

void vClearTickInterrupt( void )
{

	IRQ_ClearPending((IRQn_ID_t)SecurePhyTimer_IRQn);

}
#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* Infinite loop */
  while (1)
  {
  }
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */





