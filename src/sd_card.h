#ifndef SD_CARD_H
#define SD_CARD_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

extern SemaphoreHandle_t g_sd_mutex;

// 上锁/解锁的快捷宏，保证成对出现
#define SD_LOCK()   do { if (g_sd_mutex) xSemaphoreTakeRecursive(g_sd_mutex, portMAX_DELAY); } while (0)
#define SD_UNLOCK() do { if (g_sd_mutex) xSemaphoreGiveRecursive(g_sd_mutex); } while (0)

class SDCardModule {
public:
    SDCardModule();

    // 挂载 SD 卡，建互斥锁，打印容量信息
    bool init();
    // 是否已挂载成功
    bool isMounted() const;
    // 卡类型（CARD_NONE / CARD_MMC / CARD_SD / CARD_SDHC），掉线检测用
    uint8_t cardType();

private:
    SPIClass* hspi;  // HSPI指针
    bool mounted;
};

extern SDCardModule g_sdcard;

#endif // SD_CARD_H
