/**
  ******************************************************************************
  * @file    LwIP/LwIP_UDPTCP_Echo_Server_Netconn_RTOS/Src/app_ethernet.c 
  * @author  MCD Application Team
  * @brief   Ethernet specefic module
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
#include "lwip/opt.h"
#include "main.h"
#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif
#include "app_ethernet.h"
#include "ethernetif.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#if LWIP_DHCP
#define MAX_DHCP_TRIES  4
extern eth_conf_t eth_conf_s[NUM_OF_ETH_INSTANCE];
#endif

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Notify the User about the network interface config status
  * @param  netif: the network interface
  * @retval None
  */
void ethernet_link_status_updated(struct netif *netif)
{
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

  if (netif_is_up(netif))
  {
#if LWIP_DHCP
    /* Update DHCP state machine */
  	eth_conf_s[instance].DHCP_state = DHCP_START;
#else
    printf("Static IP address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
    BSP_LED_On(LED_BLUE);
    BSP_LED_Off(LED_GREEN);
#endif /* LWIP_DHCP */
  }
  else
  {
#if LWIP_DHCP
    /* Update DHCP state machine */
  	eth_conf_s[instance].DHCP_state = DHCP_LINK_DOWN;
#else
    printf("The network cable is not connected \n");
    BSP_LED_Off(LED_BLUE);
    BSP_LED_On(LED_GREEN);
#endif /* LWIP_DHCP */
  }
}

#if LWIP_DHCP
/**
  * @brief  DHCP Process
  * @param  argument: network interface
  * @retval None
  */
void DHCP_Thread(void * argument)
{
  struct netif *netif = NULL;
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
  struct dhcp *dhcp;
  uint8_t i = 0;

  for (;;)
  {
  	for (i = 0; i < NUM_OF_ETH_INSTANCE; i++)
  	{
  		netif = &eth_conf_s[i].gnetif;
			switch (eth_conf_s[i].DHCP_state)
			{
			case DHCP_START:
				{
					ip_addr_set_zero_ip4(&netif->ip_addr);
					ip_addr_set_zero_ip4(&netif->netmask);
					ip_addr_set_zero_ip4(&netif->gw);
					eth_conf_s[i].DHCP_state = DHCP_WAIT_ADDRESS;
					printf("  State: Looking for DHCP server ...\n");
					BSP_LED_Off(LED_GREEN);
					BSP_LED_Off(LED_BLUE);
					dhcp_start(netif);
				}
				break;
			case DHCP_WAIT_ADDRESS:
				{
					if (dhcp_supplied_address(netif))
					{
						eth_conf_s[i].DHCP_state = DHCP_ADDRESS_ASSIGNED;
						printf("IP address assigned by a DHCP server: %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
						BSP_LED_On(LED_GREEN);
						BSP_LED_Off(LED_BLUE);
					}
					else
					{
						dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);

						/* DHCP timeout */
						if (dhcp->tries > MAX_DHCP_TRIES)
						{
							eth_conf_s[i].DHCP_state = DHCP_TIMEOUT;

							/* Static address used */
							IP_ADDR4(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
							IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
							IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
							netif_set_addr(netif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));

							printf("DHCP Timeout !! \n");
							printf("Static IP address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
							BSP_LED_On(LED_BLUE);
							BSP_LED_Off(LED_GREEN);
						}
					}
				}
				break;
		case DHCP_LINK_DOWN:
			{
				eth_conf_s[i].DHCP_state = DHCP_OFF;
				printf("The network cable is not connected \n");
				BSP_LED_Off(LED_BLUE);
				BSP_LED_On(LED_GREEN);
			}
			break;
			default: break;
			}
			/* wait 50 ms */
			osDelay(50);
  	}
  }
}
#endif  /* LWIP_DHCP */


