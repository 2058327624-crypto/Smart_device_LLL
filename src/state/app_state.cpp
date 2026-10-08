#include "app_state.h"

/* ---------------- WiFi 配网 ---------------- */
char              g_wifi_ssid_buf[32]    = {0};
char              g_wifi_pwd_buf[64]     = {0};
volatile uint8_t  g_wifi_scan_req        = 0;
volatile uint8_t  g_wifi_connect_req     = 0;
volatile bool     g_wifi_init_done       = false;
SemaphoreHandle_t g_wifi_cfg_mutex       = NULL;

/* ---------------- 音频播放 ---------------- */
volatile int g_audio_cmd       = AUDIO_CMD_NONE;
volatile int g_audio_req_index = 0;
volatile int g_audio_req_vol   = 0;
volatile int g_audio_req_play  = 0;
char         g_audio_req_tts_url[512] = {0};

/* ---------------- 天气数据 ---------------- */
WeatherNow_t g_weather_now = {0};
char         g_weather_forecast[512] = {0};

/* ---------------- 小智 ---------------- */
volatile bool g_xiaozhi_ask = false;

void app_state_init(void)
{
    /* 保护 WiFi 的 ssid/pwd 两个缓冲区。普通互斥量即可：全项目只有两个
     * 用点（ui_events.c 写入、tasks.cpp 拷贝读取），都是平铺的单层临界区，
     * 不存在嵌套获取。 */
    if (g_wifi_cfg_mutex == NULL)
    {
        g_wifi_cfg_mutex = xSemaphoreCreateMutex();
    }
}
