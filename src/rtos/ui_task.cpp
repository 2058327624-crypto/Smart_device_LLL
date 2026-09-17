#include "ui_task.h"
#include <Arduino.h>
TaskHandle_t ui_task_handle = NULL;

void ui_task_create(void)
{
    xTaskCreate(ui_task,
                "ui_task",
                4500,
                NULL,
                3,
                &ui_task_handle);
}

void ui_task(void *pvParameters)
{
  while (1)
  {
    lv_timer_handler();
    calendar_update_real_time(NULL);
    music_sync_play_switch();
    vTaskDelay(pdMS_TO_TICKS(1));

  }
}
