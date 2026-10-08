#include "tasks.h"

#include <Arduino.h>
#include <WiFi.h>
#include <string>
#include <cstring>

#include "My_audio.h"
#include "My_Wifi.h"
#include "My_xiaozhi.h"
#include "sd_card.h"
#include "weather.h"
#include "state/app_state.h"
#include "view/ui_view.h"
#include <lvgl.h>
#include "ui/ui_events.h"

/* ---------------- audio_task ----------------
 * 音频解码的时间片泵。优先级最高（5）—— 解码器要是不及时喂数据，
 * 声音就会断续。audio_process_requests() 是唯一真正操作 Audio 对象
 * 的地方，别的任务只能发请求（见 state/app_state.h）。 */
void audio_task(void *pvParameters)
{
    while (1)
    {
        audio_loop();               // audio.loop()，驱动解码
        audio_process_requests();   // 执行挂起的播放/暂停/音量请求
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void audio_task_create(void)
{
    xTaskCreate(audio_task, "audio_task", 8192, NULL, 5, NULL);
}


/* ---------------- weather_task ----------------
 * 先等 WiFi 连上，然后每 5 分钟拉一次天气。
 * 拉失败等 30 秒重试 */
void weather_task(void *pvParameters)
{
    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1)
    {
        if (get_weather())
        {
            // get_weather() 只填数据。

            ui_view_weather();
            vTaskDelay(pdMS_TO_TICKS(60 * 5 * 1000));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(30 * 1000));
        }
    }
}

void weather_task_create(void)
{
    xTaskCreate(weather_task, "weather_task", 4800, NULL, 5, NULL);
}


/* ---------------- sd_task ---------------- */
void sd_task(void *pvParameters)
{
    if (g_sdcard.init())
        Serial.println("[SD卡] 挂载成功");
    else
        Serial.println("[SD卡] 挂载失败，音乐功能不可用");

    vTaskDelete(NULL);   // 一次性任务，干完就退出
}

void sd_task_create(void)
{

    xTaskCreate(sd_task, "sd_task", 4096, NULL, 4, NULL);
}


/* ---------------- xiaozhi_task ----------------*/
void xiaozhi_task(void *pvParameters)
{
    while (1)
    {
        if (g_xiaozhi_ask)
        {
            My_xiaozhi_loop();
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void xiaozhi_task_create(void)
{

    xTaskCreate(xiaozhi_task, "xiaozhi_task", 12288, NULL, 4, NULL);
}


/* ---------------- ui_task ----------------
 * LVGL 的心跳。lv_timer_handler() 必须被反复调用，LVGL 才会处理动画、
 * 事件和重绘，界面才会动。*/
void ui_task(void *pvParameters)
{
    while (1)
    {
        lv_timer_handler();                 // LVGL 心跳，必须先跑
        calendar_update_real_time(NULL);    // 日历页对齐当天
        ui_view_music_switch_sync();        // 播放开关跟随实际播放状态
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void ui_task_create(void)
{
    xTaskCreate(ui_task, "ui_task", 8192, NULL, 3, NULL);
}


/* ---------------- time_task ----------------
 * 主页时钟，一秒一跳。 */
void time_task(void *pvParameters)
{
    while (1)
    {
        ui_view_clock();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void time_task_create(void)
{
    xTaskCreate(time_task, "time_task", 4500, NULL, 3, NULL);
}


/* ---------------- uart_task ---------------- */
void uart_send_to_pc(const char *msg)
{
    if (msg == nullptr) return;
    Serial.println(msg);
}

void uart_recv_from_pc(void)
{
    if (Serial.available() == 0) return;
    String c = Serial.readString();
    Serial.println(c);
    ui_view_serial_append(c.c_str());
}

void uart_task(void *pvParameters)
{
    while (1)
    {
        uart_recv_from_pc();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void uart_task_create(void)
{
    xTaskCreate(uart_task, "uart_task", 4500, NULL, 3, NULL);
}


/* ---------------- wifi_task ----------------
 * 扫描热点 + 连接 + 刷新主页状态文字。 */
#define WIFI_SCAN_RETRY_MAX 5
#define WIFI_SCAN_RETRY_MS  3000

/* 去掉 SSID/密码尾部的换行和空格。
 * Roller 选出来的字符串、软键盘输入的内容都可能带这些。 */
static void trim_wifi_str(char *buf)
{
    if (buf == nullptr) return;
    int len = strlen(buf);
    while (len > 0)
    {
        char ch = buf[len - 1];
        if (ch == '\n' || ch == '\r' || ch == ' ')
        {
            buf[len - 1] = '\0';
            len--;
        }
        else
        {
            break;
        }
    }
}

static int      s_last_wifi_status   = -99;   // -99 = 首次运行标记
static uint32_t s_wifi_refresh_tick  = 0;
static uint32_t s_scan_retry_tick    = 0;     // 0 = 没有待重试的扫描
static int      s_scan_retry         = 0;

void wifi_task(void *pvParameters)
{
    while (!g_wifi_init_done)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    g_wifi_scan_req = 1;   // 上电自动扫描
    s_wifi_refresh_tick = xTaskGetTickCount();

    for (;;)
    {
        int cur_status = WiFi.status();
        uint32_t now = xTaskGetTickCount();

        // 首次运行 或者 状态改变 或者 每2秒强制刷新一次UI
        if (s_last_wifi_status == -99 || cur_status != s_last_wifi_status ||
            (now - s_wifi_refresh_tick > pdMS_TO_TICKS(2000)))
        {
            s_wifi_refresh_tick = now;
            s_last_wifi_status = cur_status;
            ui_view_wifi_state(cur_status == WL_CONNECTED);
        }

        // ---- 扫描请求 ----
        if (g_wifi_scan_req == 1)
        {
            g_wifi_scan_req = 0;
            int ap_cnt = wifi_Search();      // 返回热点数，失败为负

            if (ap_cnt >= 0)
            {
                s_scan_retry = 0;
                s_scan_retry_tick = 0;

                std::string roller_opt;
                if (ap_cnt == 0)
                {
                    roller_opt = "未找到WiFi";
                }
                else
                {
                    for (int i = 0; i < ap_cnt; i++)
                    {
                        roller_opt += wifi_get_ap_ssid(i).c_str();
                        roller_opt += "\n";
                    }
                }
                ui_view_wifi_list(roller_opt.c_str());
            }
            else
            {
                /* 扫描失败（多半是 STA 还在 connecting，射频没空闲）。
                 * 排个重试，不要像以前那样一次失败就永远空着列表。 */
                if (s_scan_retry < WIFI_SCAN_RETRY_MAX)
                {
                    s_scan_retry++;
                    s_scan_retry_tick = now;
                    Serial.printf("[WiFi] 扫描失败, %d/%d 次, %dms 后重试\n",
                                  s_scan_retry, WIFI_SCAN_RETRY_MAX, WIFI_SCAN_RETRY_MS);
                }
                else
                {
                    s_scan_retry_tick = 0;
                    Serial.printf("[WiFi] 扫描重试 %d 次仍失败, 放弃。"
                                  "可在设置页重新进一次触发扫描\n", WIFI_SCAN_RETRY_MAX);
                }
            }
            wifi_scan_clean();   // 释放扫描内存
        }

        // 扫描重试到点了
        if (s_scan_retry_tick != 0 &&
            (now - s_scan_retry_tick) >= pdMS_TO_TICKS(WIFI_SCAN_RETRY_MS))
        {
            s_scan_retry_tick = 0;
            g_wifi_scan_req = 1;
        }

        // ---- 连接请求 ----
        if (g_wifi_connect_req == 1)
        {
            g_wifi_connect_req = 0;

            char ssid[32] = {0};
            char pwd[64]  = {0};
            if (g_wifi_cfg_mutex) xSemaphoreTake(g_wifi_cfg_mutex, portMAX_DELAY);
            memcpy(ssid, g_wifi_ssid_buf, sizeof(ssid) - 1);
            memcpy(pwd,  g_wifi_pwd_buf,  sizeof(pwd)  - 1);
            if (g_wifi_cfg_mutex) xSemaphoreGive(g_wifi_cfg_mutex);

            trim_wifi_str(ssid);
            trim_wifi_str(pwd);

            if (!wifi_connect_ap(ssid, pwd, 15000))
            {
                Serial.printf("[WiFi] 连接 \"%s\" 失败\n", ssid);
            }
            s_last_wifi_status = -99;   // 强制下一轮刷新状态标签
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void wifi_task_create(void)
{
    xTaskCreate(wifi_task, "wifi_task", 4500, NULL, 3, NULL);
}
