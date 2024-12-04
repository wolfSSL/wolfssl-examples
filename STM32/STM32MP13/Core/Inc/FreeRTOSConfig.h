/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * this is a template configuration files
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * These parameters and more are described within the 'configuration' section of the
 * FreeRTOS API documentation available on the FreeRTOS.org web site.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

/* Ensure stdint is only used by the compiler, and not the assembler. */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
 #include <stdint.h>
 extern uint32_t SystemCoreClock;

/* External functions defined in application */
 extern void vConfigureTickInterrupt(void);
 extern void vClearTickInterrupt(void);
#endif

/*-------------------- specific defines -------------------*/
#ifndef CMSIS_device_header
#define CMSIS_device_header "stm32mp13xx.h"
#endif /* CMSIS_device_header */

 /* If No secure feature is used the configENABLE_TRUSTZONE should be set to 0
  *
  */
#define configENABLE_TRUSTZONE                  0
#define configENABLE_FPU                        1
#define configENABLE_MPU                        0

/*-----------------------------------------------------------------*/

#define configUSE_PREEMPTION              1
#define configUSE_IDLE_HOOK               0
#define configUSE_TICK_HOOK               0
#define configMAX_PRIORITIES              (56)
#define configCPU_CLOCK_HZ                (SystemCoreClock)
#define configTICK_RATE_HZ                ((TickType_t)1000)
#define configMINIMAL_STACK_SIZE          ((uint16_t)256)
#define configMAX_TASK_NAME_LEN           (20)
#define configUSE_TRACE_FACILITY          1
#define configUSE_16_BIT_TICKS            0
#define configIDLE_SHOULD_YIELD           1
#define configUSE_MUTEXES                 1
#define configQUEUE_REGISTRY_SIZE         8
#define configCHECK_FOR_STACK_OVERFLOW    2
#define configUSE_RECURSIVE_MUTEXES       1
#define configUSE_MALLOC_FAILED_HOOK      1
#define configUSE_APPLICATION_TASK_TAG    0
#define configUSE_COUNTING_SEMAPHORES     1
#define configGENERATE_RUN_TIME_STATS     0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION		0

/* Used memory allocation (heap_x.c) */
#define configFRTOS_MEMORY_SCHEME               4


 /* Memory allocation related definitions. */
 #define configSUPPORT_STATIC_ALLOCATION         0
 #define configSUPPORT_DYNAMIC_ALLOCATION        1
 #define configTOTAL_HEAP_SIZE                   ((size_t)(1024 * 1024))
 #define configAPPLICATION_ALLOCATED_HEAP        0

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES           0
#define configMAX_CO_ROUTINE_PRIORITIES (2)

/* Software timer definitions. */
#define configUSE_TIMERS             1
#define configTIMER_TASK_PRIORITY    (2)
#define configTIMER_QUEUE_LENGTH     10
#define configTIMER_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 2)


/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet       1
#define INCLUDE_uxTaskPriorityGet      1
#define INCLUDE_vTaskDelete            1
#define INCLUDE_vTaskCleanUpResources  1
#define INCLUDE_vTaskSuspend           1
#define INCLUDE_xTaskDelayUntil        1
#define INCLUDE_vTaskDelay             1
#define INCLUDE_xTaskGetSchedulerState 1


/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_xQueueGetMutexHolder            1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1

/* Normal assert() semantics without relying on the provision of an assert.h
header file. */
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* Cortex-A specific definitions. */
#define configMAX_API_CALL_INTERRUPT_PRIORITY 18

/* Systick definitions */
#define configSETUP_TICK_INTERRUPT()       vConfigureTickInterrupt()

#define configCLEAR_TICK_INTERRUPT()       vClearTickInterrupt()

/*
 * If define configUSE_TASK_FPU_SUPPORT to 1, the task will start without a floating
 * point context. A task that uses the floating point hardware must call vPortTaskUsesFPU()
 * before executing any floating point instructions.
 */
#ifndef configUSE_TASK_FPU_SUPPORT
#define configUSE_TASK_FPU_SUPPORT 1
#endif

/* GIC information defintions. */
#define configINTERRUPT_CONTROLLER_BASE_ADDRESS 0xa0021000UL
#define configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET 0x1000UL
#define configUNIQUE_INTERRUPT_PRIORITIES 32

#endif /* FREERTOS_CONFIG_H */

