/* example_main.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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
#include <wolfssl/wolfcrypt/types.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/cmac.h>
#include "iodefine.h"
#include "rh_string.h"
#include "r_system.h"
#include "r_intc.h"

void R_LIN_Init(void);
void R_UART_Init(void);
void R_UART_SendString(char string[]);

#define INTC2ICOSTM0 INTC2.ICOSTM0.UINT16

#define MSG_LEN         128

char    msg_buf[MSG_LEN + 1];
size_t  msg_offset = 0;

int rsapss_sign_verify();

/* 1ms tick timer */
static long tick = 0;
#define NUMINTCOSTM0 0x54
#pragma interrupt INTCOSTM0(enable=true, channel=NUMINTCOSTM0)
void INTCOSTM0(void)
{
    tick++;
}

#define YEAR  ( \
    ((__DATE__)[7]  - '0') * 1000 + \
    ((__DATE__)[8]  - '0') * 100  + \
    ((__DATE__)[9]  - '0') * 10   + \
    ((__DATE__)[10] - '0') * 1      \
)

#define MONTH ( \
    __DATE__[2] == 'n' ? (__DATE__[1] == 'a' ? 1 : 6) \
  : __DATE__[2] == 'b' ? 2 \
  : __DATE__[2] == 'r' ? (__DATE__[0] == 'M' ? 3 : 4) \
  : __DATE__[2] == 'y' ? 5 \
  : __DATE__[2] == 'l' ? 7 \
  : __DATE__[2] == 'g' ? 8 \
  : __DATE__[2] == 'p' ? 9 \
  : __DATE__[2] == 't' ? 10 \
  : __DATE__[2] == 'v' ? 11 \
  : 12 \
	)

time_t time(time_t *t)
{
    (void)t;
    return ((YEAR-1970)*365+30*MONTH)*24*60*60 + tick++;
}

void PrintTextln(char* TextArray)
{
    R_UART_SendString(TextArray);
    R_UART_SendString("\r\n");
}

void main(void);

#if !defined(WOLFSSL_STATIC_MEMORY)
#include    <stddef.h>
#define     SIZEOF_HEAP 0xB000
int     _REL_sysheap[SIZEOF_HEAP >> 2];
size_t  _REL_sizeof_sysheap = SIZEOF_HEAP;
#endif

void main(void)
{
   byte ret;
   
   R_SYSTEM_ClockInit();
   R_SYSTEM_TimerInit();
   R_SYSTEM_TimerStart();
   R_UART_Init();
   R_LIN_Init();
   /* Enable Table Interrupt */
   R_INTC_SetTableBit((uint16_t*)R_ICOSTM0);
   R_INTC_UnmaskInterrupt((uint16_t*)R_ICOSTM0);
   /* Enable interrupts */
    __EI();
    
   rh_string_init(MSG_LEN + 1, &msg_offset, msg_buf);
   printf("System Initialization finish.");
   printf("RSA Sign and Verify Example");
    
   ret = rsapss_sign_verify();
   if (ret != 0) {
        printf("rsapss_sign_verify failed ret=%d.", ret);
   }
}
