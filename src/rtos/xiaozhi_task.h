#ifndef __XIAOZI_TASK_H
#define __XIAOZI_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "My_xiaozhi.h"
#include "My_audio.h"

extern TaskHandle_t xiaozhi_task_handle;
void xiaozhi_task(void *pvParameters);
void xiaozhi_task_create(void);



#endif
