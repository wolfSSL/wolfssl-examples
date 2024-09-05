#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Generally you would define your own explicit list of lwIP options
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html)
//
// This example uses a common include to avoid repetition
//#include "lwipopts_examples_common.h"

#define TCPIP_THREAD_STACKSIZE (1024*10)
#define DEFAULT_THREAD_STACKSIZE 1024*10
#define DEFAULT_RAW_RECVMBOX_SIZE 8
#define TCPIP_MBOX_SIZE 8
#define DEFAULT_ACCEPTMBOX_SIZE 8
#define LWIP_TIMEVAL_PRIVATE 0

// not necessary, can be done either way
#define LWIP_TCPIP_CORE_LOCKING_INPUT 0

// ping_thread sets socket receive timeout, so enable this feature
#define LWIP_SO_RCVTIMEO 1


#define LWIP_TIMEVAL_PRIVATE 0
#define LWIP_MPU_COMPATIBLE  0

#ifdef PICO_CYW43_ARCH_POLL
#define LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX 1
#endif

#define MEMP_NUM_SYS_TIMEOUT 10
#define DEFAULT_TCP_RECVMBOX_SIZE 10
// Common settings used in most of the pico_w examples
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html for details)

// allow override in some examples
//#if NO_SYS==0
//    #ifndef LWIP_SOCKET
//    #define LWIP_SOCKET 1
//    #endif
//#endif
#if PICO_CYW43_ARCH_POLL
#define MEM_LIBC_MALLOC 1
#else
// MEM_LIBC_MALLOC is incompatible with non polling versions
#define MEM_LIBC_MALLOC 0
#endif
#define MEM_ALIGNMENT 4
#define MEM_SIZE 4000 * 10
#define MEMP_NUM_TCP_SEG 32
#define MEMP_NUM_ARP_QUEUE 10
#define PBUF_POOL_SIZE 24
#define LWIP_ARP 1
#define LWIP_ETHERNET 1
#define LWIP_ICMP 1
#define LWIP_RAW 1
#define TCP_WND (8 * TCP_MSS)
#define TCP_MSS 1460
#define TCP_SND_BUF (8 * TCP_MSS)
#define TCP_SND_QUEUELEN ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_LINK_CALLBACK 1
#define LWIP_NETIF_HOSTNAME 1
#define LWIP_NETCONN 0
#define MEM_STATS 0
#define SYS_STATS 0
#define MEMP_STATS 0
#define LINK_STATS 0
// #define ETH_PAD_SIZE                2
#define LWIP_CHKSUM_ALGORITHM 3
#define LWIP_DHCP 1
#define LWIP_IPV4 1

#define LWIP_TCP 1
#define LWIP_DNS 1
#define LWIP_TCP_KEEPALIVE 1
#define LWIP_NETIF_TX_SINGLE_PBUF 1
#define DHCP_DOES_ARP_CHECK 0
#define LWIP_DHCP_DOES_ACD_CHECK 0


#endif
