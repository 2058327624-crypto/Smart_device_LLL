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

    Serial.println("[SD卡] 初始化开始...");

    // 1. 创建互斥锁。必须在任何 SD 访问之前建好
    if (g_sd_mutex == NULL) {
        g_sd_mutex = xSemaphoreCreateRecursiveMutex();
        if (g_sd_mutex == NULL) {
            Serial.println("[SD卡] 互斥锁创建失败！");
            return false;
        }
    }

    // 2. 创建HSPI实例 (SPI3)
    hspi = new SPIClass(HSPI);
    if (!hspi) {
        Serial.println("[SD卡] HSPI创建失败！");
        return false;
    }

    // 3. 开始HSPI并设置引脚
    hspi->begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);  // SCK, MISO, MOSI, SS

    Serial.printf("[SD卡] HSPI初始化完成: SCLK=%d, MISO=%d, MOSI=%d, CS=%d\n",
                  SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

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

    // 5. 显示SD卡信息
    uint8_t type = cardType();
    const char* typeStr;
    switch (type) {
        case CARD_NONE:    typeStr = "无"; break;
        case CARD_MMC:     typeStr = "MMC"; break;
        case CARD_SD:      typeStr = "SD"; break;
        case CARD_SDHC:    typeStr = "SDHC"; break;
        default:           typeStr = "未知"; break;
    }

    Serial.printf("[SD卡] 挂载成功！类型: %s\n", typeStr);
    Serial.printf("[SD卡] 总容量: %.2f MB\n", SD.totalBytes() / (1024.0 * 1024.0));
    Serial.printf("[SD卡] 已用: %.2f MB\n", SD.usedBytes() / (1024.0 * 1024.0));

    return true;
}

uint8_t SDCardModule::cardType() {
    return SD.cardType();
}
