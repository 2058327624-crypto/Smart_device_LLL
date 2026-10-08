#include "ui.h"
#include <lv_games.h>
#include "ui_events.h"
#include "screen.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "rtos/wifi_task.h"
#include "rtos/uart_task.h"
#include "My_audio.h"
#include <Arduino.h>
#include <time.h>


extern bool getLocalTime(struct tm * info, uint32_t ms);
void Keyboard_hide(lv_event_t * e);
void Keyboard_Show(lv_event_t * e);
static void music_entry_cb(lv_event_t * e);

void ui_events_init(void)
{

    lv_obj_add_event_cb(ui_Keyboard2, Keyboard_hide, LV_EVENT_READY, NULL);    // 设置页
    lv_obj_add_event_cb(ui_Keyboard1, Keyboard1_send, LV_EVENT_READY, NULL);    // 串口页
    lv_obj_add_event_cb(ui_Music, music_entry_cb, LV_EVENT_CLICKED, NULL);    // 音乐页入口
}

/* 1. 主页 ui_Home */

void refresh_home_clock(void)
{
    struct tm tm_now;
    char time_buf[16];
    char date_buf[24];

    if(getLocalTime(&tm_now, 0U))
    {
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tm_now);
        lv_label_set_text(ui_time, time_buf);
        strftime(date_buf, sizeof(date_buf), "%Y/%m/%d", &tm_now);
        lv_label_set_text(ui_date, date_buf);
    }
    else
    {
        lv_label_set_text(ui_time, "--:--");
        lv_label_set_text(ui_date, "----/--/--");
    }
}

/* 2. 小智页 ui_xiaozhiPage */
bool ask_flag = false;

void switch_xiaozhi_cb(lv_event_t * e)
{
    lv_obj_t * switch_obj = lv_event_get_target(e);
    ask_flag = lv_obj_has_state(switch_obj, LV_STATE_CHECKED);
}

/*3. 音乐页 ui_MusicPage */

/* ---- 3.1 曲目列表 ---- */
static bool music_list_ready = false;
void music_page_on_show(void)
{
    if(music_list_ready) return;
    if(ui_ddsonglist == NULL) return;
    int n = music_scan();
    if(n <= 0)
    {
        lv_roller_set_options(ui_ddsonglist, "未找到歌曲", LV_ROLLER_MODE_NORMAL);
        music_list_ready = true;
        return;
    }
    /* 拼成 "歌名\n歌名\n..." 的格式，Roller 按换行分行 */
    static char buf[512];
    buf[0] = '\0';
    for(int i = 0; i < n; i++)
    {
        if(i > 0) strncat(buf, "\n", sizeof(buf) - strlen(buf) - 1);
        strncat(buf, music_name(i), sizeof(buf) - strlen(buf) - 1);
    }
    lv_roller_set_options(ui_ddsonglist, buf, LV_ROLLER_MODE_NORMAL);
    /* 滚到当前正在播放的那首，没有就停在第一首 */
    int cur = music_current_index();
    lv_roller_set_selected(ui_ddsonglist, (cur >= 0) ? cur : 0, LV_ANIM_OFF);
    music_list_ready = true;
}

static void music_entry_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    music_page_on_show();
}

/* ---- 3.2 音量滑块 ---- */
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

/* ---- 3.3 曲目滚轮 ---- */

static bool music_roller_syncing = false;
static void music_roller_sync(int idx)
{
    if(ui_ddsonglist == NULL) return;
    music_roller_syncing = true;
    lv_roller_set_selected(ui_ddsonglist, (uint16_t)idx, LV_ANIM_ON);
    music_roller_syncing = false;
}
void on_song_select(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    if(music_roller_syncing) return;      /* 是上一首/下一首自己拨的，忽略 */

    uint32_t idx = lv_roller_get_selected(ui_ddsonglist);
    audio_request_play((int)idx, 0);
}

/* ---- 3.4 播放开关 ---- */

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
    if(lv_obj_has_state(ui_btnplaypause, LV_STATE_CHECKED))
        audio_request_resume();
    else
        audio_request_pause();
}

void music_sync_play_switch(void)
{
    if(ui_btnplaypause == NULL) return;
    if(lv_scr_act() != ui_MusicPage) return;
    if(audio_request_pending()) return;
    if(millis() < music_switch_grace_until) return;
    bool running = (audio_is_playing() != 0);
    bool shown   = lv_obj_has_state(ui_btnplaypause, LV_STATE_CHECKED);
    if(running && !shown)        lv_obj_add_state(ui_btnplaypause, LV_STATE_CHECKED);
    else if(!running && shown)   lv_obj_clear_state(ui_btnplaypause, LV_STATE_CHECKED);
}

/* ---- 3.5 上一首 / 下一首 ---- */

void on_btn_prev(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int n = music_count();
    if(n <= 0) return;
    int cur = music_current_index();
    int target = (cur < 0) ? 0 : ((cur - 1 + n) % n);
    bool playing = lv_obj_has_state(ui_btnplaypause, LV_STATE_CHECKED);
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
    bool playing = lv_obj_has_state(ui_btnplaypause, LV_STATE_CHECKED);
    audio_request_play(target, playing ? 1 : 0);
    music_roller_sync(target);      /* 用 sync 版本，避免二次触发选曲 */
}

 /* 4. 设置页 ui_SettingsPage  (WiFi 配网 + 屏幕亮度) */

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

 /* 5. 游戏页 ui_GamePage  (羊了个羊) */

static lv_obj_t *game_root = NULL;
static void yang_exit_async_cb(void * user_data)
{
    LV_UNUSED(user_data);
    if(game_root == NULL) return;
    lv_anim_del_all();
    lv_obj_del(game_root);
    game_root = NULL;
}
static void yang_exit_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_async_call(yang_exit_async_cb, NULL);
}

void Game_yang(lv_event_t * e)
{
    LV_UNUSED(e);
    if(game_root != NULL) return;   /* 已经在游戏里了，忽略重复进入 */
    game_root = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(game_root);      /* 去掉默认白底/边框/圆角 */
    lv_obj_set_size(game_root, 320, 240);
    lv_obj_set_pos(game_root, 0, 0);
    lv_obj_clear_flag(game_root, LV_OBJ_FLAG_SCROLLABLE);

    srand(millis());    /* yang.c 自己不种随机种子，不种的话每次上电牌序完全一样 */
    yang_update(game_root);

    /* 退出按钮：建在 game_root 上（随游戏一起销毁），且建在 tileview 之后 ——
     * 作为兄弟节点永远画在 tileview 及其全部卡片之上，不受 lv_obj_move_foreground 影响。
     * 位置压在底部牌槽(到 y=205)下面的空白区，不挡任何可点卡片。 */
    lv_obj_t * btn = lv_btn_create(game_root);
    lv_obj_set_size(btn, 56, 26);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 4, -4);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xb03030), 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "退出");
    lv_obj_set_style_text_font(label, &ui_font_Font1, 0);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn, yang_exit_cb, LV_EVENT_CLICKED, NULL);
}


/* 6. 日历页 ui_CalendarPage */

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

 /* 7. 串口页 ui_SerialPortPage */
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
