#ifndef __UI_TASK_H
#define __UI_TASK_H


#include "FreeRTOS.h"
#include "task.h"
#include "ui/ui.h"

extern TaskHandle_t ui_task_handle;
void ui_task(void *pvParameters);
void ui_task_create(void);


#endif