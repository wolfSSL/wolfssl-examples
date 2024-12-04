/**
  ******************************************************************************
  * @file    LwIP/LwIP_UDPTCP_Echo_Server_Netconn_RTOS/Src/ethernetif.c
  * @author  MCD Application Team
  * @brief   This file implements Ethernet network interface drivers for lwIP
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32mp13xx_hal.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/tcpip.h"
#include "ethernetif.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* The time to block waiting for input. */
#define TIME_WAITING_FOR_INPUT                 ( osWaitForever )
/* Stack size of the interface thread */
#define INTERFACE_THREAD_STACK_SIZE            ( 1024 )

/* Define those to better describe your network interface. */
#define IFNAME0 's'
#define IFNAME1 't'

#define ETH_DMA_TRANSMIT_TIMEOUT                (20U)

#define ETH_RX_BUFFER_SIZE            1536
#define ETH_RX_BUFFER_CNT             ((ETH_RX_DESC_CNT) * 2U) // 12U
#define ETH_TX_BUFFER_MAX             ((ETH_TX_DESC_CNT) * 2U)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/*
@Note: This interface is implemented to operate in zero-copy mode only:
        - Rx Buffers will be allocated from LwIP stack memory heap,
          then passed to ETH HAL driver.
        - Tx Buffers will be allocated from LwIP stack memory heap,
          then passed to ETH HAL driver.

@Notes:
  1.a. ETH DMA Rx descriptors must be contiguous, the default count is 4,
       to customize it please redefine ETH_RX_DESC_CNT in ETH GUI (Rx Descriptor Length)
       so that updated value will be generated in stm32xxxx_hal_conf.h
  1.b. ETH DMA Tx descriptors must be contiguous, the default count is 4,
       to customize it please redefine ETH_TX_DESC_CNT in ETH GUI (Tx Descriptor Length)
       so that updated value will be generated in stm32xxxx_hal_conf.h

  2.a. Rx Buffers number: ETH_RX_BUFFER_CNT must be greater than ETH_RX_DESC_CNT.
  2.b. Rx Buffers must have the same size: ETH_RX_BUFFER_SIZE, this value must
       passed to ETH DMA in the init field (heth.Init.RxBuffLen)
*/
typedef enum
{
  RX_ALLOC_OK       = 0x00,
  RX_ALLOC_ERROR    = 0x01
} RxAllocStatusTypeDef;

typedef struct
{
  struct pbuf_custom pbuf_custom;
  uint8_t buff[(ETH_RX_BUFFER_SIZE + 31) & ~31] __ALIGNED(32);
} RxBuff_t;

typedef struct {
	ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT];
	ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT];
} eth_dma_descriptor_t;

eth_conf_t eth_conf_s[NUM_OF_ETH_INSTANCE] = {};

typedef enum
{
	ETH0_EVENT = 0,
	ETH1_EVENT = 1,
} eth_event_t;

osMessageQueueId_t RxPktQueue = NULL;
osMessageQueueId_t TxPktQueue = NULL;

#if defined ( __ICCARM__ ) /*!< IAR Compiler */

#pragma location=0x2FFE0000
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
#pragma location=0x2FFE0200
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */


#elif defined ( __CC_ARM )  /* MDK ARM Compiler */

__attribute__((section(".RxDecripSection"))) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
__attribute__((section(".TxDecripSection"))) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __GNUC__ ) /* GNU Compiler */

eth_dma_descriptor_t dma_desc_s[NUM_OF_ETH_INSTANCE] __attribute__((section(".RxDecripSection")));

#endif

/* Memory Pool Declaration */
LWIP_MEMPOOL_DECLARE(RX_POOL, (ETH_RX_BUFFER_CNT * NUM_OF_ETH_INSTANCE), sizeof(RxBuff_t), "Zero-copy RX PBUF pool");

LWIP_MEMPOOL_DECLARE(LWIP_HEAP_RAM, 1, MEM_SIZE, "LwIP heap ram");

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma location = 0x2FFE0400
extern u8_t memp_memory_RX_POOL_base[];
#pragma location = 0x2FFF0000
extern u8_t memp_memory_LWIP_HEAP_RAM_base[];

#elif defined ( __CC_ARM )  /* MDK ARM Compiler */
__attribute__((section(".Rx_PoolSection"))) extern u8_t memp_memory_RX_POOL_base[];

#elif defined ( __GNUC__ ) /* GNU Compiler */
__attribute__((section(".Rx_PoolSection"))) extern u8_t memp_memory_RX_POOL_base[];

__attribute__((section(".LwIP_Heap_RAM_Section"))) extern u8_t memp_memory_LWIP_HEAP_RAM_base[];

#endif

/* Private function prototypes -----------------------------------------------*/
extern void Error_Handler(void);
static void ethernetif_input( void * argument );
int32_t ETH_PHY_IO_Init(void);
int32_t ETH_PHY_IO_DeInit (void);
int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal);
int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal);
int32_t ETH_PHY_IO_GetTick(void);
int32_t ETH2_PHY_IO_Init(void);
int32_t ETH2_PHY_IO_DeInit (void);
int32_t ETH2_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal);
int32_t ETH2_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal);
int32_t ETH2_PHY_IO_GetTick(void);

lan8742_IOCtx_t  LAN8742_IOCtx[NUM_OF_ETH_INSTANCE] = {
		{
				.Init = ETH_PHY_IO_Init,
				.DeInit = ETH_PHY_IO_DeInit,
				.ReadReg = ETH_PHY_IO_ReadReg,
				.WriteReg = ETH_PHY_IO_WriteReg,
				.GetTick = ETH_PHY_IO_GetTick
		},
		{
				.Init = ETH2_PHY_IO_Init,
				.DeInit = ETH2_PHY_IO_DeInit,
				.ReadReg = ETH2_PHY_IO_ReadReg,
				.WriteReg = ETH2_PHY_IO_WriteReg,
				.GetTick = ETH2_PHY_IO_GetTick
		},
};

uint8_t isInitialized = 0;

/* Private functions ---------------------------------------------------------*/
void pbuf_free_custom(struct pbuf *p);
/*******************************************************************************
                       LL Driver Interface ( LwIP stack --> ETH)
*******************************************************************************/

void MX_ETH1_Init(void)
{
  uint8_t macaddress[6] = {ETH_MAC_ADDR0, ETH_MAC_ADDR1, ETH_MAC_ADDR2, ETH_MAC_ADDR3, ETH_MAC_ADDR4, ETH_MAC_ADDR5};

  eth_conf_s[0].EthHandle.Instance = ETH;
  eth_conf_s[0].EthHandle.Init.MACAddr = macaddress;
  eth_conf_s[0].EthHandle.Init.MediaInterface = HAL_ETH_RMII_MODE;
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  eth_conf_s[0].EthHandle.Init.RxDesc = DMARxDscrTab;
  eth_conf_s[0].EthHandle.Init.TxDesc = DMATxDscrTab;
#elif defined ( __GNUC__ ) /* GNU Compiler */
  eth_conf_s[0].EthHandle.Init.RxDesc = dma_desc_s[0].DMARxDscrTab;
  eth_conf_s[0].EthHandle.Init.TxDesc = dma_desc_s[0].DMATxDscrTab;
#endif
  eth_conf_s[0].EthHandle.Init.RxBuffLen = 1536;
  eth_conf_s[0].EthHandle.Init.ClockSelection = HAL_ETH1_REF_CLK_RX_CLK_PIN;

  /* configure ethernet peripheral (GPIOs, clocks, MAC, DMA) */
  HAL_ETH_Init(&eth_conf_s[0].EthHandle);
}

void MX_ETH2_Init(void)
{
  uint8_t macaddress[6] = {ETH_MAC_ADDR0, ETH_MAC_ADDR1, ETH_MAC_ADDR2, ETH_MAC_ADDR3, ETH_MAC_ADDR4, ETH_MAC_ADDR5 + 1};

  eth_conf_s[1].EthHandle.Instance = ETH2;
  eth_conf_s[1].EthHandle.Init.MACAddr = macaddress;
  eth_conf_s[1].EthHandle.Init.MediaInterface = HAL_ETH_RMII_MODE;
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  eth_conf_s[1].EthHandle.Init.RxDesc = DMARxDscrTab;
  eth_conf_s[1].EthHandle.Init.TxDesc = DMATxDscrTab;
#elif defined ( __GNUC__ ) /* GNU Compiler */
  eth_conf_s[1].EthHandle.Init.RxDesc = dma_desc_s[1].DMARxDscrTab;
  eth_conf_s[1].EthHandle.Init.TxDesc = dma_desc_s[1].DMATxDscrTab;
#endif
  eth_conf_s[1].EthHandle.Init.RxBuffLen = 1536;
  eth_conf_s[1].EthHandle.Init.ClockSelection = HAL_ETH2_REF_CLK_RCC;

  /* configure ethernet peripheral (GPIOs, clocks, MAC, DMA) */
  HAL_ETH_Init(&eth_conf_s[1].EthHandle);
}

/**
  * @brief In this function, the hardware should be initialized.
  * Called from ethernetif_init().
  *
  * @param netif the already initialized lwip network interface structure
  *        for this ethernetif
  */
static void low_level_init(struct netif *netif)
{
  osThreadAttr_t attributes;
  uint8_t instance = 0;

  if (netif == &(eth_conf_s[0].gnetif))
  {
    netif->hwaddr[5] =  ETH_MAC_ADDR5;
		instance = 0;
  }
  else if (netif == &(eth_conf_s[1].gnetif))
  {
    netif->hwaddr[5] =  ETH_MAC_ADDR5 + 1;
  	instance = 1;
  }
  else
  {
  	return;
  }

  /* set MAC hardware address length */
  netif->hwaddr_len = ETH_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] =  ETH_MAC_ADDR0;
  netif->hwaddr[1] =  ETH_MAC_ADDR1;
  netif->hwaddr[2] =  ETH_MAC_ADDR2;
  netif->hwaddr[3] =  ETH_MAC_ADDR3;
  netif->hwaddr[4] =  ETH_MAC_ADDR4;

  /* maximum transfer unit */
  netif->mtu = ETH_MAX_PAYLOAD;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags |= (NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP);

  if (0 == isInitialized)
  {
		/* Initialize the RX POOL */
		LWIP_MEMPOOL_INIT(RX_POOL);

		RxPktQueue = osMessageQueueNew(5, sizeof(eth_event_t), NULL);
		if (RxPktQueue == NULL)
		{
			return;
		}
		TxPktQueue = osMessageQueueNew(5, sizeof(eth_event_t), NULL);
		if (TxPktQueue == NULL)
		{
			return;
		}

		/* USER CODE BEGIN OS_THREAD_NEW_CMSIS_RTOS_V2 */
	  memset(&attributes, 0x0, sizeof(osThreadAttr_t));
	  attributes.name = "EthIf";
	  attributes.stack_size = INTERFACE_THREAD_STACK_SIZE;
	  attributes.priority = osPriorityRealtime;
	  osThreadNew(ethernetif_input, NULL, &attributes);
	  isInitialized = 1;
  }

  /* Set Tx packet config common parameters */
  memset(&eth_conf_s[instance].TxConfig, 0 , sizeof(ETH_TxPacketConfigTypeDef));
  eth_conf_s[instance].TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  eth_conf_s[instance].TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  eth_conf_s[instance].TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;

  /* USER CODE END OS_THREAD_NEW_CMSIS_RTOS_V2 */

  HAL_ETH_RegisterRxAllocateCallback(&eth_conf_s[instance].EthHandle, HAL_ETH_RxAllocateCallback);

  /* Set PHY IO functions */
  LAN8742_RegisterBusIO(&eth_conf_s[instance].LAN8742, &LAN8742_IOCtx[instance]);

  /* Initialize the LAN8742 ETH PHY */
  LAN8742_Init(&eth_conf_s[instance].LAN8742);
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet was sent, or ERR_IF if the packet was unable to be sent
 *
 * @note ERR_OK means the packet was sent (but not necessarily transmit complete),
 * and ERR_IF means the packet has more chained buffers than what the interface supports.
 */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  uint32_t i = 0U;
  struct pbuf *q = NULL;
  err_t errval = ERR_OK;
  ETH_BufferTypeDef Txbuffer[ETH_TX_DESC_CNT];
  uint8_t instance = 0;
  eth_event_t event;

  memset(Txbuffer, 0, ETH_TX_DESC_CNT * sizeof(ETH_BufferTypeDef));

  for(q = p; q != NULL; q = q->next)
  {
    if(i >= ETH_TX_DESC_CNT)
      return ERR_IF;

    Txbuffer[i].buffer = q->payload;
    Txbuffer[i].len = q->len;

    if(i>0)
    {
      Txbuffer[i-1].next = &Txbuffer[i];
    }

    if(q->next == NULL)
    {
      Txbuffer[i].next = NULL;
    }

    i++;
  }

  if (netif == &(eth_conf_s[0].gnetif))
  {
		instance = 0;
		event = ETH0_EVENT;
  }
  else if (netif == &(eth_conf_s[1].gnetif))
  {
  	instance = 1;
  	event = ETH1_EVENT;
  }
  else
  {
  	return ERR_VAL;
  }

  eth_conf_s[instance].TxConfig.Length = p->tot_len;
  eth_conf_s[instance].TxConfig.TxBuffer = Txbuffer;
  eth_conf_s[instance].TxConfig.pData = p;

  pbuf_ref(p);


  HAL_ETH_Transmit_IT(&eth_conf_s[instance].EthHandle, &eth_conf_s[instance].TxConfig);

  while(osMessageQueueGet(TxPktQueue, &event, NULL, TIME_WAITING_FOR_INPUT) != osOK)
  {
  }

  HAL_ETH_ReleaseTxPacket(&eth_conf_s[instance].EthHandle);

  return errval;
}

/**
  * @brief Should allocate a pbuf and transfer the bytes of the incoming
  * packet from the interface into the pbuf.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return a pbuf filled with the received packet (including MAC header)
  *         NULL on memory error
  */
static struct pbuf * low_level_input(struct netif *netif)
{
  struct pbuf *p = NULL;
  uint8_t instance = 0;

  if (netif == &(eth_conf_s[0].gnetif))
  {
		instance = 0;
  }
  else if (netif == &(eth_conf_s[1].gnetif))
  {
  	instance = 1;
  }
  else
  {
  	return NULL;
  }
  if(eth_conf_s[instance].RxAllocStatus == RX_ALLOC_OK)
  {
    HAL_ETH_ReadData(&eth_conf_s[instance].EthHandle, (void **)&p);
  }

  return p;
}

/**
 * This task should be signaled when a receive packet is ready to be read
 * from the interface.
 *
 * @param argument the lwip network interface structure for this ethernetif
 */
static void ethernetif_input( void * argument )
{
  struct pbuf *p = NULL;
  struct netif *netif = NULL;
  eth_event_t event = 0;

  for( ;; )
  {
    if (osMessageQueueGet(RxPktQueue, &event, 0, TIME_WAITING_FOR_INPUT) == osOK)
    {
    	netif = (struct netif *) &eth_conf_s[event].gnetif;
      /* wolfSSL: Looping here seems to break things */
      //do
      //{
        p = low_level_input( netif );
        if (p != NULL)
        {
          if (netif->input( p, netif) != ERR_OK )
          {
            pbuf_free(p);
          }
        }

      //}while(p!=NULL);
    }
    osDelay(100);
  }
}

/**
  * @brief Should be called at the beginning of the program to set up the
  * network interface. It calls the function low_level_init() to do the
  * actual setup of the hardware.
  *
  * This function should be passed as a parameter to netif_add().
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return ERR_OK if the loopif is initialized
  *         ERR_MEM if private data couldn't be allocated
  *         any other err_t on error
  */
err_t ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;

  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

/**
  * @brief  Custom Rx pbuf free callback
  * @param  pbuf: pbuf to be freed
  * @retval None
  */
void pbuf_free_custom(struct pbuf *p)
{
  struct pbuf_custom* custom_pbuf = (struct pbuf_custom*)p;
  LWIP_MEMPOOL_FREE(RX_POOL, custom_pbuf);

//  if (RxAllocStatus == RX_ALLOC_ERROR)
//  {
//    RxAllocStatus = RX_ALLOC_OK;
//    osSemaphoreRelease(RxPktSemaphore);
//  }
}

/**
  * @brief  Returns the current time in milliseconds
  *         when LWIP_TIMERS == 1 and NO_SYS == 1
  * @param  None
  * @retval Current Time value
  */
u32_t sys_now(void)
{
  return HAL_GetTick();
}

/**
  * @brief  Ethernet Rx Transfer completed callback
  * @param  heth: ETH handle
  * @retval None
  */
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
	eth_event_t event = 0;

  if (heth->Instance == eth_conf_s[0].EthHandle.Instance)
  {
  	event = ETH0_EVENT;
  }
  else if (heth->Instance == eth_conf_s[1].EthHandle.Instance)
  {
  	event = ETH1_EVENT;
  }
  else
  {
  	return;
  }

  osMessageQueuePut(RxPktQueue, &event, 0, 0);
}

/**
  * @brief  Ethernet Tx Transfer completed callback
  * @param  heth: ETH handle
  * @retval None
  */
void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *heth)
{
	eth_event_t event = 0;

  if (heth->Instance == eth_conf_s[0].EthHandle.Instance)
  {
  	event = ETH0_EVENT;
  }
  else if (heth->Instance == eth_conf_s[1].EthHandle.Instance)
  {
  	event = ETH1_EVENT;
  }
  else
  {
  	return;
  }

  osMessageQueuePut(TxPktQueue, &event, 0, 0);
}

/**
  * @brief  Ethernet DMA transfer error callback
  * @param  heth: ETH handle
  * @retval None
  */
void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *heth)
{
	eth_event_t event = 0;

  if (heth->Instance == eth_conf_s[0].EthHandle.Instance)
  {
  	event = ETH0_EVENT;
  }
  else if (heth->Instance == eth_conf_s[1].EthHandle.Instance)
  {
  	event = ETH1_EVENT;
  }
  else
  {
  	return;
  }

  if((HAL_ETH_GetDMAError(heth) & ETH_DMAC0SR_RBU) == ETH_DMAC0SR_RBU)
  {
  	osMessageQueuePut(RxPktQueue, &event, 0, 0);
  }

  if((HAL_ETH_GetDMAError(heth) & ETH_DMAC0SR_TBU) == ETH_DMAC0SR_TBU)
  {
  	osMessageQueuePut(TxPktQueue, &event, 0, 0);
  }
}

/*******************************************************************************
                       PHI IO Functions
*******************************************************************************/
/**
  * @brief  Initializes the MDIO interface GPIO and clocks.
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
int32_t ETH_PHY_IO_Init(void)
{
  /* We assume that MDIO GPIO configuration is already done
     in the ETH_MspInit() else it should be done here
  */

  /* Configure the MDIO Clock */
  HAL_ETH_SetMDIOClockRange(&eth_conf_s[0].EthHandle);

  return 0;
}

/**
  * @brief  De-Initializes the MDIO interface .
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
int32_t ETH_PHY_IO_DeInit (void)
{
  return 0;
}

/**
  * @brief  Read a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  pRegVal: pointer to hold the register value
  * @retval 0 if OK -1 if Error
  */
int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal)
{
  if(HAL_ETH_ReadPHYRegister(&eth_conf_s[0].EthHandle, DevAddr, RegAddr, pRegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Write a value to a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  RegVal: Value to be written
  * @retval 0 if OK -1 if Error
  */
int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal)
{
  if(HAL_ETH_WritePHYRegister(&eth_conf_s[0].EthHandle, DevAddr, RegAddr, RegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Get the time in millisecons used for internal PHY driver process.
  * @retval Time value
  */
int32_t ETH_PHY_IO_GetTick(void)
{
  return HAL_GetTick();
}

/**
  * @brief  Initializes the MDIO interface GPIO and clocks.
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
int32_t ETH2_PHY_IO_Init(void)
{
  /* We assume that MDIO GPIO configuration is already done
     in the ETH_MspInit() else it should be done here
  */

  /* Configure the MDIO Clock */
  HAL_ETH_SetMDIOClockRange(&eth_conf_s[1].EthHandle);

  return 0;
}

/**
  * @brief  De-Initializes the MDIO interface .
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
int32_t ETH2_PHY_IO_DeInit (void)
{
  return 0;
}

/**
  * @brief  Read a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  pRegVal: pointer to hold the register value
  * @retval 0 if OK -1 if Error
  */
int32_t ETH2_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal)
{
  if(HAL_ETH_ReadPHYRegister(&eth_conf_s[1].EthHandle, DevAddr, RegAddr, pRegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Write a value to a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  RegVal: Value to be written
  * @retval 0 if OK -1 if Error
  */
int32_t ETH2_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal)
{
  if(HAL_ETH_WritePHYRegister(&eth_conf_s[1].EthHandle, DevAddr, RegAddr, RegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Get the time in millisecons used for internal PHY driver process.
  * @retval Time value
  */
int32_t ETH2_PHY_IO_GetTick(void)
{
  return HAL_GetTick();
}

/**
  * @brief  Check the ETH link state and update netif accordingly.
  * @param  argument: netif
  * @retval None
  */
void ethernet_link_thread( void * argument )
{
  ETH_MACConfigTypeDef MACConf = {0};
  int32_t PHYLinkState = 0U;
  uint32_t linkchanged = 0U, speed = 0U, duplex = 0U;
  struct netif *netif = (struct netif *) argument;
  uint8_t instance = 0;

  if (netif == &(eth_conf_s[0].gnetif))
  {
		instance = 0;
  }
  else if (netif == &(eth_conf_s[1].gnetif))
  {
  	instance = 1;
  }
  else
  {
  	return;
  }

  for(;;)
  {

    PHYLinkState = LAN8742_GetLinkState(&eth_conf_s[instance].LAN8742);

    if(netif_is_link_up(netif) && (PHYLinkState <= LAN8742_STATUS_LINK_DOWN))
    {
      HAL_ETH_Stop_IT(&eth_conf_s[instance].EthHandle);
      netif_set_down(netif);
      netif_set_link_down(netif);
    }
    else if(!netif_is_link_up(netif) && (PHYLinkState > LAN8742_STATUS_LINK_DOWN))
    {
      switch (PHYLinkState)
      {
      case LAN8742_STATUS_100MBITS_FULLDUPLEX:
        duplex = ETH_FULLDUPLEX_MODE;
        speed = ETH_SPEED_100M;
        linkchanged = 1;
        break;
      case LAN8742_STATUS_100MBITS_HALFDUPLEX:
        duplex = ETH_HALFDUPLEX_MODE;
        speed = ETH_SPEED_100M;
        linkchanged = 1;
        break;
      case LAN8742_STATUS_10MBITS_FULLDUPLEX:
        duplex = ETH_FULLDUPLEX_MODE;
        speed = ETH_SPEED_10M;
        linkchanged = 1;
        break;
      case LAN8742_STATUS_10MBITS_HALFDUPLEX:
        duplex = ETH_HALFDUPLEX_MODE;
        speed = ETH_SPEED_10M;
        linkchanged = 1;
        break;
      default:
        break;
      }

      if(linkchanged)
      {
        /* Get MAC Config MAC */
        HAL_ETH_GetMACConfig(&eth_conf_s[instance].EthHandle, &MACConf);
        MACConf.DuplexMode = duplex;
        MACConf.Speed = speed;
        HAL_ETH_SetMACConfig(&eth_conf_s[instance].EthHandle, &MACConf);
        HAL_ETH_Start_IT(&eth_conf_s[instance].EthHandle);
        netif_set_up(netif);
        netif_set_link_up(netif);
      }
    }

    osDelay(100);
  }
}

void HAL_ETH_RxAllocateCallback(ETH_HandleTypeDef *heth, uint8_t **buff)
{
  struct pbuf_custom *p = LWIP_MEMPOOL_ALLOC(RX_POOL);
  if (p)
  {
    /* Get the buff from the struct pbuf address. */
    *buff = (uint8_t *)p + offsetof(RxBuff_t, buff);
    p->custom_free_function = pbuf_free_custom;
    /* Initialize the struct pbuf.
    * This must be performed whenever a buffer's allocated because it may be
    * changed by lwIP or the app, e.g., pbuf_free decrements ref. */
    pbuf_alloced_custom(PBUF_RAW, 0, PBUF_REF, p, *buff, ETH_RX_BUFFER_SIZE);
  }
  else
  {
  	if (heth == &eth_conf_s[0].EthHandle)
  	{
  		eth_conf_s[0].RxAllocStatus = RX_ALLOC_ERROR;
  	}
  	else if (heth == &eth_conf_s[1].EthHandle)
  	{
  		eth_conf_s[1].RxAllocStatus = RX_ALLOC_ERROR;
  	}
    *buff = NULL;
  }
}

void HAL_ETH_RxLinkCallback(ETH_HandleTypeDef *heth, void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
  struct pbuf **ppStart = (struct pbuf **)pStart;
  struct pbuf **ppEnd = (struct pbuf **)pEnd;
  struct pbuf *p = NULL;

  /* Get the struct pbuf from the buff address. */
  p = (struct pbuf *)(buff - offsetof(RxBuff_t, buff));
  p->next = NULL;
  p->tot_len = 0;
  p->len = Length;

  /* Chain the buffer. */
  if (!*ppStart)
  {
    /* The first buffer of the packet. */
    *ppStart = p;
  }
  else
  {
    /* Chain the buffer to the end of the packet. */
    (*ppEnd)->next = p;
  }
  *ppEnd  = p;

  /* Update the total length of all the buffers of the chain. Each pbuf in the chain should have its tot_len
   * set to its own length, plus the length of all the following pbufs in the chain. */
  for (p = *ppStart; p != NULL; p = p->next)
  {
    p->tot_len += Length;
  }

  /* Invalidate data cache because Rx DMA's writing to physical memory makes it stale. */
//  SCB_InvalidateDCache_by_Addr((uint32_t *)buff, Length);
}

void HAL_ETH_TxFreeCallback(ETH_HandleTypeDef *heth, uint32_t * buff)
{
  pbuf_free((struct pbuf *)buff);
}

