#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- WIFI 配网状态 ----------------
 * 写入：ui_events.c（用户在设置页选热点、输密码）
 * 读取：tasks/wifi_task.cpp*/
extern char              g_wifi_ssid_buf[32];
extern char              g_wifi_pwd_buf[64];
extern volatile uint8_t  g_wifi_scan_req;     // 1 = 请求扫描一次
extern volatile uint8_t  g_wifi_connect_req;  // 1 = 请求连接（用上面两个缓冲）
extern volatile bool     g_wifi_init_done;    // 上电连接流程已结束
extern SemaphoreHandle_t g_wifi_cfg_mutex;

/* ---------------- 音频播放 ----------------
 * 写入：ui_events.c（播放/暂停/音量）、My_xiaozhi.cpp（TTS 语音）
 * 读取：tasks/audio_task.cpp -> My_audio.cpp 的 audio_process_requests() */
enum {
    AUDIO_CMD_NONE = 0,
    AUDIO_CMD_PLAY_INDEX,   // 播放曲目 g_audio_req_index
    AUDIO_CMD_PAUSE,
    AUDIO_CMD_RESUME,
    AUDIO_CMD_VOLUME,       // 设置音量 g_audio_req_vol
    AUDIO_CMD_TTS_URL       // 播放 g_audio_req_tts_url（小智的语音回答）
};

extern volatile int  g_audio_cmd;
extern volatile int  g_audio_req_index;
extern volatile int  g_audio_req_vol;
extern volatile int  g_audio_req_play;   // 1=选中后立即播放，0=只选中不出声
extern char          g_audio_req_tts_url[512];

/* ---------------- 天气数据 ----------------
 * 写入：tasks/weather_task.cpp -> weather.cpp 的 get_weather()
 * 读取：view/ui_view.cpp（画到天气页） */
typedef struct {
    char city[32];          // 城市
    char temp[8];           // 当前温度
    char text[32];          // 天气描述（晴/多云等）
    char humi[8];           // 湿度
    char last_update[32];   // 更新时间
} WeatherNow_t;

extern WeatherNow_t g_weather_now;
extern char         g_weather_forecast[512];   // 未来预报文本，每行一天

extern volatile bool g_xiaozhi_ask;

/* 建互斥锁等运行时资源。在 setup() 里、任何任务启动之前调一次。 */
void app_state_init(void);

#ifdef __cplusplus
}
#endif

#endif // APP_STATE_H
