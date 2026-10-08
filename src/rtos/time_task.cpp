#include "time_task.h"
#include "ui/ui_events.h"

TaskHandle_t time_task_handle = NULL;
void time_task(void *pvParameters)
{
    while(1)
    {
        refresh_home_clock();
        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}

void time_task_create(void)
{
    xTaskCreate(time_task,
                "time_task",
                4500,
                NULL,
                3,
                &time_task_handle);
}
