#include "ui.h"
#include <lv_games.h>
#include "ui_events.h"
#include "screen.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "state/app_state.h"
#include "tasks/tasks.h"
#include "view/ui_view.h"
#include "My_audio.h"
#include <Arduino.h>
#include <time.h>


extern bool getLocalTime(struct tm * info, uint32_t ms);
void Keyboard_hide(lv_event_t * e);
void Keyboard_Show(lv_event_t * e);
static void music_entry_cb(lv_event_t * e);

// 事件注册
void ui_events_init(void)
{
    lv_obj_set_style_text_font(ui_Roller5, &ui_font_Font1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Roller4, &ui_font_Font1, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---- 设置页 ---- */
    lv_obj_add_event_cb(ui_Keyboard2, Keyboard_hide, LV_EVENT_READY, NULL);

    /* ---- 串口页 ---- */
    lv_obj_add_event_cb(ui_Keyboard1, Keyboard1_send, LV_EVENT_READY, NULL);

    /* ---- 音乐页 ----*/
    lv_obj_add_event_cb(ui_Music,   music_entry_cb,     LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Slider1, on_volume_change,   LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui_Roller4, on_song_select,     LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui_Switch2, on_btn_play_pause,  LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_add_flag(ui_Image3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui_Image4, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui_Image3,  on_btn_prev,        LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Image4,  on_btn_next,        LV_EVENT_CLICKED, NULL);
}

/*  小智页 ui_xiaozhiPage */
void switch_xiaozhi_cb(lv_event_t * e)
{
    lv_obj_t * switch_obj = lv_event_get_target(e);
    g_xiaozhi_ask = lv_obj_has_state(switch_obj, LV_STATE_CHECKED);
}

/* 音乐页 ui_MusicPage */

/* ---- 3.1 曲目列表 ---- */
static void music_entry_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_view_music_list();
}

/* ---- 音量滑块 ---- */
void on_volume_change(lv_event_t * e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);
    if(value < 0)   value = 0;
    if(value > 100) value = 100;
    int vmax = audio_volume_max();
    int vol  = value * vmax / 100;
    audio_request_volume(vol);
}

/* ---- 曲目滚轮 ---- */

static bool music_roller_syncing = false;
static void music_roller_sync(int idx)
{
    if(ui_Roller4 == NULL) return;
    music_roller_syncing = true;
    lv_roller_set_selected(ui_Roller4, (uint16_t)idx, LV_ANIM_ON);
    music_roller_syncing = false;
}
void on_song_select(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    if(music_roller_syncing) return;      /* 是上一首/下一首自己拨的，忽略 */

    uint32_t idx = lv_roller_get_selected(ui_Roller4);
    audio_request_play((int)idx, 0);
}

/* ---- 播放开关 ---- */

static uint32_t music_switch_grace_until = 0;
#define MUSIC_SWITCH_GRACE_MS 800

static void music_switch_touch(void)
{
    music_switch_grace_until = millis() + MUSIC_SWITCH_GRACE_MS;
}

/* 播放/暂停。*/
void on_btn_play_pause(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code != LV_EVENT_CLICKED && code != LV_EVENT_VALUE_CHANGED) return;
    music_switch_touch();       /* 先记时刻，等下别被同步逻辑弹回去 */
    if(lv_obj_has_state(ui_Switch2, LV_STATE_CHECKED))
        audio_request_resume();
    else
        audio_request_pause();
}

/* ---- 上一首 / 下一首 ---- */

void on_btn_prev(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int n = music_count();
    if(n <= 0) return;
    int cur = music_current_index();
    int target = (cur < 0) ? 0 : ((cur - 1 + n) % n);
    bool playing = lv_obj_has_state(ui_Switch2, LV_STATE_CHECKED);
    audio_request_play(target, playing ? 1 : 0);
    music_roller_sync(target);      /* 用 sync 版本，避免二次触发选曲 */
}
/* 下一首，到底绕回第一首 */
void on_btn_next(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int n = music_count();
    if(n <= 0) return;
    int cur = music_current_index();
    int target = (cur < 0) ? 0 : ((cur + 1) % n);
    bool playing = lv_obj_has_state(ui_Switch2, LV_STATE_CHECKED);
    audio_request_play(target, playing ? 1 : 0);
    music_roller_sync(target);      /* 用 sync 版本，避免二次触发选曲 */
}

 /* 设置页 ui_SettingsPage  (WiFi 配网 + 屏幕亮度) */

#define SCREEN_BRIGHTNESS_MIN 30
void silder_brightness_cb(lv_event_t * e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);   // 滑块范围是 0~100
    if(value < 0)   value = 0;
    if(value > 100) value = 100;
    uint8_t brightness = SCREEN_BRIGHTNESS_MIN +
                         (uint8_t)((255 - SCREEN_BRIGHTNESS_MIN) * value / 100);

    screen_set_brightness(brightness);
}

/* 点 SSID 输入框：记下 Roller 里选中的热点，然后弹出密码键盘。
 * SSID 存进 g_wifi_ssid_buf，密码在 Keyboard_hide() 里补上。 */
void Keyboard_Show(lv_event_t * e)
{
    LV_UNUSED(e);
    char buf[32] = {0};
    lv_roller_get_selected_str(ui_Roller5, buf, sizeof(buf));
    memset(g_wifi_ssid_buf, 0, sizeof(g_wifi_ssid_buf));
    strncpy(g_wifi_ssid_buf, buf, sizeof(g_wifi_ssid_buf)-1);
    lv_obj_clear_flag(ui_Keyboard2, LV_OBJ_FLAG_HIDDEN);
}

/* 键盘「对勾」键：收起键盘、读密码、请求连接。
 * 密码存入 g_wifi_pwd_buf，实际连接由 wifi_task 执行。 */
void Keyboard_hide(lv_event_t * e)
{
    lv_obj_t *kb = lv_event_get_target(e);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t* ta_pwd = lv_keyboard_get_textarea(kb);
    const char* pwd = lv_textarea_get_text(ta_pwd);
    memset(g_wifi_pwd_buf, 0, sizeof(g_wifi_pwd_buf));
    strncpy(g_wifi_pwd_buf, pwd, sizeof(g_wifi_pwd_buf)-1);
    g_wifi_connect_req = 1;
}

/* 日历页 ui_CalendarPage */

/* 是否第一次打开日历页。只有第一次才自动跳到当月，之后保留用户翻的月份。 */
static bool calendar_first_open = true;
void calendar_update_real_time(lv_event_t * e)
{
    LV_UNUSED(e);
    struct tm tm_now;
    bool ret =getLocalTime(&tm_now, 0U);
    if(!ret) return;
    uint16_t y = tm_now.tm_year + 1900;
    uint8_t  m  = tm_now.tm_mon + 1;
    uint8_t  d  = tm_now.tm_mday;
    lv_calendar_set_today_date(ui_Calendar1, y, m, d);
    if(calendar_first_open)
    {
        lv_calendar_set_showed_date(ui_Calendar1, y, m);
        calendar_first_open = false; //标记：不再自动跳转
    }
}

 /* 串口页 ui_SerialPortPage */
void Keyboard1_Show(lv_event_t * e)
{
    lv_obj_clear_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
}

void Keyboard1_send(lv_event_t * e)
{
    LV_UNUSED(e);
    const char *msg = lv_textarea_get_text(ui_TextArea2);
    if(msg == NULL || msg[0] == '\0') return;
    uart_send_to_pc(msg);
    lv_textarea_set_text(ui_TextArea2, "");
    lv_obj_add_flag(ui_Keyboard1, LV_OBJ_FLAG_HIDDEN);
}

/* 游戏页入口。SquareLine 的 Events 面板把 Game_yang 挂在游戏页那个图标上，
 * 生成的 ui.c 会调它，所以这个函数必须存在 —— 存根本身就是 SquareLine
 * 生成在这里的，实现（建/删游戏 root）在 view 层的 ui_view_game_yang()。 */
void Game_yang(lv_event_t * e)
{
    LV_UNUSED(e);
    ui_view_game_yang();
}
