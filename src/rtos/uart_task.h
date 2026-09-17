#ifndef _UART_TASK_H
#define _UART_TASK_H

#include "FreeRTOS.h"
#include "task.h"


#ifdef __cplusplus
extern "C" {
#endif

void uart_task(void *pvParameters);
void uart_task_create(void);
void uart_send_to_pc(const char *msg);
void uart_recv_from_pc(void);
#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
