#include "uart_task.h"
#include <Arduino.h>
#include <lvgl.h>
#include "ui/ui.h"

TaskHandle_t uart_task_handle = NULL;
extern "C" void uart_send_to_pc(const char *msg)
{
    if(msg == nullptr) return;
    Serial.println(msg);
}

void uart_recv_from_pc(void)
{
    if(Serial.available() == 0) return;
    String c = Serial.readString();
    Serial.println(c);
    lv_textarea_add_text(ui_TextArea3, c.c_str());

}
void uart_task(void *pvParameters)
{
    while(1)
    {
        uart_recv_from_pc();
        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }

}
void uart_task_create(void)
{
    xTaskCreate(uart_task,
                "uart_task",
                4500,
                NULL,
                3,
                &uart_task_handle);
}
