#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

static void testTask(void* pvParameters) {
    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
    
    for(;;) {
        printf("FreeRTOS Test Task Running\n");
        vTaskDelay(xDelay);
    }
}

int main(void) {
    printf("Starting FreeRTOS simulation...\n");
    
    /* Create the test task */
    xTaskCreate(testTask, "TestTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    
    /* Start the scheduler */
    vTaskStartScheduler();
    
    /* Should never reach here */
    return 0;
}
