#ifndef TASKS_H
#define TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

void audio_task_create(void);
void weather_task_create(void);
void sd_task_create(void);
void xiaozhi_task_create(void);
void ui_task_create(void);
void wifi_task_create(void);
void time_task_create(void);
void uart_task_create(void);

void audio_task(void *pvParameters);
void weather_task(void *pvParameters);
void sd_task(void *pvParameters);
void xiaozhi_task(void *pvParameters);
void ui_task(void *pvParameters);
void wifi_task(void *pvParameters);
void time_task(void *pvParameters);
void uart_task(void *pvParameters);

void uart_send_to_pc(const char *msg);
void uart_recv_from_pc(void);


#ifdef __cplusplus
}
#endif

#endif // TASKS_H
