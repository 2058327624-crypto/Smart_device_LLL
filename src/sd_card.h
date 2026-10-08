#ifndef SD_CARD_H
#define SD_CARD_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

extern SemaphoreHandle_t g_sd_mutex;

#define SD_LOCK()   do { if (g_sd_mutex) xSemaphoreTake(g_sd_mutex, portMAX_DELAY); } while (0)
#define SD_UNLOCK() do { if (g_sd_mutex) xSemaphoreGive(g_sd_mutex); } while (0)

class SDCardModule {
public:
    SDCardModule();

    // 挂载 SD 卡并建好互斥锁。失败返回 false（多半是硬件/接线问题，
    // 不重试 —— 由用户按复位键重来）
    bool init();
    // 是否已挂载成功
    bool isMounted() const;

private:
    SPIClass* hspi;  // HSPI指针
    bool mounted;
};

extern SDCardModule g_sdcard;

#endif // SD_CARD_H
