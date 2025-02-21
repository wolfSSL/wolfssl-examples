#ifndef WOLFIP_FREERTOS_H
#define WOLFIP_FREERTOS_H

#include "FreeRTOS.h"
#include "task.h"
#include "wolfip.h"

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
