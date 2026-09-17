#include "weather_task.h"
#include "weather.h"
#include <WiFi.h>

TaskHandle_t weather_task_handle = NULL;

static void weather_roller_async_cb(void *param)
{
    LV_UNUSED(param);
    update_weather_ui();
}

void weather_task(void *param)
{
    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    while(1)
    {
        if(get_weather())
        {
            lv_async_call(weather_roller_async_cb, NULL);
            vTaskDelay(pdMS_TO_TICKS(60 * 5 * 1000));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(30 * 1000));
        }
    }
}
void weather_task_create(void)
{
    xTaskCreate(weather_task,
                "weather_task",
                4800,
                NULL,
                5,
                &weather_task_handle);
}
