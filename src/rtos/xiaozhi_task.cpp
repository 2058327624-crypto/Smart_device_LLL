#include "xiaozhi_task.h"

TaskHandle_t xiaozhi_task_handle = NULL;
extern bool ask_flag;
void xiaozhi_task_create(void)
{
    xTaskCreate(xiaozhi_task,
                "xiaozhi_task",
                4500,
                NULL,
                4,
                &xiaozhi_task_handle);
}
void xiaozhi_task(void *pvParameters)
{
    
    while (1)
    {
        if(ask_flag)
        {
            My_xiaozhi_loop();
            
        } 
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
