#include "sd_task.h"

TaskHandle_t sd_task_handle = NULL;

// 健康监测周期（毫秒）
#define SD_HEALTH_INTERVAL_MS 5000

void sd_task_create(void)
{
    xTaskCreate(sd_task,
                "sd_task",
                4096,
                NULL,
                4,
                &sd_task_handle);
}

void sd_task(void *pvParameters)
{
    // 等 SD 初始化完成（g_sdcard.init() 在 setup 里已调用）
    while (!g_sdcard.isMounted())
    {
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    // 上一轮的健康状态，-1 表示还没测过
    int lastHealthy = -1;

    while (1)
    {
        // 定期确认卡还在线。读一次文件大小即可，代价很低。
        // 所有 g_sdcard 接口内部都有互斥锁，不会和音频的 audiofile.read() 抢 SPI
        bool healthy = g_sdcard.isMounted() && (g_sdcard.cardType() != CARD_NONE);

        if ((int)healthy != lastHealthy)
        {
            lastHealthy = (int)healthy;
            if (healthy)
            {
                Serial.printf("[SD任务] 卡状态正常, 剩余堆=%u\n", (unsigned)ESP.getFreeHeap());
            }
            else
            {
                Serial.printf("[SD任务] !! 检测到卡掉线, 剩余堆=%u\n", (unsigned)ESP.getFreeHeap());
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SD_HEALTH_INTERVAL_MS));
    }
}
