#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "wolfip_freertos.h"

static void testTask(void* pvParameters) {
    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
    int ret;
    
    printf("Initializing wolfIP...\n");
    ret = wolfIP_FreeRTOS_Init();
    if (ret != 0) {
        printf("Failed to initialize wolfIP\n");
        return;
    }
    
    printf("Starting wolfIP network task...\n");
    ret = wolfIP_FreeRTOS_Start();
    if (ret != 0) {
        printf("Failed to start wolfIP network task\n");
        return;
    }

    printf("Starting UDP echo server...\n");
    ret = wolfIP_Start_UDP_Echo();
    if (ret != 0) {
        printf("Failed to start UDP echo server\n");
        return;
    }
    
    printf("Network stack and UDP echo server running...\n");
    for(;;) {
        vTaskDelay(xDelay);
    }
}

int main(void) {
    printf("Starting FreeRTOS with wolfIP...\n");
    
    /* Create the test task */
    xTaskCreate(testTask, "TestTask", configMINIMAL_STACK_SIZE, 
                NULL, tskIDLE_PRIORITY + 1, NULL);
    
    /* Start the scheduler */
    vTaskStartScheduler();
    
    /* Should never reach here */
    return 0;
}
