#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Generally you would define your own explicit list of lwIP options
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html)
//
// This example uses a common include to avoid repetition
#include "lwipopts_examples_common.h"

#define TCPIP_THREAD_STACKSIZE (1024*10)
#define DEFAULT_THREAD_STACKSIZE 1024*10
#define DEFAULT_RAW_RECVMBOX_SIZE 8
#define TCPIP_MBOX_SIZE 8
#define DEFAULT_ACCEPTMBOX_SIZE 8
#define LWIP_TIMEVAL_PRIVATE 0

// ping_thread sets socket receive timeout, so enable this feature
#define LWIP_SO_RCVTIMEO 1

#define LWIP_TIMEVAL_PRIVATE 0
#define LWIP_MPU_COMPATIBLE  0

#ifdef PICO_CYW43_ARCH_POLL
#define LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX 1
#endif

#define MEMP_NUM_SYS_TIMEOUT 10
#define DEFAULT_TCP_RECVMBOX_SIZE 10

#undef MEM_SIZE
#define MEM_SIZE 4000 * 10

#endif
