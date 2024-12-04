/**
  ******************************************************************************
  * @file    LwIP/LwIP_UDPTCP_Echo_Server_Netconn_RTOS/Inc/ethernetif.h 
  * @author  MCD Application Team
  * @brief   Header for ethernetif.c module
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

#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__


#include "lwip/err.h"
#include "../Components/lan8742/lan8742.h"
#include "lwip/netif.h"
#include "cmsis_os.h"

#define NUM_OF_ETH_INSTANCE				1

typedef struct {
	struct netif gnetif;
	ETH_HandleTypeDef EthHandle;
	ETH_TxPacketConfigTypeDef TxConfig;
	lan8742_Object_t LAN8742;
	osMessageQueueId_t RxPktQueue;
	osMessageQueueId_t TxPktQueue;
	uint32_t RxAllocStatus;
	uint32_t DHCP_state;
} eth_conf_t;

/* Exported types ------------------------------------------------------------*/
/* Structure that include link thread parameters */
/* Exported functions ------------------------------------------------------- */
err_t ethernetif_init(struct netif *netif);
void MX_ETH1_Init(void);
void MX_ETH2_Init(void);
void ethernet_link_thread( void * argument );
#endif
