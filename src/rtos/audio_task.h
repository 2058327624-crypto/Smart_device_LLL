#ifndef __AUDIO_TASK_H
#define __AUDIO_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "My_audio.h"

extern TaskHandle_t audio_task_handle;
void audio_task(void *pvParameters);
void audio_task_create(void);

#endif
