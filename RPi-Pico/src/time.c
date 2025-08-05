
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include <time.h>
#include "FreeRTOS.h"
#include "task.h"

static long epoch_base;

time_t myTime(time_t *tt)
{
    return epoch_base + (xTaskGetTickCount() / configTICK_RATE_HZ);
}

int time_init()
{
    char datetime[] = DATETIME;
    struct tm t;

    printf("Date/Time = %s\n", datetime);

    if(strlen(datetime) != 12) {
        printf("Invalid Date/Time length(%s)\n", datetime);
        return -1;
    }
    if (sscanf(datetime, "%02d%02d%02d%02d%02d%02d",
               &t.tm_year, &t.tm_mon, &t.tm_mday, 
               &t.tm_hour, &t.tm_min, &t.tm_sec) != 6) {
        printf("Invalid Date/Time(%s)\n", datetime);
        return -1;
    }
    printf("%d, %d, %d, %d, %d, %d\n",
           t.tm_year, t.tm_mon, t.tm_mday,
           t.tm_hour, t.tm_min, &t.tm_sec);
        if (t.tm_year < 70)
        t.tm_year += 100; /* base year of 1900 */
    t.tm_mon--;
    epoch_base = mktime(&t);
    printf("epoch base = %d\n", epoch_base);
    return 0;

}
