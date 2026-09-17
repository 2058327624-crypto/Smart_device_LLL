#ifndef _WEATHER_TASK_H
#define _WEATHER_TASK_H

#include "weather.h"
#include "FreeRTOS.h"
#include "task.h"

void weather_task(void *param);
void weather_task_create(void);

#endif
