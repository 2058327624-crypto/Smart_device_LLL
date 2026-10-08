#include "audio_task.h"

TaskHandle_t audio_task_handle = NULL;

void audio_task_create(void)
{
    xTaskCreate(audio_task,
                "audio_task",
                8192,
                NULL,
                5,
                &audio_task_handle);
}
void audio_task(void *pvParameters)
{
  while (1)
  {
    audio_loop();
    audio_process_requests();
    vTaskDelay(pdMS_TO_TICKS(1));
  }

}
