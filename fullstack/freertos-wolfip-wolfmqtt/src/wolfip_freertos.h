/* wolfip_freertos.h
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef WOLFIP_FREERTOS_H
#define WOLFIP_FREERTOS_H

#include "FreeRTOS.h"
#include "task.h"
#include "wolfip.h"

/* Global wolfIP instance */
extern struct wolfIP *g_wolfip;

/* Network task configuration */
#define WOLFIP_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define WOLFIP_TASK_STACK_SIZE (8 * 1024)
#define WOLFIP_POLL_INTERVAL_MS 10
#define UDP_TEST_PORT 7777

/* Initialize wolfIP with FreeRTOS */
int wolfIP_FreeRTOS_Init(void);

/* Start wolfIP network task */
int wolfIP_FreeRTOS_Start(void);

/* Start UDP echo server task */
int wolfIP_Start_UDP_Echo(void);

#endif /* WOLFIP_FREERTOS_H */
