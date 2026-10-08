#include "sd_card.h"

SDCardModule g_sdcard;

// SD 总线互斥锁
SemaphoreHandle_t g_sd_mutex = NULL;

SDCardModule::SDCardModule() : mounted(false), hspi(nullptr) {}

bool SDCardModule::isMounted() const {
    return mounted;
}

bool SDCardModule::init() {
    if (mounted) return true;

    // 1. 创建互斥锁。必须在任何 SD 访问之前建好
    if (g_sd_mutex == NULL) {
        g_sd_mutex = xSemaphoreCreateMutex();
        if (g_sd_mutex == NULL) return false;
    }

    // 2. 创建HSPI实例 (SPI3)
    hspi = new SPIClass(HSPI);
    if (!hspi) return false;

    // 3. 开始HSPI并设置引脚
    hspi->begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);  // SCK, MISO, MOSI, SS

    // 4. 初始化SD卡（使用HSPI），频率用框架默认值 4MHz
    //    挂载这一步要上锁：SD.begin 内部会做完整的卡初始化和 FAT 挂载，
    //    若此时别的任务在碰文件系统，会直接把卡带进错误状态
    bool ok;
    SD_LOCK();
    ok = SD.begin(SD_CS, *hspi);
    SD_UNLOCK();
    if (!ok) {
        Serial.println("[SD卡] 挂载失败！检查接线或SD卡");
        return false;
    }

    mounted = true;
    return true;
}
