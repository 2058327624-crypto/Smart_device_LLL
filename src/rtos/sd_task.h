#ifndef __SD_TASK_H
#define __SD_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "sd_card.h"

extern TaskHandle_t sd_task_handle;
void sd_task(void *pvParameters);
void sd_task_create(void);

#endif
